/*
 * Read-only RX 7600 PCI discovery for an x86_64 macOS Hackintosh.
 * Does not map BARs, attach a kernel driver, touch firmware or enable GPU commands.
 *
 * Expected RX 7600 / Navi 33 PCI identity: AMD 1002:7480.
 */
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static bool read_u32(io_registry_entry_t entry, CFStringRef key, uint32_t *out)
{
    CFTypeRef value = IORegistryEntryCreateCFProperty(entry, key, kCFAllocatorDefault, 0);
    if (!value)
        return false;

    bool found = false;
    if (CFGetTypeID(value) == CFDataGetTypeID()) {
        CFDataRef d = (CFDataRef)value;
        if (CFDataGetLength(d) >= 4) {
            const UInt8 *p = CFDataGetBytePtr(d);
            *out = (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                   ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
            found = true;
        }
    } else if (CFGetTypeID(value) == CFNumberGetTypeID()) {
        int64_t n = 0;
        if (CFNumberGetValue((CFNumberRef)value, kCFNumberSInt64Type, &n)) {
            *out = (uint32_t)n;
            found = true;
        }
    }
    CFRelease(value);
    return found;
}

int main(void)
{
    CFMutableDictionaryRef matching = IOServiceMatching("IOPCIDevice");
    if (!matching) {
        fprintf(stderr, "IOServiceMatching failed\n");
        return 1;
    }

    io_iterator_t iterator = IO_OBJECT_NULL;
    kern_return_t kr = IOServiceGetMatchingServices(kIOMainPortDefault, matching, &iterator);
    /* IOServiceGetMatchingServices consumes the matching dictionary. */
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "IOServiceGetMatchingServices failed: 0x%x\n", (unsigned)kr);
        return 1;
    }

    unsigned amd_count = 0;
    bool found_rx7600 = false;
    io_registry_entry_t entry;
    while ((entry = IOIteratorNext(iterator)) != IO_OBJECT_NULL) {
        uint32_t vendor = 0, device = 0, revision = 0, class_code = 0;
        const bool identity_ok =
            read_u32(entry, CFSTR("vendor-id"), &vendor) &&
            read_u32(entry, CFSTR("device-id"), &device);
        if (identity_ok && vendor == 0x1002) {
            amd_count++;
            (void)read_u32(entry, CFSTR("revision-id"), &revision);
            (void)read_u32(entry, CFSTR("class-code"), &class_code);
            printf("AMD PCI device %04x:%04x rev=%02x class=%06x%s\n",
                   (unsigned)vendor, (unsigned)device,
                   (unsigned)(revision & 0xff), (unsigned)(class_code & 0xffffff),
                   device == 0x7480 ? "  [Navi 33 / RX 7600 candidate]" : "");
            if (device == 0x7480)
                found_rx7600 = true;
        }
        IOObjectRelease(entry);
    }
    IOObjectRelease(iterator);
    printf("Found %u AMD PCI device(s); RX 7600 (1002:7480): %s\n",
           amd_count, found_rx7600 ? "present" : "not found");
    return found_rx7600 ? 0 : 2;
}

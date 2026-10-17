# RX 7600 (Navi 33) native macOS / RADV bring-up

Status: experimental, **no hardware acceleration demonstrated**.
Target: macOS 26 Tahoe, x86_64 Hackintosh, AMD Radeon RX 7600 (Navi 33, PCI 1002:7480, gfx1102).
This is a native PCIe-driver experiment, **not** Reims-vGPU or QEMU virtual graphics.

## Baseline

This branch started at upstream NullMoth nvidia-macos-driver **v2.0.0**
(commit \`bb53f0a809b1b6cc99bb4a085e23ef0ef3e0e93d\`), where
\`gpu-port/\` contains a Darwin build of Mesa RADV/ANV, libdrm patches,
an XNU/FreeBSD LinuxKPI port of amdgpu/i915/xe (NMXGPU), and a
Vulkan-based Metal-plugin adaptation.

Upstream's 41/41 drm-shim chip matrix includes \`navi33\`, but that only
tests fake DRM devices and shader compilation, **not** physical RX 7600
command submission, display or Metal acceleration. Do not equate
\`vkCreateDevice\` on drm-shim with working PCIe hardware.

The existing NVIDIA paths are not our RX 7600 backend. Development
should preserve them and not change the main branch.

## Execution order / gates

| Gate | Success condition | Notes |
|---|---|---|
| G0: identity | Real IOKit PCI device reports vendor \`0x1002\`, device \`0x7480\` | Use the read-only \`rx7600_pci_probe\` utility; this does not initialize the GPU |
| G1: userspace | RADV \`navi33\` passes \`vkCreateDevice\` + compute pipeline with the **noop drm-shim** | A userspace compiler / loader smoke test only |
| G2: kernel PCI | On an isolated test system, NMXGPU discovers the actual PCI function, maps BARs, and the amdgpu driver successfully attaches | Keep kernel logs, PCI BAR lengths, MSI/IRQ state, firmware IDs and errors |
| G3: hardware compute | Real \`/dev/dri/renderD*\` is backed by amdgpu; submit a compute workload and compare a GPU-produced readback buffer | No shim or CPU-only fallback is allowed |
| G4: macOS desktop | IOFramebuffer/IOSurface/IOAccelerator/Metal plugin present frames to WindowServer, with stable real GPU command submission | Test native resolution, refresh, surface synchronization, app rendering and sleep/wake separately |

Do **not** skip G2/G3 and jump directly to Metal or framebuffer changes.

## The likely kernel blockers

1. Device claiming: the NMXGPU IOKit personality must match the real PCI GPU, while any firmware framebuffer or Apple AMD driver must not steal exclusive ownership.
2. XNU-to-FreeBSD LinuxKPI port: PCI BAR and DMA memory mapping, coherent caches, interrupts, scheduling, waits and GPU page pinning must be correct. Check logs before changing this code.
3. Navi 33 bring-up: AMDGPU PSP/SMU/GC/SDMA/DCN firmware discovery, VRAM/GART address translation, GPU VM and ring command submission must all work.
4. DRM uAPI: libdrm and RADV expect real amdgpu ioctls and mmap equivalents. A node that only responds to discovery ioctls is insufficient.
5. Display and Metal are later gates. RADV is Vulkan userspace; without a working kernel driver it cannot give macOS native graphics acceleration.

## Safe first-run commands

On macOS with Xcode Command Line Tools:

\`\`\`sh
sh gpu-port/tools/build_rx7600_pci_probe.sh
gpu-port/tools/rx7600_pci_probe
\`\`\`

Both commands are unprivileged and read-only with respect to the GPU.

If the Mesa/macOS RADV test binaries have been built, run the \`navi33\`
drm-shim smoke test with:

\`\`\`sh
bash gpu-port/tools/rx7600_radv_shim_smoke.sh /path/to/mesa/build-x86 /path/to/vkprobe
\`\`\`

This tests **no physical GPU**; it must be reported as a shim result.

Before any real NMXGPU opt-in (\`nmxamd=1\` / \`nullmoth-gpu\`):
- Prepare a bootable rescue OpenCore USB with the experimental driver disabled.
- Have an independent way to collect boot logs, e.g. SSH/serial and an alternate boot.
- Back up your existing OpenCore configuration and system.
- Do not test on a sole RX 7600 display GPU without reliable boot recovery:
  the driver explicitly warns that claiming the GPU may reset its display.
- Do not casually disable SIP or replace Apple's graphics components.
- Never treat a successful kext load as a successful hardware attach.

## Diagnostic evidence to gather next

- Linux \`lspci -nn -vv -s <BDF>\`, \`lspci -nnk\`, \`dmesg\` entries for \`amdgpu\`/firmware and \`vulkaninfo --summary\` (if available).
- macOS \`rx7600_pci_probe\` output, \`kmutil showloaded --list-only\` (NMXGPU/NMDRM/NMXAccel) and relevant \`log show\` entries **after** an isolated opt-in.
- Distinguish \`no IOPCIDevice\`, \`probe declined\`, \`firmware missing\`, \`no driver took device\`, \`ring/IRQ timeout\`, and \`framebuffer failure\` as separate fault classes.

This document is a test plan and not a claim of hardware functionality.

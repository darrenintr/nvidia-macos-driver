#!/usr/bin/env bash
# Isolate the RADV Navi 33 *drm-shim* compiler test. It does not exercise physical GPU hardware.
set -euo pipefail
if [[ $# -ne 2 ]]; then
  echo "Usage: $0 /path/to/mesa/build-x86 /path/to/vkprobe" >&2
  exit 2
fi
if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "This is for the Darwin x86_64 Mesa build." >&2
  exit 2
fi
build_dir="$1"
vkprobe="$2"
shim="$build_dir/src/amd/drm-shim/libamdgpu_noop_drm_shim.dylib"
icd="$build_dir/src/amd/vulkan/radeon_devenv_icd.x86_64.json"
for file in "$shim" "$icd" "$vkprobe"; do
  if [[ ! -f "$file" ]]; then
    echo "Missing: $file" >&2
    exit 2
  fi
done
output=$(env AMDGPU_GPU_ID=navi33 \
  DYLD_INSERT_LIBRARIES="$shim" \
  VK_DRIVER_FILES="$icd" \
  "$vkprobe" 2>&1) || {
  printf '%s\n' "$output"
  echo "Navi 33 RADV shim probe failed." >&2
  exit 1
}
printf '%s\n' "$output"
if grep -q 'vkCreateDevice: OK' <<< "$output" &&
   grep -q 'compute pipeline: OK' <<< "$output"; then
  echo "PASS (drm-shim only; physical RX 7600 GPU NOT tested)"
else
  echo "FAIL: no successful device and compute pipeline" >&2
  exit 1
fi

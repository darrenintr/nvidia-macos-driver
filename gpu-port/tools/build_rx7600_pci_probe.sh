#!/bin/sh
# Build the read-only IOKit identity probe. Requires macOS + Xcode Command Line Tools.
set -eu
if [ "$(uname -s)" != "Darwin" ]; then
  echo "Run this on macOS, not on the Linux host." >&2
  exit 1
fi
here=$(CDPATH= cd "$(dirname "$0")" && pwd)
out=$(printf '%s' "${1:-$here/rx7600_pci_probe}")
clang -std=c11 -Wall -Wextra -Werror \
  -framework IOKit -framework CoreFoundation \
  "$here/rx7600_pci_probe.c" -o "$out"
printf 'Built %s\n' "$out"

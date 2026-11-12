#!/bin/bash
# build.sh <name> <mcu> <boot_start> [extra -D flags...]  -> out/<name>.elf, prints size
# The bootloader build of the library (boot/build.sh), with its output here.
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="$HERE/out" exec "$HERE/../../../boot/build.sh" "$@"

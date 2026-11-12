#!/bin/bash
# build.sh <name> <mcu> <boot_start> [extra -D flags...]  -> out/<name>.elf, prints size
# SVC_BACK=n places the service table n bytes below the end of flash (simavr's
# ELF reader cannot load the last flash byte, so tests use SVC_BACK=4).
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"; mkdir -p "$HERE/out"
NAME="$1"; MCU="$2"; START="$3"; shift 3
FLASHEND=$(echo "#include <avr/io.h>
FLASHEND" | avr-gcc -mmcu=$MCU -E -P - | tail -n 1 | tr -d ' ()')
SVC=""
case " $* " in *ELFBOOT_FEAT_SERVICE=1*|*ELFBOOT_FEAT_MODULES=1*)
  B=${SVC_BACK:-0}
  SVC="-Wl,-u,bootsvc_table -Wl,-u,bootsvc_info -Wl,--section-start=.bootsvc=$(printf '0x%X' $(( FLASHEND - 11 - B ))) -Wl,--section-start=.bootsvc_info=$(printf '0x%X' $(( FLASHEND - 3 - B )))";;
esac
avr-gcc -std=gnu11 -Os -mmcu="$MCU" -DF_CPU=16000000UL -DBOOT_START="$START" "$@" \
  -Wall -Wextra -Wno-unused-function -Wno-unused-but-set-variable -Wno-array-bounds -ffunction-sections -fdata-sections -fno-jump-tables -mrelax \
  -nostartfiles -mstrict-X ${OPTX:-} -Wl,--gc-sections -Wl,--section-start=.text="$START" $SVC \
  -o "$HERE/out/$NAME.elf" "$HERE/${BOOTSRC:-boot.c}" 2> "$HERE/out/$NAME.log"
rc=$?
if [ $rc -ne 0 ]; then echo "$NAME: BUILD FAILED"; grep -E "error|undefined" "$HERE/out/$NAME.log" | head -n 8; exit 1; fi
grep -E "warning" "$HERE/out/$NAME.log" | head -n 3
set -- $(avr-size "$HERE/out/$NAME.elf" | tail -n 1)
printf '%-34s %-11s text=%5d data=%d bss=%4d\n' "$NAME" "$MCU" "$1" "$2" "$3"

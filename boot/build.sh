#!/bin/bash
# build.sh <name> <mcu> <boot_start> [extra -D flags...]  -> out/<name>.elf, prints size
# Builds the protoduino bootloader (boot.c) against the library in ../src.
#   ./build.sh boot-uno atmega328p default
#   ./build.sh boot-uno-modules atmega328p default -DBOOT_CONF_LOAD_SERVICE=1
# OUT=<dir> chooses where the files go (default: out/ beside this script);
# F_CPU=<hz> the clock (default 16000000).
# <boot_start> "default" takes the boot section boot.conf.h assumes for the part:
# 2 KB up to 64 KB of flash, 4 KB above.
# SVC_BACK=n places the service table n bytes below the end of flash, for a
# simulator that cannot load the last flash byte. The harness here restores
# that byte, so the tests use the real layout.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"; OUT="${OUT:-$HERE/out}"; mkdir -p "$OUT"
LIB="$HERE/../src"
NAME="$1"; MCU="$2"; START="$3"; shift 3
FLASHEND=$(echo "#include <avr/io.h>
FLASHEND" | avr-gcc -mmcu=$MCU -E -P - | tail -n 1 | tr -d ' ()')
DSTART="-DBOOT_CONF_START=$START"
if [ "$START" = default ]; then
  START=$(printf '0x%X' $(( FLASHEND + 1 - (FLASHEND > 0xFFFF ? 4096 : 2048) ))); DSTART=""
fi
SVC=""
B=${SVC_BACK:-0}
case " $* " in *BOOT_CONF_PAGE_SERVICE=1*|*BOOT_CONF_LOAD_SERVICE=1*)
  SVC="-Wl,-u,bootrom_table -Wl,-u,bootrom_info -Wl,--section-start=.bootrom=$(printf '0x%X' $(( FLASHEND - 11 - B ))) -Wl,--section-start=.bootrom_info=$(printf '0x%X' $(( FLASHEND - 3 - B )))";;
esac
avr-gcc -std=gnu11 -Os -mmcu="$MCU" -DF_CPU=${F_CPU:-16000000}UL $DSTART "$@" \
  -I"$HERE" -I"$LIB" \
  -Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-function -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-array-bounds \
  -ffunction-sections -fdata-sections -fno-jump-tables -fno-move-loop-invariants -fno-ivopts -fno-tree-scev-cprop -mrelax \
  -nostartfiles -mstrict-X ${OPTX:-} -Wl,--gc-sections -Wl,--defsym=__do_clear_bss=0 -Wl,--section-start=.text="$START" $SVC \
  -o "$OUT/$NAME.elf" "$HERE/boot.c" 2> "$OUT/$NAME.log"
rc=$?
if [ $rc -ne 0 ]; then echo "$NAME: BUILD FAILED"; grep -E "error|undefined" "$OUT/$NAME.log" | head -n 8; exit 1; fi
grep -E "warning" "$OUT/$NAME.log" | grep -v "pragma message\|#pragma" | head -n 3
set -- $(avr-size "$OUT/$NAME.elf" | tail -n 1)
printf '%-34s %-11s text=%5d data=%d bss=%4d\n' "$NAME" "$MCU" "$1" "$2" "$3"

#!/bin/bash
# build_sketch.sh <mcu> <f_cpu> <sketch.ino> [-D...]  -> tools/out/<sketch>-<mcu>.elf
# Builds a sketch with avr-gcc alone: the library sources it needs, the sketch
# as C++, and tools/core in place of an Arduino core. For parts no Arduino core
# is at hand for, and for the simulator. Prints flash and RAM.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"; LIB="$HERE/../src"; OUT="$HERE/out"; mkdir -p "$OUT"
MCU="$1"; FCPU="$2"; INO="$3"; shift 3
NAME="$(basename "$INO" .ino)-$MCU"
CF="-Os -mmcu=$MCU -DF_CPU=$FCPU -include $HERE/core/core.h -I$LIB -I$LIB/sys -ffunction-sections -fdata-sections -flto -w"
OBJ=""
for f in sys/process.c sys/ipc.c lib/ringb8.c sys/serial/serial0.c sys/process/serial0.c cpu/avr/uart0.c; do
  case "$MCU:$f" in attiny*:*serial*|attiny*:*uart*|attiny*:lib/ringb8.c) continue;; esac
  o="$OUT/$NAME-$(echo $f | tr / _ | sed s/\\.c$//).o"; avr-gcc -std=gnu11 $CF "$@" -c "$LIB/$f" -o "$o"; OBJ="$OBJ $o"
done
avr-gcc -std=gnu11 $CF -c "$HERE/core/core.c" -o "$OUT/$NAME-core.o"
avr-g++ -std=gnu++11 -fno-exceptions -fno-threadsafe-statics -fpermissive $CF "$@" -x c++ -c "$INO" -o "$OUT/$NAME-sketch.o"
avr-gcc $CF -fuse-linker-plugin -Wl,--gc-sections -o "$OUT/$NAME.elf" "$OUT/$NAME-sketch.o" "$OUT/$NAME-core.o" $OBJ
set -- $(avr-size "$OUT/$NAME.elf" | tail -n 1)
printf '%-28s flash %5d B   RAM %4d B\n' "$NAME" $(( $1 + $2 )) $(( $2 + $3 ))

#!/bin/bash
# build_kernel.sh <name> [-D flags]  -> kernel-<name>.elf and a module linked against exactly that kernel build
set -eu
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"; N="$1"; shift
avr-gcc -std=gnu11 -Os -mmcu=atmega328p -DF_CPU=16000000UL -mstrict-X -mrelax -Wall -Wno-unused-function "$@" -o kernel-$N.elf kernel_demo.c
avr-objcopy -O binary -j .text -j .data kernel-$N.elf kernel-$N.bin
KID=$(python3 -c "
import sys
c=0
for b in open(sys.argv[1],'rb').read():
    c^=b<<8
    for _ in range(8): c=((c<<1)^0x1021)&0xFFFF if c&0x8000 else (c<<1)&0xFFFF
print('0x%04x'%c)" kernel-$N.bin)
avr-nm kernel-$N.elf | awk '$2=="T" && $3 ~ /^kernel_/ {printf "PROVIDE(%s = 0x%s);\n", $3, $1}' > kernel-$N-abi.ld
avr-gcc -std=gnu11 -Os -mmcu=atmega328p -nostartfiles -nostdlib -Wl,-T,../module/pd-module.ld -Wl,-T,kernel-$N-abi.ld \
        -Wl,-e,pd_module -Wl,--defsym=__pd_kernel_id=$KID -o module-$N.elf count_mod.c
set -- $(avr-size kernel-$N.elf | tail -n 1)
printf 'kernel %-22s text=%5d data=%3d bss=%4d   id=%s   module file=%d bytes\n' "$N" "$1" "$2" "$3" "$KID" "$(stat -c %s module-$N.elf)"

#!/bin/bash
# sizes.sh - the size tables of the document: bootloader per part and option, kernel per variant.
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"
SMALL="-DELF_CONF_ARCH_CHECK=0 -DBOOT_CONF_IMAGE_CRC=0 -DBOOT_CONF_WINDOW_AFTER=15 -DBOOT_CONF_UART_RESET=0"
echo "== bootloader, bytes of flash (text) and RAM (bss) =="
for m in atmega328p atmega1284p atmega2560; do
  case $m in atmega328p) st=0x7000;; atmega1284p) st=0x1F000;; atmega2560) st=0x3F000;; esac
  ./build.sh "smallest" $m $st $SMALL
  ./build.sh "default" $m $st
  ./build.sh "default + page service" $m $st -DBOOT_CONF_PAGE_SERVICE=1
  ./build.sh "default + load thread" $m $st -DBOOT_CONF_LOAD_SERVICE=1
  ./build.sh "everything" $m $st -DBOOT_CONF_LOAD_SERVICE=1 -DBOOT_CONF_VERIFY=1 -DELF_CONF_EEPROM=1 -DELF_CONF_SIGNATURE=1 -DBOOT_CONF_INFO=1 -DELF_CONF_COALESCE=1 "-DBOOT_CONF_INFO_TEXT=\"$m\""
done
./build.sh "default, virtual reset, soft uart" attiny85 0x17C0 -DBOOT_CONF_VIRTUAL_RESET=1 -DBOOT_CONF_SOFT_UART=1
echo
echo "== one option at a time, ATmega328P, relative to the default =="
base=$(./build.sh x atmega328p 0x7000 | sed -n 's/.*text= *\([0-9]*\).*/\1/p')
one() { local n="$1"; shift; local s=$(./build.sh x atmega328p 0x7000 "$@" | sed -n 's/.*text= *\([0-9]*\).*/\1/p'); printf '%-36s %+5d  (%d)\n' "$n" $(( s - base )) "$s"; }
one "without family-check" -DELF_CONF_ARCH_CHECK=0
one "without image-crc" -DBOOT_CONF_IMAGE_CRC=0
one "without watchdog care (unsafe)" -DBOOT_CONF_WATCHDOG=0
one "wait after every reset" -DBOOT_CONF_WINDOW_AFTER=15
one "wait also after power-on, brown-out" -DBOOT_CONF_WINDOW_AFTER=7
one "without uart-reset" -DBOOT_CONF_UART_RESET=0
one "merge-segments" -DELF_CONF_COALESCE=1
one "page-service" -DBOOT_CONF_PAGE_SERVICE=1
one "load-thread (with page-service)" -DBOOT_CONF_LOAD_SERVICE=1
one "signature" -DELF_CONF_SIGNATURE=1
one "eeprom" -DELF_CONF_EEPROM=1
one "info" -DBOOT_CONF_INFO=1 '-DBOOT_CONF_INFO_TEXT="atmega328p"'
one "verify-at-start" -DBOOT_CONF_VERIFY=1
echo
echo "== test kernel on the real scheduler, ATmega328P (kernel/kernel.c) =="
(cd kernel && for v in none static rom own-bootrom own-optiboot; do set -- $(avr-size kernel-$v.elf | tail -n 1); printf '%-14s text=%5d data=%3d bss=%4d\n' $v $1 $2 $3; done)
echo
echo "== the same kernel for the ATmega2560 =="
(cd kernel && for v in rom2560 own2560; do set -- $(avr-size kernel-$v.elf | tail -n 1); printf '%-14s text=%5d data=%3d bss=%4d\n' $v $1 $2 $3; done)
echo
echo "== library code a kernel links for modules, ATmega328P, bytes of flash per object =="
LIB=../../../src
CF="-std=gnu11 -Os -mmcu=atmega328p -DF_CPU=16000000UL -include kernel/core.h -I$LIB -I$LIB/sys -ffunction-sections -w"
obj() { avr-gcc $CF "${@:2}" -c "$1" -o out/o.o 2>/dev/null && avr-size out/o.o | tail -n 1 | awk '{print $1}'; }
printf '%-56s %5s\n' "cpu/avr/flash.c   whoever is installed (the default)"  "$(obj $LIB/cpu/avr/flash.c)"
printf '%-56s %5s\n' "cpu/avr/flash.c   through the bootloader table"        "$(obj $LIB/cpu/avr/flash.c -DFLASH_CONF_DRIVER=2)"
printf '%-56s %5s\n' "cpu/avr/flash.c   through Optiboot do_spm"             "$(obj $LIB/cpu/avr/flash.c -DFLASH_CONF_DRIVER=3)"
printf '%-56s %5s\n' "sys/elf.c         stream (own thread only)"            "$(obj $LIB/sys/elf.c)"
printf '%-56s %5s\n' "sys/elf/load.c    load thread (own thread only)"       "$(obj $LIB/sys/elf/load.c)"
CF="${CF/atmega328p/atmega2560}"
printf '%-56s %5s\n' "cpu/avr/flash.c   the default, ATmega2560 (far call)"  "$(obj $LIB/cpu/avr/flash.c)"
echo
echo "== what the headers of the sketch add to a kernel, ATmega328P: their functions in kernel-rom.elf =="
avr-nm -S --size-sort kernel/kernel-rom.elf | python3 -c '
import sys, re
t = 0
for line in sys.stdin:
    f = line.split()
    if len(f) == 4 and f[2] in "tT" and re.match(r"(module_|loader_|process_thread_loader)", f[3]):
        n = int(f[1], 16); t += n
        print("%-56s %5d" % (f[3], n))
print("%-56s %5d" % ("together (sys/module.h, mod/loader.h)", t))'

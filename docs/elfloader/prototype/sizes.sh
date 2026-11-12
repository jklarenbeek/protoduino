#!/bin/bash
# sizes.sh - the size tables of the document: bootloader per part and option, kernel per variant.
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"
SMALL="-DELF_CONF_ARCH_CHECK=0 -DBOOT_CONF_IMAGE_CRC=0 -DELF_CONF_COALESCE=0 -DELF_CONF_MAX_SEGMENTS=2"
echo "== bootloader, bytes of flash (text) and RAM (bss) =="
for m in atmega328p atmega1284p atmega2560; do
  case $m in atmega328p) st=0x7000;; atmega1284p) st=0x1F000;; atmega2560) st=0x3F000;; esac
  ./build.sh "smallest" $m $st $SMALL
  ./build.sh "default" $m $st
  ./build.sh "default + page service" $m $st -DBOOT_CONF_PAGE_SERVICE=1
  ./build.sh "default + load thread" $m $st -DBOOT_CONF_LOAD_SERVICE=1
  ./build.sh "everything" $m $st -DBOOT_CONF_LOAD_SERVICE=1 -DBOOT_CONF_VERIFY=1 -DELF_CONF_EEPROM=1 -DELF_CONF_SIGNATURE=1 -DBOOT_CONF_INFO=1 "-DBOOT_CONF_INFO_TEXT=\"$m\""
done
./build.sh "default, virtual reset, soft uart" attiny85 0x17C0 -DBOOT_CONF_VIRTUAL_RESET=1 -DBOOT_CONF_SOFT_UART=1
echo
echo "== one option at a time, ATmega328P, relative to the default =="
base=$(./build.sh x atmega328p 0x7000 | sed -n 's/.*text= *\([0-9]*\).*/\1/p')
one() { local n="$1"; shift; local s=$(./build.sh x atmega328p 0x7000 "$@" | sed -n 's/.*text= *\([0-9]*\).*/\1/p'); printf '%-28s %+5d  (%d)\n' "$n" $(( s - base )) "$s"; }
one "without family-check" -DELF_CONF_ARCH_CHECK=0
one "without merge-segments" -DELF_CONF_COALESCE=0 -DELF_CONF_MAX_SEGMENTS=2
one "without image-crc" -DBOOT_CONF_IMAGE_CRC=0
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
echo "== library code a kernel links for modules, ATmega328P, bytes of flash per object =="
OVL=../overlay; LIB=../../../src
CF="-std=gnu11 -Os -mmcu=atmega328p -DF_CPU=16000000UL -include kernel/core.h -I$OVL/src -I$LIB -I$LIB/sys -DELF_CONF_WINDOW_LO=1 -ffunction-sections -w"
obj() { avr-gcc $CF "${@:2}" -c "$1" -o out/o.o 2>/dev/null && avr-size out/o.o | tail -n 1 | awk '{print $1}'; }
printf '%-52s %5s\n' "sys/module.c            start modules in flash"       "$(obj $OVL/src/sys/module.c -DMODULE_CONF_SUPPORT=1)"
printf '%-52s %5s\n' "sys/module.c            also enter them into the table" "$(obj $OVL/src/sys/module.c -DMODULE_CONF_SUPPORT=2)"
printf '%-52s %5s\n' "sys/process/loader.c    thread of the bootloader"     "$(obj $OVL/src/sys/process/loader.c -DMODULE_CONF_SUPPORT=2 -DLOADER_CONF_THREAD=1)"
printf '%-52s %5s\n' "sys/process/loader.c    own thread"                   "$(obj $OVL/src/sys/process/loader.c -DMODULE_CONF_SUPPORT=2 -DLOADER_CONF_THREAD=2)"
printf '%-52s %5s\n' "cpu/avr/flash.c         through the bootloader table"  "$(obj $OVL/src/cpu/avr/flash.c -DFLASH_CONF_DRIVER=2)"
printf '%-52s %5s\n' "cpu/avr/flash.c         through Optiboot do_spm"       "$(obj $OVL/src/cpu/avr/flash.c -DFLASH_CONF_DRIVER=3)"
printf '%-52s %5s\n' "sys/elf.c               stream (own thread only)"      "$(obj $OVL/src/sys/elf.c)"
printf '%-52s %5s\n' "sys/elf/load.c          load thread (own thread only)" "$(obj $OVL/src/sys/elf/load.c)"

#!/bin/bash
# make_fixtures.sh - build the files the simulated tests send, and the demo kernels.
# PROTODUINO defaults to the repository this file lives in.
# The Optiboot image is not part of this repository: copy optiboot_atmega328.hex
# (version 8 or later, from github.com/Optiboot/optiboot, optiboot/bootloaders/optiboot/)
# into fixtures/ to run scenarios M2, M4 and M8.
set -eu
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"
PD="${PROTODUINO:-$HERE/../../..}"
mkdir -p fixtures; F=fixtures
for M in atmega328p atmega2560; do avr-gcc -Os -g -mmcu=$M -DF_CPU=16000000UL -o $F/app-$M.elf probes/elf_layout_app.c; done
avr-gcc -Os -mmcu=atmega168p -o $F/app168.elf probes/elf_layout_app.c
avr-gcc -Os -mmcu=atmega2560 -o $F/big2560.elf big.c && avr-strip -o $F/big2560-stripped.elf $F/big2560.elf
avr-gcc -std=gnu11 -Os -mmcu=atmega328p -DF_CPU=16000000UL -DBOOTROM_CONF_TABLE=0x7FF0 -I../overlay/src -I$PD/src -o $F/svc_app.elf svc_app.c ../overlay/src/cpu/avr/flash.c
head -c 4000 /dev/urandom > $F/junk.bin
# protoduino example 12, unmodified, with a three-line stand-in for the Arduino core
C="-Os -g -mmcu=atmega328p -DF_CPU=16000000UL -I$PD/src -I$PD/src/sys"
avr-gcc -std=gnu11 $C -c $PD/src/cpu/avr/uart0.c -o $F/uart0.o 2>/dev/null
avr-gcc -std=gnu11 $C -c $PD/src/lib/ringb8.c -o $F/ringb8.o
avr-g++ -std=gnu++11 $C -x c++ -c $PD/examples/12-sys-uart-echo/12-sys-uart-echo.ino -o $F/ex12.o
avr-g++ -std=gnu++11 $C -c probes/arduino_shim.cpp -o $F/shim.o
avr-g++ $C -o $F/ex12-full.elf $F/ex12.o $F/shim.o $F/uart0.o $F/ringb8.o && avr-strip -o $F/ex12-stripped.elf $F/ex12-full.elf
# test kernels on the real scheduler, and for each a module linked against exactly that build
(cd kernel && ./build_kernel.sh none -DMODULE_CONF_SUPPORT=0 && ./build_kernel.sh static -DMODULE_CONF_SUPPORT=1 \
  && ./build_kernel.sh rom -DMODULE_CONF_SUPPORT=2 -DLOADER_CONF_THREAD=1 -DFLASH_CONF_DRIVER=2 -DBOOTROM_CONF_TABLE=0x7FF0 \
  && MODULE_BASE=0x6800 ./build_kernel.sh rom-slot1 -DMODULE_CONF_SUPPORT=2 -DLOADER_CONF_THREAD=1 -DFLASH_CONF_DRIVER=2 -DBOOTROM_CONF_TABLE=0x7FF0 \
  && ./build_kernel.sh own-bootrom -DMODULE_CONF_SUPPORT=2 -DLOADER_CONF_THREAD=2 -DFLASH_CONF_DRIVER=2 -DBOOTROM_CONF_TABLE=0x7FF0 \
  && ./build_kernel.sh own-optiboot -DMODULE_CONF_SUPPORT=2 -DLOADER_CONF_THREAD=2 -DFLASH_CONF_DRIVER=3)
echo "fixtures ready in $HERE/fixtures"

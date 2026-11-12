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
for M in atmega328p atmega1284p atmega2560; do avr-gcc -Os -g -mmcu=$M -DF_CPU=16000000UL -o $F/app-$M.elf probes/elf_layout_app.c; done
avr-gcc -Os -mmcu=atmega168p -o $F/app168.elf probes/elf_layout_app.c
avr-gcc -Os -mmcu=atmega1284 -o $F/app1284.elf probes/elf_layout_app.c     # same core family as the 1284P, other device
# simavr's ATmega2560 core has no signature row (finding S12 of the document), so its file goes without the section
avr-objcopy -R .signature $F/app-atmega2560.elf $F/app-atmega2560-nosig.elf
avr-gcc -Os -mmcu=atmega2560 -o $F/big2560.elf big.c && avr-strip -o $F/big2560-stripped.elf $F/big2560.elf
avr-gcc -std=gnu11 -Os -mmcu=atmega328p -DF_CPU=16000000UL -I../overlay/src -I$PD/src -o $F/svc_app.elf svc_app.c ../overlay/src/cpu/avr/flash.c
SVC2560="-std=gnu11 -Os -mmcu=atmega2560 -DF_CPU=16000000UL -I../overlay/src -I$PD/src"
avr-gcc $SVC2560 -o $F/svc_app2560.elf svc_app.c ../overlay/src/cpu/avr/flash.c
avr-gcc $SVC2560 -DSVC_PAGE=0x20000UL -o $F/svc_app2560-far.elf svc_app.c ../overlay/src/cpu/avr/flash.c
for M in atmega328p atmega2560; do avr-gcc -Os -mmcu=$M -DF_CPU=16000000UL -o $F/wdt_app-$M.elf wdt_app.c; done
avr-gcc -Os -mmcu=atmega328p -DF_CPU=16000000UL -DBOOT_START=0x7800UL -o $F/jump_app.elf jump_app.c
# firmware with four and with five flash segments
SEG="-Os -mmcu=atmega328p -DF_CPU=16000000UL -Wl,--section-start=.block1=0x5000 -Wl,--section-start=.block2=0x5800"
avr-gcc $SEG -o $F/seg4.elf seg_app.c
avr-gcc $SEG -DFIVE -Wl,--section-start=.block3=0x6000 -o $F/seg5.elf seg_app.c
# images that report the reset cause the bootloader hands them; stripped, for the run inside protosim
for T in S T; do avr-gcc -Os -mmcu=atmega328p -DF_CPU=16000000UL "-DTAG='$T'" -o $F/cause_app-$T.elf cause_app.c && avr-strip $F/cause_app-$T.elf; done
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
  && ./build_kernel.sh rom -DMODULE_CONF_SUPPORT=2 -DLOADER_CONF_THREAD=1 -DFLASH_CONF_DRIVER=2 \
  && MODULE_BASE=0x6800 ./build_kernel.sh rom-slot1 -DMODULE_CONF_SUPPORT=2 -DLOADER_CONF_THREAD=1 -DFLASH_CONF_DRIVER=2 \
  && ./build_kernel.sh own-bootrom -DMODULE_CONF_SUPPORT=2 -DLOADER_CONF_THREAD=2 -DFLASH_CONF_DRIVER=2 \
  && ./build_kernel.sh own-optiboot -DMODULE_CONF_SUPPORT=2 -DLOADER_CONF_THREAD=2 -DFLASH_CONF_DRIVER=3 \
  && MCU=atmega2560 ./build_kernel.sh rom2560 -DMODULE_CONF_SUPPORT=2 -DLOADER_CONF_THREAD=1 -DFLASH_CONF_DRIVER=2 \
  && MCU=atmega2560 ./build_kernel.sh own2560 -DMODULE_CONF_SUPPORT=2 -DLOADER_CONF_THREAD=2 -DFLASH_CONF_DRIVER=2)
echo "fixtures ready in $HERE/fixtures"

#!/bin/bash
# run_modules.sh - run-time module loading scenarios M1..M8 (ATmega328P, simavr).
# Needs kernel/build_kernel.sh to have been run for: none static rom linked-svc linked-optiboot
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"
K="$HERE/kernel"; E="${FIXTURES:-$HERE/fixtures}"
SVC_BACK=4 ./build.sh "328P modules" atmega328p 0x7800 -DELFBOOT_FEAT_MODULES=1 >/dev/null
SVC_BACK=4 ./build.sh "328P service" atmega328p 0x7800 -DELFBOOT_FEAT_SERVICE=1 >/dev/null
./build.sh "328P base" atmega328p 0x7800 >/dev/null
run() { echo; echo "=== $1 ==="; shift; ./sim_harness "$@" 2>&1 | grep -v "^Loaded\|would overflow"; echo "exit=${PIPESTATUS[0]}"; }
run "M1 protoduino bootloader with the module loader; the kernel calls it (ROM loader)" atmega328p "out/328P modules.elf" $K/module-rom.elf --kernel $K/kernel-rom.elf --send $K/module-rom.elf
run "M2 stock Optiboot 8.3; the kernel carries its own loader and writes through do_spm" atmega328p $E/optiboot_atmega328.hex $K/module-linked-optiboot.elf --kernel $K/kernel-linked-optiboot.elf --send $K/module-linked-optiboot.elf
run "M3 protoduino bootloader with only the page-write service; the kernel carries its own loader" atmega328p "out/328P service.elf" $K/module-linked-svc.elf --kernel $K/kernel-linked-svc.elf --send $K/module-linked-svc.elf
run "M4 kernel built to use only modules already in flash; module programmed with the firmware (Optiboot)" atmega328p $E/optiboot_atmega328.hex $K/module-static.elf --kernel $K/kernel-static.elf --preinstall --no-send
run "M5 a module linked against another kernel build is offered" atmega328p "out/328P modules.elf" $K/module-static.elf --kernel $K/kernel-rom.elf --send $K/module-static.elf
run "M6 a firmware image is offered to the module loader" atmega328p "out/328P modules.elf" $E/app-atmega328p.elf --kernel $K/kernel-rom.elf --send $E/app-atmega328p.elf
run "M7 kernel built to load, but the bootloader offers no service" atmega328p "out/328P base.elf" $K/module-rom.elf --kernel $K/kernel-rom.elf --no-send
run "M8 kernel built without module support, module present in flash" atmega328p $E/optiboot_atmega328.hex $K/module-static.elf --kernel $K/kernel-none.elf --preinstall --no-send

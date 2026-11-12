#!/bin/bash
# run_modules.sh - run-time module loading scenarios M1..M9 (ATmega328P) and M10..M12 (ATmega2560), simavr.
# The kernel is kernel/kernel.c: a sketch on the real protoduino scheduler.
# Needs make_fixtures.sh to have been run.
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"
K="$HERE/kernel"; E="${FIXTURES:-$HERE/fixtures}"
# a 2 KB boot section that offers the load thread: the configuration fit.sh selects for this block.
# The simulator needs the service table four bytes lower (SVC_BACK); it still fits.
SVC_BACK=4 ./build.sh "328P rom" atmega328p 0x7800 -DBOOT_CONF_LOAD_SERVICE=1 >/dev/null
SVC_BACK=4 ./build.sh "328P page" atmega328p 0x7800 -DBOOT_CONF_PAGE_SERVICE=1 >/dev/null
./build.sh "328P base" atmega328p 0x7800 >/dev/null
run() { echo; echo "=== $1 ==="; shift; ./sim_harness "$@" 2>&1 | grep -v "^Loaded\|would overflow"; echo "exit=${PIPESTATUS[0]}"; }
run "M1 protoduino bootloader (2 KB) that offers its load thread; the loader process schedules it" atmega328p "out/328P rom.elf" $K/module-rom.elf --kernel $K/kernel-rom.elf --send $K/module-rom.elf
run "M2 stock Optiboot 8.3; the kernel links its own load thread and writes pages through do_spm" atmega328p $E/optiboot_atmega328.hex $K/module-own-optiboot.elf --kernel $K/kernel-own-optiboot.elf --send $K/module-own-optiboot.elf
run "M3 protoduino bootloader with only the page service; the kernel links its own load thread" atmega328p "out/328P page.elf" $K/module-own-bootrom.elf --kernel $K/kernel-own-bootrom.elf --send $K/module-own-bootrom.elf
run "M4 kernel that only starts modules already in flash; module programmed with the firmware (Optiboot)" atmega328p $E/optiboot_atmega328.hex $K/module-static.elf --kernel $K/kernel-static.elf --preinstall --no-send
run "M5 a module linked against another kernel build is offered" atmega328p "out/328P rom.elf" $K/module-static.elf --kernel $K/kernel-rom.elf --send $K/module-static.elf --expect-refusal
run "M6 a firmware image is offered to the loader process" atmega328p "out/328P rom.elf" $E/app-atmega328p.elf --kernel $K/kernel-rom.elf --send $E/app-atmega328p.elf --expect-refusal
run "M7 kernel built to load, but the bootloader offers no service" atmega328p "out/328P base.elf" $K/module-rom.elf --kernel $K/kernel-rom.elf --no-send
run "M8 kernel built without module support, module present in flash" atmega328p $E/optiboot_atmega328.hex $K/module-static.elf --kernel $K/kernel-none.elf --preinstall --no-send
run "M9 a module runs from slot 0; a second one, linked for slot 1, is loaded beside it" atmega328p "out/328P rom.elf" $K/module-rom.elf --kernel $K/kernel-rom.elf --preinstall --send $K/module-rom-slot1.elf
# ATmega2560: the bootloader fills the 4 KB boot section, above 128 KB, so the kernel reaches its
# services through the far call of sys/bootrom.h. The kernel is built against a copy of the library
# whose sys/clock.h accepts the part (see kernel/build_kernel.sh).
FULL='-DBOOT_CONF_LOAD_SERVICE=1 -DBOOT_CONF_VERIFY=1 -DELF_CONF_EEPROM=1 -DELF_CONF_SIGNATURE=1 -DBOOT_CONF_INFO=1'
SVC_BACK=4 ./build.sh "2560 full" atmega2560 default $FULL '-DBOOT_CONF_INFO_TEXT="atmega2560"' >/dev/null
SVC_BACK=4 ./build.sh "2560 page" atmega2560 default -DBOOT_CONF_PAGE_SERVICE=1 >/dev/null
run "M10 2560: bootloader with every option in the 4 KB section; the loader process schedules its load thread" atmega2560 "out/2560 full.elf" $K/module-rom2560.elf --kernel $K/kernel-rom2560.elf --send $K/module-rom2560.elf
run "M11 2560: bootloader with only the page service; the kernel links its own load thread" atmega2560 "out/2560 page.elf" $K/module-own2560.elf --kernel $K/kernel-own2560.elf --send $K/module-own2560.elf
run "M12 2560: a module linked against another kernel build is offered" atmega2560 "out/2560 full.elf" $K/module-own2560.elf --kernel $K/kernel-rom2560.elf --send $K/module-own2560.elf --expect-refusal
echo
echo "=== experiment: the kernel and module of M1, built against a copy of process.c whose process_run() dispatches one event after the polls (FAIR=1) ==="
(cd kernel && FAIR=1 ./build_kernel.sh fair -DMODULE_CONF_SUPPORT=2 -DLOADER_CONF_THREAD=1 -DFLASH_CONF_DRIVER=2 -DBOOTROM_CONF_TABLE=0x7FF0 >/dev/null)
./sim_harness atmega328p "out/328P rom.elf" $K/module-fair.elf --kernel $K/kernel-fair.elf --send $K/module-fair.elf 2>&1 | grep -v "^Loaded\|would overflow"; echo "exit=${PIPESTATUS[0]}"

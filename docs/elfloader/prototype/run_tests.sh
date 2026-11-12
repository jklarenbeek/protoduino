#!/bin/bash
# run_tests.sh - the simulated scenarios T1..T13. Needs sim_harness, host tools and the test files built (see README).
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"
E="${FIXTURES:-$HERE/fixtures}"
run() { echo; echo "=== $1 ==="; shift; ./sim_harness "$@" 2>&1 | grep -v "^Loaded\|would overflow" | sed 's/\x1b\[[0-9;]*m//g; s|'"$E"'/||g'; echo "exit=${PIPESTATUS[0]}"; }
ALL='-DELFBOOT_FEAT_VERIFY_BOOT=1 -DELFLD_FEAT_EEPROM=1 -DELFLD_FEAT_SIGNATURE=1 -DELFBOOT_FEAT_INFO=1 -DELFBOOT_FEAT_MODULES=1 -DELFLD_CONF_MAX_SEGMENTS=4'
./build.sh "328P base" atmega328p 0x7800 >/dev/null
./build.sh "328P all" atmega328p 0x7000 $ALL '-DELFBOOT_INFO_STRING="m328p ps=128 hi=6f80"' >/dev/null
SVC_BACK=4 ./build.sh "328P service" atmega328p 0x7800 -DELFBOOT_FEAT_SERVICE=1 >/dev/null
./build.sh "328P nopad" atmega328p 0x7800 -DELFBOOT_FEAT_LANDING_PAD=0 >/dev/null
./build.sh "2560 base" atmega2560 0x3F000 >/dev/null
run "T1 328P: unmodified avr-gcc ELF, clean line" atmega328p "out/328P base.elf" $E/app-atmega328p.elf
run "T2 328P: block 3 corrupted, ACK of block 5 lost" atmega328p "out/328P base.elf" $E/app-atmega328p.elf --corrupt 3 --drop-ack 5
run "T3 328P: protoduino example 12, full ELF (debug info included)" atmega328p "out/328P base.elf" $E/ex12-full.elf
run "T4 328P: protoduino example 12, stripped ELF" atmega328p "out/328P base.elf" $E/ex12-full.elf --send $E/ex12-stripped.elf
run "T5 328P: empty chip offered an ATmega2560 file" atmega328p "out/328P base.elf" $E/app-atmega2560.elf --expect-refusal
run "T6 328P: empty chip offered random bytes" atmega328p "out/328P base.elf" $E/app-atmega328p.elf --send $E/junk.bin --expect-refusal
run "T7 328P: install, reset, offer a 2560 file; old image must survive" atmega328p "out/328P base.elf" $E/app-atmega328p.elf --then-send $E/app-atmega2560.elf --max-mcycles 400
run "T8 328P all features: image with EEPROM data and matching signature" atmega328p "out/328P all.elf" $E/app-atmega328p.elf
run "T9 328P all features: same core family, wrong device signature (ATmega168P file)" atmega328p "out/328P all.elf" $E/app168.elf --expect-refusal
run "T10 328P: application programs a page through the loader's service table" atmega328p "out/328P service.elf" $E/svc_app.elf --check-page 0x6000
run "T11 2560: small image" atmega2560 "out/2560 base.elf" $E/app-atmega2560.elf
run "T12 2560: 106 KB image reaching far flash" atmega2560 "out/2560 base.elf" $E/big2560.elf --send $E/big2560-stripped.elf --max-mcycles 30000
echo; echo "=== T13 328P: execution started in erased flash below the loader ==="
for A in 0x6000 0x6002; do for R in 0x00 0x80; do ./sim_harness atmega328p "out/328P base.elf" $E/app-atmega328p.elf --fallthrough $A $R 2>&1 | grep -v "^Loaded"; done; done
echo "-- control: loader built without the landing pad --"
for A in 0x6000 0x6002; do for R in 0x00 0x80; do ./sim_harness atmega328p "out/328P nopad.elf" $E/app-atmega328p.elf --fallthrough $A $R 2>&1 | grep -v "^Loaded" | sed 's/\x1b\[[0-9;]*m//g'; done; done

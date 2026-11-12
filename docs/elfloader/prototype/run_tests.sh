#!/bin/bash
# run_tests.sh - the simulated scenarios T1..T21. Needs sim_harness, host tools and the test files built (see README).
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"
E="${FIXTURES:-$HERE/fixtures}"
run() { echo; echo "=== $1 ==="; shift; ./sim_harness "$@" 2>&1 | grep -v "^Loaded\|would overflow" | sed 's/\x1b\[[0-9;]*m//g; s|'"$E"'/||g'; echo "exit=${PIPESTATUS[0]}"; }
ALL='-DBOOT_CONF_VERIFY=1 -DELF_CONF_EEPROM=1 -DELF_CONF_SIGNATURE=1 -DBOOT_CONF_INFO=1 -DBOOT_CONF_LOAD_SERVICE=1 -DELF_CONF_MAX_SEGMENTS=4'
./build.sh "328P base" atmega328p 0x7800 >/dev/null
./build.sh "328P all" atmega328p 0x7000 $ALL '-DBOOT_CONF_INFO_TEXT="m328p ps=128 hi=6f80"' >/dev/null
SVC_BACK=4 ./build.sh "328P service" atmega328p 0x7800 -DBOOT_CONF_PAGE_SERVICE=1 >/dev/null
./build.sh "328P nopad" atmega328p 0x7800 -DBOOT_CONF_LANDING_PAD=0 >/dev/null
./build.sh "2560 base" atmega2560 0x3F000 >/dev/null
# the large parts take the 4 KB boot section ("default"), and there every option fits
FULL='-DBOOT_CONF_LOAD_SERVICE=1 -DBOOT_CONF_VERIFY=1 -DELF_CONF_EEPROM=1 -DELF_CONF_SIGNATURE=1 -DBOOT_CONF_INFO=1'
./build.sh "1284P base" atmega1284p default >/dev/null
SVC_BACK=4 ./build.sh "1284P full" atmega1284p default $FULL '-DBOOT_CONF_INFO_TEXT="atmega1284p"' >/dev/null
SVC_BACK=4 ./build.sh "2560 full" atmega2560 default $FULL '-DBOOT_CONF_INFO_TEXT="atmega2560"' >/dev/null
SVC_BACK=4 ./build.sh "2560 service" atmega2560 default -DBOOT_CONF_PAGE_SERVICE=1 >/dev/null
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
run "T14 1284P: small image, default bootloader in the 4 KB section" atmega1284p "out/1284P base.elf" $E/app-atmega1284p.elf
run "T15 1284P, every option: '?' answered, EEPROM data, matching signature; then one bit of the image is flipped" atmega1284p "out/1284P full.elf" $E/app-atmega1284p.elf --ask-info --damage 0x40
run "T16 1284P, every option: same core family, wrong device signature (ATmega1284 file)" atmega1284p "out/1284P full.elf" $E/app-atmega1284p.elf --send $E/app1284.elf --expect-refusal
run "T17 1284P, default bootloader (no check at start): the same flipped bit; control for T15" atmega1284p "out/1284P base.elf" $E/app-atmega1284p.elf --damage-control 0x40
run "T18 2560, every option in the 4 KB section: '?' answered, EEPROM data; then one bit of the image is flipped" atmega2560 "out/2560 full.elf" $E/app-atmega2560-nosig.elf --ask-info --damage 0x40
run "T19 2560, every option: 106 KB image; then one bit above 64 KB is flipped" atmega2560 "out/2560 full.elf" $E/big2560.elf --send $E/big2560-stripped.elf --max-mcycles 30000 --damage 0x15000
run "T20 2560: application programs a page through the service table of the 4 KB boot section" atmega2560 "out/2560 service.elf" $E/svc_app2560.elf --check-page 0x6000
run "T21 2560: the same for a page above 64 KB" atmega2560 "out/2560 service.elf" $E/svc_app2560-far.elf --check-page 0x20000
echo; echo "--- probe timing: no sender, when each 'C' arrives (ATmega328P, then ATmega2560)"
./sim_harness atmega328p "out/328P base.elf" $E/app-atmega328p.elf --probe-times 2>&1 | grep "^probe"
./sim_harness atmega2560 "out/2560 base.elf" $E/app-atmega2560.elf --probe-times 2>&1 | grep "^probe"

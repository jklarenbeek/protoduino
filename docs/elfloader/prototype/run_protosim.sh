#!/bin/bash
# run_protosim.sh - the bootloader inside protosim itself, scenarios P1..P5.
# protosim runs the default bootloader on an empty ATmega328P with its serial port on a PTY;
# protosim_e2e.py sends files with XMODEM and resets the part through protosim: SIGUSR1 pulls
# the reset pin, SIGUSR2 cycles the power. Needs protosim with reset causes (its --reset-vector),
# python3, and make_fixtures.sh to have been run. Linux and macOS.
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE"
SIM="${PROTOSIM:-$HERE/../../../../protosim/bin/protosim}"
./build.sh "328P base" atmega328p default >/dev/null
python3 protosim_e2e.py "$SIM" "out/328P base.elf" fixtures/cause_app-S.elf fixtures/cause_app-T.elf

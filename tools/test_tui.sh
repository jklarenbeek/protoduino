#!/bin/bash
# test_tui.sh - the TUI in the simulator.
#
#   tools/test_tui.sh
#
# 22-tui-render is built twice: as it is, and with -DTUI22_FULL_REDRAW, which
# redraws everything in every frame. Both run in protosim; every screen of the
# first has to be the screen of the second, cell for cell, and frames that
# change little have to send little. Then 25-tui-robust runs its own checks.
# Needs avr-gcc, node and protosim (PROTOSIM=<path to bin/protosim>).
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE/.."
SIM="${PROTOSIM:-$HERE/../../protosim/bin/protosim}"
[ -x "$SIM" ] || { echo "no protosim at $SIM: build it, or set PROTOSIM"; exit 2; }
O=tools/out; mkdir -p $O; fails=0
RUN="-m atmega328p -f 16000000 --warp --exit-on-uart <DONE> --max-steps 900000000"

build() { bash tools/build_sketch.sh "$@" 2>/dev/null | tail -n 1; }

# the defaults differ: a log of 6 lines on a part with 2 KB of RAM, 16 on the Mega
for mcu in atmega328p atmega2560; do
  echo "== 22-tui-render: partial redraws against full ones, $mcu =="
  RUN="-m $mcu -f 16000000 --warp --exit-on-uart <DONE> --max-steps 900000000"
  build $mcu 16000000 examples/7-text-ui/22-tui-render/22-tui-render.ino -DTUI22_FULL_REDRAW
  mv $O/22-tui-render-$mcu.elf $O/22-tui-render-full-$mcu.elf
  build $mcu 16000000 examples/7-text-ui/22-tui-render/22-tui-render.ino
  "$SIM" $O/22-tui-render-full-$mcu.elf $RUN --uart0-out $O/tui-full-$mcu.txt > /dev/null 2>&1
  "$SIM" $O/22-tui-render-$mcu.elf $RUN --uart0-out $O/tui-$mcu.txt > /dev/null 2>&1
  # B: nothing changed; D: one line more in the log; H: the status bar, rewritten in place
  if node tools/vt-compare.mjs 12 40 $O/tui-$mcu.txt $O/tui-full-$mcu.txt --budget B=0 --budget D=24 --budget H=80; then
    echo "[PASS] every screen as a full redraw draws it, in the bytes the changes need"
  else
    echo "[FAIL] the screens or the byte counts above"; fails=$((fails + 1))
  fi
done
RUN="-m atmega328p -f 16000000 --warp --exit-on-uart <DONE> --max-steps 900000000"

echo "== 25-tui-robust, ATmega328P =="
build atmega328p 16000000 examples/7-text-ui/25-tui-robust/25-tui-robust.ino
"$SIM" $O/25-tui-robust-atmega328p.elf $RUN --uart0-out $O/tui-robust.txt > /dev/null 2>&1
if grep -q "<DONE>" $O/tui-robust.txt && ! grep -q "^FAIL" $O/tui-robust.txt; then
  echo "[PASS] $(grep -c '^PASS' $O/tui-robust.txt) checks"
else
  echo "[FAIL]"; grep "^FAIL" $O/tui-robust.txt | sed 's/^/       /'; fails=$((fails + 1))
fi

echo
[ $fails -eq 0 ] && echo "all passed" || echo "$fails failed"
exit $fails

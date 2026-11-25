#!/bin/bash
# test_rp2.sh - protoduino on the RP2040, run in protosim (its Bramble target)
#
#   PICO_SDK_PATH=<pico-sdk> bash tools/test_rp2.sh
#
# 1. The portable examples print the same bytes on an RP2040 as on an
#    ATmega328P: the AVR run is the reference.
# 2. The shell and the monitor answer over the pseudo terminal, with the
#    steps the AVR tests take.
# 3. The scheduler sleeps between interrupts (WFE).
# 4. The terminal UI draws every screen as a full redraw does.
# 5. Pins, the ADC and the supply.
# 6. The RP2350 builds (protosim runs it only experimentally).
#
# Needs avr-gcc, arm-none-eabi-gcc, cmake, node and protosim built with
# Bramble beside this repository (PROTOSIM=<path to bin/protosim> for
# another place).
cd "$(dirname "$0")/.."
HERE="$(pwd)/tools"
SIM="${PROTOSIM:-$HERE/../../protosim/bin/protosim}"
[ -x "$SIM" ] || { echo "no protosim at $SIM: build it, or set PROTOSIM"; exit 2; }
"$SIM" 2>&1 | grep -q "^  bramble .*RP2040" || { echo "$SIM is built without Bramble"; exit 2; }
[ -n "$PICO_SDK_PATH" ] || { echo "set PICO_SDK_PATH to a checkout of the Pico SDK 2.x"; exit 2; }
O=tools/out; mkdir -p $O; fails=0

avr() { bash tools/build_sketch.sh atmega328p 16000000 "$@" 2>/dev/null | tail -n 1; }
rp2() { local chip=$1; shift; bash tools/build_rp2.sh $chip "$@" 2>&1 | tail -n 1; }
pass() { echo "[PASS] $1"; }
fail() { echo "[FAIL] $1"; fails=$((fails + 1)); }
talk() {
  local name="$1" args="$2"; shift 2
  if node tools/talk.mjs --sim "$SIM $args" "$@" > "$O/talk.log" 2>&1; then
    pass "$name"
  else
    fail "$name"; tr -d '\r' < "$O/talk.log" | tail -n 12 | sed 's/^/       /'
  fi
}

echo "== 1. the same output as on an ATmega328P =="
SAME="1-protothreads/01-pt-wait-one-basic 1-protothreads/02-pt-wait-one-block 1-protothreads/03-pt-wait-until-while
      1-protothreads/04-pt-finally-basic 1-protothreads/05-pt-finally-extended 1-protothreads/06-pt-catch-basic
      1-protothreads/07-pt-yield-basic 1-protothreads/08-pt-yield-foreach 1-protothreads/10-pt-wait-for-timer
      2-library/11-lib-ringb8-basic 3-processes/22-process-serial 4-modules/31-filter 4-modules/33-your-own-module
      7-text-ui/23-ansi-parser 7-text-ui/24-ansi-keys 7-text-ui/26-rune-conformance"
for e in $SAME; do
  n=$(basename $e); f=examples/$e/$n.ino
  avr $f > /dev/null; rp2 rp2040 $f > /dev/null
  for t in atmega328p rp2040; do
    "$SIM" $O/$n-$t.elf --warp --uart0-in $'hello\rabc\r' --uart0-out $O/same-$n-$t.txt --run-ms 4000 > /dev/null 2>&1
  done
  if [ -s $O/same-$n-rp2040.txt ] && cmp -s $O/same-$n-atmega328p.txt $O/same-$n-rp2040.txt; then
    pass "$n: the same $(wc -c < $O/same-$n-rp2040.txt) bytes"
  else
    fail "$n: $(wc -c < $O/same-$n-atmega328p.txt) bytes on the AVR, $(wc -c < $O/same-$n-rp2040.txt) on the RP2040"
  fi
done

echo "== 2. the shell over the pseudo terminal =="
rp2 rp2040 examples/5-systems/40-shell/40-shell.ino
talk "processes, modules, pipes, pause, kill" "$O/40-shell-rp2040.elf" \
  ps expect:"ticker   W 3" mods expect:"2 upper -" "run 2" "join upper serial:0" "write upper hello" expect:HELLO \
  ipc expect:"upper > serial:0" "stop blink" ps expect:"blink    P 3" "cont blink" "kill upper" ps expect:"blink    W 3" \
  "join nobody blink" expect:"?"
# The monitor does not sleep, and Bramble runs a busy RP2040 about five times
# slower than real time: its first measurement of the supply, after 2 s, came
# after the second the steps wait. It measures every 100 ms here. The supply
# is the one simavr gives an AVR by default, 3.3 V: a third of it on GP29.
rp2 rp2040 examples/5-systems/43-monitor/43-monitor.ino -DVCC_CONF_MILLIS=100
talk "the monitor: states, connections and the log; nothing can be changed" "$O/43-monitor-rp2040.elf --adc 3:1100" \
  wait:1 ps expect:"blink    P 3" mods expect:"4 upper -" ipc expect:"vcc > keeper" log expect:"keeper 5 b1 21" \
  "kill blink" expect:"?" "run 4" expect:"?" ps expect:"blink    P 3"

echo "== 3. sleep =="
"$SIM" $O/40-shell-rp2040.elf --warp --uart0-out /dev/null --run-ms 3000 > $O/sim.log 2>&1
if grep -q "asleep 9[0-9]\." $O/sim.log; then
  pass "the shell waits with WFE between interrupts: $(grep -o 'asleep.*' $O/sim.log)"
else
  fail "the shell does not sleep while idle"; tail -n 3 $O/sim.log | sed 's/^/       /'
fi

echo "== 4. the terminal UI =="
RUN="--warp --exit-on-uart <DONE> --run-ms 600000"
rp2 rp2040 examples/7-text-ui/22-tui-render/22-tui-render.ino -DTUI22_FULL_REDRAW > /dev/null
mv $O/22-tui-render-rp2040.elf $O/22-tui-render-full-rp2040.elf
rp2 rp2040 examples/7-text-ui/22-tui-render/22-tui-render.ino
"$SIM" $O/22-tui-render-full-rp2040.elf $RUN --uart0-out $O/tui-full-rp2040.txt > /dev/null 2>&1
"$SIM" $O/22-tui-render-rp2040.elf $RUN --uart0-out $O/tui-rp2040.txt > /dev/null 2>&1
if node tools/vt-compare.mjs 12 40 $O/tui-rp2040.txt $O/tui-full-rp2040.txt --budget B=0 --budget D=24 --budget H=80; then
  pass "22-tui-render: every screen as a full redraw draws it, in the bytes the changes need"
else
  fail "22-tui-render: the screens or the byte counts above"
fi
rp2 rp2040 examples/7-text-ui/25-tui-robust/25-tui-robust.ino
"$SIM" $O/25-tui-robust-rp2040.elf $RUN --uart0-out $O/tui-robust-rp2040.txt > /dev/null 2>&1
if grep -q "<DONE>" $O/tui-robust-rp2040.txt && ! grep -q "^FAIL" $O/tui-robust-rp2040.txt; then
  pass "25-tui-robust: $(grep -c '^PASS' $O/tui-robust-rp2040.txt) checks"
else
  fail "25-tui-robust"; grep "^FAIL" $O/tui-robust-rp2040.txt | head -5 | sed 's/^/       /'
fi

echo "== 5. pins, the ADC and the supply =="
rp2 rp2040 examples/4-modules/30-ticker-blink/30-ticker-blink.ino
"$SIM" $O/30-ticker-blink-rp2040.elf --warp --pin-log GP25 --pin-duty GP25:2000 --uart0-out /dev/null --run-ms 4100 > $O/sim.log 2>&1
n=$(grep -c "^\[PIN\] GP25=" $O/sim.log)
if [ "$n" -ge 4 ] && grep -qE "^\[DUTY\] GP25 (49\.[5-9]|50\.[0-5])%" $O/sim.log; then
  pass "30-ticker-blink: the LED on GP25 changes $n times, half of the time on"
else
  fail "30-ticker-blink: GP25 changed $n times"; grep "^\[DUTY\]" $O/sim.log | sed 's/^/       /'
fi
rp2 rp2040 examples/4-modules/32-sensor/32-sensor.ino
"$SIM" $O/32-sensor-rp2040.elf --warp --adc 3:1667 --adc 3:1100@3000 --uart0-out $O/sensor-rp2040.txt --run-ms 4500 > /dev/null 2>&1
if grep -q "^5.0V" $O/sensor-rp2040.txt && grep -q "^3.3V" $O/sensor-rp2040.txt; then
  pass "32-sensor: VSYS through its divider on GP29: 5.0V, then 3.3V"
else
  fail "32-sensor: $(tr '\r\n' '  ' < $O/sensor-rp2040.txt)"
fi

echo "== 6. the RP2350 =="
for e in 5-systems/40-shell 7-text-ui/22-tui-render; do
  n=$(basename $e)
  if rp2 rp2350 examples/$e/$n.ino | grep -q "flash"; then pass "$n builds for the RP2350"; else fail "$n does not build for the RP2350"; fi
done

echo
[ $fails -eq 0 ] && echo "all passed" || echo "$fails failed"
exit $fails

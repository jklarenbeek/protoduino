#!/bin/bash
# test_systems.sh - run the systems of examples/5-systems in the simulator.
#
#   tools/test_systems.sh
#
# Builds each sketch with avr-gcc alone (tools/build_sketch.sh), the bootloader
# (boot/build.sh) and the modules (tools/module.mjs), and holds a conversation
# with each system (tools/talk.mjs) that has to go as expected.
#
# Needs avr-gcc, node and protosim, built, beside this repository
# (PROTOSIM=<path to bin/protosim> for another place). The ATtiny85 has no
# USART for protosim to connect: its system is run when tools/sim/pin_session
# has been built (see tools/sim/pin_session.c).
HERE="$(cd "$(dirname "$0")" && pwd)"; cd "$HERE/.."
SIM="${PROTOSIM:-$HERE/../../protosim/bin/protosim}"
[ -x "$SIM" ] || { echo "no protosim at $SIM: build it, or set PROTOSIM"; exit 2; }
O=tools/out; mkdir -p $O; fails=0

build() { bash tools/build_sketch.sh "$@" 2>/dev/null | tail -n 1; }
# talk <name> <protosim arguments> <step> ...
talk() {
  local name="$1" args="$2"; shift 2
  if node tools/talk.mjs --sim "$SIM $args" "$@" > "$O/talk.log" 2>&1; then
    echo "[PASS] $name"
  else
    echo "[FAIL] $name"; tr -d '\r' < "$O/talk.log" | tail -n 12 | sed 's/^/       /'; fails=$((fails + 1))
  fi
}

echo "== 40-shell: a shell and four modules, ATmega328P =="
build atmega328p 16000000 examples/5-systems/40-shell/40-shell.ino
talk "processes, modules, pipes, pause, kill" "$O/40-shell-atmega328p.elf -m atmega328p" \
  ps expect:"ticker   W 3" mods expect:"2 upper -" "run 2" "join upper serial:0" "write upper hello" expect:HELLO \
  ipc expect:"upper > serial:0" "stop blink" ps expect:"blink    P 3" "cont blink" "kill upper" ps expect:"blink    W 3" \
  "join nobody blink" expect:"?"

echo "== 43-monitor: a fixed system and a shell that only looks, ATmega328P =="
build atmega328p 16000000 examples/5-systems/43-monitor/43-monitor.ino
talk "states, connections and the log are shown; nothing can be changed" "$O/43-monitor-atmega328p.elf -m atmega328p" \
  wait:1 ps expect:"blink    P 3" mods expect:"4 upper -" ipc expect:"vcc > keeper" log expect:"keeper 5 b1 21" \
  "kill blink" expect:"?" "run 4" expect:"?" ps expect:"blink    P 3"

for part in atmega328p:0x7800 atmega2560:0x3F000; do
  mcu=${part%%:*}; boot=${part##*:}
  echo "== 42-shell-loader: modules arrive while the system runs, $mcu =="
  OUT="$HERE/out" boot/build.sh boot-$mcu $mcu default -DBOOT_CONF_LOAD_SERVICE=1
  build $mcu 16000000 examples/5-systems/42-shell-loader/42-shell-loader.ino
  K=$O/42-shell-loader-$mcu.elf
  avr-strip -o $O/kernel-$mcu.elf $K                       # the same image, a shorter file to send
  node tools/module.mjs $K upper 0 -o $O/upper-$mcu-slot0.elf
  node tools/module.mjs $K vcc 1 -o $O/vcc-$mcu-slot1.elf
  node tools/module.mjs $K vcc 0 -o $O/vcc-$mcu-slot0.elf
  other=$O/upper-$([ $mcu = atmega328p ] && echo atmega2560 || echo atmega328p)-slot0.elf
  [ -f $other ] || other=$O/40-shell-atmega328p.elf        # not a module at all
  talk "the bootloader takes the kernel; the shell takes two modules, refuses a third, keeps them over a power cycle, deletes one" \
    "$O/boot-$mcu.elf -m $mcu --reset-vector $boot" \
    send:$O/kernel-$mcu.elf wait:0.5 mods expect:"0 (free)" \
    "get 0" send:$O/upper-$mcu-slot0.elf mods expect:"0 upper +" \
    "join upper serial:0" "write upper hello" expect:HELLO \
    "get 1" send:$O/vcc-$mcu-slot1.elf mods expect:"1 vcc +" "unjoin upper" "join vcc serial:0" wait:1.5 expect:"V" "unjoin vcc" \
    "del 1" mods expect:"1 (free)" \
    "get 1" refused:$other log expect:"loader 6" mods expect:"1 (free)" \
    "get 0" expect:"?" \
    power wait:1 mods expect:"0 upper +" ps expect:"upper    W 3" \
    "del 0" "get 0" send:$O/vcc-$mcu-slot0.elf mods expect:"0 vcc +"
done

if [ -x tools/sim/pin_session ]; then
  echo "== 41-shell-tiny85: the shell on an ATtiny85, serial line on two pins =="
  build attiny85 8000000 examples/5-systems/41-shell-tiny85/41-shell-tiny85.ino
  if tools/sim/pin_session attiny85 8000000 $O/41-shell-tiny85-attiny85.elf 9600 ps mods "join ticker blink" ipc "run 2" ps > $O/talk.log 2>&1 \
     && grep -q "ticker > blink" $O/talk.log && grep -q "2 vcc -" $O/talk.log && grep -q "vcc      W 3" $O/talk.log; then
    echo "[PASS] processes, modules and pipes over the two-pin line"
  else
    echo "[FAIL] the ATtiny85 session"; tr -d '\r' < $O/talk.log | tail -n 12 | sed 's/^/       /'; fails=$((fails + 1))
  fi
else
  echo "== 41-shell-tiny85: skipped, tools/sim/pin_session is not built =="
fi

echo
[ $fails -eq 0 ] && echo "all passed" || echo "$fails failed"
exit $fails

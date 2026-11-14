#!/bin/bash
# test_systems.sh - run the systems of examples/5-systems and examples/6-attiny in the simulator.
#
#   tools/test_systems.sh
#
# Builds each sketch with avr-gcc alone (tools/build_sketch.sh), the bootloader
# (boot/build.sh) and the modules (tools/module.mjs), and holds a conversation
# with each system (tools/talk.mjs) that has to go as expected.
#
# Needs avr-gcc, node and protosim, built, beside this repository
# (PROTOSIM=<path to bin/protosim> for another place). The ATtiny parts have
# no USART: protosim bridges their serial line of two pins (--soft-uart), and
# plays the board around them with its pin and voltage options.
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

"$SIM" $O/40-shell-atmega328p.elf -m atmega328p --warp --uart0-out /dev/null --run-ms 3000 > $O/sim.log 2>&1
if grep -q "asleep 9[0-9]\." $O/sim.log; then
  echo "[PASS] idle, the shell sleeps between interrupts: $(grep -o 'asleep.*' $O/sim.log)"
else
  echo "[FAIL] the shell does not sleep while idle"; tail -n 3 $O/sim.log | sed 's/^/       /'; fails=$((fails + 1))
fi

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

echo "== 41-shell-tiny85: the shell on an ATtiny85, serial line on two pins =="
build attiny85 8000000 examples/5-systems/41-shell-tiny85/41-shell-tiny85.ino
TINY="-f 8000000 --soft-uart B0,B1,9600"
talk "processes, modules and pipes over the two-pin line" "$O/41-shell-tiny85-attiny85.elf -m attiny85 $TINY" \
  ps expect:"shell    R 2" mods expect:"2 vcc -" "join ticker blink" ipc expect:"ticker > blink" "run 2" ps expect:"vcc      W 3"

# ---- examples/6-attiny: the shell is on the PC, the part has a port ----------

# port <name> <file.elf> <protosim arguments> <command> ...   a conversation through tools/port.mjs
port() {
  local name="$1" elf="$2" args="$3"; shift 3
  if node tools/port.mjs "$elf" --sim "$SIM $elf $args" --sim-log "$O/sim.log" "$@" > "$O/talk.log" 2>&1; then
    echo "[PASS] $name"
  else
    echo "[FAIL] $name"; tail -n 12 "$O/talk.log" | sed 's/^/       /'; fails=$((fails + 1))
  fi
}
# seen <name> <file> <pattern> ...   every pattern has to be in the file
seen() {
  local name="$1" file="$2" p; shift 2
  for p in "$@"; do
    if ! grep -q -- "$p" "$file"; then
      echo "[FAIL] $name: no \"$p\""; tail -n 12 "$file" | sed 's/^/       /'; fails=$((fails + 1)); return
    fi
  done
  echo "[PASS] $name"
}

echo "== 50-port: the shell on the PC, a port on the ATtiny85 =="
build attiny85 8000000 examples/6-attiny/50-port/50-port.ino
K=$O/50-port-attiny85.elf
port "after a power-on the port only looks and does settings; a reset through the pin opens the rest" $K "-m attiny85 $TINY" \
  door expect:"levels 3 and 4: closed" ps expect:"port     R 2" mods expect:"2 ticker +" \
  "join ticker blink" expect:"are closed. Reset the part through its reset pin" \
  reset door expect:"levels 3 and 4: open" door expect:"reset pin" \
  "join ticker blink" ipc expect:"ticker > blink" "stop blink" ps expect:"blink    P 3" "cont blink" \
  "kill port" expect:"?" "join nobody blink" expect:"nobody by the name nobody"
for m in upper blink; do for sl in 0 1; do node tools/module.mjs $K $m $sl -o $O/$m-t85-slot$sl.elf -DBLINK_CONF_BIT=4 > /dev/null; done; done
bash tools/build_sketch.sh attiny85 8000000 examples/6-attiny/50-port/50-port.ino -DPORT_CONF_WINDOW=3 > /dev/null 2>&1
cp $O/50-port-attiny85.elf $O/50-port-other.elf
node tools/module.mjs $O/50-port-other.elf upper 0 -o $O/upper-other-slot0.elf > /dev/null
build attiny85 8000000 examples/6-attiny/50-port/50-port.ino    # the build the modules are for, once more
port "modules into flash slots while the system runs; refused where they do not belong; kept over a power cycle" $K \
  "-m attiny85 $TINY --reset-cause pin" door expect:"SELFPRGEN fuse: programmed" \
  "get 0 $O/upper-t85-slot0.elf" expect:"5 pages" mods expect:"0 upper +" \
  ps expect:"upper    W 3" "join upper serial:0" "write upper hello" wait:0.3 "unjoin upper" \
  "get 1 $O/blink-t85-slot1.elf" mods expect:"1 blink +" \
  "get 0 $O/blink-t85-slot0.elf" expect:"holds a module" \
  "get 1 $O/upper-t85-slot0.elf" expect:"is linked for 0x1500, slot 1 is at 0x1900" \
  "get 0 $O/upper-other-slot0.elf" expect:"another build" \
  "raw B 0 0x0100" expect:"status 3" "raw B 0 0x1d00" expect:"status 3" "raw L 3 62 1 2 3 4 5 6" expect:"status 3" \
  "raw T 1 5" expect:"status 0 a3" "raw Q 0 0" expect:"status 5" \
  "del 0" mods expect:"0 (free)" "get 0 $O/blink-t85-slot0.elf" mods expect:"0 blink +" \
  power wait:0.5 door expect:"levels 3 and 4: closed" mods expect:"0 blink +" mods expect:"1 blink +" \
  "del 1" expect:"are closed"
port "the door closes when nobody uses it for its window (3 s), and a watchdog reset does not open it" $O/50-port-other.elf \
  "-m attiny85 $TINY --reset-cause pin --reset-at 12000:watchdog" "join ticker blink" expect:"ok" wait:4 \
  "join ticker blink" expect:"are closed" wait:7 door expect:"levels 3 and 4: closed"

echo "== 50-port on the ATtiny45: level 3, no slots =="
build attiny45 8000000 examples/6-attiny/50-port/50-port.ino
port "closed after a power-on, open after a reset through the pin" $O/50-port-attiny45.elf "-m attiny45 $TINY" \
  "join ticker blink" expect:"are closed" reset "join ticker blink" ipc expect:"ticker > blink" "get 0 x.elf" expect:"no flash slots"

# The board the keeper watches: a heartbeat every 200 ms that stops after 6 s and returns at 14 s;
# a supply of 5 V that sags to 4.0 V from 20 s to 24 s. The reset line has the pull-up of the board.
BOARD="--warp --pin-pull B3:1 --pin-log B3 --vcc 5000 --pin-wave B2:200:0:6000 --pin-wave B2:200:14000 --vcc 4000@20000 --vcc 5000@24000"
LINE='B3=0 at 100[0-9][0-9]\.' ; LOW='B3=0 at 202[0-9][0-9]\.' ; BACK='B3=1 at 240[0-9][0-9]\.'

for mcu in attiny85 attiny45 attiny25; do
  case $mcu in attiny85) how="a process per stage, with the port";; attiny45) how="one process, with the port";; *) how="one process, no port";; esac

  echo "== 51-keeper on the $mcu: $how =="
  build $mcu 8000000 examples/6-attiny/51-keeper/51-keeper.ino
  E=$O/51-keeper-$mcu.elf
  if [ $mcu != attiny25 ]; then
    port "counts and settings through the port; a reset on request; nothing else without the reset pin" $E "-m $mcu $TINY $BOARD" \
      wait:1.5 resets expect:"resets after a missed heartbeat: 1" resets expect:"times the supply was low: 1" \
      actions expect:"restart" restart wait:0.3 resets expect:"resets asked for: 1" \
      "set timeout 9" settings expect:"timeout      9   (  4)" "poke 0x60 1" expect:"?"
    seen "reset after 4 s of silence, held while the supply is low, released when it is back" $O/sim.log "$LINE" "$LOW" "$BACK"
  else
    "$SIM" $E -m $mcu -f 8000000 $BOARD --run-ms 30000 --dump-eeprom 16:3 > $O/sim.log 2>&1
    seen "reset after 4 s of silence, held while the supply is low, released when it is back; both counted" $O/sim.log \
      "$LINE" "$LOW" "$BACK" "EEPROM. 0x0010: 01 01 ff"
  fi

  echo "== 52-sentinel on the $mcu: $how =="
  build $mcu 8000000 examples/6-attiny/52-sentinel/52-sentinel.ino
  E=$O/52-sentinel-$mcu.elf
  if [ $mcu != attiny25 ]; then
    port "temperature and supply recorded in EEPROM and shown" $E "-m $mcu $TINY --warp --vcc 3000 --adc temp:322" \
      wait:0.6 temperature expect:"lowest 24 C, highest 24 C" supply expect:"lowest 3.0 V, highest 3.0 V"
  else
    "$SIM" $E -m $mcu -f 8000000 --warp --vcc 3000 --adc temp:322 --run-ms 330000 --dump-eeprom 8:42 > $O/sim.log 2>&1
    seen "five samples of each in EEPROM, with lowest and highest" $O/sim.log \
      "0x0008: 40 40 40 40 40 40 40 ff" " 1e 1e 1e 1e 1e 1e 1e ff"
  fi
  "$SIM" $E -m $mcu -f 8000000 --warp --vcc 3000 --adc temp:322 --run-ms 640000 > $O/sim.log 2>&1
  seen "asleep all but a few milliseconds of ten minutes: $(grep -o 'asleep.*' $O/sim.log)" $O/sim.log "asleep 99.99%"

  echo "== 53-regulator on the $mcu: $how =="
  build $mcu 8000000 examples/6-attiny/53-regulator/53-regulator.ino
  E=$O/53-regulator-$mcu.elf
  if [ $mcu != attiny25 ]; then
    port "input below the curve: off" $E "-m $mcu $TINY --warp --vcc 5000 --adc 3:1000" wait:0.5 duty expect:"duty cycle: 0 %"
    port "input above it: full" $E "-m $mcu $TINY --warp --vcc 5000 --adc 3:4500" wait:0.5 duty expect:"duty cycle: 100 %"
    port "input on it: 60 %; a setting changed while it runs: 73 %" $E "-m $mcu $TINY --warp --vcc 5000 --adc 3:2500" \
      wait:0.5 duty expect:"duty cycle: 60 %" "set min 128" wait:0.5 duty expect:"duty cycle: 73 %"
  else
    for v in 1000:0 2500:153 4500:255; do
      "$SIM" $E -m $mcu -f 8000000 --warp --vcc 5000 --adc 3:${v%%:*} --run-ms 3000 -t 23000000 -w 0x4b:1:OCR1B > $O/sim.log 2>&1
      seen "${v%%:*} mV at the input: duty ${v##*:} of 255" $O/sim.log "= 0x.. (${v##*:})"
    done
  fi
done

echo
[ $fails -eq 0 ] && echo "all passed" || echo "$fails failed"
exit $fails

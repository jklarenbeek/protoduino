# The RP2040 and RP2350

protoduino runs on the RP2040 (Raspberry Pi Pico) and builds for the RP2350
(Pico 2, its Arm cores). The platform files are in `src/cpu/rp2/`; a sketch
for an AVR runs there unchanged when it uses the portable parts, and prints
the same bytes: `tools/test_rp2.sh` runs sixteen examples on an ATmega328P and
on an RP2040 in protosim and compares their output byte for byte.

## Building

**Arduino IDE and PlatformIO.** With the
[arduino-pico](https://github.com/earlephilhower/arduino-pico) core
(`architectures=avr,rp2040` in `library.properties`, `[env:pico]` in
`platformio.ini`). Every file under `src/` is compiled there, and a file for
another platform compiles to nothing. *Not tried with arduino-pico itself
yet*: the builds of this repository use the Pico SDK alone.

**The Pico SDK alone.** `tools/build_rp2.sh` does for an RP2040 or RP2350 what
`tools/build_sketch.sh` does for an AVR: the sketch as C++, a stand-in for an
Arduino core (`tools/core-rp2/`: `micros()`, `millis()`, `delay()`,
`random()`, `setup()` and `loop()`, and `setup1()` and `loop1()` for core 1),
and the library.

```bash
PICO_SDK_PATH=~/pico-sdk tools/build_rp2.sh rp2040 examples/5-systems/40-shell/40-shell.ino
/path/to/protosim/bin/protosim tools/out/40-shell-rp2040.elf        # the shell on /tmp/simavr-uart0
PICO_SDK_PATH=~/pico-sdk bash tools/test_rp2.sh                    # the tests
```

`WARN=1` builds the library with `-Wall -Wextra -Wcast-align` and shows its
warnings.

## What runs where

| Part | RP2040 | RP2350 | How |
|---|---|---|---|
| Protothreads, scheduler, events, errors | tested | builds | portable |
| IPC: messages, pipes, pools | tested | builds | portable; `CC_ATOMIC_RESTORE()` saves PRIMASK and restores it when the block is left, a `return` too |
| Timers, the tick (`TICK_SOURCE_CLOCK`) | tested | builds | the 1 MHz timer (`time_us_32()`) |
| Idle: `PROCESS_SLEEP_WAIT` | tested: the shell is asleep 99.7% | builds | WFE until an interrupt, an event or an alarm at the next tick; `process_poll()` and `process_post()` send an event (SEV), which is latched |
| Idle: `PROCESS_SLEEP_DEEP` | not ported | not ported | dormant needs the clocks set up again after the wake, and cannot be tested in protosim: the build stops with an error |
| `TICK_SOURCE_WATCHDOG` | not ported | not ported | an AVR's watchdog interrupt: the build stops with an error |
| UART 0 and 1, `sys/serial`, the serial process | tested | builds | the PL011 with its interrupts: receive and receive timeout; the transmit FIFO is filled when the interrupt is enabled, the interrupt refills it |
| utf8, mcurses, ANSI parser, TUI | tested | builds | portable; `PROGMEM` and `pgm_read_*` are plain reads (`cc.h`) |
| Shell, monitor | tested over the pseudo terminal | builds | portable |
| Modules ticker, upper, smooth, limit, curve | tested | builds | portable |
| blink | tested | builds | a GPIO, GP25 by default (`BLINK_CONF_GPIO`) |
| sense, vcc | tested with `--adc` | builds | the ADC: inputs 0 to 3 on GP26 to GP29, 4 the temperature sensor; the supply is VSYS / 3 on GP29 of a Pico board |
| Both cores | see below | | |
| Settings (EEPROM) | not ported | not ported | there is no EEPROM; a flash sector would do, when something on the RP2040 needs it |
| Flash loader, slots, bootrom, ELF modules | not ported | not ported | they write AVR flash and relocate AVR code |
| Port (`mod/port.h`) | not ported | not ported | its requests carry 16-bit addresses |
| Serial line on two pins (`softserial`) | not needed | not needed | the chip has two UARTs |
| pwm, pulse, keeper, record | not ported | not ported | ATtiny modules: timer 1, INT0, EEPROM |
| Examples 41, 42, 50 to 53 | | | ATtiny and the flash loader |
| Examples 09, 13 | | | Arduino IDE only, on any target (`usleep`, `Stream`) |

**The RP2350** is built by `tools/test_rp2.sh` but not run: protosim's Bramble
target runs the Pico SDK on its Cortex-M33 only as far as the UART, and not
its timer alarms (finding BR16 in protosim's report).

## Differences from an AVR

- **Time.** The clock counts microseconds as `micros()` does on an AVR; the
  tick is the same 16 ms.
- **Sleep.** Nothing wakes an RP2040 by itself, as timer 0 of an Arduino
  core wakes an AVR every millisecond: the idle wait sets an alarm for the
  next tick.
- **Interrupts.** `CC_ATOMIC_RESTORE()` holds off the interrupts of one core.
  What both cores share takes more than that (below).
- **The UART handlers** are protoduino's (`irq_set_exclusive_handler`): an
  arduino-pico `Serial1` on the same UART cannot be used beside them, as
  `USE_ARDUINO_HARDWARESERIAL` is the choice on an AVR.
- **`uart0` and `uart1`** are macros of the Pico SDK; protoduino's functions
  `uart0_open()` and so on are made from the bare words, so `cpu/rp2/uart.h`
  puts the macros away while it declares them (`push_macro`).
- **`char` is unsigned** on Arm and signed on an AVR; the examples print the
  same bytes on both.
- **`uint_fast8_t` is four bytes** on Arm: two places mixed it with `uint8_t`
  (`ringb8_init()` and the receive callback of the serial process), both
  fixed.

## Found on the way

These were wrong on the AVR too, and show more on a faster chip:

- A process that reads its input pipe with `PROCESS_WAIT_EVENT_UNTIL` takes
  one byte per event, and the pipe wakes its reader only when it goes from
  empty to not empty: "HELLO" came out of `upper` as "H", and "ELLO" only after
  the next command. The modules and examples wait with the new
  `PROCESS_WAIT_UNTIL`, which lets go only when the pipe is empty.
- The receive callback of the serial process returned nothing where `bool`
  is expected: a byte could also land in the ring buffer of the driver.
- `12-sys-uart-echo` printed nothing on an AVR: its flag, set in the receive
  interrupt, was not `volatile`.
- `04-pt-finally-basic` declared an initialised local that a protothread case
  label jumps over: C++ allows that only with `-fpermissive`, which the AVR
  core of the Arduino IDE passes and the RP2040 builds do not.

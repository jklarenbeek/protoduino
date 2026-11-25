# The RP2040 and RP2350

protoduino runs on the RP2040 (Raspberry Pi Pico) and builds for the RP2350
(Pico 2, its Arm cores). The platform files are in `src/cpu/rp2/`; a sketch
for an AVR runs there unchanged when it uses the portable parts, and prints
the same bytes: `tools/test_rp2.sh` runs fifteen examples on an ATmega328P and
on an RP2040 in protosim and compares their output byte for byte, and the
timer example line for line (its lines count passes through a busy loop,
which a faster chip makes more of).

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
| UART 0 and 1, `sys/serial`, the serial process | tested: receive, receive timeout, and the transmit FIFO refilled in the interrupt (protosim sends at the baud rate through a FIFO of 32 bytes) | builds | the PL011 with its interrupts; `tx_enable_int()` fills the FIFO, the transmit interrupt refills it; `flush()` only enables it |
| utf8, mcurses, ANSI parser, TUI | tested | builds | portable; `PROGMEM` and `pgm_read_*` are plain reads (`cc.h`) |
| Shell, monitor | tested over the pseudo terminal | builds | portable |
| Modules ticker, upper, smooth, limit, curve | tested | builds | portable |
| blink | tested | builds | a GPIO, GP25 by default (`BLINK_CONF_GPIO`) |
| sense, vcc | tested with `--adc` | builds | the ADC: inputs 0 to 3 on GP26 to GP29, 4 the temperature sensor; the supply is VSYS / 3 on GP29 of a Pico board |
| Both cores: a scheduler on each | tested: nine interleavings, the shell across cores | builds | `PROCESS_CONF_CORES 2`; mailboxes, pipes and pools that hold across cores (below) |
| Settings (EEPROM) | not ported | not ported | there is no EEPROM; a flash sector would do, when something on the RP2040 needs it |
| Flash loader, slots, bootrom, ELF modules | not ported | not ported | they write AVR flash and relocate AVR code |
| Port (`mod/port.h`) | not ported | not ported | its requests carry 16-bit addresses |
| Serial line on two pins (`softserial`) | not needed | not needed | the chip has two UARTs |
| pwm, pulse, keeper, record | not ported | not ported | ATtiny modules: timer 1, INT0, EEPROM |
| Examples 41, 42, 50 to 53 | | | ATtiny and the flash loader |
| Examples 09, 13 | | | Arduino IDE only, on any target (`usleep`, `Stream`) |

**The RP2350** is built by `tools/test_rp2.sh` (the shell, the TUI, and both
multicore examples) but not run: protosim's Bramble
target runs the Pico SDK on its Cortex-M33 only as far as the UART, and not
its timer alarms (finding BR16 in protosim's report).

## Both cores

With `PROCESS_CONF_CORES 2` each core runs a scheduler of its own: the same
`process_loop()` in `loop()` on core 0 and in `loop1()` on core 1 (as in the
arduino-pico core; `tools/core-rp2` does the same).

```cpp
void setup(void)  { process_init(&shell_proc.base); /* the processes of core 0 */ }
void loop(void)   { process_loop(); }
void setup1(void) { process_init(NULL); process_new(&process_type_upper, &upper_proc); }
void loop1(void)  { process_loop(); }
```

- **A process belongs to the core that starts it**, and only that core runs
  it and writes its state.
- **`process_post()` and `process_poll()`** for a process of the other core
  go through a mailbox: a ring with one writer (the sending core, interrupts
  off while it writes) and one reader (the receiving core, in
  `process_run()`), so neither needs a lock; an SEV wakes the other core
  from its WFE. An event waits in the mailbox while the queue of its core is
  full; a poll that finds the mailbox full is counted, and the receiving
  core then polls all of its processes, so that no wake is lost.
  `PROCESS_CONF_MAILBOX_SIZE` is 16.
- **`process_destroy()`, `process_pause()` and `process_resume()`** of a
  process of the other core go through the mailbox too: the shell on core 0
  stops and kills modules on core 1.
- **A broadcast** (`process_post(NULL, ...)`) reaches the processes of the
  core that posts it: each core has its own tick.
- **`process_next()`, `process_lookup()` and `process_reader()`** see the
  processes of both cores: `ps` in the shell lists them all.
- **Pipes** may have writers on both cores (the shell and a module both write
  to the serial line), and **pools** are taken from on one core and given back
  on the other: both take the hardware spinlock 15 of the SIO with interrupts
  off (`CC_ATOMIC_SHARED()`), with a memory barrier before an index is
  published.
- **The log** (`process_warn()` and the rest) is kept per core.
- **`process_connect()` and `process_disconnect()`** for a process of the
  other core go through the mailbox as well: the bits of a process (its
  state, its poll, whether it has pipes) share one byte, and only its own
  core writes them. The shell's `join` and `unjoin` work across cores so.
- **The shell's `run`** starts a module on the shell's core; a sketch puts a
  module on core 1 by starting it in `setup1()`.
- `PROCESS_CONF_CORES` must be the same in every file of the build: set it
  in `src/protoduino-config.h` or as a build flag (`-DPROCESS_CONF_CORES=2`),
  not in the sketch alone. With one core, an AVR build is byte for byte the
  same as without this code.

**Tested** in protosim: `examples/8-multicore/61-cross-core` sends 3000
numbers from core 0 to core 1 as events and as messages from a pool of four,
and core 1 sends each back through a pipe, under nine orders of the two
cores (`--core-skew`: n steps of core 0, then m of core 1, from 64:1 to
1:64); every count and sum is right. `60-two-cores` runs the shell's steps
of `40-shell` with the modules on core 1. Two cores that run at the very
same moment, as on a chip, protosim does not show: what such a race would
break is guarded by the barriers and the spinlock, not tested.

Found by the orders: a process that waits for room in a pipe with
`PROCESS_WAIT_EVENT()` takes the next event and loses it. The sink of the
example writes as much as the pipe takes and never waits for room.

## Differences from an AVR

- **Time.** The clock counts microseconds as `micros()` does on an AVR; the
  tick is the same 16 ms.
- **Sleep.** Nothing wakes an RP2040 by itself, as timer 0 of an Arduino
  core wakes an AVR every millisecond: the idle wait sets an alarm for the
  next tick.
- **Interrupts.** `CC_ATOMIC_RESTORE()` holds off the interrupts of one core.
  What both cores share takes more than that (above).
- **The UART handlers** are protoduino's (`irq_set_exclusive_handler`): an
  arduino-pico `Serial1` on the same UART cannot be used beside them, as
  `USE_ARDUINO_HARDWARESERIAL` is the choice on an AVR.
- **`uart0` and `uart1`** are macros of the Pico SDK; protoduino's functions
  `uart0_open()` and so on are made from the bare words, so `cpu/rp2/uart.h`
  puts the macros away while it declares them (`push_macro`).
- **`char` is unsigned** on Arm and signed on an AVR; the examples print the
  same bytes on both.
- **Alignment.** A Cortex-M0+ faults on an unaligned word access, where
  protosim does not: `ipc_pool_init()` refuses a buffer or a block size
  that is not aligned for a pointer on an Arm.
- **The RP2350's hardware spinlocks** are unsafe (erratum RP2350-E2):
  `CC_ATOMIC_SHARED()` takes the SDK's spin lock, which is a lock in memory
  there and the SIO's spinlock on an RP2040.
- **`uint_fast8_t` is four bytes** on Arm: two places mixed it with `uint8_t`
  (`ringb8_init()` and the receive callback of the serial process), both
  fixed.

## Found on the way

These were wrong on the AVR too, and show more on a faster chip:

- `timer_diff()` took a clock that had not moved since `timer_set()` for one
  that had gone all the way round, and the timer expired at once. An AVR's
  `micros()` moves in steps of 4 us and never met it; a 125 MHz RP2040 reads
  its 1 us clock twice in the same microsecond. It subtracts unsigned now,
  which is right across the wrap too (it was one off there).

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

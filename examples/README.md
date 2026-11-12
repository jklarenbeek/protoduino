# The examples, in order

The examples are a course. Each track builds on the one before it, and inside a
track each sketch adds one idea. The Arduino IDE shows the tracks as submenus of
**File > Examples > protoduino**.

Every sketch starts with a comment that says what it demonstrates. The sketches
of tracks 4 and 5 talk at 115200 baud unless their comment says otherwise.

## 1 Protothreads

How a stackless thread works, without a scheduler: one function that returns
where it waits and continues there on the next call. These are plain sketches;
nothing in them is a module.

| Sketch | What it adds |
|--------|--------------|
| `01-pt-wait-one-basic` | Three threads that take turns: `PT_BEGIN`, `PT_WAIT_ONE`, `PT_END`. |
| `02-pt-wait-one-block` | What happens when a thread ends or exits: in v2 it stays ended until it is reset. |
| `03-pt-wait-until-while` | `PT_WAIT_UNTIL` and `PT_WAIT_WHILE`: waiting for a condition. |
| `04-pt-finally-basic` | `PT_FINALLY`: the code that runs when a thread ends, whatever ended it. |
| `05-pt-finally-extended` | The same with several threads and every way out. |
| `06-pt-catch-basic` | `PT_RAISE`, `PT_CATCH`, `PT_CATCHANY`: errors without a stack. |
| `07-pt-yield-basic` | `PT_YIELD`: a thread that hands out values. |
| `08-pt-yield-foreach` | `PT_FOREACH`: a thread that consumes them, also nested. |
| `09-pt-sem-buffer` | Semaphores: a bounded buffer between a producer and a consumer. |
| `10-pt-wait-for-timer` | Time: a thread that waits for a timer. |

## 2 Library

The pieces under the scheduler, each on its own.

| Sketch | What it adds |
|--------|--------------|
| `11-lib-ringb8-basic` | The ring buffer the serial drivers are built on. |
| `12-sys-uart-echo` | The UART driver and the ring buffer: an echo. |
| `13-pt-basic-echo` | A protothread that echoes keys, decoded by the ANSI parser. |

## 3 Processes

A process is a protothread the scheduler knows: it has a name, a priority,
events and pipes.

| Sketch | What it adds |
|--------|--------------|
| `20-process-basic` | `PROCESS_DEFINE`, `PROCESS_THREAD`, `process_run`: processes and events. |
| `22-process-serial` | The serial line as a device process, read and written through pipes. Written for the ATmega2560 at 9600 baud. |

## 4 Modules

A module is a process type in a header of its own (`src/mod/`). It reads its
input, writes its output and knows nothing about who is at the other end. The
sketch decides what is connected to what.

| Sketch | What it adds |
|--------|--------------|
| `30-ticker-blink` | Two modules and one pipe: `ticker \| blink`. `process_connect()`, the tick. |
| `31-filter` | A filter between a device and itself: `serial:0 \| upper \| serial:0`. |
| `32-sensor` | A source that measures: `vcc \| serial:0`. No part and no pin needed. |
| `33-your-own-module` | Writing one: the five rules of a module, in a sketch. |

## 5 Systems

Modules, a command line and, where the part allows it, a way to add modules
while the system runs.

| Sketch | Part | What it adds |
|--------|------|--------------|
| `40-shell` | ATmega328P, ATmega2560 | A shell: list processes and modules, start, pause, end and connect them, read the log. |
| `41-shell-tiny85` | ATtiny85 | The same shell in 8 KB, on a serial line made of two pins. |
| `42-shell-loader` | ATmega328P, ATmega2560 | Flash slots: `get` receives a module that was not part of the sketch, `del` removes it. Needs a bootloader that programs pages for the running system. |
| `43-monitor` | ATmega328P, ATmega2560 | A finished system: modules chosen and connected by the sketch, and a shell that can only look. |

`docs/loader.md` is the manual for this track: modules, the shell, flash slots,
the bootloader and the tools.

## 6 ATtiny

The same framework on the smallest parts, put together differently. An
ATtiny85 has room for the shell of track 5, as `41-shell-tiny85` shows, but
not for much beside it. Here the shell moves to the PC and the part keeps a
port: a process that reads memory, writes memory and calls a function when
asked. What is left is room for work.

| Sketch | Part | What it adds |
|--------|------|--------------|
| `50-port` | ATtiny85, 45 | The port, and `tools/port.mjs`, the shell on the PC: `ps`, `mods`, `ipc`, `stop`, `join`, `run`, `write` as in example 40. The watchdog as the tick, and sleep while idle. |
| `51-keeper` | ATtiny85, 45, 25 | A chip that looks after a bigger board: resets it when its heartbeat stops, holds it in reset while the supply is low, resets it on request, and counts what it did in EEPROM. |
| `52-sentinel` | ATtiny85, 45, 25 | Asleep nearly always: wakes on its watchdog, measures the temperature of the chip and its supply, keeps the last samples and the extremes in EEPROM. |
| `53-regulator` | ATtiny85, 45, 25 | `sense \| smooth \| curve \| pwm`: a signal in, a smooth answer out. A fan that follows a temperature, as built. |

Three things run through the last three:

- **A byte is a sample.** The modules for signals (`sense`, `pulse`,
  `smooth`, `limit`, `curve`, `pwm`, `record`) pass numbers from 0 to 255,
  not text.
- **Settings.** What an appliance runs on is a list of named bytes
  (`sys/settings.h`): built in, and changeable on the finished chip from the
  PC, where the changed values are kept in EEPROM.
- **The RAM of the part decides how it is put together.** On an ATtiny85
  every stage is a process with a pipe to the next and the port is there. On
  an ATtiny45 the stages are functions of one process, with the port. On an
  ATtiny25 they are functions of one process and there is no port. It is the
  same module code each time, and the sketch chooses.

`docs/loader.md` has the details and the figures.

## 7 Text UI

Terminal output and input, independent of tracks 4 to 6.

| Sketch | What it adds |
|--------|--------------|
| `21-process-mcurses` | Cursor, colours and boxes on a VT terminal, from a process. |
| `22-tui-render` | The layout engine. Needs the RAM of an ATmega2560. |
| `23-ansi-parser` | Reading escape sequences: the parser. |
| `24-ansi-keys` | Keys: arrows, function keys, modifiers. |
| `25-tui-robust` | The layout engine against hostile input. |
| `26-rune-conformance` | The Unicode tables, checked on the part. |

## Building outside the Arduino IDE

- **protosim.** `tools/build_sketch.sh <mcu> <f_cpu> <sketch.ino>` builds a
  sketch with avr-gcc alone and writes `tools/out/<sketch>-<mcu>.elf`, which
  protosim runs. `tools/talk.mjs --sim "<protosim> <file.elf> -m <mcu>" ps mods`
  then holds a conversation with a shell. `tools/test_systems.sh` does both for
  every system of tracks 5 and 6.
- **PlatformIO.** The repository is an Arduino library with a
  `library.properties`, so `pio ci --lib . --board uno examples/4-modules/30-ticker-blink`
  builds an example against it. This has not been run for this revision: the
  PlatformIO registry was not reachable where it was written.
- **ATtiny.** No ATtiny core is needed for `41-shell-tiny85` and track 6 when
  they are built with `tools/build_sketch.sh attiny85 8000000 ...` (or
  `attiny45`, `attiny25`). In the Arduino IDE they need an ATtiny core such as
  ATTinyCore; that has not been tried. protosim runs them with
  `-m attiny85 -f 8000000 --soft-uart B0,B1,9600`, which puts their serial
  line of two pins on its pseudo terminal.

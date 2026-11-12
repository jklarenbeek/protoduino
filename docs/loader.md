# Modules, the shell and the loader

A protoduino system is a set of processes that talk through pipes. This
document covers the parts that turn processes into something you can assemble
and operate:

- **modules**: process types you connect instead of write,
- **the shell**: a command line for the processes of a running system,
- **flash slots and the loader**: modules that arrive while the system runs,
- **the bootloader** that makes this possible,
- **the small parts**, where the shell moves to the PC and the part keeps a
  port, and
- **the tools** on the host.

The sketches of `examples/4-modules`, `examples/5-systems` and
`examples/6-attiny` show each of them at work; `examples/README.md` lists them in order.

Nothing here has run on hardware yet. Everything in this document was built
with avr-gcc 7.3 and run in simavr and protosim; where a figure is given it was
measured there.

## Modules

A module is one process type in one header, under `src/mod/`:

| Header | What it does |
|--------|--------------|
| `mod/ticker.h` | A source: writes a digit to its output every 500 ms. |
| `mod/blink.h` | A sink: changes the LED for every byte it reads. |
| `mod/upper.h` | A filter: what it reads leaves in capital letters. |
| `mod/vcc.h` | A sensor: measures the supply voltage and writes it as text, `4.9V`. |
| `mod/shell.h` | The command line described below. |
| `mod/loader.h` | Receives a module into a flash slot (not loadable itself). |

A sketch includes the headers it wants, gives each process type an instance and
connects them:

```cpp
#include <protoduino.h>
#include <sys/process.h>
#include <sys/tick.h>
#include <mod/ticker.h>
#include <mod/blink.h>

PROCESS_INSTANCE(ticker, ticker_proc);
PROCESS_INSTANCE(blink, blink_proc);

static bool connected;

void setup(void)
{
  process_init(NULL);
  process_new(&process_type_ticker, &ticker_proc);
  process_new(&process_type_blink, &blink_proc);
}

void loop(void)
{
  process_run();
  tick_run();
  if (!connected)   /* ticker | blink, once blink has published its input */
    connected = process_connect(&ticker_proc.base, &blink_proc.base);
}
```

Settings of a module (`TICKER_CONF_MILLIS`, `BLINK_CONF_PIN`, ...) are defined
in the sketch, before the header: a module is compiled as part of the sketch.

### The five rules of a module

1. One process type, and all of its state in the process struct. No static
   variables: two instances must not share anything, and a module in a flash
   slot has no RAM of its own.
2. It owns its input pipe and publishes it when it starts, unless somebody
   connected one before. It reads through `PROCESS_PIPEIN()`.
3. It never blocks. It waits: `PROCESS_WAIT_EVENT_UNTIL()` for a condition,
   `PROCESS_PIPE_WRITE()` for room in its output.
4. Nothing is kept in a local variable across a wait. In a sketch, which is
   C++, a local with an initial value may not even stand before a wait in the
   same block: keep such code in a plain function.
5. It ends with a `PT_FINALLY` block. That is what runs when the process is
   ended, and without it the thread has no end to go to.

`examples/4-modules/33-your-own-module` is a module written by these rules.

### What the core provides

| Function | Use |
|----------|-----|
| `process_connect(writer, reader)` | The output of `writer` becomes the input of `reader`. A device (the serial line) can be either side. |
| `process_disconnect(writer)` | Takes the output of `writer` away. |
| `process_pause(p)`, `process_resume(p)` | A paused process gets no events and no polls. |
| `process_next(p)`, `process_reader(pipe)`, `process_log_entry(age)` | Walk the processes, find who reads a pipe, read the log. |
| `tick_run()` (`sys/tick.h`) | Posts one event to every process each 16 ms, so that timers are noticed and writers get through a full pipe. Call it in `loop()`. |

The tick carries no data. A process that posts an event to itself to get its
next turn has to tell that event from the tick; one that answers every tick
with another event of its own fills the event queue.

## The shell

`mod/shell.h` is a process that reads a line, carries it out and writes the
answer. Only the first letter of a command counts.

| Command | Effect |
|---------|--------|
| `ps` | Processes: name, state (`I` started, `R` running, `W` waiting, `P` paused, `X` ending), priority. |
| `stop <name>`, `cont <name>` | Pause a process, let it continue. |
| `kill <name>` | End a process. |
| `mods` | The modules that can be run: number, name, `+` when running. |
| `run <number>` | Start a module. |
| `ipc` | Connections: `ticker > blink`. |
| `join <a> <b>`, `unjoin <a>` | Connect the output of `a` to the input of `b`; take it away. |
| `write <name> text` | Write text to the input of a process. |
| `log` | What the kernel logged: name, severity, event, code. |
| `get <number>`, `del <number>` | Receive a module into a flash slot; remove it. Only with the loader. |

The sketch lists the modules the shell may start:

```cpp
#include <mod/shell.h>

PROCESS_INSTANCE(shell, shell_proc);

SHELL_MODULES_BEGIN
  SHELL_MODULE(ticker, ticker_proc)
  SHELL_MODULE(blink, blink_proc)
SHELL_MODULES_END

void setup(void)
{
  process_init(&shell_proc.base);   /* the shell keeps the log */
  process_new(&process_type_serial_process0, &serial_process0_proc);
  serial_process0_proc.baud = 115200;
  process_new(&process_type_shell, &shell_proc);
}
```

| Setting | Meaning |
|---------|---------|
| `SHELL_CONF_DEVICE` | The device the shell talks on; `serial_process0_proc` by default. |
| `SHELL_CONF_LINE` | Longest command line, 24 characters by default. |
| `SHELL_CONF_ECHO` | Echo what is typed; on by default. |
| `SHELL_CONF_READONLY` | A monitor: `ps`, `mods`, `ipc` and `log` only. Whoever sits at the line can look at the system and cannot change it (`examples/5-systems/43-monitor`). |

On a part without a USART the line is `sys/process/softserial.h`: a serial
device on two pins (RX PB0, TX PB1, 9600 baud, half duplex). The shell with
three modules takes 5,914 bytes of flash and 275 bytes of RAM on an ATtiny85
(`examples/5-systems/41-shell-tiny85`).

## Flash slots and the loader

A loadable module is the same header, built on its own as an ELF file and
linked to run in one flash slot against the functions of one kernel build.
Nothing is relocated when it arrives.

The sketch that is the kernel says where the slots are, before it includes the
loader:

```cpp
#define MODULE_CONF_SLOTS     2        /* two slots ...            */
#define MODULE_CONF_SLOT_SIZE 1024     /* ... of 1 KB ...          */
#define MODULE_CONF_BASE      0x6000   /* ... starting here        */
#include <mod/loader.h>
#include <mod/shell.h>

void setup(void)
{
  ...
  module_init();        /* which build is this? */
  ...
  module_autostart();   /* start what the module table lists */
}
```

- The slots are followed by one page that holds the **module table**. A module
  is entered there after it has arrived completely and has been checked, so a
  load that was cut short leaves nothing that could be started. What the table
  lists starts again with the system after a reset.
- Slots and table lie above the sketch, below the bootloader and its image
  record (one page under the boot section), and in the first 64 KB of flash.
- A module only runs on the kernel build it was linked against. The identity of
  a build is a CRC-16 of its image; a module for another build is refused while
  it arrives.
- `MODULE_CONF_RAM_SIZE` (48 bytes) is the RAM a slot offers for the process
  struct of its module.
- With the shell, the slots come first in `mods`, so the number of a slot there
  is the number of the slot. `get 0` hands the line to the loader: send the
  file with XMODEM-CRC, and the shell is back when the loader has ended.

Without a shell, `loader_start(slot, &serial_process0_proc.base)` starts a load
and `module_start(slot)` starts a module.

### What a kernel offers to its modules

A module calls the functions of the kernel, and a build only contains the
functions somebody in it refers to. `sys/module.h` therefore keeps a table of
the functions a module is likely to need (`process_post`, `process_poll`,
`process_new`, `process_destroy`, `process_lookup_n`, `process_connect`,
`process_disconnect`, `process_ipc_wake`, the `ipc_pipe_*` functions and
`micros`). Add your own in the sketch:

```cpp
#define MODULE_CONF_API (module_api_fn)analogRead, (module_api_fn)digitalWrite,
```

### Who programs the pages

An AVR executes the instruction that programs flash only from its boot section,
so a running system needs somebody there.

| Bootloader | Setting in the sketch | Cost in the sketch |
|------------|----------------------|--------------------|
| protoduino, built with `-DBOOT_CONF_LOAD_SERVICE=1` | none | The loader schedules the load thread inside the bootloader: about 1.3 KB for slots and loader. |
| protoduino with only `-DBOOT_CONF_PAGE_SERVICE=1`, or Optiboot 8 or later | `#define LOADER_CONF_THREAD 2` before `<mod/loader.h>` | The same plus the load thread of the library, about 1.4 KB more. |

Which of the two bootloaders is installed is found out at run time
(`FLASH_DRIVER_AUTO` in `src/cpu/avr/flash.conf.h`).

While a module arrives the other processes keep running: the loader takes one
byte per turn, and a page write stops the system for about 8 ms.

## The bootloader

`boot/` holds the protoduino bootloader: it receives the unmodified `.elf` file
of a sketch with XMODEM-CRC, and stays in flash as a ROM the running system can
call (`src/sys/bootrom.h`).

```sh
boot/build.sh boot-uno  atmega328p default -DBOOT_CONF_LOAD_SERVICE=1   # 2,020 bytes, 2 KB boot section
boot/build.sh boot-mega atmega2560 default -DBOOT_CONF_LOAD_SERVICE=1   # 2,460 bytes, 4 KB boot section
```

The files land in `boot/out/`. Program one with an ISP programmer and set the
fuses for a boot section of that size with the reset vector in it. It waits for
a file after a reset through the reset pin; after power-on it starts the
installed sketch at once. `boot/boot.conf.h` lists the options;
`docs/elf-loader-design.pdf` explains the design and gives the sizes.

Optiboot remains usable: it uploads the sketch as it always did, and from
version 8 on it can program pages for a running sketch, which is all the loader
needs.

## The small parts: ATtiny85, 45 and 25

An ATtiny85 runs the shell (`41-shell-tiny85`), and the shell is then more
than a third of what it holds. These parts are not small computers; they are
made for one job on a few pins. The framework fits them when it is put
together differently, and the examples of `examples/6-attiny` show how. The
ATmega328P and ATmega2560 keep their shell on the part; nothing below is
needed there.

### The port, and the shell on the PC

`mod/port.h` is a process that does three things when asked: read memory,
flash or EEPROM, write memory or EEPROM, and call a function with up to three
arguments. The shell runs on the PC, as `tools/port.mjs`, and takes every
name and address from the `.elf` file of the sketch:

```sh
node tools/port.mjs 50-port.elf --port /dev/ttyUSB0 ps mods "join ticker blink" ipc
```

The commands and what they print are those of the shell on the part: `ps`,
`mods`, `ipc`, `log`, `stop`, `cont`, `kill`, `run`, `join`, `unjoin`,
`write`. Without a command the tool reads them from its input. It first
checks that the part runs the file it was given.

| `PORT_CONF_LEVEL` | The PC can | ATtiny85, `50-port` |
|-------------------|------------|---------------------|
| 1 look | read: `ps`, `mods`, `ipc`, `log`, settings, views. A monitor. | 3,520 B |
| 2 change | also write memory and EEPROM: `set` | 3,632 B |
| 3 work | also call functions: the whole shell. The default. | 3,836 B |

The same three modules with the shell on the part are 5,914 bytes.

- A request is twelve bytes with a check byte; the port looks at the last
  twelve it received, so a damaged request costs nothing but itself and
  nothing is written from one that does not check out.
- The line belongs to the port. A process that writes text to the same device
  garbles its answers.
- At level 2 or 3 the port is a debug port: whoever reaches the line can do
  anything to the part.
- The port works on any part and any serial device; the examples use it where
  it is needed.

### The tick from the watchdog, and sleep

```cpp
#define TICK_CONF_SOURCE TICK_SOURCE_WATCHDOG   // before <sys/tick.h>
#include <sys/tick.h>
#include <sys/idle.h>

void setup(void) { ...  idle_stop_core_clock();  tick_start(); }
void loop(void)  { process_run();  tick_run();  idle_run(); }
```

- The watchdog interrupt is the tick: no timer and no clock of a core is
  needed, and it runs in the deepest sleep. `TICK_CONF_WATCHDOG` is its
  period, `WDTO_15MS` to `WDTO_8S`. With `TICK_CONF_WATCHDOG_RESET` a tick the
  main loop does not take resets the part.
- `PROCESS_WAIT_TICKS(counter, n)` waits for n ticks, counted in a field of
  the process. It takes the place of a timer.
- `idle_run()` sleeps until the next interrupt when the scheduler has nothing
  to do. `IDLE_CONF_MODE` is `SLEEP_MODE_IDLE` by default, in which timers and
  PWM run on, or `SLEEP_MODE_PWR_DOWN`, in which only the watchdog and the pin
  change interrupts do. The first bit of a request on the serial line wakes
  the part.
- `idle_stop_core_clock()` stops timer 0, which an Arduino core runs for
  `millis()` and which would wake the processor 488 times a second.

### Modules for signals

In these modules a byte is a sample, 0 to 255, not a character.

| Header | Kind | What it does |
|--------|------|--------------|
| `mod/sense.h` | source | Measures every period: the supply in tenths of a volt (`sense_supply`), the temperature of the chip in degrees above -40 C (`sense_chip`), or an ADC pin (`sense_pin`). The first two need no pin and no part. |
| `mod/pulse.h` | source | Writes a 1 for every tick in which the INT0 pin changed: a sign of life. |
| `mod/smooth.h` | filter | Running average; `shift` says how calm. |
| `mod/limit.h` | filter | Turns samples into 1 and 0, with two thresholds so that it does not chatter. |
| `mod/curve.h` | filter | Maps a range of input onto a range of output: 0 below `lo`, then `min` rising to `max` at `hi`. |
| `mod/pwm.h` | sink | A sample is the duty cycle of PB4, 31 kHz. ATtiny25/45/85 only. |
| `mod/record.h` | sink | Keeps the last samples, the lowest and the highest in EEPROM. |
| `mod/keeper.h` | sink | Looks after another board through its reset line; it has two inputs. |

Each of them is two things in one header: plain functions on a small struct
(`smooth_step()` on a `smooth_t`), and a process that wraps them with an input
pipe. A stage as a process costs about 30 bytes of RAM, a stage as a function
costs what its struct takes, so the sketch chooses:

```cpp
/* a process per stage: connected with process_connect(), seen by "ps" and "ipc" */
sense | smooth | curve | pwm

/* or the stages as functions of one process */
pwm_set(curve_map(&self->curve, smooth_step(&self->smooth, sense_read(&self->sense))));
```

`process_connect_pipe(writer, pipe)` connects a writer to one particular pipe
of a reader that has more than one input, as the keeper has.

### Settings

```cpp
#define SETTINGS(X) \
  X(timeout, 4,  "seconds without a heartbeat before the board is reset") \
  X(grace,   10, "seconds a board may take to start")
#include <sys/settings.h>

keeper.timeout = setting(SETTING_timeout);
```

A setting is a named byte. Its value is built in, and it can be changed on the
finished chip:

```sh
node tools/port.mjs 51-keeper.elf --port /dev/ttyUSB0 settings "set timeout 10"
```

The changed values are kept in EEPROM. The names and help texts are kept in
the `.elf` file, in a section that is not loaded, and cost the part nothing;
so do the views a sketch defines with `PORT_VIEW`, such as `resets` of the
keeper and `temperature` of the sentinel.

### Three appliances

| Sketch | What it is | Pins beside the serial line |
|--------|-----------|-----------------------------|
| `51-keeper` | Resets a board when its heartbeat stops, holds it in reset while the supply is low, resets it when the PC says so (which is what opens the upload window of the bootloader in `boot/`), and counts all three in EEPROM. | PB2 heartbeat in, PB3 reset out (pulled low, never driven high), PB4 LED |
| `52-sentinel` | Sleeps with only the watchdog running, wakes every `period` ticks of 8 s, records the temperature of the chip and the supply. | none |
| `53-regulator` | `sense \| smooth \| curve \| pwm`: a fan that follows a temperature, as built. | PB3 sensor in, PB4 PWM out |

Each sketch has two build settings with defaults that follow the RAM of the
part, `<NAME>_CONF_FUSED` and `<NAME>_CONF_PORT`:

| | ATtiny85, 512 B RAM | ATtiny45, 256 B | ATtiny25, 128 B |
|---|---|---|---|
| put together as | a process per stage, the port | one process, the port | one process, no port |
| `51-keeper` | 4,666 B flash, 288 B RAM | 3,916 B, 166 B | 1,804 B, 67 B |
| `52-sentinel` | 3,902 B, 280 B | 3,664 B, 155 B | 1,726 B, 70 B |
| `53-regulator` | 4,492 B, 286 B | 3,566 B, 149 B | 1,592 B, 64 B |

Without the port a part runs on its settings as built, or as a programmer
wrote them into EEPROM, and the records of the sentinel are read with a
programmer.

What was run, in protosim (`tools/test_systems.sh`), on all three parts:

- Keeper: a heartbeat that stops at 6 s is answered with a reset at 10.0 s; a
  supply that sags to 4.0 V holds the board in reset until it is back; a reset
  on request; all three counted; a setting changed through the port.
- Sentinel: both records filled and shown. Asleep for all but 36 to 42 ms of
  ten minutes with a sample every 64 s.
- Regulator: 1.0 V at the input gives 0 %, 2.5 V gives 60 %, 4.5 V gives 100 %;
  `set min 128` while it runs gives 73 %.

protosim runs these parts with their line of two pins on its pseudo terminal
and plays the board around them:

```sh
protosim 51-keeper.elf -m attiny85 -f 8000000 --soft-uart B0,B1,9600 --warp \
    --pin-pull B3:1 --pin-log B3 --vcc 5000 \
    --pin-wave B2:200:0:6000 --vcc 4000@20000 --vcc 5000@24000
```

## The tools

All of them are in `tools/`. The two `.mjs` files need Node.js 22 and nothing
else.

| Tool | Use |
|------|-----|
| `module.mjs <kernel.elf> <module> <slot>` | Builds a loadable module for one kernel build and one slot. `<module>` is the name of a header of `src/mod/`, a header of your own, or a `.c` file that ends with `MODULE(name);`. It reads the slots, the part and the clock from the kernel file. Needs avr-gcc. |
| `port.mjs <sketch.elf> --port <tty> <command> ...` | The shell on the PC for a part that runs a port: the commands of the shell, `settings`, `set`, the views of the sketch, `read`, `poke`, `eeprom`. `--sim` as for `talk.mjs`. |
| `talk.mjs --port <tty> <step> ...` | Talks to a system over its serial line: types commands, waits for the prompt, sends files with XMODEM-CRC (`send:<file>`), checks answers (`expect:<text>`). `--sim "<protosim> <file.elf> -m <mcu>"` talks to a simulated part instead. Linux and macOS. |
| `build_sketch.sh <mcu> <f_cpu> <sketch.ino>` | Builds a sketch with avr-gcc alone, for the simulator and for parts no Arduino core is at hand for. |
| `test_systems.sh` | Builds and runs the systems of `examples/5-systems` and `examples/6-attiny` in protosim and checks each conversation. |

A whole session, on a board with the protoduino bootloader:

```sh
# the kernel: build 42-shell-loader, then send its .elf file to the bootloader
node tools/talk.mjs --port /dev/ttyUSB0 send:42-shell-loader.ino.elf mods

# a module for that build and slot 0, and into the running system with it
node tools/module.mjs 42-shell-loader.ino.elf upper 0
node tools/talk.mjs --port /dev/ttyUSB0 "get 0" send:upper-slot0.elf mods \
     "join upper serial:0" "write upper hello"
```

The Arduino IDE writes the `.elf` file of a sketch with **Sketch > Export
Compiled Binary**. Any terminal program that speaks XMODEM can take the place
of `talk.mjs`.

## Figures

Built with avr-gcc 7.3.0, `-Os` and link-time optimisation.

| System | Part | Flash | RAM |
|--------|------|-------|-----|
| `40-shell`: shell, ticker, blink, upper, vcc | ATmega328P | 7,430 | 517 |
| `41-shell-tiny85`: shell, ticker, blink, vcc, two-pin serial line | ATtiny85 | 5,914 | 275 |
| `42-shell-loader`: shell, ticker, blink, two slots, loader | ATmega328P | 8,738 | 970 |
| the same | ATmega2560 | 8,906 | 1,116 |
| `43-monitor`: monitor, ticker, blink, vcc, upper, keeper | ATmega328P | 6,990 | 557 |
| `50-port`: port, ticker, blink, upper, two-pin serial line | ATtiny85 | 3,836 | 247 |

| Loadable module, ATmega328P | Flash in its slot |
|-----------------------------|-------------------|
| `blink` | 211 |
| `upper` | 303 |
| `ticker` | 480 |
| `vcc` | 645 |

The Arduino IDE builds the same sketches about 200 bytes larger, because its
core brings more than the stand-in of `tools/core`.

## Limits

- Nothing has run on hardware.
- The ATtiny parts do not load modules at run time. With the port the PC
  could do the work of the loader, since these parts can program their own
  flash from any address; that has not been built.
- The ATtiny examples were built with avr-gcc and a stand-in for an Arduino
  core, not with an ATtiny core in the Arduino IDE.
- What the sentinel draws was not measured: a simulator has no current. From
  the data sheet, a part that sleeps with only its watchdog running takes a
  few microamperes.
- simavr does not make the PWM of timer 1 of an ATtiny at its pin, so the
  regulator was checked by the duty cycle the timer is given, not by the pin.
- `pulse` needs the I/O clock to notice an edge, so a keeper sleeps in
  `SLEEP_MODE_IDLE`, not in power-down.
- `port.mjs` and `talk.mjs` were used on protosim's pseudo terminal only, not
  on a real serial port.
- Slots lie in the first 64 KB of flash: the scheduler calls the thread of a
  process through a 16-bit pointer.
- There is one loader, so one module arrives at a time.
- A module is tied to one build of its kernel. After the sketch has changed,
  its modules have to be built and sent again.
- `del` takes a module out of the module table and ends its process. The flash
  of the slot is left as it is until the next module arrives.
- The shell reads one character per turn and has no history or editing beyond
  backspace.

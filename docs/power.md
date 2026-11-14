# Power: sleeping while idle

A cooperative system that waits is a loop that spins: the processor runs at
full power to find, again and again, that nothing has to be done. With sleep
switched on, each pass of the loop ends with a wait for the next thing that
can give a process work, and the processor sleeps until then.

```cpp
#include <protoduino.h>
#include <sys/process.h>
#include <sys/tick.h>

#define PROCESS_CONF_SLEEP PROCESS_SLEEP_WAIT
#include <sys/idle.h>

void setup(void)
{
  process_init(NULL);
  /* ... start the processes ... */
  idle_start();
}

void loop(void)
{
  process_loop(); /* the scheduler, the tick, and sleep until there is work */
}
```

`process_loop()` is `process_run()`, `tick_run()` and `idle_run()` in one
call. With `PROCESS_SLEEP_NONE`, the default, it is the loop as it always was.

## The levels

They are the same words on every platform; what they mean on a chip is in
`src/cpu/<platform>/idle.h`.

| `PROCESS_CONF_SLEEP` | What stops | What ends the wait | What the sketch gives up |
|---|---|---|---|
| `PROCESS_SLEEP_NONE` | nothing | nothing waits | nothing: the default |
| `PROCESS_SLEEP_WAIT` | the processor | any interrupt | nothing: timers, the UART, PWM and the clock of the core run on |
| `PROCESS_SLEEP_DEEP` | the clocks; memory is kept | the tick, pins | the clock of the core (`micros()`, protoduino's timers: wait with `PROCESS_WAIT_TICKS` instead) and the UART receiving; needs a tick that runs in this state |

The question whether there is work and the wait are one step: an interrupt
that arrives in between ends the wait at once. Two things keep a processor
awake: a process that keeps itself busy with events of its own, and a driver
that polls a UART instead of using its interrupt.

A level that loses memory, so that waking up is a restart, is not an idle
level: it ends every protothread. A sketch that wants it asks for it itself.

## On an AVR

| Level | The chip | Datasheet figure, ATmega328P, typical |
|---|---|---|
| `WAIT` | idle mode | a few mA, against about 10 mA running at 16 MHz and 5 V |
| `DEEP` | power-down; the watchdog ticks; the brown-out detector is off while asleep where the part can | a few µA |

- With an Arduino core, timer 0 wakes the processor about once a
  millisecond in idle mode for `millis()`. The shell example (`40-shell`)
  still sleeps 98% of the time in protosim, and answers as before.
- `DEEP` needs `TICK_CONF_SOURCE TICK_SOURCE_WATCHDOG` (the build stops with
  an error otherwise). A UART does not receive in power-down; a serial line
  on two pins does (`sys/process/softserial.h`). `idle_stop_core_clock()`
  stops timer 0 for good.
- `idle_start()` switches off what the build says it does not use:
  `CPU_CONF_POWER_OFF` holds bits for the power reduction register (for
  example `_BV(PRTWI) | _BV(PRSPI)`), and `CPU_CONF_COMPARATOR_OFF` switches
  the analog comparator off.
- On an Arduino board the USB serial chip, the regulator and the power LED
  draw more than the processor, so a sleeping Uno still takes tens of mA. The
  gain is on a bare chip or a board made for batteries.
- The sentinel (`examples/6-attiny/52-sentinel`) sleeps in `DEEP` on an
  8-second watchdog and is awake 36 to 42 ms of almost eleven minutes in
  protosim.

None of the currents above was measured here: a simulator has no current.
What protosim measures is the share of time asleep: its `[SLEEP]` line at
the end of a run.

## Other platforms

protoduino builds for AVR today. The levels were chosen so that other chips
are a file of their own, not exceptions in the scheduler:

| | `WAIT` | `DEEP` | Restart on waking (not an idle level) |
|---|---|---|---|
| AVR | idle mode | power-down | does not exist |
| ESP32 | the loop task blocks until notified or the tick is due; FreeRTOS stops the CPU | the same, with automatic light sleep (about 0.8 mA) | deep sleep, hibernation |
| RP2040 | wait for interrupt | dormant: oscillators off, a pin wakes it (the RTC only on an external clock) | nothing on the chip |
| RP2350 | wait for interrupt | dormant, or power domains switched off with SRAM kept | the core domain off |

Two hooks make that possible:

- The platform file implements `cpu_idle_wait(level, max_ms)`. `max_ms`
  comes from `tick_millis_left()`: how long until the tick is due, so that a
  platform that can sleep for a known time (the ESP32 with tickless idle)
  knows it. An AVR ignores it: its tick is itself an interrupt.
- `PROCESS_CONF_WAKE()` runs after `process_poll()` and a successful
  `process_post()`. It is empty on an AVR, where an interrupt ends the sleep
  by itself. On the ESP32 the loop is a FreeRTOS task that blocks to save
  power, and an interrupt that gives a process work has to wake that task.

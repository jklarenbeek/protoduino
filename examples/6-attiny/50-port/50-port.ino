// file: examples/6-attiny/50-port/50-port.ino

/*
 * 50-port.ino  –  the shell moves to the PC
 * ================================================================
 *
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   Example 41 puts a shell on an ATtiny85, and the shell takes more than
 *   a third of the part. This sketch runs modules with a port in its place:
 *   a process that reads and writes memory and calls functions when it is
 *   asked to, and nothing else. The shell runs on the PC, which has the
 *   .elf file of this sketch and so knows every name and address:
 *
 *     node tools/port.mjs 50-port.elf --port /dev/ttyUSB0 ps mods
 *     serial:0 W 1
 *     port     R 2
 *     ticker   W 3
 *     blink    W 3
 *     0 (free)
 *     1 (free)
 *     2 ticker +
 *     3 blink +
 *
 * HOW FAR THE DOOR OPENS
 * ----------------------
 *   PORT_CONF_LEVEL: 1 look, 2 also settings, 3 also write and call
 *   anything ("join", "stop", "write" ...), 4 also load modules into flash.
 *   Levels 3 and 4 open only for a minute after a reset through the reset
 *   pin, and stay open while they are used: somebody has to be at the
 *   board. Press the reset button, then:
 *
 *     node tools/port.mjs 50-port.elf --port /dev/ttyUSB0 "join ticker blink" ipc
 *
 * MODULES THAT WERE NOT PART OF THE SKETCH   (ATtiny85)
 * ------------------------------------------
 *   Two slots of 1 KB take modules while the system runs. The PC builds a
 *   module for this very build and sends it page by page; the part checks
 *   that each page lies in a free slot, and enters the module into its
 *   table only when the module checks out:
 *
 *     node tools/module.mjs 50-port.elf upper 0
 *     node tools/port.mjs 50-port.elf --port /dev/ttyUSB0 "get 0 upper-slot0.elf" mods
 *     ...
 *     0 upper +
 *
 *   "del 0" takes it out again. A module in the table starts with the
 *   system. The chip programs its own flash for this, which it does only
 *   with the SELFPRGEN fuse programmed: the fuses below say so, and a
 *   programmer has to write them once.
 *
 * THE SMALL PARTS
 * ---------------
 *   - The tick comes from the watchdog, not from a timer, and the ticker
 *     counts ticks. No clock of a core is needed.
 *   - The loop is process_loop() with PROCESS_CONF_SLEEP set: the
 *     processor sleeps until the next interrupt whenever no process has
 *     anything to do (sys/idle.h).
 *
 * WIRING   (ATtiny85 or 45 at 8 MHz, internal oscillator)
 *   PB0  serial in     to TX of a USB serial adapter, 9600 baud
 *   PB1  serial out    to RX of the adapter
 *   PB4  LED
 *   PB5  reset         a button to ground opens levels 3 and 4
 */

#include <protoduino.h>
#include <sys/process.h>
#include <sys/process/softserial.h>

/* the watchdog as the tick, every 16 ms; a digit every 32 ticks */
#define TICK_CONF_SOURCE  TICK_SOURCE_WATCHDOG
#define TICKER_CONF_TICKS 32
#include <sys/tick.h>
#define PROCESS_CONF_SLEEP PROCESS_SLEEP_WAIT
#include <sys/idle.h>

#define BLINK_CONF_BIT 4
#include <mod/ticker.h>
#include <mod/blink.h>

#if defined(__AVR_ATtiny85__)
/* two slots of 1 KB above the sketch, and the table behind them: the
 * sketch has to end below MODULE_CONF_BASE, which tools/module.mjs checks */
#define MODULE_CONF_SUPPORT   1
#define MODULE_CONF_SLOTS     2
#define MODULE_CONF_SLOT_SIZE 1024
#define MODULE_CONF_BASE      0x1500
#define MODULE_CONF_RAM_SIZE  40
#include <sys/module.h>

/* 8 MHz internal oscillator, and self-programming on */
#include <avr/fuse.h>
FUSES = { 0xE2, 0xDF, 0xFE };
#endif

#ifndef PORT_CONF_LEVEL
#if defined(__AVR_ATtiny85__)
#define PORT_CONF_LEVEL PORT_LEVEL_LOAD
#else
#define PORT_CONF_LEVEL PORT_LEVEL_WORK
#endif
#endif
#include <mod/port.h>

PROCESS_INSTANCE(ticker, ticker_proc);
PROCESS_INSTANCE(blink, blink_proc);

/* what the PC may start */
PORT_MODULES_BEGIN
  PORT_MODULE(ticker, ticker_proc)
  PORT_MODULE(blink, blink_proc)
PORT_MODULES_END

void setup(void)
{
  process_init(NULL);

  process_new(&process_type_softserial, &softserial_proc);
  process_new(&process_type_port, &port_proc);
  process_new(&process_type_ticker, &ticker_proc);
  process_new(&process_type_blink, &blink_proc);

#if defined(__AVR_ATtiny85__)
  module_init();       /* which build is this? modules are linked against one */
  module_autostart();  /* what the table lists */
#endif

  /* time comes from the watchdog here: the clock of the core can stop */
  idle_stop_core_clock();
  idle_start();
  tick_start();
}

void loop(void)
{
  process_loop(); /* the scheduler, the tick, and sleep until there is work */
}

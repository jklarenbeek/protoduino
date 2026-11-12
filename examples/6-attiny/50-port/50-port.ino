// file: examples/6-attiny/50-port/50-port.ino

/*
 * 50-port.ino  –  the shell moves to the PC
 * ================================================================
 *
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   Example 41 puts a shell on an ATtiny85, and the shell takes more than
 *   a third of the part. This sketch runs the same modules with a port in
 *   its place: a process that can read memory, write memory and call a
 *   function, and nothing else. The shell itself runs on the PC, which
 *   has the .elf file of this sketch and so knows every name and address:
 *
 *     node tools/port.mjs 50-port.elf --port /dev/ttyUSB0 ps mods
 *     serial:0 W 1
 *     port     R 2
 *     ticker   W 3
 *     blink    W 3
 *     0 ticker +
 *     1 blink +
 *     2 upper -
 *
 *     node tools/port.mjs 50-port.elf --port /dev/ttyUSB0 "join ticker blink" ipc
 *     ok
 *     serial:0 > port
 *     port > serial:0
 *     ticker > blink
 *
 *   "stop", "cont", "kill", "run", "unjoin" and "write" work as they do in
 *   the shell of example 40. With PORT_CONF_LEVEL 1 the port only reads,
 *   and what is left is a monitor.
 *
 * THE SMALL PARTS
 * ---------------
 *   Two more things here are the way of the small parts:
 *
 *   - The tick comes from the watchdog, not from a timer, and the ticker
 *     counts ticks. No clock of a core is needed.
 *   - The loop ends with idle_run(): the processor sleeps until the next
 *     interrupt whenever no process has anything to do.
 *
 * WIRING   (ATtiny85 or 45 at 8 MHz, internal oscillator; an ATtiny25 has
 *           no room for a port beside processes, see the next examples)
 *   PB0  serial in     to TX of a USB serial adapter, 9600 baud
 *   PB1  serial out    to RX of the adapter
 *   PB4  LED
 */

#include <protoduino.h>
#include <sys/process.h>
#include <sys/process/softserial.h>

/* the watchdog as the tick, every 16 ms; a digit every 32 ticks */
#define TICK_CONF_SOURCE  TICK_SOURCE_WATCHDOG
#define TICKER_CONF_TICKS 32
#include <sys/tick.h>
#include <sys/idle.h>

#define BLINK_CONF_BIT 4
#include <mod/ticker.h>
#include <mod/blink.h>
#if defined(__AVR_ATtiny85__)
#include <mod/upper.h> /* a third module, where there is RAM for it */
#endif

/* 1 look, 2 also change, 3 also call: the whole shell */
#ifndef PORT_CONF_LEVEL
#define PORT_CONF_LEVEL 3
#endif
#include <mod/port.h>

PROCESS_INSTANCE(ticker, ticker_proc);
PROCESS_INSTANCE(blink, blink_proc);
#if defined(__AVR_ATtiny85__)
PROCESS_INSTANCE(upper, upper_proc);
#endif

/* what the PC may start */
PORT_MODULES_BEGIN
  PORT_MODULE(ticker, ticker_proc)
  PORT_MODULE(blink, blink_proc)
#if defined(__AVR_ATtiny85__)
  PORT_MODULE(upper, upper_proc)
#endif
PORT_MODULES_END

void setup(void)
{
  process_init(NULL);

  process_new(&process_type_softserial, &softserial_proc);
  process_new(&process_type_port, &port_proc);
  process_new(&process_type_ticker, &ticker_proc);
  process_new(&process_type_blink, &blink_proc);

  /* time comes from the watchdog here: the clock of the core can stop */
  idle_stop_core_clock();
  tick_start();
}

void loop(void)
{
  process_run();
  tick_run();
  idle_run();
}

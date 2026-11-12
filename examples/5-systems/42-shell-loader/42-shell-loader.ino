// file: examples/5-systems/42-shell-loader/42-shell-loader.ino

/*
 * 42-shell-loader.ino  –  a system that takes new modules while it runs
 * ================================================================
 *
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   The shell of example 40 with two flash slots behind its module list.
 *   A module that was not part of this sketch is sent over the line, put
 *   into a slot and started, while the ticker keeps ticking and the LED
 *   keeps blinking:
 *
 *     > mods
 *     0 (free)
 *     1 (free)
 *     2 ticker +
 *     3 blink +
 *     > get 0                  now send upper-slot0.elf with XMODEM-CRC
 *     ok
 *     > mods
 *     0 upper +
 *     1 (free)
 *     2 ticker +
 *     3 blink +
 *     > join upper serial:0
 *     ok
 *     > write upper hello
 *     ok
 *     > HELLO
 *     > del 0
 *     ok
 *
 *   A module in a slot is listed in the module table, so it starts again
 *   with the system after a reset.
 *
 * WHAT IT NEEDS
 * -------------
 *   Somebody in the boot section who programs the pages:
 *
 *   - the protoduino bootloader with its load thread, which this sketch
 *     then schedules instead of carrying its own:
 *
 *       boot/build.sh boot-uno atmega328p default -DBOOT_CONF_LOAD_SERVICE=1
 *
 *   - or Optiboot 8 or later. Then the sketch brings the load thread of
 *     the library: uncomment LOADER_CONF_THREAD below.
 *
 *   A module is built for one build of this sketch and one slot:
 *
 *       node tools/module.mjs <this sketch>.elf upper 0
 *
 *   and sent with any terminal program that speaks XMODEM, or with
 *
 *       node tools/talk.mjs --port /dev/ttyUSB0 "get 0" send:upper-slot0.elf mods
 *
 * THE SETTINGS
 * ------------
 *   They stand before the headers they belong to: the slots and the loader
 *   are compiled as part of this sketch.
 *
 * TRY IT   (115200 baud; ATmega328P or ATmega2560)
 */

#include <protoduino.h>
#include <sys/process.h>
#include <sys/process/serial.h>
#include <sys/tick.h>

/* two slots of 1 KB at 0x6000, the module table behind them: above this
 * sketch and below the bootloader, on both parts */
#define MODULE_CONF_SLOTS     2
#define MODULE_CONF_SLOT_SIZE 1024
#define MODULE_CONF_BASE      0x6000
// #define LOADER_CONF_THREAD 2   /* LOADER_THREAD_OWN: for Optiboot */
#include <mod/loader.h>

#include <mod/ticker.h>
#include <mod/blink.h>
#include <mod/shell.h>

PROCESS_INSTANCE(shell, shell_proc);
PROCESS_INSTANCE(ticker, ticker_proc);
PROCESS_INSTANCE(blink, blink_proc);

SHELL_MODULES_BEGIN
  SHELL_MODULE(ticker, ticker_proc)
  SHELL_MODULE(blink, blink_proc)
SHELL_MODULES_END

void setup(void)
{
  /* the shell is the logger: "log" shows why a load failed */
  process_init(&shell_proc.base);

  process_new(&process_type_serial_process0, &serial_process0_proc);
  serial_process0_proc.baud = 115200;

  /* which build is this? modules are linked against exactly one */
  module_init();

  process_new(&process_type_shell, &shell_proc);
  process_new(&process_type_ticker, &ticker_proc);
  process_new(&process_type_blink, &blink_proc);

  /* what the module table lists */
  module_autostart();
}

void loop(void)
{
  process_run();
  tick_run();
}

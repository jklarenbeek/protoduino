// file: examples/31-shell-tiny85/31-shell-tiny85.ino

/*
 * 31-shell-tiny85.ino  –  the shell of example 30 on an ATtiny85
 * ================================================================
 *
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   The same shell and the same modules on a part with 8 KB of flash,
 *   512 bytes of RAM and no serial port. Three things change, and none of
 *   them is in the shell or in a module:
 *
 *     - the line is two port pins (sys/process/softserial.h):
 *       RX on PB0, TX on PB1, 9600 baud;
 *     - the LED of blink is on PB4;
 *     - buffers are smaller: the settings below.
 *
 * TRY IT   (9600 baud; the line is half duplex, so type, do not paste)
 * ------
 *   > ps
 *   > join ticker blink
 *   > ipc
 *   > stop ticker
 *   > cont ticker
 *   > run 2
 *   > join vcc serial:0
 */

#define SHELL_CONF_LINE 20

#include <protoduino.h>
#include <sys/process.h>
#include <sys/process/softserial.h>
#include <sys/tick.h>

#define SHELL_CONF_DEVICE (&softserial_proc.base)

#include <mod/ticker.h>
#include <mod/blink.h>
#include <mod/vcc.h>
#include <mod/shell.h>

PROCESS_INSTANCE(shell, shell_proc);
PROCESS_INSTANCE(ticker, ticker_proc);
PROCESS_INSTANCE(blink, blink_proc);
PROCESS_INSTANCE(vcc, vcc_proc);

SHELL_MODULES_BEGIN
  SHELL_MODULE(ticker, ticker_proc)
  SHELL_MODULE(blink, blink_proc)
  SHELL_MODULE(vcc, vcc_proc)
SHELL_MODULES_END

void setup(void)
{
  process_init(&shell_proc.base);

  process_new(&process_type_softserial, &softserial_proc);
  process_new(&process_type_shell, &shell_proc);

  process_new(&process_type_ticker, &ticker_proc);
  process_new(&process_type_blink, &blink_proc);
}

void loop(void)
{
  process_run();
  tick_run();
}

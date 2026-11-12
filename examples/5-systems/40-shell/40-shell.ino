// file: examples/5-systems/40-shell/40-shell.ino

/*
 * 40-shell.ino  –  a command line on a running system
 * ================================================================
 *
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   Five processes that know nothing of each other, and a shell that shows
 *   them, pauses them and connects them while they run.
 *
 *     ticker   writes a digit every half second          (mod/ticker.h)
 *     blink    changes the LED for every byte it gets    (mod/blink.h)
 *     upper    passes text on in capitals                (mod/upper.h)
 *     vcc      measures the supply voltage of the chip   (mod/vcc.h)
 *     shell    the command line                          (mod/shell.h)
 *
 *   Each of them is one header that defines one process type. This sketch
 *   only says which of them exist and starts two.
 *
 * TRY IT   (115200 baud, line ending CR or LF)
 * ------
 *   > ps                    what runs:  name, state, priority
 *   > mods                  what can be run
 *   > join ticker blink     the LED changes every half second
 *   > ipc                   who is connected to whom
 *   > stop ticker           the LED stands still ...
 *   > cont ticker           ... and goes on
 *   > run 3                 start the voltage monitor
 *   > join vcc serial:0     its readings appear on this line
 *   > unjoin vcc
 *   > join ticker upper
 *   > join upper serial:0   the digits of the ticker, through a filter
 *   > write blink x         one byte to blink: the LED changes
 *   > kill ticker
 *   > log
 */

#include <protoduino.h>
#include <sys/process.h>
#include <sys/process/serial.h>
#include <sys/tick.h>

#include <mod/ticker.h>
#include <mod/blink.h>
#include <mod/upper.h>
#include <mod/vcc.h>
#include <mod/shell.h>

PROCESS_INSTANCE(shell, shell_proc);
PROCESS_INSTANCE(ticker, ticker_proc);
PROCESS_INSTANCE(blink, blink_proc);
PROCESS_INSTANCE(upper, upper_proc);
PROCESS_INSTANCE(vcc, vcc_proc);

/* What "mods" lists and "run" starts. */
SHELL_MODULES_BEGIN
  SHELL_MODULE(ticker, ticker_proc)
  SHELL_MODULE(blink, blink_proc)
  SHELL_MODULE(upper, upper_proc)
  SHELL_MODULE(vcc, vcc_proc)
SHELL_MODULES_END

void setup(void)
{
  /* The shell is also the logger: "log" shows what the kernel reported. */
  process_init(&shell_proc.base);

  /* The device first: the shell connects to it when it starts. */
  process_new(&process_type_serial_process0, &serial_process0_proc);
  serial_process0_proc.baud = 115200; /* after process_new(), which clears the instance */
  process_new(&process_type_shell, &shell_proc);

  process_new(&process_type_ticker, &ticker_proc);
  process_new(&process_type_blink, &blink_proc);
}

void loop(void)
{
  process_run();
  tick_run();
}

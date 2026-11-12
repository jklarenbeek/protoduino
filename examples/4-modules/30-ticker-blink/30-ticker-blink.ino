// file: examples/4-modules/30-ticker-blink/30-ticker-blink.ino

/*
 * 30-ticker-blink.ino  –  two modules and one pipe
 * ================================================================
 *
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   A module is a process type in a header of its own. This sketch takes
 *   two of them and connects them:
 *
 *     ticker   writes a digit every half second     (mod/ticker.h)
 *     blink    changes the LED for every byte       (mod/blink.h)
 *
 *     ticker | blink        the LED changes every half second
 *
 *   Neither module knows the other. ticker writes to "its output", blink
 *   reads "its input", and process_connect() decides that these are the
 *   same pipe.
 *
 * WHAT TO LOOK AT
 * ---------------
 *   1. The sketch contains no behaviour, only the choice of modules.
 *   2. blink owns the pipe: it publishes it when it starts. So the
 *      connection can only be made once blink has started, which is why it
 *      is made in loop() and not in setup().
 *   3. loop() calls tick_run(): ticker waits for a timer, and the tick is
 *      what gives it a turn to look at it (sys/tick.h).
 */

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

  /* ticker | blink, as soon as blink has an input to offer */
  if (!connected)
    connected = process_connect(&ticker_proc.base, &blink_proc.base);
}

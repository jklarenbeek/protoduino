// file: ./src/mod/ticker.h

#ifndef __MOD_TICKER_H__
#define __MOD_TICKER_H__

/* ticker: a source. Writes one digit to its output every period:
 * 0 1 2 ... 9 0 1 ...
 *
 *   ticker | blink        the LED changes every period
 *   ticker | serial:0     the digits appear on the line
 *
 * - A source never waits for its reader: with no output connected, or with
 *   a full pipe, the digit is dropped.
 * - Needs the tick (sys/tick.h) to notice that its timer has expired.
 * - No static variables: the state is in the process struct, so the same
 *   file builds as a loadable module.
 */

#include <sys/process.h>
#include <sys/tick.h>

#ifndef TICKER_CONF_MILLIS
#define TICKER_CONF_MILLIS 500
#endif

PROCESS_DEFINE(ticker, "ticker", 3,
  struct timer period;  /* time between two digits */
  uint8_t      digit;   /* the digit written last  */
);

PROCESS_THREAD(ticker, ev, data)
{
  process_ticker_t *self = PROCESS_SELF(ticker);
  (void)ev;
  (void)data;

  PROCESS_BEGIN();

  timer_set(&self->period, clock_from_millis(TICKER_CONF_MILLIS));

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(timer_expired(&self->period));
    timer_reset(&self->period);

    if (++self->digit > '9' - '0')
      self->digit = 0;

    if (pt_process->has_pipeout && ipc_pipe_space(PROCESS_PIPEOUT()) > 0)
    {
      uint8_t ch = (uint8_t)('0' + self->digit);
      ipc_pipe_write(PROCESS_PIPEOUT(), &ch, 1);
    }
  }

  /* What "kill" runs. Every process that can be ended needs this block,
   * also when it has nothing to give back: it is where the thread ends. */
  PT_FINALLY(&pt_process->pt)

  PROCESS_END();
}

#endif /* __MOD_TICKER_H__ */

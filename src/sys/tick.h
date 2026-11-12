// file: ./src/sys/tick.h

#ifndef __TICK_H__
#define __TICK_H__

/* The tick: one event for every process, at a steady rate.
 *
 * A process only runs when an event or a poll reaches it. A process that
 * waits for a timer, or for room in a pipe, therefore needs somebody to
 * give it a turn now and then. The tick is that somebody: tick_run() posts
 * EVENT_TICK to all processes once per period, and each of them looks at
 * whatever it is waiting for.
 *
 *   void setup(void)
 *   {
 *     ...
 *     tick_start();
 *   }
 *
 *   void loop(void)
 *   {
 *     process_run();
 *     tick_run();
 *   }
 *
 *   PROCESS_WAIT_EVENT_UNTIL(timer_expired(&self->period));   // a timer
 *   PROCESS_WAIT_TICKS(self->ticks, 30);                      // or 30 ticks
 *
 * - One event in the queue, whatever the number of processes.
 * - The tick carries no data. A process that posts an event to itself to
 *   get its next turn has to tell that event from the tick, by its number
 *   or by its data: one that answers every tick with another event of its
 *   own fills the queue.
 * - A paused process does not get it.
 *
 * Where the tick comes from is a setting, TICK_CONF_SOURCE:
 *
 *   TICK_SOURCE_CLOCK     the clock of the core, micros(). The period is
 *                         TICK_CONF_MILLIS, 16 ms by default. It bounds how
 *                         late a timer is noticed and how fast a writer
 *                         gets through a full pipe.
 *   TICK_SOURCE_WATCHDOG  the watchdog interrupt. It needs no timer and no
 *                         clock of the core, and it also runs while the
 *                         part sleeps with everything else switched off:
 *                         the source for the small parts (see sys/idle.h).
 *                         The period is one of the watchdog's own,
 *                         TICK_CONF_WATCHDOG: WDTO_15MS ... WDTO_8S. The
 *                         watchdog oscillator is good for about 10 %.
 *                         With TICK_CONF_WATCHDOG_RESET the watchdog is a
 *                         watchdog as well: a tick the main loop does not
 *                         take in time resets the part.
 */

#include <protoduino.h>
#include <sys/process.h>

#define TICK_SOURCE_CLOCK    0
#define TICK_SOURCE_WATCHDOG 1

#ifndef TICK_CONF_SOURCE
#define TICK_CONF_SOURCE TICK_SOURCE_CLOCK
#endif

#define EVENT_TICK EVENT_INTERVAL

/**
 * Wait for n ticks, n at least 1. The count is kept in `counter`, a field
 * of the process struct. For a thread whose event and data arguments are
 * called ev and data.
 */
#define PROCESS_WAIT_TICKS(counter, n) \
  do { \
    (counter) = (n); \
    PROCESS_WAIT_EVENT_UNTIL(ev == EVENT_TICK && data == NULL && --(counter) == 0); \
  } while (0)

#if TICK_CONF_SOURCE == TICK_SOURCE_CLOCK

#include <sys/timer.h>

#ifndef TICK_CONF_MILLIS
#define TICK_CONF_MILLIS 16
#endif

static CC_ALWAYS_INLINE void tick_start(void)
{
}

// the clock is looked at on every pass: nothing is ever pending in between
static CC_ALWAYS_INLINE bool tick_is_pending(void)
{
  return false;
}

static CC_ALWAYS_INLINE void tick_run(void)
{
  static clock_time_t next;

  if ((int32_t)(clock_time() - next) >= 0)
  {
    next = clock_time() + clock_from_millis(TICK_CONF_MILLIS);
    process_post(NULL, EVENT_TICK, NULL);
  }
}

#else /* TICK_SOURCE_WATCHDOG */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

// the period of the watchdog: WDTO_15MS, WDTO_30MS ... WDTO_8S
#ifndef TICK_CONF_WATCHDOG
#define TICK_CONF_WATCHDOG WDTO_15MS
#endif

// flag for a watchdog that also watches: the interrupt is switched on
// again by tick_run(), and a timeout without it is a reset.
#ifndef TICK_CONF_WATCHDOG_RESET
#define TICK_CONF_WATCHDOG_RESET 0
#endif

// the watchdog control register under its two names
#ifdef WDTCSR
#define TICK_WDT WDTCSR
#else
#define TICK_WDT WDTCR
#endif

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)

/* The handler sets one bit of a general purpose register and returns. That
 * is a single instruction which changes no flag and needs no register, so
 * the handler saves nothing: four bytes. */
#define TICK_FLAG_REG GPIOR1
#define TICK_FLAG_BIT 0

ISR(WDT_vect, ISR_NAKED)
{
  TICK_FLAG_REG |= _BV(TICK_FLAG_BIT);
  reti();
}

static CC_ALWAYS_INLINE bool tick_is_pending(void)
{
  return (TICK_FLAG_REG & _BV(TICK_FLAG_BIT)) != 0;
}

static CC_ALWAYS_INLINE void tick_take(void)
{
  TICK_FLAG_REG &= (uint8_t)~_BV(TICK_FLAG_BIT);
}

#else

static volatile uint8_t tick_flag;

ISR(WDT_vect)
{
  tick_flag = 1;
}

static CC_ALWAYS_INLINE bool tick_is_pending(void)
{
  return tick_flag != 0;
}

static CC_ALWAYS_INLINE void tick_take(void)
{
  tick_flag = 0;
}

#endif

#define TICK_WDT_BITS \
  ((uint8_t)(_BV(WDIE) | (TICK_CONF_WATCHDOG_RESET ? _BV(WDE) : 0) \
             | ((TICK_CONF_WATCHDOG) & 7) | (((TICK_CONF_WATCHDOG) & 8) ? _BV(WDP3) : 0)))

// start the watchdog as the source of the tick. Call once, in setup().
static inline void tick_start(void)
{
  uint8_t sreg = SREG;

  cli();
  wdt_reset();
  MCUSR &= (uint8_t)~_BV(WDRF); // or the watchdog cannot be changed
  TICK_WDT = _BV(WDCE) | _BV(WDE);
  TICK_WDT = TICK_WDT_BITS;
  SREG = sreg;
}

static CC_ALWAYS_INLINE void tick_run(void)
{
#if TICK_CONF_WATCHDOG_RESET
  /* the main loop is alive: the next timeout is an interrupt again */
  TICK_WDT |= _BV(WDIE);
#endif
  if (tick_is_pending())
  {
    tick_take();
    process_post(NULL, EVENT_TICK, NULL);
  }
}

#endif /* TICK_CONF_SOURCE */

#endif /* __TICK_H__ */

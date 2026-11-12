// file: ./src/sys/idle.h

#ifndef __IDLE_H__
#define __IDLE_H__

/* Idle: sleep while no process has anything to do.
 *
 * A cooperative system that waits is a loop that spins. idle_run() ends a
 * pass of that loop with a sleep when the scheduler has neither a poll nor
 * an event left; the next interrupt wakes the processor, and the loop goes
 * round again.
 *
 *   void loop(void)
 *   {
 *     process_run();
 *     tick_run();
 *     idle_run();
 *   }
 *
 * How deep the sleep is, is a setting, IDLE_CONF_MODE:
 *
 *   SLEEP_MODE_IDLE      the processor stops, timers and the I/O clock run
 *                        on: PWM continues, every interrupt wakes it. The
 *                        default.
 *   SLEEP_MODE_PWR_DOWN  everything stops but the watchdog and the pin
 *                        change interrupts: a few microamperes. Needs the
 *                        watchdog as the source of the tick (sys/tick.h),
 *                        and the ADC switched off by whoever used it.
 *
 * - The question and the sleep are one step: interrupts are off while the
 *   scheduler is asked, and the instruction that switches them on again is
 *   followed by the sleep before any handler can run. An interrupt that
 *   arrives in between wakes the processor at once.
 * - A process that keeps itself busy with events of its own keeps the
 *   processor awake.
 * - So does every interrupt, for a moment. idle_stop_core_clock() stops the
 *   one an Arduino core brings along.
 */

#include <protoduino.h>
#include <sys/process.h>
#include <sys/tick.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#ifndef IDLE_CONF_MODE
#define IDLE_CONF_MODE SLEEP_MODE_IDLE
#endif

/**
 * Stop timer 0, the clock of an Arduino core: micros(), millis() and delay()
 * stand still from here on. For a sketch that takes its time from the
 * watchdog (sys/tick.h). The timer would otherwise wake a processor that
 * sleeps in SLEEP_MODE_IDLE 488 times a second at 8 MHz, for nothing.
 */
static inline void idle_stop_core_clock(void)
{
#if defined(TCCR0B)
  TCCR0B = 0;
#endif
#if defined(TIMSK0)
  TIMSK0 &= (uint8_t)~_BV(TOIE0);
#elif defined(TIMSK)
  TIMSK &= (uint8_t)~_BV(TOIE0);
#endif
#if defined(PRR) && defined(PRTIM0)
  PRR |= _BV(PRTIM0);
#endif
}

static CC_ALWAYS_INLINE void idle_run(void)
{
  cli();
  if (process_is_idle() && !tick_is_pending())
  {
    set_sleep_mode(IDLE_CONF_MODE);
    sleep_enable();
#if defined(sleep_bod_disable) && IDLE_CONF_MODE == SLEEP_MODE_PWR_DOWN
    sleep_bod_disable(); // the brown-out detector sleeps too
#endif
    sei();
    sleep_cpu();
    sleep_disable();
  }
  sei();
}

#endif /* __IDLE_H__ */

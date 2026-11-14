// file: ./src/cpu/avr/idle.h

#ifndef __AVR_IDLE_H__
#define __AVR_IDLE_H__

/* The levels of sys/idle.h on an AVR.
 *
 *   PROCESS_SLEEP_WAIT  idle mode: the CPU stops, timers, the UART, the ADC
 *                       and the clock of the core run on. Any interrupt
 *                       wakes it; with an Arduino core that includes timer 0
 *                       about once a millisecond.
 *   PROCESS_SLEEP_DEEP  power-down: everything stops but the watchdog, the
 *                       pin change interrupts and INT0. A few microamperes on
 *                       a bare chip. Needs TICK_SOURCE_WATCHDOG; the UART
 *                       does not receive (a serial line on pins does, through
 *                       its pin change interrupt, see sys/process/softserial.h).
 *                       The brown-out detector is switched off during the
 *                       sleep where the part can.
 *
 * Settings, before sys/idle.h:
 *
 *   CPU_CONF_POWER_OFF      bits for the power reduction register (PRR) that
 *                           idle_start() sets: peripherals the build does not
 *                           use, e.g. _BV(PRTWI) | _BV(PRSPI). 0 by default.
 *                           A peripheral switched off this way does not work
 *                           until its bit is cleared again.
 *   CPU_CONF_COMPARATOR_OFF 1: switch the analog comparator off. 0 by default.
 *
 * On a board with a USB serial chip, a regulator and a power LED, those draw
 * more than the processor: the gain is on a bare chip or a battery board.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#ifndef CPU_CONF_POWER_OFF
#define CPU_CONF_POWER_OFF 0
#endif

#ifndef CPU_CONF_COMPARATOR_OFF
#define CPU_CONF_COMPARATOR_OFF 0
#endif

#if PROCESS_CONF_SLEEP == PROCESS_SLEEP_DEEP && TICK_CONF_SOURCE != TICK_SOURCE_WATCHDOG
#error "idle: PROCESS_SLEEP_DEEP stops every clock but the watchdog: define TICK_CONF_SOURCE TICK_SOURCE_WATCHDOG"
#endif

/**
 * Stop timer 0, the clock of an Arduino core: micros(), millis() and delay()
 * stand still from here on. For a sketch that takes its time from the
 * watchdog (sys/tick.h): the timer would otherwise wake a processor that
 * waits in idle mode about once a millisecond, for nothing.
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

static inline void cpu_idle_start(void)
{
#if defined(PRR)
  PRR |= (uint8_t)(CPU_CONF_POWER_OFF);
#elif defined(PRR0)
  PRR0 |= (uint8_t)(CPU_CONF_POWER_OFF);
#endif
#if CPU_CONF_COMPARATOR_OFF && defined(ACSR)
  ACSR |= _BV(ACD);
#endif
}

/* Wait at the given level until an interrupt arrives. An AVR does not need
 * the time until the next tick: the tick is itself an interrupt (timer 0
 * through the clock of the core, or the watchdog). */
static CC_ALWAYS_INLINE void cpu_idle_wait(uint8_t level, uint16_t max_ms)
{
  (void)max_ms;

  cli();
  if (process_is_idle() && !tick_is_pending())
  {
    set_sleep_mode(level == PROCESS_SLEEP_DEEP ? SLEEP_MODE_PWR_DOWN : SLEEP_MODE_IDLE);
    sleep_enable();
#if defined(sleep_bod_disable)
    if (level == PROCESS_SLEEP_DEEP)
      sleep_bod_disable(); // the brown-out detector sleeps too
#endif
    sei();      // the instruction after SEI runs before any interrupt:
    sleep_cpu(); // an interrupt that is already pending ends the sleep at once
    sleep_disable();
  }
  sei();
}

#endif /* __AVR_IDLE_H__ */

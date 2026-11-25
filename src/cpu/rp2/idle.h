// file: ./src/cpu/rp2/idle.h

#ifndef __RP2_IDLE_H__
#define __RP2_IDLE_H__

/* The levels of sys/idle.h on the RP2040 and RP2350.
 *
 *   PROCESS_SLEEP_WAIT  the core waits with WFE: its clock stops, the timer,
 *                       the UARTs, PWM and the other core run on. An
 *                       interrupt ends the wait, and so does an event:
 *                       process_poll() and process_post() send one (SEV,
 *                       PROCESS_CONF_WAKE), which is latched, so that work
 *                       given between the look and the wait is not missed.
 *                       Nothing wakes the core by itself on these chips, so
 *                       a timer alarm is set for the next tick.
 *   PROCESS_SLEEP_DEEP  dormant (oscillators off, memory kept): not here yet.
 *                       It needs the clocks set up again after the wake and
 *                       cannot be tested in protosim; the build stops.
 */

#include "pico/time.h"
#include "hardware/sync.h"

#if PROCESS_CONF_SLEEP == PROCESS_SLEEP_DEEP
#error "idle: PROCESS_SLEEP_DEEP (dormant) is not available on the RP2040 and RP2350 yet: use PROCESS_SLEEP_WAIT"
#endif

/* Nothing to switch off by default; peripherals are clocked only when
 * their reset is released. */
static inline void cpu_idle_start(void)
{
}

/* Wait for an interrupt, an event, or max_ms, whichever is first. */
static CC_ALWAYS_INLINE void cpu_idle_wait(uint8_t level, uint16_t max_ms)
{
  (void)level;
  if (process_is_idle() && !tick_is_pending())
  {
    if (max_ms == 0)
      max_ms = 1;
    best_effort_wfe_or_timeout(make_timeout_time_ms(max_ms));
  }
}

#endif /* __RP2_IDLE_H__ */

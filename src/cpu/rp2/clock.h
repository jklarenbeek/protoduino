// file: ./src/cpu/rp2/clock.h

#ifndef __RP2_CLOCK_H__
#define __RP2_CLOCK_H__

/* The clock of the RP2040 and RP2350: the 1 MHz count of the TIMER (TIMER0
 * on the RP2350), the same base as micros() of an Arduino core. */

#include <stdint.h>
#include <cc.h>
#include "hardware/timer.h"

typedef uint32_t clock_time_t;

#define CLOCK_SECOND 1000000L
#define CLOCK_MILLIS 1000L

static CC_ALWAYS_INLINE clock_time_t clock_time(void)
{
  return (clock_time_t)time_us_32();
}

static CC_ALWAYS_INLINE clock_time_t clock_from_seconds(uint16_t seconds)
{
  return (clock_time_t)seconds * CLOCK_SECOND;
}

static CC_ALWAYS_INLINE clock_time_t clock_from_millis(uint16_t milliseconds)
{
  return (clock_time_t)milliseconds * CLOCK_MILLIS;
}

#endif /* __RP2_CLOCK_H__ */

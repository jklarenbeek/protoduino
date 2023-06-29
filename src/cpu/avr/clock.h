
#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <stdint.h>
#include <sys/cc.h>

typedef uint32_t clock_time_t;


#define CLOCK_SECOND 1000000L
#define CLOCK_MILLIS 1000L


/**
 * Get the current clock time.
 *
 * This function returns the current system clock time.
 *
 * \return The current clock time, measured in system ticks.
 */
CC_ALWAYS_INLINE clock_time_t clock_time() {
  return ((clock_time_t)micros());
}

CC_ALWAYS_INLINE clock_time_t clock_from_seconds(uint16_t seconds)
{
  return (clock_time_t)seconds * CLOCK_SECOND;
}

CC_ALWAYS_INLINE clock_time_t clock_from_millis(uint16_t milliseconds)
{
  return (clock_time_t)milliseconds * CLOCK_MILLIS;
}


#endif // __CLOCK_H__

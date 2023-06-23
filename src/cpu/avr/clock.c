#include <sys/clock.h>

/**
 * Get the current clock time.
 *
 * This function returns the current system clock time.
 *
 * \return The current clock time, measured in system ticks.
 */
CC_INLINE clock_time_t clock_time() {
  return ((clock_time_t)micros());
}

CC_INLINE clock_time_t clock_from_seconds(uint16_t seconds)
{
  return (clock_time_t)seconds * CLOCK_SECOND;
}

CC_INLINE clock_time_t clock_from_millis(uint16_t milliseconds)
{
  return (clock_time_t)milliseconds * CLOCK_MILLIS;
}


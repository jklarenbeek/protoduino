#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <stdint.h>
#include "cc.h"

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
CC_EXTERN CC_INLINE clock_time_t clock_time();

CC_EXTERN CC_INLINE clock_time_t clock_from_seconds(uint16_t seconds);

CC_EXTERN CC_INLINE clock_time_t clock_from_millis(uint16_t milliseconds);

#endif // __CLOCK_H__

#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
#include "clock.h"

/**
 * A timer.
 *
 * This structure is used for declaring a timer. The timer must be set
 * with timer_set() before it can be used.
 *
 * \hideinitializer
 */
struct timer {
  clock_time_t start;
  clock_time_t interval;
};

/**
 * Set a timer.
 *
 * This function is used to set a timer for a time sometime in the
 * future. The function timer_expired() will evaluate to true after
 * the timer has expired.
 *
 * \param t A pointer to the timer
 * \param interval The interval before the timer expires.
 *
 */
CC_EXTERN CC_INLINE void timer_set(struct timer *t, const clock_time_t interval);

/**
 * Calculate the difference from start of timer.
 * 
 * This functions calculates the time past from the start of the timer.
 * 
 * \param t A pointer to the timer
 * 
 * \return The differences in microseconds.
 */
CC_EXTERN CC_INLINE clock_time_t timer_diff(const struct timer *t);

/**
 * Check if a timer has expired.
 *
 * This function tests if a timer has expired and returns
 * the difference if true or zero when it has not expired.
 *
 * \param t A pointer to the timer
 *
 * \return Non-zero if the timer has expired, zero otherwise.
 *
 */
CC_EXTERN CC_INLINE uint32_t timer_expired(const struct timer *t);

/**
 * Reset the timer with the same interval.
 *
 * This function resets the timer with the same interval that was
 * given to the timer_set() function. The start point of the interval
 * is the exact time that the timer last expired. Therefore, this
 * function will cause the timer to be stable over time, unlike the
 * timer_restart() function. If this is executed before the
 * timer expired, this function has no effect.
 *
 * \param t A pointer to the timer.
 * \sa timer_restart()
 */
CC_EXTERN CC_INLINE void timer_reset(struct timer *t);

/**
 * Restart the timer from the current point in time
 *
 * This function restarts a timer with the same interval that was
 * given to the timer_set() function. The timer will start at the
 * current time.
 *
 * \note A periodic timer will drift if this function is used to reset
 * it. For preioric timers, use the timer_reset() function instead.
 *
 * \param t A pointer to the timer.
 *
 * \sa timer_reset()
 */
CC_EXTERN CC_INLINE void timer_restart(struct timer *t);

/**
 * The time until the timer expires
 *
 * This function returns the time until the timer expires.
 *
 * \param t A pointer to the timer
 *
 * \return The time until the timer expires
 *
 */
CC_EXTERN CC_INLINE clock_time_t timer_remaining(struct timer *t);

#endif // __TIMER_H__

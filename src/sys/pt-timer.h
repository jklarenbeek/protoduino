/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * Author: Adam Dunkels <adam@sics.se>
 * Author: Joham https://github.com/jklarenbeek
 * 
 * $Id: pt-timer.h,v 1.8 2022/11/04 14:55:17 joham Exp $
 */

/**
 * \file
 * Timer library header file.
 * \author
 * Adam Dunkels <adam@sics.se>
 * Johannes Klarenbeek <jklarenbeek@gmail.com>
 */

/** \addtogroup timers
 * @{ */

/**
 * \defgroup timer Timer library
 *
 * The timer library provides functions for setting, resetting and
 * restarting timers, and for checking if a timer has expired. An
 * application must "manually" check if its timers have expired; this
 * is not done automatically.
 *
 * A timer is declared as a \c struct \c timer and all access to the
 * timer is made by a pointer to the declared timer.
 *
 * \note The timer library is not able to post events when a timer
 * expires. The \ref etimer "Event timers" should be used for this
 * purpose.
 *
 * \note The timer library uses the \ref clock "Clock library" to
 * measure time. Intervals should be specified in the format used by
 * the clock library.
 *
 * \sa \ref etimer "Event timers"
 *
 * @{
 */

#ifndef __PT_TIMER_H__
#define __PT_TIMER_H__

#include <protoduino.h>
#include "pt.h"

typedef uint32_t clock_time_t;

#define CLOCK_SECOND 1000000L
#define CLOCK_MILLIS 1000L

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
 * Get the current clock time.
 *
 * This function returns the current system clock time.
 *
 * \return The current clock time, measured in system ticks.
 */
inline clock_time_t clock_time() {
  return ((clock_time_t)micros());
}

inline clock_time_t get_clock_seconds(uint16_t seconds)
{
  return (clock_time_t)seconds * CLOCK_SECOND;
}

inline clock_time_t get_clock_millis(uint16_t milliseconds)
{
  return (clock_time_t)milliseconds * CLOCK_MILLIS;
}

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
inline void timer_set(struct timer *t, const clock_time_t interval)
{
  t->interval = interval;
  t->start = clock_time();
}

/**
 * Calculate the difference from start of timer.
 * 
 * This functions calculates the time past from the start of the timer.
 * 
 * \param t A pointer to the timer
 * 
 * \return The differences in microseconds.
 */
inline clock_time_t timer_diff(const struct timer *t) {
  register clock_time_t d = clock_time();
  register clock_time_t s = t->start;
  if (d > s) { 
    return d - s; 
  }
  else {
    return (0xFFFFFFFFL - s) + d; 
  }
}

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
inline uint32_t timer_expired(const struct timer *t) {
  register uint32_t d = (uint32_t)timer_diff(t);
  if (t->interval == 0) {
#if defined ARDUSIM
    return std::min(1,(int)d);
#else
    return min(1, d);
#endif
  }
  if (d > t->interval)
    return d;
  else
    return 0L;
}

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
inline void timer_reset(struct timer *t)
{
  if(timer_expired(t)) {
    t->start += t->interval;
  }
}

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
inline void timer_restart(struct timer *t)
{
  t->start = clock_time();
}

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
inline clock_time_t timer_remaining(struct timer *t)
{
  return (t->start + t->interval) - clock_time();
}

/**
 * Puts a protostate in the WAITING state, until an
 * amount of time has expired.
 *
 * \param pt A pointer to the protothread control structure.
 * \param milliseconds The number of milliseconds to wait.
 *
 * \note WARNING! You can not use this macro in a protothread that
 *       has multiple instances, since it will corrupt the timer
 *       state.
 *
 */

#define PT_WAIT_DELAY(pt, milliseconds) \
  do { \
    static struct timer CC_CONCAT2(__DELAY__, __LINE__); \
    timer_set(&CC_CONCAT2(__DELAY__, __LINE__), get_clock_millis(milliseconds)); \
    PT_WAIT_UNTIL(pt, timer_expired(&CC_CONCAT2(__DELAY__, __LINE__))); \
  } while(0);

#endif

/** @} */
/** @} */
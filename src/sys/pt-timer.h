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
 * Johannes Klarenbeek https://github.com/jklarenbeek
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
#include "timer.h"

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
    timer_set(&CC_CONCAT2(__DELAY__, __LINE__), clock_from_millis(milliseconds)); \
    PT_WAIT_UNTIL(pt, timer_expired(&CC_CONCAT2(__DELAY__, __LINE__))); \
  } while(0);

#endif

/** @} */
/** @} */
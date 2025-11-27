// file:./src/sys/pt.h

/*
 * Copyright (c) 2004-2005, Swedish Institute of Computer Science.
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
 * This file is part of the protoduino operating system.
 *
 * Author: Adam Dunkels <adam@sics.se>
 * Author: Joham https://github.com/jklarenbeek
 *
 * $Id: pt.h,v 1.7 2006/10/02 07:52:56 adam Exp $
 * $Id: pt.h,v 1.8 2022/11/04 14:55:17 joham Exp $
 * $Id: pt.h,v 2.0 2023/20/06 03:43:01 joham Exp $
 */

/**
 * \addtogroup pt
 * @{
 */

/**
 * \file
 * Protothreads implementation.
 * \authors
 * Adam Dunkels <adam@sics.se>
 * Joham https://github.com/jklarenbeek
 *
 */

#ifndef __PT_H__
#define __PT_H__

#include <cc.h>
#include "lc.h"

struct pt {
  lc_t lc;
};

typedef enum
{
  PT_WAITING   = 0, // its doing something
  PT_YIELDED   = 1, // as the caller we expect a value
  PT_EXITED    = 2, // exited, but needs to be finalized
  PT_ENDED     = 3, // ended, but needs to be finalized
  PT_ERROR     = 4, // any value >= 4 is an error
  PT_FINALIZED = 255 // this is the end, the thread will never start again
} ptstate_t;

/**
 * \name Initialization
 * @{
 */

/**
 * Initialize a protothread.
 *
 * Initializes a protothread. Initialization must be done prior to
 * starting to execute the protothread.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \sa PT_SPAWN()
 *
 * \hideinitializer
 */
#define PT_INIT(pt)   LC_INIT((pt)->lc)

/**
 * Set the protothread to the finalize state.
 *
 * A protothread should NEVER call PT_FINAL(). Setting the protothread
 * to an exit state must be done by the caller when a protothread exits
 * with PT_EXITED, PT_ENDED or any PT_ERROR states.
 */
#define PT_FINAL(child) LC_FINAL((child)->lc);

/** @} */

/**
 * \name Declaration and definition
 * @{
 */

/**
 * Declaration of a protothread.
 *
 * This macro is used to declare a protothread. All protothreads must
 * be declared with this macro.
 *
 * \param name_args The name and arguments of the C function
 * implementing the protothread.
 *
 * \hideinitializer
 */
#define PT_THREAD(name_args) ptstate_t name_args

/**
 * Declare the start of a protothread inside the C function
 * implementing the protothread.
 *
 * This macro is used to declare the starting point of a
 * protothread. It should be placed at the start of the function in
 * which the protothread runs. All C statements above the PT_BEGIN()
 * invokation will be executed each time the protothread is scheduled.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_BEGIN(pt) { \
  ptstate_t CC_UNUSED PT_ERROR_STATE = PT_ERROR; \
  LC_RESUME((pt)->lc)

/**
 * Declare the end of a protothread.
 *
 * This macro is used for declaring that a protothread ends. It must
 * always be used together with a matching PT_BEGIN() macro. When
 * the protothread structure is not breaked out off in any other arbitrary
 * way then using the protothread library, the protothread will stay in
 * the PT_FINALIZED state.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_END(pt) \
  LC_END((pt)->lc, return PT_FINALIZED); \
  }

/** @} */

/**
 * \name Blocked wait
 * @{
 */

/**
 * Block and wait one cycle
 *
 * This macro blocks the protothread until next entry.
 *
 * \param pt A pointer to the protothread control structure
 *
 * \hideinitializer
 */
#define PT_WAIT_ONE(pt)				\
  do {						\
    LC_RET((pt)->lc, return PT_WAITING);	\
  } while(0)

/**
 * Block and wait until condition is true.
 *
 * This macro blocks the protothread until the specified condition is
 * true.
 *
 * \param pt A pointer to the protothread control structure.
 * \param condition The condition.
 *
 * \hideinitializer
 */
#define PT_WAIT_UNTIL(pt, condition) \
  do { \
    LC_SET((pt)->lc); \
    if(!(condition)) { \
      return PT_WAITING; \
    } \
  } while(0)

/**
 * Block and wait while condition is true.
 *
 * This function blocks and waits while condition is true. See
 * PT_WAIT_UNTIL().
 *
 * \param pt A pointer to the protothread control structure.
 * \param cond The condition.
 *
 * \hideinitializer
 */
#define PT_WAIT_WHILE(pt, cond) PT_WAIT_UNTIL((pt), !(cond))

/** @} */

/**
 * \name Protothread iterators
 * @{
 */

/**
 * Yield from the current protothread.
 *
 * This function will yield the protothread, thereby allowing other
 * processing to take place in the system.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_YIELD(pt) \
  do { \
    LC_RET((pt)->lc, return PT_YIELDED); \
  } while(0)

/**
 * \brief      Yield from the protothread until a condition occurs.
 *
 * \param pt   A pointer to the protothread control structure.
 * \param cond The condition.
 *
 *             This function will yield the protothread, until the
 *             specified condition evaluates to true.
 *
 * \warning I have worked with this macro many times, and want to
 *          caution you of using it. In this case the yield will
 *          remain until the condition is met. But instead, logically
 *          it should return PT_WAITING
 *
 * \hideinitializer
 */
#define PT_YIELD_UNTIL(pt, cond) \
  do { \
    LC_RET((pt)->lc, return PT_YIELDED); \
    if(!(cond)) {	\
      return PT_YIELDED; \
    } \
  } while(0)

/** @} */


/**
 * \name Exiting and restarting
 * @{
 */

/**
 * Restart the protothread.
 *
 * This macro will block and cause the running protothread to restart
 * its execution at the place of the PT_BEGIN() call.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_RESTART(pt)				\
  do {						\
    PT_INIT(pt);				\
    return PT_WAITING;			\
  } while(0)

/**
 * Exit the protothread.
 *
 * This macro causes the protothread to exit. If the protothread was
 * spawned by another protothread, the parent protothread will become
 * unblocked and can continue to run.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_EXIT(pt) \
  do { \
    LC_SET((pt)->lc) return PT_EXITED; \
  } while(0)

/** @} */

/**
 * \name Finalizing Protothreads
 * @{
 */

/**
 * Declare the start of an exit handler
 *
 * This macro is trigger when a protothread exits with
 * PT_EXITED, PT_ENDED or any PT_ERROR. It must be set by the caller
 * in order to execute. If not manually started, a protothread
 * will never just flow into a finally block. It does not recover
 * any error code that was raised, catched or throwed.
 *
 * \param pt A pointer to the protothread control structure.
*/
#define PT_FINALLY(pt) \
  do { \
    LC_SET((pt)->lc) return PT_ENDED; \
    LC_FINALLY((pt)->lc); \
  } while(0);

/** @} */

/**
 * \name Exception Handling
 * @{
 */

/**
 * Raise an exception within the protothread.
 *
 * This macro will block and cause the running protothread to be set
 * in an error state, which is the exception. The exception can be catched with the PT_CATCH()
 * and PT_CATCHANY() macro declarations the next time the protothread
 * is scheduled.
 *
 * \warning err must not be greater then max value ptstate_t -1 and not less the PT_ERROR!
 *
 * \param pt A pointer to the protothread control structure
 * \param err An integer between 4 and 254 representing the error code
 *
 * \hideinitializer
 */
#define PT_RAISE(pt, err) \
  do { \
    LC_RAISE((pt)->lc, err); \
    return PT_WAITING; \
  } while(0)

/**
 * Declare the start of catch block.
 *
 * This macro is used for declaring a protothread with an error
 * handling structure and can occur multiple times within a
 * protothread. The error code MUST be unique within the protothread
 * declaration and catches only one error. When a protothread runs
 * without producing any errors, it will never flow into this macro.
 * A PT_CATCH() declaration will ALWAYS end gracefully with PT_ENDED
 * if the error is not thrown with PT_THROW() or PT_RETHROW().
 *
*/
#define PT_CATCH(pt, err) \
  do { \
    LC_SET((pt)->lc) return PT_ENDED; \
    LC_CATCH((pt)->lc, err); \
    PT_ERROR_STATE = (ptstate_t)LC_ERRDEC((pt)->lc, PT_ERROR); \
  } while(0);

/**
 * Declare the start of a catch any block.
 *
 * This macro is used for declaring a protothread with an error
 * handling structure and can only occur one time within a
 * protothread. It catches all errors not handled by PT_CATCH().
 * When a protothread runs without producing any errors, it will
 * never flow into this macro. A PT_CATCHANY() declaration will
 * ALWAYS end gracefully with PT_ENDED if the error is not thrown
 * with PT_THROW() or PT_RETHROW().
 *
 * \param pt A pointer to the protothread control structure.
*/
#define PT_CATCHANY(pt) \
  do { \
    LC_SET((pt)->lc) return PT_ENDED; \
    LC_CATCHANY((pt)->lc); \
    PT_ERROR_STATE = (ptstate_t)LC_ERRDEC((pt)->lc, PT_ERROR); \
  } while(0);

/**
 * Declare the start of an exit handler
 *
 * This macro is trigger when a protothread exits with
 * PT_EXITED, PT_ENDED or PT_ERROR. It must be set by the caller
 * in order to execute. If not manually started, a protothread
 * will never just flow into a finally block. It does not recover
 * any error code that was raised, catched or throwed.
 *
 * \param pt A pointer to the protothread control structure.
*/
#define PT_FINALLY(pt) \
  do { \
    LC_SET((pt)->lc) return PT_ENDED; \
    LC_FINALLY((pt)->lc); \
  } while(0);

/** @} */


/**
 * \name Error Handling
 * @{
 */

 /**
  * Expression to check if the state is an error
  *
  * \param state the returned state of a protothread
  *
  */
 #define PT_ISERROR(state) (state >= PT_ERROR && state != PT_FINALIZED)

/**
 * Test if error occured after spawning a thread
 *
 * If a child thread is spawn in a protothread, one can
 * test for the error with this macro. Child threads are typically
 * run by PT_WAIT_THREAD, PT_SPAWN or PT_FOREACH.
 *
 * \param state the returned state of a protothread
 *
 */
#define PT_ONERROR(state) \
    if (PT_ISERROR(state))


/**
 * Throws an error within a protothread PT_CATCH() block.
 *
 * This macro causes the protothread to exit with an error.
 * It MUST only be used inside a PT_CATCH() or PT_CACHANY()
 * block. It behaves the same as PT_EXITED or PT_ENDED but
 * with an additional error code encoded.
 *
 * \param pt A pointer to the protothread control structure.
 * \param err An error code within the range of 4 - 254
 *
 */
#define PT_THROW(pt, err) \
  do { \
    LC_SET((pt)->lc) return (PT_ISERROR(err)?(err):PT_ERROR); \
  } while(0)

/**
 * ReThrows an error within a protothread PT_CATCHANY() block.
 *
 * This macro causes the protothread to exit with an error.
 * It must always be used inside a PT_CATCH() or PT_CACHANY()
 * block. It behaves the same as PT_EXITED or PT_ENDED but
 * with an additional error code encoded.
 *
 * \param pt A pointer to the protothread control structure.
 *
 */
#define PT_RETHROW(pt) \
  do { \
    LC_RET((pt)->lc, return PT_ERROR_STATE); \
    return PT_ERROR; \
  } while(0)

/** @} */

/**
 * \name Hierarchical protothreads
 * @{
 */

/**
 * Test if a protothread is running.
 *
 * Warning: The state of the protothread is not saved. Use PT_SCHEDULE
 * instead when inside a protothread. This function schedules a
 * protothread. The return value of the function is non-zero if the
 * protothread is running or zero if the protothread has exited.
 *
 * \param f The call to the C function implementing the protothread to
 * be scheduled
 *
 * \hideinitializer
 */
#define PT_ISRUNNING(f) ((f) < PT_EXITED)

/**
 * Schedule a protothread.
 *
 * This function schedules a protothread. The return value of the
 * function must be of type ptstate_t and is saved in the current
 * protothread local PT_ERROR_STATE variable. Must be used within
 * a PT_BEGIN() and PT_END()/PT_ENDCATCH declaration.
 *
 * \param f The call to the C function implementing the protothread to
 * be scheduled
 *
 * \hideinitializer
 */
#define PT_SCHEDULE(f) (PT_ISRUNNING(PT_ERROR_STATE = (f)))

/**
 * Block and wait until a child protothread completes.
 *
 * This macro schedules a child protothread. The current protothread
 * will block until the child protothread completes. The child
 * protothread MUST be manually initialized with the PT_INIT() function
 * before this function is used.
 *
 * \param pt A pointer to the protothread control structure.
 * \param thread The child protothread with arguments
 *
 * \sa PT_SPAWN()
 *
 * \example
 *
 * PT_WAIT_THREAD(pt, thread(&child));
 * PT_ONERROR(PT_ERROR_STATE) {
 *  PT_RAISE(pt, PT_ERROR_STATE);
 * }
 *
 * \hideinitializer
 */
#define PT_WAIT_THREAD(pt, thread) PT_WAIT_WHILE((pt), PT_SCHEDULE(thread))

/**
 * Spawn a child protothread and wait until it exits.
 *
 * This macro spawns a child protothread and waits until it exits.
 * The macro can only be used within a protothread. It differs from
 * PT_WAIT_THREAD by re-initializing the child protothread.
 *
 * \param pt A pointer to the protothread control structure.
 * \param child A pointer to the child protothread's control structure.
 * \param thread The child protothread with arguments
 *
 * \hideinitializer
 */
#define PT_SPAWN(pt, child, thread)		\
  do {						\
    PT_INIT(child);				\
    PT_WAIT_THREAD(pt, thread);		\
    PT_ONERROR(PT_ERROR_STATE) \
      PT_RAISE(pt, PT_ERROR_STATE); \
  } while(0)

/**
 * Spawn a protothread and wait until it yields.
 *
 * This macro can be nested.
 *
 * see: PT_SPAWN()
 *
 * \param pt A pointer to the protothread control structure.
 * \param child A pointer to the child protothread's control structure.
 * \param thread The child protothread with arguments
 *
 * \hideinitializer
 */
#define PT_FOREACH(pt, child, thread) \
  do { \
    PT_INIT((child)); \
    while(PT_SCHEDULE(thread)) { \
      if (PT_ERROR_STATE == PT_WAITING) \
        PT_WAIT_ONE(pt); \
      else if (PT_ERROR_STATE == PT_YIELDED) \


/**
 * Declare the end of the foreach control structure.
 *
 * This macro is used for declaring that a foreach loop ends.
 * It must always be used together with a matching PT_FOREACH()
 * macro.
 *
 * \todo this is a nonsensical way to finalize it. rewrite.
 *
 * \param pt A pointer to the protothread control structure.
 *
 * \hideinitializer
 */
#define PT_ENDEACH(pt) \
    } \
    if (PT_ERROR_STATE != PT_FINALIZED) { \
      if (PT_ERROR_STATE >= PT_ERROR) \
        PT_RAISE(pt, PT_ERROR_STATE); \
    } \
  } while(0)

/** @} */


#endif /* __PT_H__ */

/** @} */

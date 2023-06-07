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
 * This file is part of the Contiki operating system.
 *
 * Author: Adam Dunkels <adam@sics.se>
 * Author: Joham https://github.com/jklarenbeek
 *
 * $Id: lc-switch.h,v 1.4 2006/06/03 11:29:43 adam Exp $
 * $Id: lc-switch.h,v 1.5 2022/11/04 14:55:17 joham Exp $
 */

/**
 * \addtogroup lc
 * @{
 */


/**
 * @file lc-switch.h
 * @brief Implementation of local continuations based on switch() statement
 * @authors
 *    Adam Dunkels <adam@sics.se>
 *    Joham https://github.com/jklarenbeek
 *
 * @note This implementation of local continuations uses the C switch() statement to resume execution of a function somewhere inside the function's body. It is based on the fact that switch() statements can jump directly into the bodies of control structures such as if() or while() statements.
 *
 * @note This implementation borrows heavily from Simon Tatham's coroutines implementation in C.
 * http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
 * 
 * @warning lc implementation using switch() does not work if an LC_SET() is done within another switch() statement!
 */

#ifndef __LC_SWITCH_H__
#define __LC_SWITCH_H__

#include "cc.h"
#include <limits.h>
#include <stdint.h>

/** \hideinitializer */

/**
 * @typedef lc_t
 * @brief Local continuation type
 */
typedef uint16_t lc_t;

/**
 * @def LC_INIT(s)
 * @brief Initialize the local continuation variable
 * @param s The local continuation variable to be initialized
 */
#define LC_INIT(s) s = 0;

/**
 * @def LC_RESUME(s)
 * @brief Start or resume the local continuation
 * @param s The local continuation variable
 */
#define LC_RESUME(s) switch(s) { case 0:

/**
 * @def LC_SET(s)
 * @brief Set the local continuation to the current line and continue execution
 * @param s The local continuation variable
 */
#define LC_SET(s) s = __LINE__; case __LINE__:

/**
 * @def LC_RET(s, r)
 * @brief Set the local continuation to the current line, execute the specified expression, and continue execution
 * @param s The local continuation variable
 * @param r The expression to be executed
 */
#define LC_RET(s,r) s = __LINE__; r; case __LINE__:

/**
 * @def LC_END(s, r)
 * @brief Set the local continuation to the current line, execute the specified expression, and end the local continuation
 * @param s The local continuation variable
 * @param r The expression to be executed
 */
#define LC_END(s,r) LC_SET(s); r; }

/**
 * @def LC_ERRENC(err)
 * @brief Return an error code as an lc_t value
 * @param err The error code
 * @return The lc_t value representing the error
 * @note returns lc_t as an error ((65535 - 255) + (pstate_t)err)
 */
#define LC_ERRENC(err) ((USHRT_MAX - 255) + err)

/**
 * @def LC_ERRDEC(s, defval)
 * @brief Get the error code from an lc_t value or a default value
 * @param s The lc_t value
 * @param defval The default value if the lc_t value is not an error
 * @return The error code or the default value
 */
#define LC_ERRDEC(s, defval) (s >= LC_ERRENC(0) \
   ? (s - LC_ERRENC(0)) \
   : defval)

/**
 * @def LC_RAISE(s, err)
 * @brief Set the local continuation to an error state
 * @param s The local continuation variable
 * @param err The error code
 */
#define LC_RAISE(s, err) s = LC_ERRENC(err)

/**
 * @def LC_FINAL(s)
 * @brief Set the local continuation to the finalize state
 * @param s The local continuation variable
 */
#define LC_FINAL(s) s = LC_ERRENC(255);

/**
 * @def LC_CATCH(s, r, err)
 * @brief Catch a specific error from the state and execute the specified expression
 * @param s The local continuation variable
 * @param r The expression to be executed
 * @param err The error code to catch
 */
#define LC_CATCH(s, r, err) r; case LC_ERRENC(err):

/**
 * @def LC_CATCHANY(s, r)
 * @brief Catch any error that has not been caught and execute the specified expression
 * @param s The local continuation variable
 * @param r The expression to be executed
 */
#define LC_CATCHANY(s, r) r; default:

/**
 * @def LC_FINALLY(s, r)
 * @brief Catch the final error state and execute the specified expression
 * @param s The local continuation variable
 * @param r The expression to be executed
 */
#define LC_FINALLY(s, r) LC_CATCH(s, r, 255)

#endif /* __LC_SWITCH_H__ */

/** @} */

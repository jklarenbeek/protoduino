// file ./src/sys/lc-counter.h extends ./src/sys/lc.h

/**
 * \addtogroup lc
 * @{
 */

/**
 * @file lc-counter.h
 * @brief Implementation of local continuations based on switch() statement,
 *        with continuation points numbered 1, 2, 3 ...
 *
 * @note This is lc-switch.h with one difference: a continuation point is
 * identified by the next value of the compiler's __COUNTER__ and not by
 * the line it stands on. The numbers stay below 256 in any file with fewer
 * than 255 continuation points, so storing one and comparing against one
 * take a single byte, and the size of a thread no longer depends on where
 * in its file it stands. Select it with -DLC_INCLUDE='"lc-counter.h"'.
 *
 * @note The numbers are unique within a translation unit, also across
 * threads. A thread compiled with this header and one compiled with
 * lc-switch.h can live in the same program: a continuation is only ever
 * read by the code that stored it.
 *
 * @warning As with lc-switch.h: does not work if an LC_SET() is done within
 * another switch() statement!
 */

#ifndef __LC_COUNTER_H__
#define __LC_COUNTER_H__

#include <cc.h>
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
 * @brief Set the local continuation to the next number and continue execution
 * @param s The local continuation variable
 * @note the argument of LC_SET_N() is expanded once, before it is used
 *       twice, so both uses see the same number
 */
#define LC_SET_N(s, n) s = (n); case (n):
#define LC_SET(s) LC_SET_N(s, __COUNTER__ + 1)

/**
 * @def LC_RET(s, r)
 * @brief Set the local continuation to the next number, execute the specified expression, and continue execution
 * @param s The local continuation variable
 * @param r The expression to be executed
 */
#define LC_RET_N(s, r, n) s = (n); r; case (n):
#define LC_RET(s,r) LC_RET_N(s, r, __COUNTER__ + 1)

/**
 * @def LC_END(s, r)
 * @brief Set the local continuation to the current line, execute the specified expression, and end the local continuation
 * @param s The local continuation variable
 * @param r The expression to be executed
 */
#define LC_END(s,r) LC_SET(s) r; }

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
#define LC_FINAL(s) s = LC_ERRENC(255)

/**
 * @def LC_CATCH(s, r, err)
 * @brief Catch a specific error from the state and execute the specified expression
 * @param s The local continuation variable
 * @param r The expression to be executed
 * @param err The error code to catch
 */
#define LC_CATCH(s, err) case LC_ERRENC(err):

/**
 * @def LC_CATCHANY(s, r)
 * @brief Catch any error that has not been caught and execute the specified expression
 * @param s The local continuation variable
 * @param r The expression to be executed
 */
#define LC_CATCHANY(s) default:

/**
 * @def LC_FINALLY(s, r)
 * @brief Catch the final error state and execute the specified expression
 * @param s The local continuation variable
 * @param r The expression to be executed
 */
#define LC_FINALLY(s) LC_CATCH(s, 255)

#endif /* __LC_COUNTER_H__ */

/** @} */

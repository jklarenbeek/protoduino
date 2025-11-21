// file: ./src/cc.h

/**
 * @brief Common definitions and macros for Protoduino
 *
 * This file provides common definitions and macros used in the Protoduino library.
 * It includes various macros for attribute handling, inlining functions, concatenating
 * strings/identifiers, defining constant arrays and strings stored in program memory,
 * declaring externally defined constant arrays and strings, and determining the number
 * of elements in an array.
 *
 */


#ifndef __CC_H__
#define __CC_H__

#include <avr/pgmspace.h>

/**
 * @def CC_UNUSED
 * @brief Macro to mark a variable as unused
 */
#define CC_UNUSED __attribute__ ((unused))

/**
 * @def CC_INLINE
 * @brief Macro to inline a function or code snippet
 */
#define CC_INLINE inline

/**
 * @def CC_ALWAYS_INLINE
 * @brief Macro to define an inline function with the given attribute
 * @param f The function definition
 */
#ifndef CC_ALWAYS_INLINE
#if defined(__GNUC__) && !defined(__MINGW32__)
#define CC_ALWAYS_INLINE __attribute__((__always_inline__)) inline
#elif defined(_MSC_VER)
#define CC_ALWAYS_INLINE __forceinline
#else
#define CC_ALWAYS_INLINE inline
#endif
#endif

/**
 *
*/
#define CC_FLATTEN __attribute__((flatten))

/**
 *
*/
#define CC_NO_INLINE __attribute__((noinline))

/**
 * @def CC_EXTERN
 * @brief Macro to define an extern c function
*/
#ifdef __cplusplus
#define CC_EXTERN extern "C"
#else
#define CC_EXTERN
#endif

/**
 * @def CC_WEAKFN(f)
 * @brief Macro to define a weak function
 * @param f The function definition
 */
#define CC_WEAKFN(f) f __attribute__((weak))

/**
 * @def CC_CONCAT2EXT(s1, s2)
 * @brief Macro to concatenate two strings or identifiers
 * @param s1 The first string or identifier
 * @param s2 The second string or identifier
 */
#define CC_CONCAT2EXT(s1, s2) s1##s2

/**
 * @def CC_CONCAT2(s1, s2)
 * @brief Macro to concatenate two strings or identifiers
 * @param s1 The first string or identifier
 * @param s2 The second string or identifier
 */
#define CC_CONCAT2(s1, s2) CC_CONCAT2EXT(s1, s2)

/**
 * @def CC_CONCAT3EXT(s1, s2, s3)
 * @brief Macro to concatenate three strings or identifiers
 * @param s1 The first string or identifier
 * @param s2 The second string or identifier
 * @param s3 The thirth string or identifier
 */
#define CC_CONCAT3EXT(s1, s2, s3) s1##s2##s3

/**
 * @def CC_CONCAT3(s1, s2, s3)
 * @brief Macro to concatenate three strings or identifiers
 * @param s1 The first string or identifier
 * @param s2 The second string or identifier
 * @param s3 The thirth string or identifier
 */
#define CC_CONCAT3(s1, s2, s3) CC_CONCAT3EXT(s1, s2, s3)

/**
 * @def CC_CONCAT4EXT(s1, s2, s3, s4)
 * @brief Macro to concatenate three strings or identifiers
 * @param s1 The first string or identifier
 * @param s2 The second string or identifier
 * @param s3 The thirth string or identifier
 * @param s4 The fourth string or identifier
 */
#define CC_CONCAT4EXT(s1, s2, s3, s4) s1##s2##s3##s4

/**
 * @def CC_CONCAT4(s1, s2, s3)
 * @brief Macro to concatenate three strings or identifiers
 * @param s1 The first string or identifier
 * @param s2 The second string or identifier
 * @param s3 The thirth string or identifier
 * @param s4 The fourth string or identifier
 */
#define CC_CONCAT4(s1, s2, s3, s4) CC_CONCAT4EXT(s1, s2, s3, s4)

/**
 * @def CC_CONST_PTYPE_ARRAY(type, varname)
 * @brief Macro to define a constant array stored in program memory
 * @param type The data type of the array elements
 * @param varname The name of the variable representing the array
 */
#define CC_CONST_PTYPE_ARRAY(type, varname) const type varname[] PROGMEM

/**
 * @def CC_EXPORT_CONST_PTYPE_ARRAY(type, varname)
 * @brief Macro to declare an externally defined constant array stored in program memory
 * @param type The data type of the array elements
 * @param varname The name of the variable representing the array
 */
#define CC_EXPORT_CONST_PTYPE_ARRAY(type, varname) extern const type varname[]

/**
 * @def CC_CONST_PSTR(varname, value)
 * @brief Macro to define a constant string stored in program memory
 * @param varname The name of the variable representing the string
 * @param value The value of the string
 */
#define CC_CONST_PSTR(varname, value) CC_CONST_PTYPE_ARRAY(char, varname) = value;
//#define CC_CONST_PSTR(var, val) const char var[] PROGMEM = val

/**
 * @def CC_EXPORT_CONST_PSTR(varname)
 * @brief Macro to declare an externally defined constant string stored in program memory
 * @param varname The name of the variable representing the string
 */
#define CC_EXPORT_CONST_PSTR(varname) CC_EXPORT_CONST_PTYPE_ARRAY(char, varname)

/**
 * @def CC_CONST_PSTRUCT_ARR(type, varname)
 * @brief Macro to define a constant array of structures stored in program memory
 * @param type The data type of the structures in the array
 * @param varname The name of the variable representing the array
 */
#define CC_CONST_PSTRUCT_ARR(type, varname) CC_CONST_PTYPE_ARRAY(struct type, varname)
//#define CC_CONST_PSTRUCT_ARR(type, varname) const struct type varname[] PROGMEM

/**
 * @def CC_NELEM(x)
 * @brief Macro to determine the number of elements in an array
 * @param x The array
 * @return The number of elements in the array
 */
#define CC_NELEM(x) (sizeof (x)/sizeof (x)[0])

#define CC_ALIGN(n) __attribute__((__aligned__(n)))

/**
 * @def CC_MAIN_CONSTRUCTOR(name)
 * @brief function called before the void main() entry point
*/
#define CC_MAIN_CONSTRUCTOR(name) void __attribute__((constructor)) name();

/**
 * @def CC_MAIN_DESTRUCTOR(name)
 * @brief function called after the void main() entry point
*/
#define CC_MAIN_DESTRUCTOR(name) void __attribute__((destructor)) name();

#define CC_TMPL_PREFIX unknown
#define CC_TMPL_VAR(name) CC_CONCAT4(_,CC_TMPL_PREFIX,_,name)
#define CC_TMPL_FN(method) CC_CONCAT3(CC_TMPL_PREFIX,_,method)

#define CC_TMPL2_PREFIX unknown2
#define CC_TMPL2_VAR(name) CC_CONCAT4(_,CC_TMPL2_PREFIX,_,name)
#define CC_TMPL2_FN(method) CC_CONCAT3(CC_TMPL2_PREFIX,_,method)

#endif
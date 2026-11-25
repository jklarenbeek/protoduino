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

/* The platform. CC_ARCH_AVR or CC_ARCH_RP2 (RP2040 and RP2350, with the
 * Pico SDK or the arduino-pico core); neither for a build on the host. */
#if defined(__AVR__)
#define CC_ARCH_AVR 1
#elif (defined(PICO_RP2040) && PICO_RP2040) || (defined(PICO_RP2350) && PICO_RP2350) || defined(ARDUINO_ARCH_RP2040)
#define CC_ARCH_RP2 1
#endif

/* Helper for ISR-safety and portability: you may override these macros */
#ifdef __AVR__
#include <util/atomic.h>
#include <avr/pgmspace.h>

typedef float float32_t;
typedef double float64_t;

#define CC_PROGMEM PROGMEM

#define CC_ATOMIC_RESTORE() ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
#define CC_ATOMIC_FORCEON() ATOMIC_BLOCK(ATOMIC_FORCEON)

/* true while interrupts are enabled */
#define CC_IRQ_ENABLED() bit_is_set(SREG, SREG_I)

#else

#include <stdint.h>
#include <string.h>

typedef float float32_t;
typedef double float64_t;

/* Program memory: where flash is in the address space, as on an Arm, a
 * constant is read like any other. An arduino-pico core brings these
 * itself (pgmspace.h). */
#if defined(__has_include)
#if __has_include(<avr/pgmspace.h>)
#include <avr/pgmspace.h>
#endif
#endif
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef PGM_P
#define PGM_P const char *
#endif
#ifndef PSTR
#define PSTR(x)                                 (x)
#endif
#ifndef pgm_read_byte
#define pgm_read_byte(a)                        (*(const uint8_t *)(a))
#define pgm_read_word(a)                        (*(const uint16_t *)(a))
#define pgm_read_dword(a)                       (*(const uint32_t *)(a))
#define pgm_read_ptr(a)                         (*(void *const *)(a))
#endif
#ifndef pgm_read_byte_near
#define pgm_read_byte_near(a)                   pgm_read_byte(a)
#define pgm_read_word_near(a)                   pgm_read_word(a)
#endif
#ifndef strlen_P
#define strlen_P(s)                             strlen(s)
#define strcpy_P(d, s)                          strcpy((d), (s))
#define strncpy_P(d, s, n)                      strncpy((d), (s), (n))
#define strcmp_P(a, b)                          strcmp((a), (b))
#define strncmp_P(a, b, n)                      strncmp((a), (b), (n))
#define memcpy_P(d, s, n)                       memcpy((d), (s), (n))
#endif

#define CC_PROGMEM

#if defined(CC_ARCH_RP2)
/* Interrupts off for a block, as ATOMIC_BLOCK does on an AVR: PRIMASK is
 * saved, and put back when the block is left, by a return too. This holds
 * off interrupts of this core only; what both cores share takes a lock
 * (sys/process.c with PROCESS_CONF_CORES). */
static inline uint32_t cc_irq_save(void)
{
  uint32_t primask;
  __asm volatile("mrs %0, primask\n\tcpsid i" : "=r"(primask)::"memory");
  return primask;
}
static inline void cc_irq_restore(const uint32_t *primask)
{
  __asm volatile("msr primask, %0" ::"r"(*primask) : "memory");
}
static inline void cc_irq_on(const uint32_t *unused)
{
  (void)unused;
  __asm volatile("cpsie i" ::: "memory");
}
#define CC_ATOMIC_RESTORE() \
  for (uint32_t cc_primask_ __attribute__((cleanup(cc_irq_restore))) = cc_irq_save(), cc_once_ = 1; cc_once_; cc_once_ = 0)
#define CC_ATOMIC_FORCEON() \
  for (uint32_t cc_primask_ __attribute__((cleanup(cc_irq_on))) = cc_irq_save(), cc_once_ = 1; cc_once_; cc_once_ = 0)
static inline int cc_irq_enabled(void)
{
  uint32_t primask;
  __asm volatile("mrs %0, primask" : "=r"(primask));
  return !(primask & 1);
}
#define CC_IRQ_ENABLED() cc_irq_enabled()
#else
/* a build on the host: no interrupts */
#define CC_ATOMIC_RESTORE()
#define CC_ATOMIC_FORCEON()
#define CC_IRQ_ENABLED() 1
#endif

#endif /* __AVR__ */

#endif /* __CC_H_ */
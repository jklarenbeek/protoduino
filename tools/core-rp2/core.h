/* core.h - the few things protoduino and its examples take from an Arduino
 * core, for an RP2040 or RP2350 build with the Pico SDK alone: micros(),
 * millis(), delay(), random(), setup() and loop(), and setup1() and loop1()
 * for the second core as in arduino-pico. Force-included (-include) into
 * every file of such a build. See core.c. */
#if !defined(PROTODUINO_TOOLS_CORE_H) && !defined(__ASSEMBLER__)
#define PROTODUINO_TOOLS_CORE_H
#ifdef __cplusplus
extern "C" {
#endif
unsigned long micros(void);
unsigned long millis(void);
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);
long core_random(long howbig);
long core_random_range(long howsmall, long howbig);
void randomSeed(unsigned long seed);
void setup(void);
void loop(void);
void setup1(void) __attribute__((weak));
void loop1(void) __attribute__((weak));
#ifdef __cplusplus
}
static inline long random(long howbig) { return core_random(howbig); }
static inline long random(long howsmall, long howbig) { return core_random_range(howsmall, howbig); }
#endif
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#include <string.h>
#endif

/* core.h - the few things protoduino takes from an Arduino core, for a build
 * without one: micros(), millis(), delay(), random(), setup() and loop(). Force-included (-include) into
 * every file of such a build. See core.c. */
#if !defined(PROTODUINO_TOOLS_CORE_H) && !defined(__ASSEMBLER__)
#define PROTODUINO_TOOLS_CORE_H
#ifdef __cplusplus
extern "C" {
#endif
unsigned long micros(void);
unsigned long millis(void);
void delay(unsigned long ms);
long core_random(long howbig);
long core_random_range(long howsmall, long howbig);
void randomSeed(unsigned long seed);
void setup(void);
void loop(void);
#ifdef __cplusplus
}
static inline long random(long howbig) { return core_random(howbig); }
static inline long random(long howsmall, long howbig) { return core_random_range(howsmall, howbig); }
#endif
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#include <string.h>
#endif

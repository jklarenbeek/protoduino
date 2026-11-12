/* core.h - the few things protoduino takes from an Arduino core, for a build
 * without one: micros(), setup() and loop(). Force-included (-include) into
 * every file of such a build. See core.c. */
#ifndef PROTODUINO_TOOLS_CORE_H
#define PROTODUINO_TOOLS_CORE_H
#ifdef __cplusplus
extern "C" {
#endif
unsigned long micros(void);
void setup(void);
void loop(void);
#ifdef __cplusplus
}
#endif
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#include <string.h>
#endif

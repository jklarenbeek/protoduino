/* core.h - the few things protoduino takes from the Arduino core, for a
 * build without it. Force-included (-include) into every file. */
#if !defined(KERNEL_CORE_H) && !defined(__ASSEMBLER__)
#define KERNEL_CORE_H
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

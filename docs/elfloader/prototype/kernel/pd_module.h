/* pd_module.h - what the kernel and a module agree on (prototype). */
#ifndef PD_MODULE_H
#define PD_MODULE_H
#include <stdint.h>

#define PD_MODULE_MAGIC 0x4D70      /* 'p' 'M' */

/* First bytes of every module; the ELF entry point is its address. */
typedef struct {
  uint16_t magic;
  uint8_t  abi;
  uint8_t  flags;
  uint16_t kernel_id;               /* identity of the kernel build it was linked against */
  uint8_t  (*thread)(void *self);   /* stands in for a protoduino process thread */
  uint8_t  size;                    /* bytes of state the kernel must provide */
} pd_module_t;

/* what the kernel offers to modules in this demo */
uint8_t kernel_add(uint8_t a, uint8_t b);
void    kernel_note(uint8_t v);

#endif

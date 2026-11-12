/* count_mod.c - a loadable module: no start-up code, no static RAM.
 * Its state is the byte the kernel hands it; it calls two kernel functions. */
#include "pd_module.h"

extern const uint8_t __pd_kernel_id;                 /* value supplied at link time */

static uint8_t count_thread(void *self)
{
  uint8_t *n = self;
  (*n)++;
  kernel_note(kernel_add(*n, 0x40));
  return 0;
}

const pd_module_t pd_module __attribute__((section(".pd.module"), used)) = {
  PD_MODULE_MAGIC, 1, 0, (uint16_t)&__pd_kernel_id, count_thread, 1
};

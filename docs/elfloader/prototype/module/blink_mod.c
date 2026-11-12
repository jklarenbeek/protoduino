/* A module: no startup code, no static RAM, entry point = its descriptor. */
#include <avr/io.h>
#include <avr/pgmspace.h>
extern uint8_t kernel_add(uint8_t a, uint8_t b);
extern void    kernel_led(uint8_t on);
extern const uint8_t __pd_kernel_id;                 /* value supplied at link time */
typedef struct { uint16_t magic; uint8_t abi, flags; uint16_t kernel_id; uint8_t (*thread)(void *self); uint8_t size; } pd_module_t;
static uint8_t blink_thread(void *self) { uint8_t *n = self; kernel_led(kernel_add(*n, 1) & 1); (*n)++; return 0; }
const pd_module_t pd_module __attribute__((section(".pd.module"), used)) = { 0x4D70, 1, 0, (uint16_t)&__pd_kernel_id, blink_thread, 1 };

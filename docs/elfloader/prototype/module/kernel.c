#include <avr/io.h>
#include <avr/pgmspace.h>
__attribute__((noinline, used)) uint8_t kernel_add(uint8_t a, uint8_t b) { return a + b; }
__attribute__((noinline, used)) void kernel_led(uint8_t on) { if (on) PORTB |= 0x20; else PORTB &= ~0x20; }
int main(void) { DDRB = 0x20; for (;;) kernel_led(kernel_add(PINB, 1) & 1); }

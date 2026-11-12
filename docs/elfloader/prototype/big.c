#include <avr/io.h>
#include <avr/pgmspace.h>
#define ROW(n) n, n+1, n+2, n+3, n+4, n+5, n+6, n+7
#define R8(n)  ROW(n), ROW(n+8), ROW(n+16), ROW(n+24), ROW(n+32), ROW(n+40), ROW(n+48), ROW(n+56)
#define R64(n) R8(n), R8(n+64), R8(n+128), R8(n+192)
#define K1 R64(0), R64(3), R64(7), R64(11)
#define K8 K1, K1, K1, K1, K1, K1, K1, K1
#define FAR(name) const uint8_t name[] __attribute__((section(".fini0"), used)) = { K8, K8 }
const uint8_t t0[] PROGMEM = { K8 };
FAR(f0); FAR(f1); FAR(f2); FAR(f3); FAR(f4); FAR(f5);
volatile uint8_t sel = 7;
int main(void) { for (;;) { PORTB = pgm_read_byte(&t0[sel]) + pgm_read_byte_far(pgm_get_far_address(f5) + sel); } }

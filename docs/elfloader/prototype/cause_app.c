/* cause_app.c - an image that says once, at its start, which reset cause the bootloader
 * handed it (GPIOR0) and what it finds in MCUSR. -DTAG='T' gives it another letter, to
 * tell a newly uploaded image from the previous one. */
#include <avr/io.h>

#ifndef TAG
#define TAG 'S'
#endif

static void put(char c) { while (!(UCSR0A & _BV(UDRE0))) { } UDR0 = c; }
static void hex(uint8_t v) { const char *d = "0123456789abcdef"; put(d[v >> 4]); put(d[v & 15]); }

int main(void)
{
  uint8_t passed = GPIOR0, mcusr = MCUSR;

  UCSR0A = _BV(U2X0); UBRR0H = 0; UBRR0L = 16; UCSR0B = _BV(TXEN0);
  put('['); put(TAG); put(' '); hex(passed); put(' '); hex(mcusr); put(']');
  for (;;) { }
}

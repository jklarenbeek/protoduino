/* wdt_app.c - an application that is reset by the watchdog: it says that it started
 * and what it finds as reset cause, arms the watchdog at 250 ms and never feeds it.
 * This is what a hung application looks like, and also how an application resets
 * itself on purpose. After a watchdog reset the AVR keeps the watchdog running at
 * 16 ms until software clears WDRF and switches it off: a bootloader that waits
 * for a sender has to deal with that, or it is reset again before it gets anywhere. */
#include <avr/io.h>
#include <avr/wdt.h>

static void put(char c) { while (!(UCSR0A & _BV(UDRE0))) { } UDR0 = c; }
static void hex(uint8_t v) { const char *d = "0123456789abcdef"; put(d[v >> 4]); put(d[v & 15]); }

int main(void)
{
  uint8_t passed = GPIOR0, mcusr = MCUSR;      /* what the bootloader hands over; the register itself */

  UCSR0A = _BV(U2X0); UBRR0H = 0; UBRR0L = 16; UCSR0B = _BV(TXEN0);
  put('['); put('A'); put(' '); hex(passed); put(' '); hex(mcusr); put(']');
  while (!(UCSR0A & _BV(TXC0))) { }
  wdt_enable(WDTO_250MS);
  for (;;) { }
}

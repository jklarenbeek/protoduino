/* wdt_probe.c - how long after a watchdog reset does the watchdog reset the part again,
 * if software leaves it alone? On the device: 16 ms, the shortest period, because the
 * reset puts the prescaler back to zero and WDRF keeps the watchdog enabled.
 * The program arms the watchdog at 250 ms once, hangs, and after the reset only prints
 * a dot every 10 ms without touching the watchdog or MCUSR. */
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

static void put(char c) { while (!(UCSR0A & _BV(UDRE0))) { } UDR0 = c; }
static void hex(uint8_t v) { const char *d = "0123456789abcdef"; put(d[v >> 4]); put(d[v & 15]); }

int main(void)
{
  uint8_t cause = MCUSR;

  UCSR0A = _BV(U2X0); UBRR0L = 16; UCSR0B = _BV(TXEN0);
  put('\n'); put('m'); hex(cause); put(' '); put('w'); hex(WDTCSR); put(' ');
  if (!(cause & _BV(WDRF))) {
    while (!(UCSR0A & _BV(TXC0))) { }
    wdt_enable(WDTO_250MS);
    for (;;) { }
  }
  for (uint16_t i = 0; i < 400; i++) { put('.'); _delay_ms(10); }
  put('D'); put('O'); put('N'); put('E'); put('\n');
  for (;;) { }
}

/* reset_probe.c - watchdog reset under the simulator: reset cause flags, and
 * whether protosim keeps injecting --uart0-in bytes after the reset. */
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
static void put(char c) { while (!(UCSR0A & _BV(UDRE0))) { } UDR0 = c; }
static void puts_(const char *s) { while (*s) put(*s++); }
static void hex(uint8_t v) { const char *d = "0123456789abcdef"; put(d[v >> 4]); put(d[v & 15]); }
int main(void)
{
  uint8_t cause = MCUSR, n = 0;
  MCUSR = 0;
  wdt_disable();
  UCSR0A = _BV(U2X0); UBRR0L = 16; UCSR0B = _BV(TXEN0);          /* TX only: receiver still off */
  puts_("start mcusr="); hex(cause); put('\n');
  if (!(cause & _BV(WDRF))) {
    puts_("first run: receiver off for 15 ms, then watchdog reset\n");
    while (!(UCSR0A & _BV(TXC0))) { }
    wdt_enable(WDTO_15MS);
    for (;;) { }
  }
  UCSR0B = _BV(TXEN0) | _BV(RXEN0);
  puts_("after reset, received: ");
  for (uint16_t i = 0; i < 60000; i++) {
    if (UCSR0A & _BV(RXC0)) { put(UDR0); n++; }
    _delay_us(10);
  }
  puts_(" count="); hex(n); puts_("\nDONE\n");
  for (;;) { }
}

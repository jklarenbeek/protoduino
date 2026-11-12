/* late_rx_probe.c - the receiver is enabled 5 ms after start; which injected bytes arrive? */
#include <avr/io.h>
#include <util/delay.h>
static void put(char c) { while (!(UCSR0A & _BV(UDRE0))) { } UDR0 = c; }
static void puts_(const char *s) { while (*s) put(*s++); }
int main(void)
{
  UCSR0A = _BV(U2X0); UBRR0L = 16; UCSR0B = _BV(TXEN0);
  _delay_ms(5);
  UCSR0B = _BV(TXEN0) | _BV(RXEN0);
  puts_("received: ");
  for (uint16_t i = 0; i < 40000; i++) { if (UCSR0A & _BV(RXC0)) put(UDR0); _delay_us(10); }
  puts_("\nDONE\n");
  for (;;) { }
}

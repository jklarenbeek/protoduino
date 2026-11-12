/* erased_probe.c - jump into erased flash (all 0xFF) and see what the simulator does.
 * On a real AVR 0xFFFF behaves like "sbrs r31,7": execution runs on to the end of
 * flash and wraps to address 0, so this program would start again and print twice. */
#include <avr/io.h>
static void put(char c) { while (!(UCSR0A & _BV(UDRE0))) { } UDR0 = c; }
static void puts_(const char *s) { while (*s) put(*s++); }
static uint8_t runs __attribute__((section(".noinit")));
int main(void)
{
  UCSR0A = _BV(U2X0); UBRR0L = 16; UCSR0B = _BV(TXEN0);
  if (runs == 0xA5) { puts_("second start: execution ran through erased flash\nDONE\n"); for (;;) { } }
  runs = 0xA5;
  puts_("first start: jumping into erased flash at 0x6000\n");
  while (!(UCSR0A & _BV(TXC0))) { }
  ((void (*)(void))(0x6000 / 2))();
  puts_("returned?!\nDONE\n");
  for (;;) { }
}

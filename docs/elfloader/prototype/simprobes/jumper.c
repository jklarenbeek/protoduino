/* Jump into erased flash with a chosen r31; a loader sits at the top of flash. */
#include <avr/io.h>
static void put(char c) { while (!(UCSR0A & _BV(UDRE0))) { } UDR0 = c; }
int main(void)
{
  UCSR0A = _BV(U2X0); UBRR0L = 16; UCSR0B = _BV(TXEN0);
  put('J'); while (!(UCSR0A & _BV(TXC0))) { }
  __asm__ __volatile__("ldi r31, 0x80\n jmp 0x6002\n");
  for (;;) { }
}

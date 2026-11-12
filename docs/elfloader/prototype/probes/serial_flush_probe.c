/* serial0_flush() behaviour with interrupts off vs on (protoduino serial layer). */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <protoduino.h>
#include <sys/serial.h>

static void put(char c) { while (!(UCSR0A & _BV(UDRE0))) { } UDR0 = c; }
static void puts_(const char *s) { while (*s) put(*s++); }
static void putu(uint16_t v) { char b[6]; uint8_t i = 0; do { b[i++] = '0' + v % 10; v /= 10; } while (v); while (i) put(b[--i]); }

int main(void)
{
  uint8_t before, after;
  uint16_t spins;

  serial0_open(9600);

#if MODE == 0
  /* interrupts OFF: queue bytes, then call flush() repeatedly */
  cli();
  for (char c = 'a'; c <= 'j'; c++) serial0_write8(c);
  before = serial0_flush();
  for (spins = 0; spins < 60000 && serial0_flush() > 0; spins++) { }
  after = serial0_flush();
  _delay_ms(20);
  UCSR0B &= ~_BV(UDRIE0);
  puts_("\n[irq-off] left_before="); putu(before);
  puts_(" left_after="); putu(after);
  puts_(" flush_calls="); putu(spins);
#else
  /* interrupts ON: queue bytes, then call flush() repeatedly */
  sei();
  for (char c = 'a'; c <= 'j'; c++) serial0_write8(c);
  before = serial0_flush();
  for (spins = 0; spins < 60000 && serial0_flush() > 0; spins++) { }
  after = serial0_flush();
  _delay_ms(20);
  cli();
  UCSR0B &= ~_BV(UDRIE0);
  puts_("\n[irq-on] left_before="); putu(before);
  puts_(" left_after="); putu(after);
  puts_(" flush_calls="); putu(spins);
#endif
  puts_("\nDONE\n");
  for (;;) { }
}

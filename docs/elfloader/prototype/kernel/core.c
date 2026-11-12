/* core.c - stand-in for the Arduino core: timer 0 as in wiring.c, micros(),
 * and a main() that calls setup() once and loop() for ever. */
#include <avr/io.h>
#include <avr/interrupt.h>

static volatile unsigned long timer0_overflow_count;

ISR(TIMER0_OVF_vect)
{
  timer0_overflow_count++;
}

unsigned long micros(void)
{
  unsigned long m;
  uint8_t sreg = SREG, t;

  cli();
  m = timer0_overflow_count;
  t = TCNT0;
  if ((TIFR0 & _BV(TOV0)) && t < 255)
    m++;
  SREG = sreg;
  return ((m << 8) + t) * 4;   /* prescaler 64 at 16 MHz: 4 us per count */
}

int main(void)
{
  TCCR0B = _BV(CS01) | _BV(CS00);
  TIMSK0 = _BV(TOIE0);
  sei();
  setup();
  for (;;)
    loop();
}

/* core.c - stand-in for an Arduino core: timer 0 as in wiring.c, micros(),
 * and a main() that calls setup() once and loop() for ever. For builds with
 * avr-gcc alone, such as the simulator runs of tools/. */
#include <avr/io.h>
#include <avr/interrupt.h>

#ifdef TIMSK0
#define CORE_TIMSK TIMSK0
#define CORE_TIFR  TIFR0
#else
#define CORE_TIMSK TIMSK /* ATtiny25/45/85 */
#define CORE_TIFR  TIFR
#endif

/* timer 0 counts the clock divided by 64 */
#define CORE_MICROS_PER_COUNT (64000000UL / F_CPU)

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
  if ((CORE_TIFR & _BV(TOV0)) && t < 255)
    m++;
  SREG = sreg;
  return ((m << 8) + t) * CORE_MICROS_PER_COUNT;
}

extern void setup(void);
extern void loop(void);

int main(void)
{
  TCCR0B = _BV(CS01) | _BV(CS00);
  CORE_TIMSK |= _BV(TOIE0);
  sei();
  setup();
  for (;;)
    loop();
}

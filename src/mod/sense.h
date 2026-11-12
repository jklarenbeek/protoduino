// file: ./src/mod/sense.h

#ifndef __MOD_SENSE_H__
#define __MOD_SENSE_H__

/* sense: a source of samples. Measures a voltage every period and writes
 * it to its output as one byte.
 *
 *   sense | smooth | curve | pwm       a pin, turned into a duty cycle
 *   sense | limit                      the supply, turned into "low" and "ok"
 *   sense | record                     the temperature, kept in EEPROM
 *
 * In the modules for signals a byte is a sample, not a character: 0 to 255,
 * and what that means is between the source and whoever reads it.
 *
 * What it measures is set in the instance, after process_new(). Three
 * functions do it for the usual cases:
 *
 *   sense_supply(&p.sense)   the supply voltage, in tenths of a volt: 50 at 5 V.
 *                      Needs no pin and no part: the ADC measures its own
 *                      1.1 V reference against the supply.
 *   sense_chip(&p.sense)     the temperature of the chip, in degrees above -40 C:
 *                      65 at 25 C. Give or take ten: the sensor differs
 *                      from chip to chip, and SENSE_CONF_TEMP_OFFSET is the
 *                      place to correct it.
 *   sense_pin(&p.sense, n)   ADC channel n against the supply: 0 to 255.
 *
 * and each of them sets these, which can also be set by hand:
 *
 *   sense.admux    what is measured, as the ADMUX register takes it
 *   sense.offset   taken off the 10-bit reading
 *   sense.shift    the rest is shifted right by this
 *   sense.inverse  when not 0: the sample is this divided by the reading
 *
 * `period`, beside them in the instance, is the number of ticks between two
 * samples (sys/tick.h).
 *
 * What does not fit a byte is 255.
 *
 * - The measuring itself is sense_read(), a plain function on a sense_t,
 *   for a sketch that has no RAM for a process per stage and calls the
 *   stages itself (examples/6-attiny).
 * - A measurement takes about 1.5 ms, and the process does not let go of
 *   the processor for that time: the reference needs a moment to settle
 *   and the first conversion after a change is thrown away.
 * - The ADC is switched off after every measurement. Left on, it would
 *   cost a sleeping part a quarter of a milliampere.
 * - A source never waits for its reader: with no output connected, or with
 *   a full pipe, nothing is measured.
 * - Needs the tick. No static variables.
 */

#include <sys/process.h>
#include <sys/tick.h>
#include <avr/io.h>
#include <util/delay.h>

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#define SENSE_VCC     (_BV(MUX3) | _BV(MUX2))
#define SENSE_TEMP    (_BV(REFS1) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1) | _BV(MUX0))
#define SENSE_PIN(n)  (n)
#else
#define SENSE_VCC     (_BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1))
#define SENSE_TEMP    (_BV(REFS1) | _BV(REFS0) | _BV(MUX3))
#define SENSE_PIN(n)  (_BV(REFS0) | (n))
#endif

// 1.1 V at 1024 steps, in tenths of a volt: divided by the reading of the
// reference it is the supply
#define SENSE_SUPPLY_INVERSE 11264u

// the reading of the temperature sensor at -40 C, typically
#ifndef SENSE_CONF_TEMP_OFFSET
#define SENSE_CONF_TEMP_OFFSET 235
#endif

// microseconds a reference gets to settle
#ifndef SENSE_CONF_SETTLE
#define SENSE_CONF_SETTLE 1000
#endif

// the ADC wants 50 to 200 kHz
#if F_CPU >= 12800000UL
#define SENSE_ADPS (_BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0))
#elif F_CPU >= 6400000UL
#define SENSE_ADPS (_BV(ADPS2) | _BV(ADPS1))
#elif F_CPU >= 3200000UL
#define SENSE_ADPS (_BV(ADPS2) | _BV(ADPS0))
#else
#define SENSE_ADPS (_BV(ADPS1) | _BV(ADPS0))
#endif

typedef struct sense {
  uint8_t  admux;   /* what is measured                  */
  uint16_t offset;  /* taken off the reading             */
  uint8_t  shift;   /* the rest is shifted right by this */
  uint16_t inverse; /* or: this divided by the reading   */
} sense_t;

static uint8_t sense_read(const sense_t *s)
{
  uint16_t x;
  uint8_t i;

  ADMUX  = s->admux;
  ADCSRA = _BV(ADEN) | SENSE_ADPS;
  _delay_us(SENSE_CONF_SETTLE);
  for (i = 2; i; i--) // the first one is thrown away
  {
    ADCSRA |= _BV(ADSC);
    while (ADCSRA & _BV(ADSC)) { }
  }
  x = ADC;
  ADCSRA = 0;

  if (s->inverse)
    x = x ? s->inverse / x : 255;
  else
    x = (x > s->offset ? x - s->offset : 0) >> s->shift;
  return x > 255 ? 255 : (uint8_t)x;
}

static inline void sense_supply(sense_t *s)
{
  s->admux   = SENSE_VCC;
  s->inverse = SENSE_SUPPLY_INVERSE;
}

static inline void sense_chip(sense_t *s)
{
  s->admux   = SENSE_TEMP;
  s->offset  = SENSE_CONF_TEMP_OFFSET;
  s->shift   = 0;
  s->inverse = 0;
}

static inline void sense_pin(sense_t *s, uint8_t channel)
{
  s->admux   = SENSE_PIN(channel);
  s->offset  = 0;
  s->shift   = 2;
  s->inverse = 0;
}

PROCESS_DEFINE(sense, "sense", 3,
  sense_t sense;   /* what is measured, and how it is scaled */
  uint8_t period;  /* ticks between two samples              */
  uint8_t ticks;   /* ticks until the next one               */
  uint8_t byte;    /* the sample                             */
);

PROCESS_THREAD(sense, ev, data)
{
  process_sense_t *self = PROCESS_SELF(sense);

  PROCESS_BEGIN();

  while (1)
  {
    PROCESS_WAIT_TICKS(self->ticks, self->period ? self->period : 1);

    if (pt_process->has_pipeout && ipc_pipe_space(PROCESS_PIPEOUT()) > 0)
    {
      self->byte = sense_read(&self->sense);
      ipc_pipe_write(PROCESS_PIPEOUT(), &self->byte, 1);
    }
  }

  PT_FINALLY(&pt_process->pt)
  {
    ADCSRA = 0;
  }

  PROCESS_END();
}

#endif /* __MOD_SENSE_H__ */

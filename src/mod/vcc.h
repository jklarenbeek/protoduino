// file: ./src/mod/vcc.h

#ifndef __MOD_VCC_H__
#define __MOD_VCC_H__

/* vcc: a sensor. Measures the supply voltage of the chip and writes it to
 * its output every period, as text: "5.0V" and a line end.
 *
 *   vcc | serial:0        a battery monitor on the line
 *
 * No part and no pin is needed: the ADC measures its own 1.1 V reference
 * against the supply, and the supply follows from the reading:
 *
 *   reading = 1.1 V / Vcc * 1024      Vcc = 1126.4 V / reading
 *
 * - The conversion takes about 100 microseconds; the process waits for it
 *   like for anything else, and everybody else runs meanwhile.
 * - The first conversion after the input is selected is thrown away: the
 *   reference needs to settle.
 * - Needs the tick (sys/tick.h).
 */

#include <sys/process.h>
#include <sys/tick.h>
#include <avr/io.h>

#ifndef VCC_CONF_MILLIS
#define VCC_CONF_MILLIS 2000
#endif

// the 1.1 V reference as input, the supply as reference
#if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny25__)
#define VCC_ADMUX 0x0C
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#define VCC_ADMUX (_BV(REFS0) | 0x1E)
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__)
#define VCC_ADMUX (_BV(REFS0) | 0x0E)
#else
#error "vcc: no reference channel known for this part"
#endif

PROCESS_DEFINE(vcc, "vcc", 3,
  struct timer period;
  uint8_t      pass;      /* the first conversion is thrown away */
  char         text[6];
);

PROCESS_THREAD(vcc, ev, data)
{
  process_vcc_t *self = PROCESS_SELF(vcc);
  (void)ev;
  (void)data;

  PROCESS_BEGIN();

  timer_set(&self->period, clock_from_millis(VCC_CONF_MILLIS));

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(timer_expired(&self->period));
    timer_reset(&self->period);

    ADMUX = VCC_ADMUX;
    for (self->pass = 0; self->pass < 2; self->pass++)
    {
      ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
      PROCESS_WAIT_EVENT_UNTIL(!(ADCSRA & _BV(ADSC)));
    }

    {
      uint16_t reading = ADC;
      uint8_t tenths = reading ? (uint8_t)(11264u / reading) : 0; /* 1.1 * 1024 * 10 */

      self->text[0] = (char)('0' + tenths / 10);
      self->text[1] = '.';
      self->text[2] = (char)('0' + tenths % 10);
      self->text[3] = 'V';
      self->text[4] = '\r';
      self->text[5] = '\n';
    }
    ADCSRA = 0; /* the ADC is off between two measurements */

    if (pt_process->has_pipeout)
      PROCESS_PIPE_WRITE(self->text, sizeof(self->text));
  }

  /* ended, perhaps in the middle of a conversion: the ADC is switched off */
  PT_FINALLY(&pt_process->pt)
  ADCSRA = 0;

  PROCESS_END();
}

#endif /* __MOD_VCC_H__ */

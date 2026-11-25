// file: ./src/mod/vcc.h

#ifndef __MOD_VCC_H__
#define __MOD_VCC_H__

/* vcc: a sensor. Measures the supply voltage of the chip and writes it to
 * its output every period, as text: "5.0V" and a line end.
 *
 *   vcc | serial:0        a battery monitor on the line
 *
 * On an AVR no part and no pin is needed: the ADC measures its own 1.1 V
 * reference against the supply, and the supply follows from the reading:
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
#include <sys/timer.h>

#ifndef VCC_CONF_MILLIS
#define VCC_CONF_MILLIS 2000
#endif

#if defined(CC_ARCH_RP2)
/* RP2040, RP2350: the supply as a Pico board shows it, a third of VSYS on
 * GP29 (ADC input 3), against the 3.3 V reference. A board without that
 * divider reads what is on GP29. */
#include "hardware/adc.h"
#else
#include <avr/io.h>

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
#endif /* CC_ARCH_RP2 */

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

#if defined(CC_ARCH_RP2)
    {
      /* a conversion takes 2 microseconds: no need to let go meanwhile */
      adc_init();
      adc_gpio_init(29);
      adc_select_input(3);
      uint8_t tenths = (uint8_t)(((uint32_t)adc_read() * 99u + 2048u) / 4096u); /* 3 x 3.3 V x 10, rounded */
#else
    ADMUX = VCC_ADMUX;
    for (self->pass = 0; self->pass < 2; self->pass++)
    {
      ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
      PROCESS_WAIT_EVENT_UNTIL(!(ADCSRA & _BV(ADSC)));
    }

    {
      uint16_t reading = ADC;
      uint8_t tenths = reading ? (uint8_t)(11264u / reading) : 0; /* 1.1 * 1024 * 10 */
#endif

      self->text[0] = (char)('0' + tenths / 10);
      self->text[1] = '.';
      self->text[2] = (char)('0' + tenths % 10);
      self->text[3] = 'V';
      self->text[4] = '\r';
      self->text[5] = '\n';
    }
#if !defined(CC_ARCH_RP2)
    ADCSRA = 0; /* the ADC is off between two measurements */
#endif

    if (pt_process->has_pipeout)
      PROCESS_PIPE_WRITE(self->text, sizeof(self->text));
  }

  /* ended, perhaps in the middle of a conversion: the ADC is switched off */
  PT_FINALLY(&pt_process->pt)
#if !defined(CC_ARCH_RP2)
  ADCSRA = 0;
#endif

  PROCESS_END();
}

#endif /* __MOD_VCC_H__ */

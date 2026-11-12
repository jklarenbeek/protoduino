// file: ./src/mod/pwm.h

#ifndef __MOD_PWM_H__
#define __MOD_PWM_H__

/* pwm: a sink for samples. A sample is the duty cycle of pin PB4: 0 is
 * off, 255 is on all the time.
 *
 *   sense | smooth | curve | pwm
 *
 * - Timer 1 of an ATtiny25/45/85, output OC1B. PWM_CONF_CLOCK chooses its
 *   clock: 1 gives the system clock and 31 kHz at 8 MHz, which a fan does
 *   not sing along to; each step up halves it (2: 15.6 kHz ... 15).
 * - The last sample is kept in the instance, where a monitor can read it.
 * - pwm_start(), pwm_set() and pwm_stop() are plain functions.
 * - When the process ends the pin is low.
 * - No static variables.
 */

#include <sys/process.h>
#include <avr/io.h>

#if !(defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__))
#error "pwm: written for timer 1 of the ATtiny25/45/85"
#endif

#ifndef PWM_CONF_CLOCK
#define PWM_CONF_CLOCK 1
#endif

static void pwm_set(uint8_t duty)
{
  OCR1B = duty;
  if (duty)
    GTCCR = _BV(PWM1B) | _BV(COM1B1); // the timer has the pin
  else
    GTCCR = _BV(PWM1B);               // the port has it, and holds it low
}

static void pwm_start(void)
{
  PORTB &= (uint8_t)~_BV(PB4);
  DDRB  |= _BV(PB4);
  OCR1C  = 255;
  TCCR1  = (PWM_CONF_CLOCK) & 0x0F;
  pwm_set(0);
}

static void pwm_stop(void)
{
  pwm_set(0);
  TCCR1 = 0;
}

PROCESS_DEFINE(pwm, "pwm", 3,
  ipc_pipe_t in;
  uint8_t    buf[2];
  uint8_t    duty;   /* the last sample */
);

PROCESS_THREAD(pwm, ev, data)
{
  process_pwm_t *self = PROCESS_SELF(pwm);
  (void)ev;
  (void)data;

  PROCESS_BEGIN();

  if (!pt_process->has_pipein)
  {
    ipc_pipe_init(&self->in, self->buf, sizeof(self->buf), process_ipc_wake, (void *)pt_process);
    PROCESS_SET_PIPEIN(&self->in);
  }

  pwm_start();

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ipc_pipe_available(PROCESS_PIPEIN()) > 0);
    ipc_pipe_read(PROCESS_PIPEIN(), &self->duty, 1);
    pwm_set(self->duty);
  }

  PT_FINALLY(&pt_process->pt)
  {
    pwm_stop();
  }

  PROCESS_END();
}

#endif /* __MOD_PWM_H__ */

// file: ./src/mod/pulse.h

#ifndef __MOD_PULSE_H__
#define __MOD_PULSE_H__

/* pulse: a source that says whether a pin is alive. Writes a 1 for every
 * tick in which the level of the INT0 pin changed, and nothing otherwise.
 *
 *   pulse | keeper      a board that toggles a pin shows that it runs
 *
 * - The pin is INT0: PB2 on an ATtiny25/45/85, PD2 on an ATmega328P. The
 *   hardware notes the edge, however short the pulse; no interrupt handler
 *   is used, so the pin change interrupts stay free for the serial line.
 * - Edges are noted while the I/O clock runs: with sleep, SLEEP_MODE_IDLE.
 * - PULSE_CONF_PULLUP switches the pull-up of the pin on; the default.
 * - A source never waits for its reader.
 * - pulse_start() and pulse_seen() are plain functions.
 * - Needs the tick. No static variables, and no fields either.
 */

#include <sys/process.h>
#include <sys/tick.h>
#include <avr/io.h>

#ifndef PULSE_CONF_PULLUP
#define PULSE_CONF_PULLUP 1
#endif

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#define PULSE_FLAGS   GIFR
#define PULSE_SENSE   MCUCR
#define PULSE_PORT    PORTB
#define PULSE_BIT     PB2
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega88__)
#define PULSE_FLAGS   EIFR
#define PULSE_SENSE   EICRA
#define PULSE_PORT    PORTD
#define PULSE_BIT     PD2
#else
#error "pulse: written for the ATtiny25/45/85 and the ATmega328P family"
#endif

static void pulse_start(void)
{
#if PULSE_CONF_PULLUP
  PULSE_PORT |= _BV(PULSE_BIT);
#endif
  PULSE_SENSE = (uint8_t)((PULSE_SENSE & ~_BV(ISC01)) | _BV(ISC00)); // any change
  PULSE_FLAGS = _BV(INTF0);
}

// true when the pin changed since the last call
static uint8_t pulse_seen(void)
{
  if (!(PULSE_FLAGS & _BV(INTF0)))
    return 0;
  PULSE_FLAGS = _BV(INTF0); // written as 1, the flag is cleared
  return 1;
}

PROCESS_DEFINE(pulse, "pulse", 3,
  uint8_t byte;
);

PROCESS_THREAD(pulse, ev, data)
{
  process_pulse_t *self = PROCESS_SELF(pulse);

  PROCESS_BEGIN();

  pulse_start();

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == EVENT_TICK && data == NULL);

    if (pulse_seen())
    {
      self->byte = 1;
      if (pt_process->has_pipeout && ipc_pipe_space(PROCESS_PIPEOUT()) > 0)
        ipc_pipe_write(PROCESS_PIPEOUT(), &self->byte, 1);
    }
  }

  PT_FINALLY(&pt_process->pt)

  PROCESS_END();
}

#endif /* __MOD_PULSE_H__ */

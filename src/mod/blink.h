// file: ./src/mod/blink.h

#ifndef __MOD_BLINK_H__
#define __MOD_BLINK_H__

/* blink: a sink. Changes the state of the LED for every byte it receives.
 *
 *   ticker | blink        the LED changes every period of the ticker
 *
 * - Owns its input pipe, so any process can be connected to it with
 *   process_connect(writer, blink). It reads through PROCESS_PIPEIN(), so
 *   a device can be connected to it as well.
 * - The LED is the one on the board: PB5 on an ATmega328P (Uno), PB7 on an
 *   ATmega2560 (Mega). On an ATtiny85 it is PB4, because PB0 and PB1 carry
 *   the serial line. BLINK_CONF_PORT, _DDR, _PIN and _BIT choose another.
 */

#include <sys/process.h>
#include <avr/io.h>

#ifndef BLINK_CONF_PORT
#define BLINK_CONF_PORT PORTB
#define BLINK_CONF_DDR  DDRB
#define BLINK_CONF_PIN  PINB
#endif

#ifndef BLINK_CONF_BIT
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#define BLINK_CONF_BIT 7
#elif defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny25__)
#define BLINK_CONF_BIT 4 /* PB0 and PB1 are the serial line there */
#else
#define BLINK_CONF_BIT 5
#endif
#endif

PROCESS_DEFINE(blink, "blink", 3,
  ipc_pipe_t in;       /* what is written to blink arrives here */
  uint8_t    buf[4];
  uint8_t    byte;
);

PROCESS_THREAD(blink, ev, data)
{
  process_blink_t *self = PROCESS_SELF(blink);
  (void)ev;
  (void)data;

  PROCESS_BEGIN();

  /* The input: a pipe of its own, unless somebody connected one before
   * the process got to start (process_connect() from a device). */
  if (!pt_process->has_pipein)
  {
    ipc_pipe_init(&self->in, self->buf, sizeof(self->buf), process_ipc_wake, (void *)pt_process);
    PROCESS_SET_PIPEIN(&self->in);
  }
  BLINK_CONF_DDR |= _BV(BLINK_CONF_BIT);

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ipc_pipe_available(PROCESS_PIPEIN()) > 0);

    while (ipc_pipe_read(PROCESS_PIPEIN(), &self->byte, 1) == 1)
      BLINK_CONF_PIN = _BV(BLINK_CONF_BIT); /* writing the PIN register toggles the pin */
  }

  /* ended: the pin is given back as it was found */
  PT_FINALLY(&pt_process->pt)
  BLINK_CONF_PORT &= (uint8_t)~_BV(BLINK_CONF_BIT);
  BLINK_CONF_DDR &= (uint8_t)~_BV(BLINK_CONF_BIT);

  PROCESS_END();
}

#endif /* __MOD_BLINK_H__ */

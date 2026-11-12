// file: ./src/sys/process/softserial.h

#ifndef __PROCESS_SOFTSERIAL_H__
#define __PROCESS_SOFTSERIAL_H__

/* softserial: a serial line on two port pins, as a device process.
 *
 * For parts without a USART (ATtiny25/45/85). It looks like serial_process0
 * to everybody else: it is called "serial:0", what it receives is its
 * pipein and what it transmits is its pipeout, so a shell connects to it
 * with process_connect() and never knows the difference.
 *
 *   #include <sys/process/softserial.h>
 *   process_new(&process_type_softserial, &softserial_proc);
 *
 * - 8N1, half duplex: a byte that arrives while one is being sent is lost.
 *   A person at a terminal does not notice; a program has to wait for the
 *   echo before it sends the next byte.
 * - Receive is the pin-change interrupt on the RX pin. The handler takes
 *   the whole byte, which keeps interrupts off for one byte time.
 * - Transmit is one byte per turn of the device, also with interrupts off.
 * - RX is PB0 and TX is PB1; SOFTSERIAL_CONF_BAUD is 9600 by default.
 * - One instance, softserial_proc, defined here: include this file in one
 *   source file only.
 */

#include <sys/process.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay_basic.h>

#ifndef SOFTSERIAL_CONF_BAUD
#define SOFTSERIAL_CONF_BAUD 9600UL
#endif

// bytes the device holds in each direction. A part with 256 bytes of RAM
// or less gets short ones: whoever writes more waits, and a sender that
// paces its bytes loses none.
#ifndef SOFTSERIAL_CONF_RX_SIZE
#if RAMEND < 0x200
#define SOFTSERIAL_CONF_RX_SIZE 4
#else
#define SOFTSERIAL_CONF_RX_SIZE 8
#endif
#endif

#ifndef SOFTSERIAL_CONF_TX_SIZE
#if RAMEND < 0x200
#define SOFTSERIAL_CONF_TX_SIZE 4
#else
#define SOFTSERIAL_CONF_TX_SIZE 16
#endif
#endif

#define SOFTSERIAL_RX PB0
#define SOFTSERIAL_TX PB1

// iterations of _delay_loop_2(), four cycles each, in one bit time; the
// loop around it takes about ten cycles
#define SOFTSERIAL_BIT ((uint16_t)((F_CPU / SOFTSERIAL_CONF_BAUD - 10) / 4))

DEVICE_DEFINE(softserial, "serial:0", 1,
  ipc_pipe_t rx_pipe;
  ipc_pipe_t tx_pipe;
  uint8_t    rx_buf[SOFTSERIAL_CONF_RX_SIZE];
  uint8_t    tx_buf[SOFTSERIAL_CONF_TX_SIZE];
  uint8_t    byte;
);

static process_softserial_t softserial_proc;

// a start bit: take the byte
ISR(PCINT0_vect)
{
  uint8_t data = 0;
  uint8_t i;

  if (PINB & _BV(SOFTSERIAL_RX))
    return; // the line went up, not down

  _delay_loop_2(SOFTSERIAL_BIT / 2);
  for (i = 8; i; i--)
  {
    _delay_loop_2(SOFTSERIAL_BIT);
    data >>= 1;
    if (PINB & _BV(SOFTSERIAL_RX))
      data |= 0x80;
  }
  _delay_loop_2(SOFTSERIAL_BIT);

  ipc_pipe_write(&softserial_proc.rx_pipe, &data, 1); // wakes whoever reads it
  GIFR = _BV(PCIF); // the edges of the data bits are not start bits
}

static void softserial_write8(uint8_t data)
{
  uint16_t frame = ((uint16_t)data << 1) | 0x200; // start, 8 data, stop
  uint8_t i;

  CC_ATOMIC_RESTORE()
  {
    for (i = 10; i; i--)
    {
      if (frame & 1)
        PORTB |= _BV(SOFTSERIAL_TX);
      else
        PORTB &= (uint8_t)~_BV(SOFTSERIAL_TX);
      frame >>= 1;
      _delay_loop_2(SOFTSERIAL_BIT);
    }
  }
}

PROCESS_THREAD(softserial, ev, data)
{
  process_softserial_t *self = PROCESS_SELF(softserial);
  (void)ev;
  (void)data;

  PROCESS_BEGIN();

  /* RX pipe: the reader sets its own wake when it connects.
   * TX pipe: a write wakes this device. */
  ipc_pipe_init(&self->rx_pipe, self->rx_buf, sizeof(self->rx_buf), NULL, NULL);
  ipc_pipe_init(&self->tx_pipe, self->tx_buf, sizeof(self->tx_buf), process_ipc_wake, (void *)pt_process);
  PROCESS_SET_PIPEIN(&self->rx_pipe);
  PROCESS_SET_PIPEOUT(&self->tx_pipe);

  PORTB |= _BV(SOFTSERIAL_TX) | _BV(SOFTSERIAL_RX); // idle high; pull-up on RX
  DDRB |= _BV(SOFTSERIAL_TX);
  PCMSK |= _BV(SOFTSERIAL_RX);
  GIMSK |= _BV(PCIE);

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ipc_pipe_available(&self->tx_pipe) > 0);

    ipc_pipe_read(&self->tx_pipe, &self->byte, 1);
    softserial_write8(self->byte);

    /* one byte per turn: the next one after everybody else had theirs */
    if (ipc_pipe_available(&self->tx_pipe) > 0)
      process_poll(pt_process);
  }

  PROCESS_END(); /* a device runs until reset */
}

#endif /* __PROCESS_SOFTSERIAL_H__ */

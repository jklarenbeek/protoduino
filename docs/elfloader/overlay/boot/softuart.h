// file: ./boot/softuart.h

#ifndef __BOOT_SOFTUART_H__
#define __BOOT_SOFTUART_H__

/* uart0 on two port pins, for parts without a USART (ATtiny25/45/85).
 *
 * Half duplex, 8N1, RX = PB0, TX = PB1. Only the polled part of
 * ./src/cpu/avr/uart.h, with the same names, so that the bootloader reads
 * the same on every part.
 */

#include <avr/io.h>
#include <util/delay_basic.h>

#define SOFTUART_BIT ((uint8_t)((F_CPU / BOOT_CONF_BAUD - 8) / 3))

static CC_ALWAYS_INLINE void uart0_open(uint32_t baud)
{
  (void)baud; // fixed at build time
  PORTB |= _BV(PB1) | _BV(PB0);
  DDRB |= _BV(PB1);
}

// a start bit is on the line
static CC_ALWAYS_INLINE bool uart0_rx_is_ready(void)
{
  return (PINB & _BV(PB0)) == 0;
}

static uint_fast8_t uart0_rx_read8(void)
{
  uint8_t data = 0;

  _delay_loop_1(SOFTUART_BIT / 2);
  for (uint8_t i = 8; i; i--)
  {
    _delay_loop_1(SOFTUART_BIT);
    data >>= 1;
    if (PINB & _BV(PB0))
      data |= 0x80;
  }
  _delay_loop_1(SOFTUART_BIT);
  return data;
}

static CC_ALWAYS_INLINE bool uart0_tx_is_ready(void)
{
  return true;
}

static void uart0_tx_write8(const uint_fast8_t data)
{
  uint16_t frame = ((uint16_t)data << 1) | 0x200; // start, 8 data, stop

  for (uint8_t i = 10; i; i--)
  {
    if (frame & 1)
      PORTB |= _BV(PB1);
    else
      PORTB &= ~_BV(PB1);
    frame >>= 1;
    _delay_loop_1(SOFTUART_BIT);
  }
}

#endif /* __BOOT_SOFTUART_H__ */

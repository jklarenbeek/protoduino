
#include <avr/interrupt.h>

#include "serial0.h"

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#if defined(UBRRH) && defined(UBRRL)
#define __UBRRH__ UBRRH
#define __UBRRL__ UBRRL
#define __UCSRA__ UCSRA
#define __UCSRB__ UCSRB
#define __UCSRC__ UCSRC
#define __UDR__ UDR
#else
#define __UBRRH__ UBRR0H
#define __UBRRL__ UBRR0L
#define __UCSRA__ UCSR0A
#define __UCSRB__ UCSR0B
#define __UCSRC__ UCSR0C
#define __UDR__ UDR0
#endif

#if defined(__AVR_ATmega128__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined (__AVR_ATmega328__)
#define __UDRE__ UDRE0
#define __RXC__ RXC0
#define __TXC__ TXC0
#define __U2X__ U2X0
#else
#define __UDRE__ UDRE
#define __RXC__ RXC
#define __TXC__ TXC
#define __U2X__ U2X
#define __MPCM__ MPCM
#endif

#if defined(MPCM0)
#define __MPCM__ MPCM0
#endif

static volatile void (*serial0_callback)(uint_fast8_t)=0;
static uint32_t serial0_baudrate = 0;

void serial0_register(void (*callback)(uint_fast8_t))
{
  serial0_callback = callback;
}

#if defined(USE_PROTODUINO_SERIAL) || defined(USE_PROTODUINO_SERIAL0)
#if defined(USART_RX_vect)
ISR(USART_RX_vect)
#elif defined(USART0_RX_vect)
ISR(USART0_RX_vect)
#elif defined(USART_RXC_vect)
ISR(USART_RXC_vect) // ATmega8
#else
  #error "Don't know what the Data Received vector is called for Serial"
#endif
{
  uint_fast8_t ch =__UDR__; // must read, to clear the interrupt flag
  if(serial0_baudrate != 0 && serial0_callback != 0) 
  {
    serial0_callback(ch);
  }
}
#endif

void serial0_open(uint32_t baud, uint8_t config)
{
  // Try u2x mode first
  uint16_t baud_setting = (F_CPU / 4 / baud - 1) / 2;
  __UCSRA__ = 1 << U2X0;

  // hardcoded exception for 57600 for compatibility with the bootloader
  // shipped with the Duemilanove and previous boards and the firmware
  // on the 8U2 on the Uno and Mega 2560. Also, The baud_setting cannot
  // be > 4095, so switch back to non-u2x mode if the baud rate is too
  // low.
  if (((F_CPU == 16000000UL) && (baud == 57600)) || (baud_setting >4095))
  {
    __UCSRA__ = 0;
    baud_setting = (F_CPU / 8 / baud - 1) / 2;
  }
 

  // assign the baud_setting, a.k.a. ubrr (USART Baud Rate Register)
  __UBRRH__ = baud_setting >> 8;
  __UBRRL__ = baud_setting;

  //set the data bits, parity, and stop bits
#if defined(__AVR_ATmega8__)
  config |= 0x80; // select UCSRC register (shared with UBRRH)
#endif
  __UCSRC__ = config;

  serial0_baudrate = F_CPU / (8 * (baud_setting + 1));

  sbi(__UCSRB__, RXEN0);
  sbi(__UCSRB__, TXEN0);
  sbi(__UCSRB__, RXCIE0);
  cbi(__UCSRB__, UDRIE0);

}

void serial0_close()
{
  serial0_baudrate = 0;
  // TODO: disable interupt?
}

uint32_t serial0_get_baudrate()
{
  return serial0_baudrate;
}

uint_fast8_t serial0_read_available()
{
  return (__UCSRA__ & _BV(__RXC__)) ? 1 : 0;
}

uint_fast8_t serial0_read8_unchecked()
{
  return __UDR__;
}

CC_FLATTEN int_fast16_t serial0_read8()
{
  if (serial0_read_available()) {
    return serial0_read8_unchecked();
  }
  return -1;
}

uint_fast8_t serial0_write_available()
{
  return (__UCSRA__ & _BV(__UDRE__)) ? 1 : 0;
}

void serial0_write8_unchecked(uint_fast8_t data)
{
  __UDR__ = data;

  // clear the TXC bit -- "can be cleared by writing a one to its bit
  // location". This makes sure flush() won't return until the bytes
  // actually got written. Other r/w bits are preserved, and zeroes
  // written to the rest.
#ifdef __MPCM__
  __UCSRA__ = ((__UCSRA__) & ((1 << __U2X__) | (1 << __MPCM__))) | (1 << __TXC__);
#else
  __UCSRA__ = ((__UCSRA__) & ((1 << __U2X__) | (1 << __TXC__)));
#endif
}

CC_FLATTEN uint_fast8_t serial0_write8(uint_fast8_t data)
{
  if (serial0_write_available())
  {
    return serial0_write8_unchecked(data);
  }
  return 0;
}


#include <avr/interrupt.h>

#include "ringb8.h"
#include "serial0.h"

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#if defined(__AVR_ATmega128__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined (__AVR_ATmega328__)

// USART baud rate register high 
#define __UBRRH__ UBRR0H
// USART baud rate register low
#define __UBRRL__ UBRR0L
// UCSRnA – USART Control and Status Register n A
#define __UCSRA__ UCSR0A
// UCSRnB – USART Control and Status Register n B
#define __UCSRB__ UCSR0B
// UCSRnC – USART Control and Status Register n C
#define __UCSRC__ UCSR0C
// USART I/O data register
#define __UDR__ UDR0


//
// Flags for the UCSR0A register
//
// set recieve complete flag (RXC0 = 1)
#define __RXC__ RXC0
// set transmit complete flag (TXC0 = 1)
#define __TXC__ TXC0
// Data Register Empty tells you when the transmit buffer is able to accept another byte (at which time the actual shift register is still shifting out the previous byte.)
#define __UDRE__ UDRE0
// clear Frame Error flag (FE0 = 0)
#define __FE__ FE0
// clear Data overrun flag (DOR0 = 0)
#define __DOR__ DOR0
// clear Parity overrun flag (UPE0 = 0)
#define __UPE__ UPE0
// disable doubling of USART transmission speed (U2X0 = 0)
#define __U2X__ U2X0
#if defined(MPCM0)
// Disable Multi-Processor Communication Mode (MCPM0 = 0)
#define __MPCM__ MPCM0
#endif

//
// Flags for the UCSR0B register
//
// Enable Receive Interrupt (RXCIE0 = 1)
#define __RXCIE__ RXCIE0
// Disable Tranmission Interrupt (TXCIE = 0)
#define __TXCIE__ TXCIE0
// Disable Data Register Empty interrupt (UDRIE0 = 0)
#define __UDRIE__ UDRIE0
// Enable reception (RXEN0 = 1)
#define __RXEN__ RXEN0
// Enable transmission (TXEN0 = 1)
#define __TXEN__ TXEN0
// Set 8-bit character mode (UCSZ00, UCSZ01, and UCSZ02 together control this, But UCSZ00, UCSZ01 are in Register UCSR0C)
#define __UCSZ2__ UCSZ02
// TODO
#define __RXB8__ RXB80
// TODO
#define __TXB8__ TXB80

//
// Flags for the UCSR0C register
//
// USART Mode select -- UMSEL00 = 0 and UMSEL01 = 0 for asynchronous mode
#define __UMSEL0__ UMSEL00
#define __UMSEL1__ UMSEL01
// disable parity mode -- UPM00 = 0 and UPM01 = 0
#define __UPM0__ UPM00
#define __UPM1__ UPM01
// Disabilitato: Set USBS = 1 to configure to 2 stop bits per DMX standard.  The USART receiver ignores this 
// setting anyway, and will only set a frame error flag if the first stop bit is 0.  
#define __USBS__ USBS0
// Finish configuring for 8 data bits by setting UCSZ00 and UCSZ01 to 1
#define __UCSZ0__ UCSZ00
#define __UCSZ1__ UCSZ01
// Set clock parity to 0 for asynchronous mode (UCPOL0 = 0). */
#define __UCPOL__ UCPOL0

#else

#define __UBRRH__ UBRRH
#define __UBRRL__ UBRRL

#define __UCSRA__ UCSRA
#define __UCSRB__ UCSRB
#define __UCSRC__ UCSRC

#define __UDR__ UDR

#define __UDRE__ UDRE
#define __RXC__ RXC
#define __TXC__ TXC
#define __U2X__ U2X
#define __MPCM__ MPCM
#endif


static volatile void (*serial0_on_recieve_complete)(uint_fast8_t)=0;
static volatile void (*serial0_on_transmit_complete)(void)=0;

static uint32_t serial0_baudrate = 0;

void serial0_on_recieved(void (*callback)(uint_fast8_t))
{
  serial0_on_recieve_complete = callback;
}

void serial0_on_transmitted(void (*callback)(void))
{
  serial0_on_transmit_complete = callback;
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
  if(serial0_baudrate != 0 && serial0_on_recieve_complete != 0) 
  {
    serial0_on_recieve_complete(ch);
  }
}

#if defined(UART0_UDRE_vect)
ISR(UART0_UDRE_vect)
#elif defined(UART_UDRE_vect)
ISR(UART_UDRE_vect)
#elif defined(USART0_UDRE_vect)
ISR(USART0_UDRE_vect)
#elif defined(USART_UDRE_vect)
ISR(USART_UDRE_vect)
#else
  #error "Don't know what the Data Register Empty vector is called for Serial"
#endif
{
  if(serial0_baudrate != 0 && serial0_on_recieve_complete != 0) 
  {
    serial0_on_transmit_complete();
  }
}

#endif

/**
 * SerialPort::begin(uint32_t baud, uint8_t options = SP_8_BIT_CHAR)

Sets the data rate in bits per second (baud) for serial data transmission.

Parameters:
[in] baud rate in bits per second (baud)
[in] options constructed by a bitwise-inclusive OR of values from the following list.

Choose one value for stop bit, parity, and character size.

The default is SP_8_BIT_CHAR which results in one stop bit, no parity, and 8-bit characters.

SP_1_STOP_BIT - use one stop bit (default if stop bit not specified)

SP_2_STOP_BIT - use two stop bits

SP_NO_PARITY - no parity bit (default if parity not specified)

SP_EVEN_PARITY - add even parity bit

SP_ODD_PARITY - add odd parity bit

SP_5_BIT_CHAR - use 5-bit characters (default if size not specified)

SP_6_BIT_CHAR - use 6-bit characters

SP_7_BIT_CHAR - use 7-bit characters

SP_8_BIT_CHAR - use 8-bit characters

*/
void serial0_open(uint32_t baud, uint8_t config)
{
  // Try u2x mode first
  uint16_t baud_setting = (F_CPU / 4 / baud - 1) / 2;
  __UCSRA__ = 1 << __U2X__;

  // hardcoded exception for 57600 for compatibility with the bootloader
  // shipped with the Duemilanove and previous boards and the firmware
  // on the 8U2 on the Uno and Mega 2560. Also, The baud_setting cannot
  // be > 4095, so switch back to non-u2x mode if the baud rate is too
  // low.
  if (((F_CPU == 16000000UL) && (baud == 57600)) || (baud_setting > 4095))
  {
    baud_setting = (F_CPU / 8 / baud - 1) / 2;
    __UCSRA__ = 0;
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

  sbi(__UCSRB__, __RXEN__);
  sbi(__UCSRB__, __TXEN__);
  sbi(__UCSRB__, __RXCIE__);
  cbi(__UCSRB__, __UDRIE__);

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
  // is the recieved package complete?
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
  // is transmit buffer ready to accept another byte?
  return (__UCSRA__ & _BV(__UDRE__)) ? 1 : 0;
}

void serial0_write8_unchecked(uint_fast8_t data)
{
  __UDR__ = data;
}

void serial0_clear_txc()
{
  // clear the TXC bit -- "can be cleared by writing a one to its bit
  // location". This makes sure flush() won't return until the bytes
  // actually got written. Other r/w bits are preserved, and zeroes
  // written to the rest.
#ifdef __MPCM__
  //__UCSRA__ = ((__UCSRA__) & ((1 << __U2X__) | (1 << __MPCM__))) | (1 << __TXC__);
  sbi(__UCSRA__, __TXC__);
#else
  sbi(__UCSRA__, __TXC__);
  //__UCSRA__ = ((__UCSRA__) & ((1 << __U2X__) | (1 << __TXC__)));
#endif

}

CC_FLATTEN uint_fast8_t serial0_write8(uint_fast8_t data)
{
  if (serial0_write_available())
  {
    serial0_write8_unchecked(data);
    serial0_clear_txc();
    return 1;
  }
  return 0;
}


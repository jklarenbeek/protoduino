
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "ringb8.h"
#include "serial0.h"
#include "pt-errors.h"

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
// Bit 1 – RXB8n: Receive Data Bit 8 n
#define __RXB8__ RXB80
// Bit 0 – TXB8n: Transmit Data Bit 8 n
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
// Set USBS = 1 to configure to 2 stop bits per DMX standard.  The USART receiver ignores this 
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

#define __RXC__ RXC
#define __TXC__ TXC
#define __UDRE__ UDRE
#define __FE__ FE
#define __DOR__ DOR
#define __UPE__ UPE
#define __U2X__ U2X
#if defined(MPCM)
#define __MPCM__ MPCM
#endif

#define __RXCIE__ RXCIE
#define __TXCIE__ TXCIE
#define __UDRIE__ UDRIE
#define __RXEN__ RXEN
#define __TXEN__ TXEN
#define __UCSZ2__ UCSZ2
#define __RXB8__ RXB8
#define __TXB8__ TXB8

#define __UMSEL0__ UMSEL0
#define __UMSEL1__ UMSEL1
#define __UPM0__ UPM0
#define __UPM1__ UPM1
#define __USBS__ USBS
#define __UCSZ0__ UCSZ0
#define __UCSZ1__ UCSZ1
#define __UCPOL__ UCPOL

#endif

#if defined(USART_RX_vect)
#define __ISR_RX_VECT__ USART_RX_vect
#elif defined(USART0_RX_vect)
#define __ISR_RX_VECT__ USART0_RX_vect
#elif defined(USART_RXC_vect)
#define __ISR_RX_VECT__ USART_RXC_vect
#else
  #error "Don't know what Data Received vector is called for serial0"
#endif

#if defined(UART0_UDRE_vect)
#define __ISR_UDRE_VECT__ UART0_UDRE_vect
#elif defined(UART_UDRE_vect)
#define __ISR_UDRE_VECT__ UART_UDRE_vect
#elif defined(USART0_UDRE_vect)
#define __ISR_UDRE_VECT__ USART0_UDRE_vect
#elif defined(USART_UDRE_vect)
#define __ISR_UDRE_VECT__ USART_UDRE_vect
#else
  #error "Don't know what Data Register Empty vector is called for serial0"
#endif

#define CC_NM_PREFIX serial0_
#define CC_NM(method) CC_CONCAT2(CC_NM_PREFIX,method)

static volatile uint_fast8_t (*serial0_on_rx_complete)(uint_fast8_t)=0;
static volatile void (*serial0_on_tx_complete)(void)=0;

static uint32_t serial0_baudrate = 0;

RINGB8(serial0_rx, SERIAL_BUFFER_RX_SIZE);
RINGB8(serial0_tx, SERIAL_BUFFER_TX_SIZE);

void serial0_onrecieved(uint_fast8_t (*callback)(uint_fast8_t))
{
  serial0_on_rx_complete = callback;
}

void serial0_ontransmitted(void (*callback)(void))
{
  serial0_on_tx_complete = callback;
}

#if defined(USE_PROTODUINO_SERIAL) || defined(USE_PROTODUINO_SERIAL0)
ISR(__ISR_RX_VECT__)
{
  uint_fast8_t data;

  // is there an error in the recieved packet?
  if (__UCSRA__ & (_BV(__FE__) | _BV(__DOR__) | _BV(__UPE__)))
  {
    // fetch the data register and discard it.
    // must read, to clear the interrupt flag.
    data = __UDR__; 
    return;
  }

  data =__UDR__; // fetch the packet

  // if the callback function returns true
  if (serial0_on_rx_complete != 0 && serial0_on_rx_complete(data) > 0)
  {
    // skip adding byte to the rx buffer.
    return;
  }

  // is there room in the buffer?
  if (ringb8_available(&VAR_RINGB8(serial0_rx)) > 0)
  {
    ringb8_put(&VAR_RINGB8(serial0_rx), data);
  }
}

ISR(__ISR_UDRE_VECT__)
{
  uint_fast8_t cnt = ringb8_count(&VAR_RINGB8(serial0_tx);

  // this should ALWAYS be true, since an interupt should only
  // be generated when there is still data in the tx buffer.
  if (cnt > 0))
  {
    __UDR__ = ringb8_get(&VAR_RINGB8(serial_tx));
  }

  // clear the TXC bit, by setting it to 1
  sbi(__UCSRA__, __TXC__);

  // was this the last byte in the buffer?
  if (cnt == 1)
  [
    // disable the data register empty interrupt
    cbi(__UCSRB__, __UDRIE__);

    // call the tx complete event handler
    if (serial0_on_tx_complete != 0)
      serial0_on_tx_complete();
  ]
}

#endif

void serial0_open(uint32_t baud)
{
  // data bits = 8 (__UCSZ1__ and __UCSZ2 = 1)
  // stop bits = 1 (__USBS__ = 0)
  // no parity (__UPM0__ and __UPM1__ = 0)
  serial0_openex(baud, (__UCSZ1__ | __UCSZ2__));
}
void serial0_openex(uint32_t baud, uint8_t options)
{
  // disable USART interrupts.  Set UCSRB to reset values.
  __UCSRB__ = 0;

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
  options |= 0x80; // select UCSRC register (shared with UBRRH)
#endif
  __UCSRC__ = options;

  serial0_baudrate = F_CPU / (8 * (baud_setting + 1));

  sbi(__UCSRB__, __RXEN__); // enable reciever
  sbi(__UCSRB__, __TXEN__); // enable transmitter
  sbi(__UCSRB__, __RXCIE__); // enable receive complete interrupt (RX)
  cbi(__UCSRB__, __UDRIE__); // disable data register empty interrupt (TX)

}

void serial0_close(void)
{
  serial0_baudrate = 0;
  cli(); // diable all interrupts
  cbi(__UCSRB__, __RXEN__); // disable reciever
  cbi(__UCSRB__, __TXEN__); // disable transmitter
  cbi(__UCSRB__, __RXCIE__); // disable recieve complete interrupt (RX)
  cbi(__UCSRB__, __UDRIE__); // disable data register empty interrupt (TX)
  sei(); // enable all interrupts
}

uint_fast32_t serial0_get_baudrate(void)
{
  return serial0_baudrate;
}

uint_fast8_t serial0_rx_available(void)
{
  return (__UCSRA__ & _BV(__RXC__)) ? 1 : 0; 
}

uint_fast8_t serial0_rx_error(void)
{
  uint8_t r = __UCSRA__ & (_BV(__FE__) | _BV(__DOR__) | _BV(__UPE__));
  if (r == 0) 
    return PT_ERR_SUCCESS;
  if (r & __BV(__FE__)) 
    return PT_ERR_FRAME_ERROR; // frame error
  if (r & __BV(__DOR__)) 
    return PT_ERR_DATA_OVERFLOW; // data overrun
  else 
    return PT_ERR_PARITY_ERROR; // parity error
}

uint_fast8_t serial0_rx_read8(void)
{
  uint_fast8_t d = __UDR__;
  return d;
}

uint_fast8_t serial0_tx_available(void)
{
  return !((__UCSRB__ & _BV(__UDRIE__)) || !(__UCSRA__ & _BV(__UDRE__))) ? 1 : 0;
}

void serial0_tx_write8(uint_fast8_t data)
{
      // write the byte to the data register immediatly
    __UDR__ = data;
      // clear the TXC bit by setting it.
      sbi(__UCSRA__, __TXC__);
}

uint_fast8_t serial0_read_available(void)
{
  return ringb8_count(&VAR_RINGB8(serial0_rx));
}

int_fast16_t serial0_peek(void)
{
  uint_fast8_t cnt = serial0_read_available();
  if (cnt == 0)
    return -1;
  else
    return ringb8_peek(&VAR_RINGB8(serial0_rx));
}

int_fast16_t serial0_read8(void)
{
  uint_fast8_t cnt = serial0_read_available();
  if (cnt == 0)
    return -1;
  else
    return ringb8_get(&VAR_RINGB8(serial0_rx));
}

int_fast32_t serial0_read16(void)
{
  uint_fast8_t cnt = serial0_read_available();
  if (cnt < 2)
    return -1;
  
  union {
    uint16_t data;
    uint8_t buf[2];
  } tmp;

  tmp.buf[0] = ringb8_get(&VAR_RINGB8(serial0_rx));
  tmp.buf[1] = ringb8_get(&VAR_RINGB8(serial0_rx));

  return tmp.data;
}

int_fast32_t serial0_read24(void)
{
  uint_fast8_t cnt = serial0_read_available();
  if (cnt < 3)
    return -1;
  
  union {
    int32_t data;
    uint8_t buf[4];
  } tmp;

  tmp.buf[0] = ringb8_get(&VAR_RINGB8(serial0_rx));
  tmp.buf[1] = ringb8_get(&VAR_RINGB8(serial0_rx));
  tmp.buf[2] = ringb8_get(&VAR_RINGB8(serial0_rx));
  tmp.buf[3] = 0;

  return tmp.data;
}

int_fast32_t serial0_read32(void)
{
  uint_fast8_t cnt = serial0_read_available();
  if (cnt < 4)
    return 0; // WATCH OUT! caller needs to care of available here!!
  
  union {
    int32_t data;
    uint8_t buf[4];
  } tmp;

  tmp.buf[0] = ringb8_get(&VAR_RINGB8(serial0_rx));
  tmp.buf[1] = ringb8_get(&VAR_RINGB8(serial0_rx));
  tmp.buf[2] = ringb8_get(&VAR_RINGB8(serial0_rx));
  tmp.buf[3] = ringb8_get(&VAR_RINGB8(serial0_rx));

  return tmp.data;
}

uint_fast8_t serial0_write_available(void)
{
  return ringb8_available(&VAR_RINGB8(serial0_tx));
}

CC_FLATTEN uint_fast8_t serial0_write8(uint_fast8_t data)
{
  uint8_t cnt = ringb8_count(&VAR_RINGB8(serial0_tx));

  // when the buffer is empty and the data register is empty
  if (cnt == 0 && bit_is_set(__UCSRA__, __UDRE__))
  {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
      // write the byte to the data register immediatly
      __UDR__ = data;
      // clear the TXC bit by setting it.
      sbi(__UCSRA__, __TXC__);
    }
    return 1;
  }

  // we need to add data to the buffer
  uint8_t available = ringb8_size(&VAR_RINGB8(serial0_tx)) - cnt;
  if (available == 0) // but if we can't, we can't
  {
    return 0; // let the user flush it manually
  }

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    // add data to the ringbuffer
    ringb8_put(&VAR_RINGB8(serial0_tx), data);

    // enable data register empty interrupt
    sbi(__UCSRB__, __UDRIE__);
  }

  return 1;
}

CC_FLATTEN uint_fast8_t serial0_write16(uint_fast16_t data)
{
  uint8_t available = serial0_write_available();
  if (available < 2)
    return 0; // let the user handle this

  union {
    uint16_t data;
    uint8_t buf[2];
  } tmp;

  tmp.data = data;
  
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    // add data to the ringbuffer
    ringb8_put(&VAR_RINGB8(serial0_tx), tmp.buf[0]);
    ringb8_put(&VAR_RINGB8(serial0_tx), tmp.buf[1]);

    // enable data register empty interrupt
    sbi(__UCSRB__, __UDRIE__);
  }

  return 2;  
}

CC_FLATTEN uint_fast8_t serial0_write24(uint_fast32_t data)
{
  uint8_t available = serial0_write_available();
  if (available < 3)
    return 0; // let the user handle this

  union {
    uint32_t data;
    uint8_t buf[4];
  } tmp;

  tmp.data = data;

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    // add data to the ringbuffer
    ringb8_put(&VAR_RINGB8(serial0_tx), tmp.buf[0]);
    ringb8_put(&VAR_RINGB8(serial0_tx), tmp.buf[1]);
    ringb8_put(&VAR_RINGB8(serial0_tx), tmp.buf[2]);

    // enable data register empty interrupt
    sbi(__UCSRB__, __UDRIE__);
  }

  return 3;  
}

CC_FLATTEN uint_fast8_t serial0_write32(uint_fast32_t data)
{
  uint8_t available = serial0_write_available();
  if (available < 4)
    return 0; // let the user handle this

  union {
    uint32_t data;
    uint8_t buf[4];
  } tmp;

  tmp.data = data;

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    // add data to the ringbuffer
    ringb8_put(&VAR_RINGB8(serial0_tx), tmp.buf[0]);
    ringb8_put(&VAR_RINGB8(serial0_tx), tmp.buf[1]);
    ringb8_put(&VAR_RINGB8(serial0_tx), tmp.buf[2]);
    ringb8_put(&VAR_RINGB8(serial0_tx), tmp.buf[3]);

    // enable data register empty interrupt
    sbi(__UCSRB__, __UDRIE__);
  }

  return 4;
}

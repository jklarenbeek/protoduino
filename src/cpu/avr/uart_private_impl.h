#include "../../sys/cc.h"
#include <avr/interrupt.h>
#include <util/atomic.h>

static volatile uart_on_rx_complete_fn CC_TMPL_VAR(on_rx_complete) = 0;
static volatile uart_on_rx_error_fn CC_TMPL_VAR(on_rx_error) = 0;
static volatile uart_on_tx_complete_fn CC_TMPL_VAR(on_tx_complete) = 0;

static uint32_t CC_TMPL_VAR(baudrate) = 0;

void CC_TMPL_FN(on_rx_complete)(const uart_on_rx_complete_fn callback)
{
  CC_TMPL_VAR(on_rx_complete) = callback;
}

void CC_TMPL_FN(on_rx_error)(const uart_on_rx_error_fn callback)
{
  CC_TMPL_VAR(on_rx_error) = callback;
}

void CC_TMPL_FN(on_tx_complete)(const uart_on_tx_complete_fn callback)
{
  CC_TMPL_VAR(on_tx_complete) = callback;
}


void CC_TMPL_FN(open)(uint32_t baud)
{
  // data bits = 8 (__UCSZ1__ and __UCSZ2 = 1)
  // stop bits = 1 (__USBS__ = 0)
  // no parity (__UPM0__ and __UPM1__ = 0)
  CC_TMPL_FN(openex)(baud, (__UCSZ1__ | __UCSZ2__));
}

void CC_TMPL_FN(openex)(uint32_t baud, uint8_t options)
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

  CC_TMPL_VAR(baudrate) = F_CPU / (8 * (baud_setting + 1));

  sbi(__UCSRB__, __RXEN__); // enable reciever
  sbi(__UCSRB__, __TXEN__); // enable transmitter

  sbi(__UCSRB__, __RXCIE__); // enable receive complete interrupt (RX)
  cbi(__UCSRB__, __UDRIE__); // disable data register empty interrupt (TX)

}

void CC_TMPL_FN(close)(void)
{
  cli(); // diable all interrupts
  CC_TMPL_VAR(baudrate) = 0;
  cbi(__UCSRB__, __RXEN__); // disable reciever
  cbi(__UCSRB__, __TXEN__); // disable transmitter
  cbi(__UCSRB__, __RXCIE__); // disable recieve complete interrupt (RX)
  cbi(__UCSRB__, __UDRIE__); // disable data register empty interrupt (TX)
  sei(); // enable all interrupts
}

uint_fast32_t CC_TMPL_FN(baudrate)(void)
{
  return CC_TMPL_VAR(baudrate);
}


bool CC_TMPL_FN(rx_is_ready)(void)
{
  return (__UCSRA__ & _BV(__RXC__)) != 0; 
}

uint_fast8_t CC_TMPL_FN(rx_error)(void)
{
  uint8_t r = __UCSRA__ & (_BV(__FE__) | _BV(__DOR__) | _BV(__UPE__));
  if (r == 0) 
    return ERR_SUCCESS;
  if (r & _BV(__FE__))
    return ERR_FRAME_ERROR; // frame error
  if (r & _BV(__DOR__)) 
    return ERR_DATA_OVERFLOW; // data overrun
  else 
    return ERR_PARITY_ERROR; // parity error
}

uint_fast8_t CC_TMPL_FN(rx_read8)(void)
{
  uint_fast8_t d = __UDR__;
  return d;
}

void CC_TMPL_FN(tx_enable)(void)
{
  // enable data register empty interrupt
  sbi(__UCSRB__, __UDRIE__);
}

bool CC_TMPL_FN(tx_is_enabled)(void)
{
  return (__UCSRB__ & _BV(__UDRIE__)) != 0;
}

bool CC_TMPL_FN(tx_is_ready)(void)
{
  return (__UCSRA__ & _BV(__UDRE__)) != 0;
}

CC_FLATTEN bool CC_TMPL_FN(tx_is_available)(void)
{
  // if the data register empty interrupt is not set and the data register is empty
  return !CC_TMPL_FN(tx_is_enabled)() && CC_TMPL_FN(tx_is_ready)();
}

void CC_TMPL_FN(tx_write8)(const uint_fast8_t data)
{
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    // write the byte to the data register immediatly
    __UDR__ = data;

    // clear the TXC bit by setting it.
    sbi(__UCSRA__, __TXC__);
  }
}

#ifndef USE_ARDUINO_HARDWARESERIAL
#pragma message "uart0: using protoduino interrupt service routines"

// This interrupt is called when the cpu recieved a packet on the bus
ISR(__ISR_RX_VECT__)
{
  uint_fast8_t err = CC_TMPL_FN(rx_error)();

  // fetch the packet, must read, to clear the interrupt flag.
  uint_fast8_t data =__UDR__;

  // is there an error in the recieved packet?
  if (err != ERR_SUCCESS)
  {
    if (CC_TMPL_VAR(on_rx_error) != 0)
      CC_TMPL_VAR(on_rx_error)(err);
  }
  else
  {
    if (CC_TMPL_VAR(on_rx_complete) != 0)
      CC_TMPL_VAR(on_rx_complete)(data);
  }
}

// This interrupt is called when the transmit register is empty in order to send another character
ISR(__ISR_UDRE_VECT__)
{
  if (CC_TMPL_VAR(on_tx_complete) != 0)
  {
    int_fast16_t data = CC_TMPL_VAR(on_tx_complete)();
    if (data >= 0)
    {
      __UDR__ = (uint8_t)(data & 0xFF);

      // clear the TXC bit, by setting it to 1
      sbi(__UCSRA__, __TXC__);

      return;
    }
  }

  // clear the TXC bit, by setting it to 1
  sbi(__UCSRA__, __TXC__);

  // disable the data register empty interrupt
  cbi(__UCSRB__, __UDRIE__);
}

#endif /* USE_ARDUINO_HARDWARESERIAL */


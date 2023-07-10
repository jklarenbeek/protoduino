#include "SerialClass.hpp"

#include "ringb8.h"
#include "uart.h"

RINGB8(serial0_rx, SERIAL_RX_BUFFER_SIZE);
RINGB8(serial0_tx, SERIAL_TX_BUFFER_SIZE);

static void serial0_on_rx_complete(uint_fast8_t data)
{
  // is there room in the buffer?
  uint_fast8_t available = ringb8_available(&VAR_RINGB8(serial0_rx));
  if (available > 0)
  {
    // add the byte to the buffer
    ringb8_put(&VAR_RINGB8(serial0_rx), data);
  }
}

static uint32_t serial0_rx_errcnt = 0;

static void serial0_on_rx_error(uint_fast8_t err)
{
  serial0_rx_errcnt++;
  uart0_rx_clear_errors();
}

static int_fast16_t serial0_on_tx_complete(void)
{
  // is there anything to transmit?
  uint_fast8_t cnt = ringb8_count(&VAR_RINGB8(serial0_tx));
  if (cnt == 0)
    return -1;
    
  // send the next byte from the buffer
  return ringb8_get(&VAR_RINGB8(serial0_tx));
}

void Serial0Class::begin(uint32_t baud)
{
  uart0_on_rx_complete(serial0_on_rx_complete);
  uart0_on_rx_error(serial0_on_rx_error);
  uart0_on_tx_complete(serial0_on_tx_complete);
  uart0_open(baud);
}

void Serial0Class::begin(uint32_t baud, uint8_t config)
{
  uart0_on_rx_complete(serial0_on_rx_complete);
  uart0_on_rx_error(serial0_on_rx_error);
  uart0_on_tx_complete(serial0_on_tx_complete);
  uart0_openex(baud, config);
}

void Serial0Class::end(void)
{
  flush();
  uart0_close();
}

ptstate_t Serial0Class::end(struct pt *pt)
{
  PT_BEGIN(pt);
  PT_WAIT_WHILE(pt, (flushex() > 0));
  uart0_close();
  PT_END(pt);
}

int Serial0Class::available(void)
{
  return ringb8_count(&VAR_RINGB8(serial0_rx));
}

int Serial0Class::peek(void)
{
  return (ringb8_count(&VAR_RINGB8(serial0_rx)) > 0)
    ? ringb8_peek(&VAR_RINGB8(serial0_rx))
    : -1;    
}

int Serial0Class::read(void)
{
  return (ringb8_count(&VAR_RINGB8(serial0_rx)) > 0)
    ? ringb8_get(&VAR_RINGB8(serial0_rx))
    : -1;
}

int Serial0Class::availableForWrite(void)
{
  return ringb8_available(&VAR_RINGB8(serial0_tx));
}

int Serial0Class::flushex(void)
{
  // make sure the interrupt is enabled
  uart0_tx_enable_int();
  // return the amount of data still in the buffer to be transmitted.  
  return ringb8_count(&VAR_RINGB8(serial0_tx));
}

ptstate_t Serial0Class::flush(struct pt *pt)
{
  PT_BEGIN(pt);
  PT_WAIT_WHILE(pt, (flushex() > 0));
  PT_END(pt);
}

size_t Serial0Class::write(uint8_t data)
{
  if (ringb8_available(&VAR_RINGB8(serial0_tx)) > 0)
  {
    // make sure the interrupt is enabled
    uart0_tx_enable_int();
    // add packet to the transmit buffer
    ringb8_put(&VAR_RINGB8(serial0_tx), data);
    return 1;
  }
  
  return 0;
}

// size_t write ( uint8_t* val, int num ) => { while(idx < num) write(val[idx++]); }

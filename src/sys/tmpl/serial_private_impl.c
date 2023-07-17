
#include "protoduino-config.h"
#include "../serial.h"

#include "../uart.h"

#include "../ringb8.h"

#include <util/atomic.h>

RINGB8(serial0_rx, SERIAL_RX_BUFFER_SIZE);
RINGB8(serial0_tx, SERIAL_TX_BUFFER_SIZE);

static volatile serial_onrecieved_fn _serial0_onrecieved_callback = 0;

static void serial0_on_rx_complete(uint_fast8_t data)
{
  // the subscriber of this one, needs to get out of here a.s.a.p.
  if (_serial0_onrecieved_callback != 0 && _serial0_onrecieved_callback(data) == true)
    return;
    
  // is there room in the buffer?
  uint_fast8_t available = ringb8_available(&VAR_RINGB8(serial0_rx));
  if (available > 0)
  {
    // add the byte to the buffer
    ringb8_put(&VAR_RINGB8(serial0_rx), data);
  }

}

static uint32_t _serial0_rx_errcnt = 0;

static void serial0_on_rx_error(uint_fast8_t err)
{
  _serial0_rx_errcnt++;
  uart0_rx_clear_errors();
}

static int_fast16_t serial0_on_tx_complete(void)
{
  // is there anything to transmit?
  return ringb8_count(&VAR_RINGB8(serial0_tx)) > 0
    ? ringb8_get(&VAR_RINGB8(serial0_tx))
    : -1;
}

void serial0_on_recieved(const serial_onrecieved_fn callback)
{
  _serial0_onrecieved_callback = callback;
}

void serial0_open(uint32_t baud)
{
  uart0_on_rx_complete(serial0_on_rx_complete);
  uart0_on_rx_error(serial0_on_rx_error);
  uart0_on_tx_complete(serial0_on_tx_complete);
  uart0_open(baud);
}

void serial0_openex(uint32_t baud, uint8_t config)
{
  uart0_on_rx_complete(serial0_on_rx_complete);
  uart0_on_rx_error(serial0_on_rx_error);
  uart0_on_tx_complete(serial0_on_tx_complete);
  uart0_openex(baud, config);
}

void serial0_close(void)
{
  serial0_flush();
  uart0_close();
}

uint_fast8_t serial0_read_available(void)
{
  return ringb8_count(&VAR_RINGB8(serial0_rx));
}

int_fast16_t serial0_peek8(void)
{
  return serial0_read_available() > 0
    ? ringb8_peek(&VAR_RINGB8(serial0_rx))
    : -1;
}

int_fast16_t serial0_read8(void)
{
  return serial0_read_available() > 0
    ? ringb8_get(&VAR_RINGB8(serial0_rx))
    : -1;
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

uint_fast32_t serial0_read32(void)
{
  uint_fast8_t cnt = serial0_read_available();
  if (cnt < 4)
    return 0; // WATCH OUT! caller needs to care of available here!!
  
  union {
    uint32_t data;
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

CC_FLATTEN uint_fast8_t serial0_write8(const uint_fast8_t data)
{
  uint8_t cnt = ringb8_count(&VAR_RINGB8(serial0_tx));

  // when the buffer is empty and the data register is empty
  if (cnt == 0 && uart0_tx_is_ready())
  {
    uart0_tx_write8(data);
    return 1;
  }

  // we need to add data to the buffer
  uint8_t available = ringb8_size(&VAR_RINGB8(serial0_tx)) - cnt;
  if (available == 0) // but if we can't, we can't
  {
    return 0; // let the user handle it manually
  }

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    // add data to the ringbuffer
    ringb8_put(&VAR_RINGB8(serial0_tx), data);

    uart0_tx_enable_int();
  }

  return 1;
}

CC_FLATTEN uint_fast8_t serial0_write16(const uint_fast16_t data)
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

    uart0_tx_enable_int();
  }

  return 2;  
}

CC_FLATTEN uint_fast8_t serial0_write24(const uint_fast32_t data)
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

    uart0_tx_enable_int();
  }

  return 3;  
}

CC_FLATTEN uint_fast8_t serial0_write32(const uint_fast32_t data)
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

    uart0_tx_enable_int();
  }

  return 4;
}

uint_fast8_t serial0_flush(void)
{
  // get the amount of bytes still to be transmitted
  uint_fast8_t cnt = ringb8_count(&VAR_RINGB8(serial0_tx));
  // is there anything left in the buffer
  if (cnt == 0)
    return 0;

  // are the global interrupts disabled?
  if (bit_is_set(SREG, SREG_I))
  {
    // is the transmit register empty?
    if (!uart0_tx_is_ready())
      return cnt;

    // the transmit register is empty
    uart0_tx_write8(ringb8_get(&VAR_RINGB8(serial0_tx)));
    return cnt - 1;
  }

  // make sure the interrupt is enabled
  uart0_tx_enable_int();

  // and return the number of bytes in the buffer
  return cnt;
}

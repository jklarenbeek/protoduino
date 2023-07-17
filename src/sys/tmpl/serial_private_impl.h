
#include "../serial.h"

#include "../uart.h"

#include "../ringb8.h"

#include <util/atomic.h>

RINGB8(CC_TMPL_VAR(rx), SERIAL_RX_BUFFER_SIZE);
RINGB8(CC_TMPL_VAR(tx), SERIAL_TX_BUFFER_SIZE);

#define VAR_RX VAR_RINGB8(CC_TMPL_VAR(rx))
#define VAR_TX VAR_RINGB8(CC_TMPL_VAR(tx))

static volatile serial_onrecieved_fn CC_TMPL_VAR(onrecieved_callback) = 0;

#ifdef SERIAL_REGISTER_ERRORS
static uint32_t CC_TMPL_VAR(rx_errcnt) = 0;
static uint32_t CC_TMPL_VAR(rx_errofw) = 0;
#endif

static void CC_TMPL_FN(on_rx_complete)(uint_fast8_t data)
{
  // the subscriber of this one, needs to get out of here a.s.a.p.
  if (CC_TMPL_VAR(onrecieved_callback) != 0 && CC_TMPL_VAR(onrecieved_callback)(data) == true)
    return;
    
  // is there room in the buffer?
  uint_fast8_t available = ringb8_available(&VAR_RX);
  if (available > 0)
  {
    // add the byte to the buffer
    ringb8_put(&VAR_RX, data);
  }
#ifdef SERIAL_REGISTER_ERRORS
  else
  {
      CC_TMPL_VAR(rx_errofw)++;
  }
#endif
}

static void CC_TMPL_FN(on_rx_error)(uint_fast8_t err)
{
#ifdef SERIAL_REGISTER_ERRORS
  CC_TMPL_VAR(rx_errcnt)++;
#endif
  CC_TMPL2_FN(rx_clear_errors)();
}

static int_fast16_t CC_TMPL_FN(on_tx_complete)(void)
{
  // is there anything to transmit?
  return ringb8_count(&VAR_TX) > 0
    ? ringb8_get(&VAR_TX)
    : -1;
}

void CC_TMPL_FN(on_recieved)(const serial_onrecieved_fn callback)
{
  CC_TMPL_VAR(onrecieved_callback) = callback;
}

void CC_TMPL_FN(open)(uint32_t baud)
{
  CC_TMPL2_FN(on_rx_complete)(CC_TMPL_FN(on_rx_complete));
  CC_TMPL2_FN(on_rx_error)(CC_TMPL_FN(on_rx_error));
  CC_TMPL2_FN(on_tx_complete)(CC_TMPL_FN(on_tx_complete));
  CC_TMPL2_FN(open)(baud);
}

void CC_TMPL_FN(openex)(uint32_t baud, uint8_t config)
{
  CC_TMPL2_FN(on_rx_complete)(CC_TMPL_FN(on_rx_complete));
  CC_TMPL2_FN(on_rx_error)(CC_TMPL_FN(on_rx_error));
  CC_TMPL2_FN(on_tx_complete)(CC_TMPL_FN(on_tx_complete));
  CC_TMPL2_FN(openex)(baud, config);
}

void CC_TMPL_FN(close)(void)
{
  CC_TMPL_FN(flush)();
  CC_TMPL2_FN(close)();
}

uint_fast8_t CC_TMPL_FN(read_available)(void)
{
  return ringb8_count(&VAR_RX);
}

int_fast16_t CC_TMPL_FN(peek8)(void)
{
  return CC_TMPL_FN(read_available)() > 0
    ? ringb8_peek(&VAR_RX)
    : -1;
}

int_fast16_t CC_TMPL_FN(read8)(void)
{
  return CC_TMPL_FN(read_available)() > 0
    ? ringb8_get(&VAR_RX)
    : -1;
}

int_fast32_t CC_TMPL_FN(read16)(void)
{
  uint_fast8_t cnt = CC_TMPL_FN(read_available)();
  if (cnt < 2)
    return -1;
  
  union {
    uint16_t data;
    uint8_t buf[2];
  } tmp;

  tmp.buf[0] = ringb8_get(&VAR_RX);
  tmp.buf[1] = ringb8_get(&VAR_RX);

  return tmp.data;
}

int_fast32_t CC_TMPL_FN(read24)(void)
{
  uint_fast8_t cnt = CC_TMPL_FN(read_available)();
  if (cnt < 3)
    return -1;
  
  union {
    int32_t data;
    uint8_t buf[4];
  } tmp;

  tmp.buf[0] = ringb8_get(&VAR_RX);
  tmp.buf[1] = ringb8_get(&VAR_RX);
  tmp.buf[2] = ringb8_get(&VAR_RX);
  tmp.buf[3] = 0;

  return tmp.data;
}

uint_fast32_t CC_TMPL_FN(read32)(void)
{
  uint_fast8_t cnt = CC_TMPL_FN(read_available)();
  if (cnt < 4)
    return 0; // WATCH OUT! caller needs to care of available here!!
  
  union {
    uint32_t data;
    uint8_t buf[4];
  } tmp;

  tmp.buf[0] = ringb8_get(&VAR_RX);
  tmp.buf[1] = ringb8_get(&VAR_RX);
  tmp.buf[2] = ringb8_get(&VAR_RX);
  tmp.buf[3] = ringb8_get(&VAR_RX);

  return tmp.data;
}

uint_fast8_t CC_TMPL_FN(write_available)(void)
{
  return ringb8_available(&VAR_TX);
}

CC_FLATTEN uint_fast8_t CC_TMPL_FN(write8)(const uint_fast8_t data)
{
  uint8_t cnt = ringb8_count(&VAR_TX);

  // when the buffer is empty and the data register is empty
  if (cnt == 0 && CC_TMPL2_FN(tx_is_ready)())
  {
    CC_TMPL2_FN(tx_write8)(data);
    return 1;
  }

  // we need to add data to the buffer
  uint8_t available = ringb8_size(&VAR_TX) - cnt;
  if (available == 0) // but if we can't, we can't
  {
    return 0; // let the user handle it manually
  }

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    // add data to the ringbuffer
    ringb8_put(&VAR_TX, data);

    CC_TMPL2_FN(tx_enable_int)();
  }

  return 1;
}

CC_FLATTEN uint_fast8_t CC_TMPL_FN(write16)(const uint_fast16_t data)
{
  uint8_t available = CC_TMPL_FN(write_available)();
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
    ringb8_put(&VAR_TX, tmp.buf[0]);
    ringb8_put(&VAR_TX, tmp.buf[1]);

    CC_TMPL2_FN(tx_enable_int)();
  }

  return 2;  
}

CC_FLATTEN uint_fast8_t CC_TMPL_FN(write24)(const uint_fast32_t data)
{
  uint8_t available = CC_TMPL_FN(write_available)();
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
    ringb8_put(&VAR_TX, tmp.buf[0]);
    ringb8_put(&VAR_TX, tmp.buf[1]);
    ringb8_put(&VAR_TX, tmp.buf[2]);

    CC_TMPL2_FN(tx_enable_int)();
  }

  return 3;  
}

CC_FLATTEN uint_fast8_t CC_TMPL_FN(write32)(const uint_fast32_t data)
{
  uint8_t available = CC_TMPL_FN(write_available)();
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
    ringb8_put(&VAR_TX, tmp.buf[0]);
    ringb8_put(&VAR_TX, tmp.buf[1]);
    ringb8_put(&VAR_TX, tmp.buf[2]);
    ringb8_put(&VAR_TX, tmp.buf[3]);

    CC_TMPL2_FN(tx_enable_int)();
  }

  return 4;
}

uint_fast8_t CC_TMPL_FN(flush)(void)
{
  // get the amount of bytes still to be transmitted
  uint_fast8_t cnt = ringb8_count(&VAR_TX);
  // is there anything left in the buffer
  if (cnt == 0)
    return 0;

  // are the global interrupts disabled?
  if (bit_is_set(SREG, SREG_I))
  {
    // is the transmit register empty?
    if (!CC_TMPL2_FN(tx_is_ready)())
      return cnt;

    // the transmit register is empty
    CC_TMPL2_FN(tx_write8)(ringb8_get(&VAR_TX));
    return cnt - 1;
  }

  // make sure the interrupt is enabled
  CC_TMPL2_FN(tx_enable_int)();

  // and return the number of bytes in the buffer
  return cnt;
}

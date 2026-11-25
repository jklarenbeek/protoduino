// file: ./src/cpu/rp2/uart_private_impl.h

/* One PL011 UART, for uart0.c and uart1.c: CC_TMPL_PREFIX is uart0 or
 * uart1, __UART__ the SDK's instance, __UART_IRQ__ its interrupt, and
 * __UART_TX__ and __UART_RX__ the pins.
 *
 * As on an AVR the receive interrupt hands each byte, or its error, to a
 * callback, and the transmit interrupt asks a callback for the next byte
 * until it answers -1. A PL011 raises its transmit interrupt when its FIFO
 * drains below a level, not while it is empty: tx_enable_int() fills the
 * FIFO itself first, and the interrupt takes over from there. */

#include <cc.h>
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/structs/uart.h"

static volatile uart_on_rx_complete_fn CC_TMPL_VAR(on_rx_complete) = 0;
static volatile uart_on_rx_error_fn CC_TMPL_VAR(on_rx_error) = 0;
static volatile uart_on_tx_complete_fn CC_TMPL_VAR(on_tx_complete) = 0;
static uint32_t CC_TMPL_VAR(baudrate) = 0;
static volatile bool CC_TMPL_VAR(tx_int) = false;

static void CC_TMPL_FN(irq)(void);

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
  CC_TMPL_FN(openex)(baud, 0x06);
}

/* options as UCSRnC of an AVR: bits 2 and 1 the data bits minus 5, bit 3
 * two stop bits, bits 5 and 4 the parity (10 even, 11 odd) */
void CC_TMPL_FN(openex)(uint32_t baud, uint8_t options)
{
  uart_inst_t *u = __UART__;
  CC_TMPL_VAR(baudrate) = uart_init(u, baud);
  uint data_bits = 5 + ((options >> 1) & 3);
  uint stop_bits = (options & 0x08) ? 2 : 1;
  uart_parity_t parity = ((options >> 4) & 3) == 2 ? UART_PARITY_EVEN : ((options >> 4) & 3) == 3 ? UART_PARITY_ODD : UART_PARITY_NONE;
  uart_set_format(u, data_bits, stop_bits, parity);
  uart_set_fifo_enabled(u, true);
  gpio_set_function(__UART_TX__, GPIO_FUNC_UART);
  gpio_set_function(__UART_RX__, GPIO_FUNC_UART);
  CC_TMPL_VAR(tx_int) = false;
  irq_set_exclusive_handler(__UART_IRQ__, CC_TMPL_FN(irq));
  /* receive, and the receive timeout for the bytes below the FIFO level */
  uart_get_hw(u)->imsc = UART_UARTIMSC_RXIM_BITS | UART_UARTIMSC_RTIM_BITS;
  irq_set_enabled(__UART_IRQ__, true);
}

void CC_TMPL_FN(close)(void)
{
  irq_set_enabled(__UART_IRQ__, false);
  CC_TMPL_VAR(baudrate) = 0;
  CC_TMPL_VAR(tx_int) = false;
  uart_deinit(__UART__);
}

uint_fast32_t CC_TMPL_FN(baudrate)(void)
{
  return CC_TMPL_VAR(baudrate);
}

bool CC_TMPL_FN(rx_is_ready)(void)
{
  return uart_is_readable(__UART__);
}

uint_fast8_t CC_TMPL_FN(rx_error)(void)
{
  uint32_t rsr = uart_get_hw(__UART__)->rsr;
  if (rsr & UART_UARTRSR_OE_BITS)
    return ERR_SERIAL_OVERRUN;
  if (rsr & UART_UARTRSR_FE_BITS)
    return ERR_SERIAL_FRAME;
  if (rsr & UART_UARTRSR_PE_BITS)
    return ERR_SERIAL_PARITY;
  return ERR_SUCCESS;
}

void CC_TMPL_FN(rx_clear_errors)(void)
{
  uart_get_hw(__UART__)->rsr = UART_UARTRSR_BITS;
}

uint_fast8_t CC_TMPL_FN(rx_read8)(void)
{
  return (uint_fast8_t)(uart_get_hw(__UART__)->dr & 0xFF);
}

bool CC_TMPL_FN(tx_is_int_enabled)(void)
{
  return CC_TMPL_VAR(tx_int);
}

bool CC_TMPL_FN(tx_is_ready)(void)
{
  return uart_is_writable(__UART__);
}

CC_FLATTEN bool CC_TMPL_FN(tx_is_available)(void)
{
  return !CC_TMPL_FN(tx_is_int_enabled)() && CC_TMPL_FN(tx_is_ready)();
}

void CC_TMPL_FN(tx_write8)(const uint_fast8_t data)
{
  uart_get_hw(__UART__)->dr = data;
}

/* bytes from the callback while the FIFO has room; interrupts are off */
static void CC_TMPL_FN(tx_fill)(void)
{
  uart_hw_t *hw = uart_get_hw(__UART__);
  while (CC_TMPL_VAR(tx_int) && !(hw->fr & UART_UARTFR_TXFF_BITS))
  {
    int_fast16_t data = CC_TMPL_VAR(on_tx_complete) ? CC_TMPL_VAR(on_tx_complete)() : -1;
    if (data < 0)
    {
      CC_TMPL_VAR(tx_int) = false;
      hw->imsc &= ~UART_UARTIMSC_TXIM_BITS;
      return;
    }
    hw->dr = (uint8_t)data;
  }
}

/* Fills the FIFO, on every call: a caller that waits for the ring to empty
 * with interrupts off (flush() in a loop) gets it emptied all the same. */
void CC_TMPL_FN(tx_enable_int)(void)
{
  CC_ATOMIC_RESTORE()
  {
    CC_TMPL_VAR(tx_int) = true;
    CC_TMPL_FN(tx_fill)();
    if (CC_TMPL_VAR(tx_int))
      uart_get_hw(__UART__)->imsc |= UART_UARTIMSC_TXIM_BITS;
  }
}

static void CC_TMPL_FN(irq)(void)
{
  uart_hw_t *hw = uart_get_hw(__UART__);
  while (!(hw->fr & UART_UARTFR_RXFE_BITS))
  {
    uint32_t dr = hw->dr; /* the byte, and its errors in bits 8 to 11 */
    uint_fast8_t err = (dr & UART_UARTDR_OE_BITS) ? ERR_SERIAL_OVERRUN
                       : (dr & UART_UARTDR_FE_BITS) ? ERR_SERIAL_FRAME
                       : (dr & UART_UARTDR_PE_BITS) ? ERR_SERIAL_PARITY
                                                    : ERR_SUCCESS;
    if (err != ERR_SUCCESS)
    {
      hw->rsr = UART_UARTRSR_BITS;
      if (CC_TMPL_VAR(on_rx_error))
        CC_TMPL_VAR(on_rx_error)(err);
    }
    else if (CC_TMPL_VAR(on_rx_complete))
    {
      CC_TMPL_VAR(on_rx_complete)((uint_fast8_t)(dr & 0xFF));
    }
  }
  if (CC_TMPL_VAR(tx_int))
  {
    hw->icr = UART_UARTICR_TXIC_BITS;
    CC_TMPL_FN(tx_fill)();
  }
}

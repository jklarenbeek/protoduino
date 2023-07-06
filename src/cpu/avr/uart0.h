#ifndef __UART0_H__
#define __UART0_H__

#include "uart.h"

#if defined(HAVE_HW_UART0)
CC_EXTERN void uart0_on_rx_complete(uart_on_rx_complete_fn callback);
CC_EXTERN void uart0_on_rx_error(uart_on_rx_error_fn callback);
CC_EXTERN void uart0_on_tx_complete(uart_on_tx_complete_fn callback);

CC_EXTERN void uart0_open(uint32_t baud);
CC_EXTERN void uart0_openex(uint32_t baud, uint8_t options);
CC_EXTERN void uart0_close(void);
CC_EXTERN uint_fast32_t uart0_baudrate(void);

CC_EXTERN bool uart0_rx_is_ready(void);
CC_EXTERN uint_fast8_t uart0_rx_error(void);
CC_EXTERN uint_fast8_t uart0_rx_read8(void);

CC_EXTERN void uart0_tx_enable();
CC_EXTERN bool uart0_tx_is_enabled(void);
CC_EXTERN bool uart0_tx_is_ready(void);
CC_EXTERN bool uart0_tx_is_available(void);
CC_EXTERN void uart0_tx_write8(const uint_fast8_t data);
#endif

#endif

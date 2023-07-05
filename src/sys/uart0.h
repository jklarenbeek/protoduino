#ifndef __UART0_H__
#define __UART0_H__

#include "cc.h"
#include <stdbool.h>

#define SERIAL_BAUD_1200 1200
#define SERIAL_BAUD_2400 2400
#define SERIAL_BAUD_4800 4800
#define SERIAL_BAUD_9600 9600
#define SERIAL_BAUD_19200 19200
#define SERIAL_BAUD_38400 38400
#define SERIAL_BAUD_57600 57600
#define SERIAL_BAUD_115200 115200 

// send flowcontrol xoff message to sender (force sender to wait)
#define SERIAL_FLOWCONTROL_XOFF 0x13
// send flowcontrol xon to sender (tell sender to start transmitting again)
#define SERIAL_FLOWCONTROL_XON 0x11

typedef uint_fast8_t (*uart0_on_rx_complete_fn)(uint_fast8_t);
typedef void (*uart0_on_rx_error_fn)(uint_fast8_t);
typedef void (*uart0_on_tx_complete_fn)(uint_fast8_t sended, uint_fast8_t misses);

CC_EXTERN void uart0_on_rx_complete(uart0_on_rx_complete_fn callback);
CC_EXTERN void uart0_on_rx_error(uart0_on_rx_error_fn callback);
CC_EXTERN void uart0_on_tx_complete(uart0_on_tx_complete_fn callback);

CC_EXTERN void uart0_open(uint32_t baud);
CC_EXTERN void uart0_openex(uint32_t baud, uint8_t options);
CC_EXTERN void uart0_close(void);
CC_EXTERN uint_fast32_t uart0_baudrate(void);

CC_EXTERN bool uart0_rx_is_available(void);
CC_EXTERN uint_fast8_t uart0_rx_error(void);
CC_EXTERN uint_fast8_t uart0_rx_read8(void);

CC_EXTERN bool uart0_tx_is_interrupt_enabled(void);
CC_EXTERN bool uart0_tx_is_empty(void);
CC_EXTERN bool uart0_tx_is_available(void);
CC_EXTERN void uart0_tx_write8(const uint_fast8_t data);

#endif

// file: ./src/cpu/rp2/uart.h

#ifndef __RP2_UART_H__
#define __RP2_UART_H__

/* The two PL011 UARTs of the RP2040 and RP2350, with the functions of
 * cpu/avr/uart.h: uart0_open(), uart0_tx_write8(), the callbacks ...
 *
 *   uart0  TX GP0, RX GP1     uart1  TX GP4, RX GP5     (UART_CONF_*_TX/RX)
 *
 * The interrupt handlers are protoduino's (irq_set_exclusive_handler): an
 * arduino-pico Serial1 or Serial2 on the same UART cannot be used with them. */

#include <protoduino-config.h>
#include <cc.h>
#include "../../sys/errors.h"

#include <stdbool.h>
#include <stdint.h>

#define UART_BAUD_1200 1200
#define UART_BAUD_2400 2400
#define UART_BAUD_4800 4800
#define UART_BAUD_9600 9600
#define UART_BAUD_19200 19200
#define UART_BAUD_38400 38400
#define UART_BAUD_57600 57600
#define UART_BAUD_115200 115200

// XOFF character for software flow control
#define UART_FLOWCONTROL_XOFF 0x13
// XON character for software flow control
#define UART_FLOWCONTROL_XON 0x11

#ifndef UART_CONF_UART0_TX
#define UART_CONF_UART0_TX 0
#endif
#ifndef UART_CONF_UART0_RX
#define UART_CONF_UART0_RX 1
#endif
#ifndef UART_CONF_UART1_TX
#define UART_CONF_UART1_TX 4
#endif
#ifndef UART_CONF_UART1_RX
#define UART_CONF_UART1_RX 5
#endif

typedef void (*uart_on_rx_complete_fn)(uint_fast8_t);
typedef void (*uart_on_rx_error_fn)(uint_fast8_t);
typedef int_fast16_t (*uart_on_tx_complete_fn)(void);

/* uart0 and uart1 are macros of the Pico SDK (hardware/uart.h); the names
 * of these functions are made from the bare words */
#pragma push_macro("uart0")
#pragma push_macro("uart1")
#undef uart0
#undef uart1

#define HAVE_HW_UART0
#undef CC_TMPL_PREFIX
#define CC_TMPL_PREFIX uart0
#include "../avr/uart_private_header.h"

#define HAVE_HW_UART1
#undef CC_TMPL_PREFIX
#define CC_TMPL_PREFIX uart1
#include "../avr/uart_private_header.h"

#pragma pop_macro("uart0")
#pragma pop_macro("uart1")

#endif /* __RP2_UART_H__ */

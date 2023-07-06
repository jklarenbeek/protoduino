#ifndef __UART_H__
#define __UART_H__

#include "../../protoduino-config.h"
#include "../../sys/cc.h"
#include "../../sys/errors.h"

#include <stdbool.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define UART_BAUD_1200 1200
#define UART_BAUD_2400 2400
#define UART_BAUD_4800 4800
#define UART_BAUD_9600 9600
#define UART_BAUD_19200 19200
#define UART_BAUD_38400 38400
#define UART_BAUD_57600 57600
#define UART_BAUD_115200 115200 

// send flowcontrol xoff message to sender (force sender to wait)
#define UART_FLOWCONTROL_XOFF 0x13
// send flowcontrol xon to sender (tell sender to start transmitting again)
#define UART_FLOWCONTROL_XON 0x11

typedef void (*uart_on_rx_complete_fn)(uint_fast8_t);
typedef void (*uart_on_rx_error_fn)(uint_fast8_t);
typedef int_fast16_t (*uart_on_tx_complete_fn)(void);

#if defined(UBRRH) || defined(UBRR0H)
#define HAVE_HW_UART0
#undef CC_NAME_PREFIX
#define CC_NAME_PREFIX uart0
#include "uart_private_header.h"
#endif

#if defined(UBRR1H)
#define HAVE_HW_UART1
#undef CC_NAME_PREFIX
#define CC_NAME_PREFIX uart1
#include "uart_private_header.h"
#endif

#if defined(UBRR2H)
#define HAVE_HW_UART2
#undef CC_NAME_PREFIX
#define CC_NAME_PREFIX uart2
#include "uart_private_header.h"
#endif

#if defined(UBRR3H)
#define HAVE_HW_UART3
#undef CC_NAME_PREFIX
#define CC_NAME_PREFIX uart3
#include "uart_private_header.h"
#endif

#endif
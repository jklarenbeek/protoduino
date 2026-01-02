#ifndef __UART_H__
#define __UART_H__

#if defined (__AVR_ATmega328P__) || defined(__AVR_ATmega328__) || defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega88__)
#include "../cpu/avr/uart.h"

// #pragma message "uart: Using AVR."

#else

#error "uart: Unsupported Platform!"

#endif

#endif /* __UART_H__ */

#ifndef __UART_H__
#define __UART_H__

/*
 * Platform selector for UART hardware support.
 *
 * cpu/avr/uart.h itself uses UBRR0H / UBRR1H / UBRR2H / UBRR3H presence
 * guards, so it works correctly on any AVR that has those registers
 * (ATmega328P, ATmega2560, ATmega1280, etc.).
 *
 * Add new architectures here as needed.
 */
#if defined(__AVR__)
#include "../cpu/avr/uart.h"

#else

#error "uart: Unsupported Platform!"

#endif

#endif /* __UART_H__ */

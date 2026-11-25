#ifndef __UART_H__
#define __UART_H__

/*
 * Platform selector for UART hardware support.
 *
 * cpu/avr/uart.h itself uses UBRR0H / UBRR1H / UBRR2H / UBRR3H presence
 * guards, so it works correctly on any AVR that has those registers
 * (ATmega328P, ATmega2560, ATmega1280, etc.). cpu/rp2/uart.h has the two
 * UARTs of the RP2040 and RP2350.
 *
 * Add new architectures here as needed.
 */
#include <cc.h>

#if defined(__AVR__)
#include "../cpu/avr/uart.h"

#elif defined(CC_ARCH_RP2)
#include "../cpu/rp2/uart.h"

#else

#error "uart: Unsupported Platform!"

#endif

#endif /* __UART_H__ */

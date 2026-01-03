// file: ./src/sys/serial.h

#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <protoduino-config.h>
#include <cc.h>
#include "errors.h"
#include "uart.h"
#include <stdbool.h>


/*
 * compile-time selector: tiny (short) vs verbose messages.
 */
#ifndef S
  #if ERRORS_CONF_VERBOSE_MESSAGES
    #define S(tiny, verbose) (verbose)
  #else
    #define S(tiny, verbose) (tiny)
  #endif
#endif

CC_EXTERN typedef bool (*serial_onrecieved_fn)(uint_fast8_t);

#ifdef HAVE_HW_UART0
#undef CC_TMPL_PREFIX
#define CC_TMPL_PREFIX serial0
#include "tmpl/serial_private_header.h"
#endif

#ifdef HAVE_HW_UART1
#undef CC_TMPL_PREFIX
#define CC_TMPL_PREFIX serial1
#include "tmpl/serial_private_header.h"
#endif

#ifdef HAVE_HW_UART2
#undef CC_TMPL_PREFIX
#define CC_TMPL_PREFIX serial2
#include "tmpl/serial_private_header.h"
#endif

#ifdef HAVE_HW_UART3
#undef CC_TMPL_PREFIX
#define CC_TMPL_PREFIX serial3
#include "tmpl/serial_private_header.h"
#endif

CC_EXTERN void printchar(uint_fast8_t c);
CC_EXTERN void println(void);

CC_EXTERN void print_P(const char *s);
CC_EXTERN void println_P(const char *s);

CC_EXTERN void print(const char *s);
CC_EXTERN void print_dec(uint8_t val);

#endif /* __SERIAL_H__ */
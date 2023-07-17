#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "../protoduino-config.h"
#include "cc.h"
#include "errors.h"
#include "uart.h"

#include <stdbool.h>


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

#ifdef HAVE_HW_UART1
#undef CC_TMPL_PREFIX
#define CC_TMPL_PREFIX serial1
#include "tmpl/serial_private_header.h"
#endif

#ifdef HAVE_HW_UART1
#undef CC_TMPL_PREFIX
#define CC_TMPL_PREFIX serial1
#include "tmpl/serial_private_header.h"
#endif

#endif /* __SERIAL_H__ */
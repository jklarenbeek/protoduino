#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "protoduino-config.h"
#include "cc.h"
#include "errors.h"
#include "uart.h"

#include <stdbool.h>


CC_EXTERN typedef bool (*serial_onrecieved_fn)(uint_fast8_t);

#ifdef HAVE_HW_UART0
#include "tmpl/serial0.h"
#endif

#endif /* __SERIAL_H__ */
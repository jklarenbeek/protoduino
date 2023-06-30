#ifndef __SERIAL0_H__
#define __SERIAL0_H__

#include "cc.h"
//#include <stdint.h>
#include <stdbool.h>
//#include <stdio.h>

CC_EXTERN void serial0_register(void (*callback)(uint_fast8_t));
CC_EXTERN void serial0_open(uint32_t baud, uint8_t config);
CC_EXTERN void serial0_close();
CC_EXTERN uint32_t serial0_get_baudrate();

CC_EXTERN uint_fast8_t serial0_read_available();
CC_EXTERN uint_fast8_t serial0_read8_unchecked();
CC_EXTERN int_fast16_t serial0_read8();

CC_EXTERN bool serial0_write_available();
CC_EXTERN void serial0_write8_unchecked(uint_fast8_t data);
CC_EXTERN bool serial0_write8(uint_fast8_t data);

#endif

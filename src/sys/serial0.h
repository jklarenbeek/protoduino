#ifndef __SERIAL0_H__
#define __SERIAL0_H__

#include "cc.h"
//#include <stdint.h>
#include <stdbool.h>
//#include <stdio.h>

CC_EXTERN void serial0_register(void (*callback)(uint_fast8_t));
CC_EXTERN uint32_t serial0_begin(uint32_t baud, uint8_t config);
CC_EXTERN void serial0_close();

CC_EXTERN bool serial0_read_available();
CC_EXTERN uint8_t serial0_read_unchecked();
CC_EXTERN int16_t serial0_read();
CC_EXTERN bool serial0_write_available();
CC_EXTERN void serial0_write_unchecked(uint8_t data);
CC_EXTERN bool serial0_write(uint8_t data);

#endif

#ifndef __SERIAL0_H__
#define __SERIAL0_H__

#include "../cc.h"
#include <stdbool.h>

CC_EXTERN void serial0_on_recieved(const serial_onrecieved_fn callback);

CC_EXTERN void serial0_open(uint32_t baud);
CC_EXTERN void serial0_openex(uint32_t baud, uint8_t config);
CC_EXTERN void serial0_close(void);

CC_EXTERN uint_fast8_t serial0_read_available(void);
CC_EXTERN int_fast16_t serial0_peek8(void);
CC_EXTERN int_fast16_t serial0_read8(void);
CC_EXTERN int_fast32_t serial0_read16(void);
CC_EXTERN int_fast32_t serial0_read24(void);
CC_EXTERN uint_fast32_t serial0_read32(void);

CC_EXTERN uint_fast8_t serial0_write_available();
CC_EXTERN uint_fast8_t serial0_write8(const uint_fast8_t data);
CC_EXTERN uint_fast8_t serial0_write16(const uint_fast16_t data);
CC_EXTERN uint_fast8_t serial0_write24(const uint_fast32_t data);
CC_EXTERN uint_fast8_t serial0_write32(const uint_fast32_t data);

CC_EXTERN uint_fast8_t serial0_flush(void);

#define serial0_peek() serial0_peek8(void)
#define serial0_read() serial0_read8(void)

#define serial0_write(data) _Generic((data), uint8_fast8_t: serial0_write8, uint_fast16_t: serial0_write16, uint_fast32_t: serial0_write32)(data)

#endif

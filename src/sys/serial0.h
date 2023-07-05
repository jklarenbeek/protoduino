#ifndef __SERIAL0_H__
#define __SERIAL0_H__

#include "cc.h"
#include <stdbool.h>


#define SERIAL_BAUD_1200 1200
#define SERIAL_BAUD_2400 2400
#define SERIAL_BAUD_4800 4800
#define SERIAL_BAUD_9600 9600
#define SERIAL_BAUD_19200 19200
#define SERIAL_BAUD_38400 38400
#define SERIAL_BAUD_57600 57600
#define SERIAL_BAUD_115200 115200 

// send flowcontrol xoff message to sender (force sender to wait)
#define SERIAL_FLOWCONTROL_XOFF 0x13
// send flowcontrol xon to sender (tell sender to start transmitting again)
#define SERIAL_FLOWCONTROL_XON 0x11

CC_EXTERN void serial0_on_rx_complete(uint_fast8_t (*callback)(uint_fast8_t));
CC_EXTERN void serial0_on_rx_error(void (*callback)(uint_fast8_t));
CC_EXTERN void serial0_on_tx_complete(void (*callback)(uint_fast8_t sended, uint_fast8_t misses));

CC_EXTERN void serial0_open(uint32_t baud);
CC_EXTERN void serial0_openex(uint32_t baud, uint8_t options);
CC_EXTERN void serial0_close(void);
CC_EXTERN uint_fast32_t serial0_baudrate(void);

CC_EXTERN bool serial0_rx_is_available(void);
CC_EXTERN uint_fast8_t serial0_rx_error(void);
CC_EXTERN uint_fast8_t serial0_rx_read8(void);

CC_EXTERN bool serial0_tx_is_interrupt_enabled(void);
CC_EXTERN bool serial0_tx_is_empty(void);
CC_EXTERN bool serial0_tx_is_available(void);
CC_EXTERN void serial0_tx_write8(const uint_fast8_t data);

CC_EXTERN uint_fast8_t serial0_read_available(void);
CC_EXTERN int_fast16_t serial0_peek8(void);
CC_EXTERN int_fast16_t serial0_read8(void);
CC_EXTERN int_fast32_t serial0_read16(void);
CC_EXTERN int_fast32_t serial0_read24(void);
CC_EXTERN int_fast32_t serial0_read32(void);

CC_EXTERN uint_fast8_t serial0_write_available();
CC_EXTERN uint_fast8_t serial0_write8(const uint_fast8_t data);
CC_EXTERN uint_fast8_t serial0_write16(const uint_fast16_t data);
CC_EXTERN uint_fast8_t serial0_write24(const uint_fast32_t data);
CC_EXTERN uint_fast8_t serial0_write32(const uint_fast32_t data);
CC_EXTERN uint_fast8_t serial0_flush(void);

#define serial0_write(data) _Generic((data), uint8_fast8_t: serial0_write8, uint_fast16_t: serial0_write16, uint_fast32_t: serial0_write32)(data)

#endif

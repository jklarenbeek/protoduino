#include "../cc.h"

CC_EXTERN void CC_TMPL_FN(on_recieved)(const serial_onrecieved_fn callback);

CC_EXTERN void CC_TMPL_FN(open)(uint32_t baud);
CC_EXTERN void CC_TMPL_FN(openex)(uint32_t baud, uint8_t config);
CC_EXTERN void CC_TMPL_FN(close)(void);

CC_EXTERN uint_fast8_t CC_TMPL_FN(read_available)(void);
CC_EXTERN int_fast16_t CC_TMPL_FN(peek8)(void);
CC_EXTERN int_fast16_t CC_TMPL_FN(read8)(void);
CC_EXTERN int_fast32_t CC_TMPL_FN(read16)(void);
CC_EXTERN int_fast32_t CC_TMPL_FN(read24)(void);
CC_EXTERN uint_fast32_t CC_TMPL_FN(read32)(void);

CC_EXTERN uint_fast8_t CC_TMPL_FN(write_available)();
CC_EXTERN uint_fast8_t CC_TMPL_FN(write8)(const uint_fast8_t data);
CC_EXTERN uint_fast8_t CC_TMPL_FN(write16)(const uint_fast16_t data);
CC_EXTERN uint_fast8_t CC_TMPL_FN(write24)(const uint_fast32_t data);
CC_EXTERN uint_fast8_t CC_TMPL_FN(write32)(const uint_fast32_t data);

CC_EXTERN uint_fast8_t CC_TMPL_FN(flush)(void);

//#define serial0_peek() serial0_peek8(void)
//#define serial0_read() serial0_read8(void)

//#define serial0_write(data) _Generic((data), uint8_fast8_t: serial0_write8, uint_fast16_t: serial0_write16, uint_fast32_t: serial0_write32)(data)

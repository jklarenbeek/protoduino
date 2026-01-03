// file: ./src/dbg/print.h

#ifndef __DBG_PRINT_H__
#define __DBG_PRINT_H__

#include <protoduino.h>

extern int print_count;

CC_EXTERN void print_setup();
CC_EXTERN void print_info_P(const char *msg);

CC_EXTERN void print_state_ex_P(const ptstate_t s, const lc_t lc);
CC_EXTERN void print_state_P(const char *msg, const ptstate_t s);
CC_EXTERN void print_state_lc_P(const char *msg, const ptstate_t s, const lc_t lc);
CC_EXTERN void print_state_val_P(const char *msg, const ptstate_t s, const uint8_t value);
CC_EXTERN void print_state_full_P(const char *msg, const ptstate_t s, const lc_t lc, const uint8_t value);

CC_EXTERN void print_line_P(const char *msg);
CC_EXTERN void print_line_lc_P(const char *msg, const lc_t lc);
CC_EXTERN void print_line_val_P(const char *msg, uint8_t value);
CC_EXTERN void print_line_full_P(const char *msg, const lc_t lc, uint8_t value);

CC_EXTERN void print_error_P(const char *msg, uint8_t err);

#endif /* __DBG_PRINT_H__ */

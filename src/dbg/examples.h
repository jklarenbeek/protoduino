// file: ./src/dbg/examples.h

#ifndef __EXAMPLES_H__
#define __EXAMPLES_H__

#include <protoduino.h>
#ifndef USE_ARDUINO_HARDWARESERIAL
#include "../sys/tmpl/SerialClass.hpp"
#endif

extern int print_count;

void print_setup();
void print_info(const char *msg);

void print_state_ex(const ptstate_t s, const lc_t lc);
void print_state(const char *msg, const ptstate_t s);
void print_state(const char *msg, const ptstate_t s, const lc_t lc);
void print_state(const char *msg, const ptstate_t s, const uint8_t value);
void print_state(const char *msg, const ptstate_t s, const lc_t lc, const uint8_t value);

void print_state(const __FlashStringHelper *msg, const ptstate_t s);
void print_state(const __FlashStringHelper *msg, const ptstate_t s, const lc_t lc);
void print_state(const __FlashStringHelper *msg, const ptstate_t s, const uint8_t value);
void print_state(const __FlashStringHelper *msg, const ptstate_t s, const lc_t lc, const uint8_t value);

void print_line(const char *msg);
void print_line(const char *msg, const lc_t lc);
void print_line(const char *msg, uint8_t value);
void print_line(const char *msg, const lc_t lc, uint8_t value);

void print_line(const __FlashStringHelper *msg);
void print_line(const __FlashStringHelper *msg, const lc_t lc);
void print_line(const __FlashStringHelper *msg, uint8_t value);
void print_line(const __FlashStringHelper *msg, const lc_t lc, uint8_t value);

void print_error(const char *msg, uint8_t err);
void print_error(const __FlashStringHelper *msg, uint8_t err);

#endif

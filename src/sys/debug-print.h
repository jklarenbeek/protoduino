
#ifndef __DEBUG_PRINT_H__
#define __DEBUG_PRINT_H__

#include <protoduino.h>

#include "pt.h"

extern int print_count;

void print_setup();
void print_info(const char * msg);
void print_state(const ptstate_t s, const char * msg);
void print_error(const char * str, uint8_t err);
void print_line(const char * str);

#endif

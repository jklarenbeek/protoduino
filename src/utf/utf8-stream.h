#ifndef __UTF8_STREAM_H__
#define __UTF8_STREAM_H__

#include <protoduino.h>
#include <Stream.h>
#include "utf8.h"

int8_t utf8_getr(Stream *st, rune16_t *rune);
int8_t utf8_putr(Stream *st, const rune16_t rune);
int utf8_puts(Stream *st, const rune16_t * rune);
int utf8_puts_P(Stream *st, const rune16_t * rune);
int utf8_puti(Stream *st, uint8_t value);

#endif

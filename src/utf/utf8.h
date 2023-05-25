/**
 * This module provides utf8 support
 * Some functions are taken from https://github.com/9fans/plan9port
 * 
 */
#ifndef __UTF8_H__
#define __UTF8_H__

#include <protoduino.h>

// https://www.ssec.wisc.edu/~tomw/java/unicode.html
// https://gist.github.com/jpassaro/bf400b0410810a071a7fb3509ef6c2c3
// https://www.asciitable.com/
typedef uint16_t rune16_t;
typedef uint32_t rune32_t;

/* decoding error in UTF */
#define UTF8_DECODE_ERROR 0xFFFD

#define UTF8_RET_SUCCESS 0
#define UTF8_RET_CORRUPT -1
#define UTF8_RET_OVERFLOW -2
#define UTF8_RET_INCOMPLETE -3

char * utf8_ecpy(char *to, char *e, char *from);
uint8_t utf8_torune16(rune16_t *rune, const char *str);
uint8_t utf8_fromrune16(char *str, const rune16_t rune);

int utf8_len(char *s);

uint8_t utf8_rune16len(const rune16_t rune);
int utf8_rune16nlen(rune16_t * rune, int nlen);

#endif
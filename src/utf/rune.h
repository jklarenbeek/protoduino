/**
 * This module provides unicode support
 * most functions are taken from https://github.com/9fans/plan9port
 * 
 */
#ifndef __RUNE_H__
#define __RUNE_H__

#include <protoduino.h>
#include "utf8.h"

rune16_t * rune16_strcat(rune16_t *s1, rune16_t *s2);

rune16_t * rune16_strchr(rune16_t *s, rune16_t c);

int rune16_strcmp(rune16_t *s1, rune16_t *s2);

rune16_t * rune16_strcpy(rune16_t *s1, rune16_t *s2);

/// rune16_t * rune16_strdup(rune16_t *s);

rune16_t * rune16_strecpy(rune16_t *s1, rune16_t *es1, rune16_t *s2);

long rune16_strlen(rune16_t *s);

rune16_t * rune16_strncat(rune16_t *s1, rune16_t *s2, long n);

int rune16_strncmp(rune16_t *s1, rune16_t *s2, long n);

rune16_t * rune16_strncpy(rune16_t *s1, rune16_t *s2, long n);

rune16_t * rune16_strrchr(rune16_t *s, rune16_t c);

rune16_t * rune16_strstr(rune16_t *s1, rune16_t *s2);

rune16_t rune16_tolower(const rune16_t c);

rune16_t rune16_toupper(const rune16_t c);

rune16_t rune16_totitle(const rune16_t c);

bool rune16_islower(const rune16_t c);

bool rune16_isupper(const rune16_t c);

bool rune16_isalpha(const rune16_t c);

bool rune16_istitle(const rune16_t c);

bool rune16_isspace(const rune16_t c);

#endif
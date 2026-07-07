/*
 * The authors of this software are Rob Pike and Ken Thompson.
 *              Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE
 * ANY REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

/**
 * modified for arduino by
 * Joham (http://github.com/jklarenbeek)
 *
 */
#include "rune16.h"

#if PROTODUINO_UNICODE_LEVEL >= 1
#include "rune16_private.h"
#endif

/*
 * PROGMEM read abstraction for the classification / case tables.
 * On AVR the tables are stored in flash (CC_PROGMEM) and MUST be read
 * with pgm_read_word(); a plain dereference reads the same address in
 * SRAM and returns garbage.  On every other target the tables are
 * ordinary const data.
 */
#ifdef __AVR__
#  include <avr/pgmspace.h>
#  define _R16(p)   ((rune16_t)pgm_read_word(p))
#else
#  define _R16(p)   (*(p))
#endif

rune16_t * rune16_strcat(rune16_t *s1, rune16_t *s2)
{
	rune16_strcpy(rune16_strchr(s1, 0), s2);
	return s1;
}

rune16_t * rune16_strchr(rune16_t *s, rune16_t c)
{
	rune16_t c0 = c;
	rune16_t c1;

	if(c == 0)
	{
		while(*s++);
		return s-1;
	}

	while(c1 = *s++)
		if(c1 == c0)
			return s-1;
	return 0;
}

int rune16_strcmp(rune16_t *s1, rune16_t *s2)
{
	rune16_t c1, c2;

	for(;;)
	{
		c1 = *s1++;
		c2 = *s2++;
		if(c1 != c2)
		{
			if(c1 > c2)
				return 1;
			return -1;
		}
		if(c1 == 0)
			return 0;
	}
}

rune16_t * rune16_strcpy(rune16_t *s1, rune16_t *s2)
{
	rune16_t * os1;

	os1 = s1;
	while(*s1++ = *s2++);
	return os1;
}

rune16_t * rune16_strecpy(rune16_t *s1, rune16_t *es1, rune16_t *s2)
{
	if(s1 >= es1)
		return s1;

	while(*s1++ = *s2++)
	{
		if(s1 == es1){
			*--s1 = '\0';
			break;
		}
	}
	return s1;
}

long rune16_strlen(rune16_t *s)
{
	return rune16_strchr(s, 0) - s;
}

rune16_t * rune16_strncat(rune16_t *s1, rune16_t *s2, long n)
{
	rune16_t *os1;

	os1 = s1;
	s1 = rune16_strchr(s1, 0);
	while(*s1++ = *s2++)
	{
		if(--n < 0) {
			s1[-1] = 0;
			break;
		}
	}
	return os1;
}

int rune16_strncmp(rune16_t *s1, rune16_t *s2, long n)
{
	rune16_t c1, c2;

	while(n > 0) {
		c1 = *s1++;
		c2 = *s2++;
		n--;
		if(c1 != c2) {
			if(c1 > c2)
				return 1;
			return -1;
		}
		if(c1 == 0)
			break;
	}
	return 0;
}

rune16_t * rune16_strncpy(rune16_t *s1, rune16_t *s2, long n)
{
	int i;
	rune16_t *os1;

	os1 = s1;
	for(i = 0; i < n; i++)
	{
		if((*s1++ = *s2++) == 0)
		{
			while(++i < n)
				*s1++ = 0;
			return os1;
		}
	}
	return os1;
}

rune16_t * rune16_strrchr(rune16_t *s, rune16_t c)
{
	rune16_t *r;

	if(c == 0)
		return rune16_strchr(s, 0);
	r = 0;
	while(s = rune16_strchr(s, c))
		r = s++;
	return r;
}

rune16_t * rune16_strstr(rune16_t *s1, rune16_t *s2)
{
	rune16_t *p, *pa, *pb;
	int c0, c;

	c0 = *s2;
	if(c0 == 0)
		return s1;
	s2++;
	for(p=rune16_strchr(s1, c0); p; p=rune16_strchr(p+1, c0))
	{
		pa = p;
		for(pb=s2;; pb++)
		{
			c = *pb;
			if(c == 0)
				return p;
			if(c != *++pa)
				break;
		}
	}
	return 0;
}

#if PROTODUINO_UNICODE_LEVEL == 0

/* =========================================================================
 * ASCII-only tier – no flash tables, pure computation (~4.5 KB smaller).
 * ========================================================================= */

rune16_t rune16_tolower(const rune16_t c)
{
	return (c >= 'A' && c <= 'Z') ? (rune16_t)(c + 32u) : c;
}

rune16_t rune16_toupper(const rune16_t c)
{
	return (c >= 'a' && c <= 'z') ? (rune16_t)(c - 32u) : c;
}

rune16_t rune16_totitle(const rune16_t c)
{
	return rune16_toupper(c);
}

bool rune16_islower(const rune16_t c)
{
	return c >= 'a' && c <= 'z';
}

bool rune16_isupper(const rune16_t c)
{
	return c >= 'A' && c <= 'Z';
}

bool rune16_isalpha(const rune16_t c)
{
	return rune16_isupper(c) || rune16_islower(c);
}

bool rune16_istitle(const rune16_t c)
{
	(void)c;
	return false;   /* titlecase digraphs do not exist in ASCII */
}

bool rune16_isspace(const rune16_t c)
{
	return c == ' ' || (c >= 0x09u && c <= 0x0Du);   /* SP TAB LF VT FF CR */
}

#else /* PROTODUINO_UNICODE_LEVEL >= 1 : full BMP tables */

/*
 * All table lookups go through this binary search.  Tables are arrays of
 * `ne`-column records sorted ascending by their first column; the search
 * returns the last record whose first column is <= c (or NULL), and the
 * caller checks the record's upper bound / exact match.
 *
 * Every table element is read through _R16() — see the PROGMEM note above.
 */
static const rune16_t * rune16_bsearch(const rune16_t c, const rune16_t *t, int n, const int ne)
{
	const rune16_t *p;
	int m;

	while(n > 1)
	{
		m = n/2;
		p = t + m*ne;
		if(c >= _R16(p))
		{
			t = p;
			n = n-m;
		} else
			n = m;
	}
	if(n && c >= _R16(t))
		return t;
	return 0;
}

/*
 * Stride-2 range test: record (lo, hi, excess) matches c when
 * lo <= c <= hi and c has the same parity as lo.  These tables compress
 * the long alternating upper/lower singlet runs of Latin Extended,
 * Cyrillic, Greek and Latin Extended Additional (~2 KB of flash saved).
 */
static bool rune16_instride2(const rune16_t c, const rune16_t *p)
{
	return p && c >= _R16(p) && c <= _R16(p+1)
	         && (((c - _R16(p)) & 1u) == 0u);
}

rune16_t rune16_tolower(const rune16_t c)
{
	const rune16_t *p;

	p = rune16_bsearch(c, __tolower2, CC_NELEM(__tolower2)/3, 3);
	if(p && c >= _R16(p) && c <= _R16(p+1))
		return (rune16_t)(c + _R16(p+2) - 500u);
	p = rune16_bsearch(c, __tolower_s2, CC_NELEM(__tolower_s2)/3, 3);
	if(rune16_instride2(c, p))
		return (rune16_t)(c + _R16(p+2) - 500u);
	p = rune16_bsearch(c, __tolower1, CC_NELEM(__tolower1)/2, 2);
	if(p && c == _R16(p))
		return (rune16_t)(c + _R16(p+1) - 500u);
	return c;
}

rune16_t rune16_toupper(const rune16_t c)
{
	const rune16_t *p;

	p = rune16_bsearch(c, __toupper2, CC_NELEM(__toupper2)/3, 3);
	if(p && c >= _R16(p) && c <= _R16(p+1))
		return (rune16_t)(c + _R16(p+2) - 500u);
	p = rune16_bsearch(c, __toupper_s2, CC_NELEM(__toupper_s2)/3, 3);
	if(rune16_instride2(c, p))
		return (rune16_t)(c + _R16(p+2) - 500u);
	p = rune16_bsearch(c, __toupper1, CC_NELEM(__toupper1)/2, 2);
	if(p && c == _R16(p))
		return (rune16_t)(c + _R16(p+1) - 500u);
	return c;
}

rune16_t rune16_totitle(const rune16_t c)
{
	const rune16_t *p;

	p = rune16_bsearch(c, __totitle1, CC_NELEM(__totitle1)/2, 2);
	if(p && c == _R16(p))
		return (rune16_t)(c + _R16(p+1) - 500u);
	return c;
}

bool rune16_islower(const rune16_t c)
{
	const rune16_t *p;

	p = rune16_bsearch(c, __toupper2, CC_NELEM(__toupper2)/3, 3);
	if(p && c >= _R16(p) && c <= _R16(p+1))
		return true;
	p = rune16_bsearch(c, __toupper_s2, CC_NELEM(__toupper_s2)/3, 3);
	if(rune16_instride2(c, p))
		return true;
	p = rune16_bsearch(c, __toupper1, CC_NELEM(__toupper1)/2, 2);
	if(p && c == _R16(p))
		return true;
	return false;
}

bool rune16_isupper(const rune16_t c)
{
	const rune16_t *p;

	p = rune16_bsearch(c, __tolower2, CC_NELEM(__tolower2)/3, 3);
	if(p && c >= _R16(p) && c <= _R16(p+1))
		return true;
	p = rune16_bsearch(c, __tolower_s2, CC_NELEM(__tolower_s2)/3, 3);
	if(rune16_instride2(c, p))
		return true;
	p = rune16_bsearch(c, __tolower1, CC_NELEM(__tolower1)/2, 2);
	if(p && c == _R16(p))
		return true;
	return false;
}

bool rune16_isalpha(const rune16_t c)
{
	const rune16_t *p;

	if(rune16_isupper(c) || rune16_islower(c))
		return true;
	p = rune16_bsearch(c, __alpha2, CC_NELEM(__alpha2)/2, 2);
	if(p && c >= _R16(p) && c <= _R16(p+1))
		return true;
	p = rune16_bsearch(c, __alpha1, CC_NELEM(__alpha1), 1);
	if(p && c == _R16(p))
		return true;
	return false;
}

bool rune16_istitle(const rune16_t c)
{
	return rune16_isupper(c) && rune16_islower(c);
}

bool rune16_isspace(const rune16_t c)
{
	const rune16_t *p;

	p = rune16_bsearch(c, __space2, CC_NELEM(__space2)/2, 2);
	if(p && c >= _R16(p) && c <= _R16(p+1))
		return true;
	return false;
}

#endif /* PROTODUINO_UNICODE_LEVEL */

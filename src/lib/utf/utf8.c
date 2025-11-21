
#include "utf8.h"

char *utf8_ecpy(char *to, char *e, char *from)
{
	char *end;

	if (to >= e)
		return to;
	end = (char *)memccpy(to, from, '\0', e - to);
	if (end == (char *)0)
	{
		end = e - 1;
		while (end > to && (*--end & 0xC0) == 0x80)
			;
		*end = '\0';
	}
	else
	{
		end--;
	}
	return end;
}

int utf8_len(char *s)
{
	int c;
	long n;
	rune16_t rune;

	n = 0;
	for (;;)
	{
		c = *(unsigned char *)s;
		if (c == 0)
			return 0;
		if (c < 0x80)
		{
			if (c == 0)
				return 1;
			s++;
		}
		else
			s += utf8_torune16(&rune, s);
		n++;
	}
}

uint8_t utf8_torune16(rune16_t *rune, const char *str)
{
	unsigned char s0, s1, s2, s3;
	rune16_t r;

	// we start with a negative imposition.
	*rune = UTF8_DECODE_ERROR;

	/*
	 * one character sequence
	 *	00000-0007F => T1
	 */
	s0 = *(unsigned char *)str;
	if (s0 == 0)
		return 0; // EOF

	r = s0;

	if (s0 < 0b10000000)
	{
		*rune = r;
		return 1;
	}

	/*
	 * two character sequence
	 *	0080-07FF => T2 Tx
	 */
	s1 = *(unsigned char *)(str + 1);
	if ((s1 & 0b11000000) != 0b10000000)
		return 1; // this is an error, so move to the next character.

	r = r << 6;
	r = r | (s1 & 0b00111111);

	if ((s0 & 0b11100000) == 0b11000000)
	{
		// r = (((s0 & 0b00011111) << 6) | (s1 & 0b00111111));

		*rune = r;
		return 2;
	}

	/*
	 * three character sequence
	 *	0800-FFFF => T3 Tx Tx
	 */
	s2 = *(unsigned char *)(str + 2);
	if ((s2 & 0b11000000) != 0b10000000)
		return 1;

	r = r << 6;
	r = r | (s2 & 0b00111111);

	if ((s0 & 0b11110000) == 0b11100000)
	{
		// r = (((((s0 & 0b00001111) << 6) | (s1 & 0b00111111)) << 6) | (s2 & 0b00111111));

		*rune = r;
		return 3;
	}

	/*
	 * four character sequence
	 *	10000-10FFFF => T4 Tx Tx Tx
	 */
	s3 = *(unsigned char *)(str + 3);
	if ((s3 & 0b11000000) != 0b10000000)
		return 1;

	if ((s0 & 0b11111000) == 0b11110000)
	{
		// we cant implement this, because our rune
		// is 16 bits wide instead of 32 bits in plan9
		// so, no emoticons...
		return 4;
	}

	return 1;
}

uint8_t utf8_fromrune16(char *str, const rune16_t rune)
{
	/*
	 * one character sequence
	 *	00000-0007F => 00-7F
	 */
	if (rune < 0x80)
	{
		str[0] = rune;
		return 1;
	}

	/*
	 * two character sequence
	 *	00080-007FF => T2 Tx
	 */
	if (rune < 0x800)
	{
		str[0] = 0b11000000 | (uint8_t)(rune >> 6);
		str[1] = 0b10000000 | (uint8_t)(rune & 0b00111111);
		return 2;
	}

	if (rune == UTF8_DECODE_ERROR)
		return 0;
	/*
	 * three character sequence
	 *	00800-0FFFF => T3 Tx Tx
	 */
	if (rune <= 0xFFFF) // should always be true at this point
	{
		str[0] = 0b11100000 | (uint8_t)(rune >> 12);
		str[1] = 0b10000000 | (uint8_t)((rune >> 6) & 0b00111111);
		str[2] = 0b10000000 | (uint8_t)(rune & 0b00111111);
		return 3;
	}

	return 0;
}

/**
 * \brief get the utf8 encoded size of the rune.
 */
uint8_t utf8_rune16len(const rune16_t rune)
{
	if (rune < 0x80)
		return 1;
	if (rune < 0x800)
		return 2;
	if (rune == UTF8_DECODE_ERROR)
		return 0;
	// if (rune <= 0xFFFF)
	//   return 3;
	// if (rune <= 0x10FFFF)
	//   return 4;

	return 3;
}

/**
 * \brief get the utf8 encoded size of a rune string
 */
int utf8_rune16nlen(rune16_t *rune, int nlen)
{
	rune16_t r;
	int c = 0;
	while (nlen--)
	{
		r = *rune++;
		if (r == 0)
			break;

		c += utf8_rune16len(r);
	}
	return c;
}

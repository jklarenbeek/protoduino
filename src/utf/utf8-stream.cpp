
#include "utf8-stream.h"


int8_t utf8_getr(Stream *st, rune16_t *rune)
{
  uint8_t s0, s1, s2, s3;
  rune16_t r;
  int size;

  *rune = 0;

  size = st->available();
  if (size == 0)
    return 0;

  *rune = UTF8_DECODE_ERROR;

  /*
	 * one character sequence
	 *	00000-0007F => T1 
   *  0b0xxxxxxx
	 */    
  s0 = (uint8_t)st->peek();
  if (s0 < 0b10000000) 
  {
    s0 = (uint8_t)st->read();
    *rune = s0;
    return 1;
  }
  
  /*
	 * two character sequence
	 *	0080-07FF => T2 Tx
   *  0b110xxxxx 	0b10xxxxxx
	 */
  if ((s0 & 0b11100000) == 0b11000000)
  {
    if (size < 2)
      return 0;

    s0 = (uint8_t)st->read();
    s1 = (uint8_t)st->read();
    if ((s1 & 0b11000000) != 0b10000000)
      return UTF8_RET_CORRUPT;

    r = (((s0 & 0b00011111) << 6) | (s1 & 0b00111111));
    
    *rune = r;
    return 2;
  }

  /*
   * three character sequence
   *	0800-FFFF => T3 Tx Tx
   *  0b1110xxxx 	0b10xxxxxx 	0b10xxxxxx
   */
  if((s0 & 0b11110000) == 0b11100000)
  {
    if (size < 3)
      return 0;

    s0 = (uint8_t)st->read();
    s1 = (uint8_t)st->read();
    if ((s1 & 0b11000000) != 0b10000000)
      return UTF8_RET_CORRUPT;
    s2 = (uint8_t)st->read();
    if ((s2 & 0b11000000) != 0b10000000)
      return UTF8_RET_CORRUPT;

    r = (((((s0 & 0b00001111) << 6) | (s1 & 0b00111111)) << 6) | (s2 & 0b00111111));
    
    *rune = r;
    return 3;
  }

	/*
	 * four character sequence
	 *	10000-10FFFF => T4 Tx Tx Tx
   *  0b11110xxx 	0b10xxxxxx 	0b10xxxxxx 	0b10xxxxxx
	 */
  if ((s0 & 0b11111000) == 0b11110000)
  {
    // we cant implement this, because our rune is 16 bits wide.
    // we take it from the stream anyways.

    if (size < 4)
      return 0;

    s0 = (uint8_t)st->read();
    s1 = (uint8_t)st->read();
    if ((s1 & 0b11000000) != 0b10000000)
      return UTF8_RET_CORRUPT;
    s2 = (uint8_t)st->read();
    if ((s2 & 0b11000000) != 0b10000000)
      return UTF8_RET_CORRUPT;
    s3 = (uint8_t)st->read();
    if ((s3 & 0b11000000) != 0b10000000)
      return UTF8_RET_CORRUPT;

    return UTF8_RET_OVERFLOW;

  }

  return UTF8_RET_INCOMPLETE;
}

int8_t utf8_putr(Stream *st, const rune16_t rune)
{
  int size = st->availableForWrite();
	
  /*
	 * one character sequence
	 *	00000-0007F => 00-7F
	 */
	if (rune < 0x80) {
    if (size < 1)
      return 0;

		st->write((uint8_t)rune);
		return 1;
	}

	/*
	 * two character sequence
	 *	00080-007FF => T2 Tx
	 */
	if(rune < 0x800) {
    if (size < 2)
      return 0;

		st->write(0b11000000 | (uint8_t)(rune >> 6));
		st->write(0b10000000 | (uint8_t)(rune & 0b00111111));
		return 2;
	}

	/*
	 * three character sequence
	 *	00800-0FFFF => T3 Tx Tx
	 */
	if(rune <= 0xFFFF) { // should always be true at this point
    if (size < 3)
      return 0;

		st->write(0b11100000 | (uint8_t)(rune >> 12));
		st->write(0b10000000 | (uint8_t)((rune >> 6) & 0b00111111));
		st->write(0b10000000 | (uint8_t)(rune & 0b00111111));
		return 3;
	}

  return 0;
}

int utf8_puts(Stream *st, const rune16_t * str)
{
  int ret = 0;
  while (*str != '\0')
  {
      if (utf8_putr(st, *str++) == 0)
        break;

      ret++;
      str++;
  }
  return ret;
}

int utf8_puts_P(Stream *st, const rune16_t * str)
{
  rune16_t ch;
  int ret = 0;
  while ((ch = pgm_read_word(str)) != '\0')
  {
      if (utf8_putr(st, ch) == 0)
        break;

      str++;
      ret++;
  }
  return ret;
}

int utf8_puti(Stream *st, uint8_t value)
{
  return st->print(value);
}

// file: ./src/sys/pt/types.h
#ifndef __PT_TYPES_H__
#define __PT_TYPES_H__

#include "lc.h"
#include "../../utf/rune16.h"

struct value_pt
{
    lc_t lc;
    uint8_t value; // T value
}

struct rune16_pt
{
    lc_t lc;
    rune16_t value;
}

struct buf8_pt
{
    lc_t lc;
    uint8_t idx;       // current index of buffer
    uint8_t buf[idx];  // current escape buffer
}

struct stream8_pt
{
  lc_t lc;
  Stream * stream;              // stream for getc and putc
}

#endif /* __PT_TYPES_H__ */

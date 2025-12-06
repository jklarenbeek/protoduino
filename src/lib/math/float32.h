#ifndef _FLOAT32_H_
#define _FLOAT32_H_

#include <cc.h>
#include <stdbool.h>

/**
 * Fast inverse square-root
 *
 * @see: http://en.wikipedia.org/wiki/Fast_inverse_square_root
*/
CC_ALWAYS_INLINE float32_t mathf32_rsqrt(float32_t x)
{
	union { float32_t f; uint32_t l; } t;
	t.f = x;
	t.l = 0x5f3759df - (t.l >> 1);
    return t.f * (1.5f - ((0.5f * x) * t.f * t.f));
}

CC_ALWAYS_INLINE float fast_log2(float x) {
  union { float f; uint32_t i; } u = { x };
  uint32_t xi = u.i;
  int32_t exp = (int32_t)((xi >> 23) & 0xFF) - 127;      // extract exponent
  xi = (xi & 0x7FFFFF) | (127 << 23);                   // mantissa -> [1.0, 2.0)
  u.i = xi;
  float mant = u.f;                                     // 1.0 ≤ mant < 2.0

  // 5th-order minimax polynomial approximation for log2(mant)
  float p =       1.5374734f;
  p = p * mant  - 3.7547452f;
  p = p * mant  + 3.9488196f;
  p = p * mant  - 2.0430683f;
  p = p * mant  + 0.5362593f;
  p = p * mant  + 0.00015889f;   // tiny constant to improve accuracy

  return (float)exp + p;
}

CC_ALWAYS_INLINE very_fast_log2(float val) {
  union { float f; uint32_t i; } convert;
  convert.f = val;
  return (float)((convert.i >> 23) - 127);   // only integer part, error up to ~1
}

#endif

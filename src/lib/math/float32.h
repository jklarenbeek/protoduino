
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
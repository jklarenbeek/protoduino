#ifndef __MATH_LERP_H__
#define __MATH_LERP_H__

#include <Arduino.h>
#include <protoduino.h>

/*********************************/
/* Linear Interpolation Routines */
/*********************************/

static CC_INLINE int lerp_int(int a, int b, float f) {
    //float diff = (float)(b-a);
    //float frac = f*diff;
    //return a + (int)frac;
    return a + (int)(f * (float)(b-a));
}

static CC_INLINE  uint8_t lerp_uint8(uint8_t a, uint8_t b, uint8_t t) {
    return a + (abs((int16_t)t*(int16_t)(b-a)) >> 8);
}

static CC_INLINE  uint16_t lerp_uint16(uint16_t a, uint16_t b, uint8_t t) {
    return a + (abs((int32_t)t*(int32_t)(b-a)) >> 8);
}

#endif
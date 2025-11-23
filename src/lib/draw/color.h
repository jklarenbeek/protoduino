#ifndef __COLOR_H__
#define __COLOR_H__

#include <Arduino.h>
#include <protoduino.h>

#define BIGHUE_LENGTH 1536
#define BIGHUE_SET(x, l) map(x, 0, BIGHUE_LENGTH-1, 0, l)

// Sometimes chipsets wire in a backwards sort of way
struct rgb { uint8_t r; uint8_t g; uint8_t b; };
struct hsv { uint8_t h; uint8_t s; uint8_t v; };

static void hsv2rgb(struct rgb * left, struct hsv * right, int16_t bighue) {

  static uint16_t hsv2rgb_h = 0;
  static uint8_t hsv2rgb_lo = 0;
  static uint16_t hsv2rgb_s1 = 0;
  static uint16_t hsv2rgb_v1 = 0;

  if (right->s == 0) {
    left->r = left->g = left->b = right->v;
  }
  else {
    if (bighue>=0) {
      hsv2rgb_h = (uint16_t)(bighue & 2047);
    }
    else {
      // multiply by six to get a sextant
      hsv2rgb_h = ((uint16_t)right->h)+1;
      hsv2rgb_h = (hsv2rgb_h << 2) + (hsv2rgb_h << 1);
      hsv2rgb_h--;
    }
    // hue is calculated according to a circle of 360 degrees divided in 6 parts
    // use low part of sextant as wheel color mix
    hsv2rgb_lo = (uint8_t)((hsv2rgb_h)&255);
    switch(hsv2rgb_h>>8) {
      default:
      case 0 : // R to Y
        left->r = 255;              left->g = hsv2rgb_lo;       left->b = 0; break;
      case 1 : // Y to G
        left->r = 255 - hsv2rgb_lo; left->g = 255;              left->b = 0; break;
      case 2 : // G to C
        left->r =   0;              left->g = 255;              left->b =  hsv2rgb_lo; break;
      case 3 : // C to B
        left->r =  0;               left->g = 255 - hsv2rgb_lo; left->b = 255; break;
      case 4 : // B to M
        left->r =  hsv2rgb_lo;      left->g =   0;              left->b = 255; break;
      case 5 : // M to R
        left->r = 255;              left->g =   0;              left->b = 255 - hsv2rgb_lo; break;
    }

    // Saturation: add 1 so range is 1 to 256, allowig a quick shift operation
    // on the result rather than a costly divide, while the type upgrade to int
    // avoids repeated type conversions in both directions.
    hsv2rgb_s1 = right->s + 1;
    left->r = 255 - ((hsv2rgb_s1 * (255 - left->r)) >> 8);
    left->g = 255 - ((hsv2rgb_s1 * (255 - left->g)) >> 8);
    left->b = 255 - ((hsv2rgb_s1 * (255 - left->b)) >> 8);

    // Value (brightness) and 24-bit color concat merged: similar to above, add
    // 1 to allow shifts, and upgrade to long makes other conversions implicit.
    hsv2rgb_v1 = right->v + 1;
    left->r = (hsv2rgb_v1 * left->r) >> 8;
    left->g = (hsv2rgb_v1 * left->g) >> 8;
    left->b = (hsv2rgb_v1 * left->b) >> 8;
  }
}

// GLOBAL VARIABLES --------------------------------------------------------
// from: https://learn.adafruit.com/sipping-power-with-neopixels/demo-code

// This bizarre construct isn't Arduino code in the conventional sense.
// It exploits features of GCC's preprocessor to generate a CC_PROGMEM
// table (in flash memory) holding an 8-bit unsigned sine wave (0-255).
const int _SBASE_ = __COUNTER__ + 1; // Index of 1st __COUNTER__ ref below
#define _S1_ (sin((__COUNTER__ - _SBASE_) / 128.0 * M_PI) + 1.0) * 127.5 + 0.5,
#define _S2_ _S1_ _S1_ _S1_ _S1_ _S1_ _S1_ _S1_ _S1_ // Expands to 8 items
#define _S3_ _S2_ _S2_ _S2_ _S2_ _S2_ _S2_ _S2_ _S2_ // Expands to 64 items
CC_CONST_PTYPE_ARRAY(uint8_t, sineTable) = { _S3_ _S3_ _S3_ _S3_ }; // 256 items

// Similar to above, but for an 8-bit gamma-correction table.
#define _GAMMA_ 2.6
const int _GBASE_ = __COUNTER__ + 1; // Index of 1st __COUNTER__ ref below
#define _G1_ pow((__COUNTER__ - _GBASE_) / 255.0, _GAMMA_) * 255.0 + 0.5,
#define _G2_ _G1_ _G1_ _G1_ _G1_ _G1_ _G1_ _G1_ _G1_ // Expands to 8 items
#define _G3_ _G2_ _G2_ _G2_ _G2_ _G2_ _G2_ _G2_ _G2_ // Expands to 64 items
CC_CONST_PTYPE_ARRAY(uint8_t, gammaTable) = { _G3_ _G3_ _G3_ _G3_ }; // 256 items

static void rgbgc(struct rgb * left)
{
    left->r = gammacorrect(left->r);
    left->g = gammacorrect(left->g);
    left->b = gammacorrect(left->b);
}

CC_INLINE uint8_t gammacorrect(const uint8_t color)
{
  return pgm_read_byte(&gammaTable[color]);
}

#endif
#ifndef __DBG_ERRORS_H__
#define __DBG_ERRORS_H__

/*
 * ONTOLOGICAL 8-BIT ERROR TAXONOMY v0.2
 * -----------------------------------------------------------------------------
 * Geometry:  16x16 Matrix (0..255) mapped to 4 Attractor Basins.
 * Logic:     Convergence via ((Child << 1) & 0xF0) | ((Child >>> 1) & 0x0F)
 * -----------------------------------------------------------------------------
 */
#include <protoduino.h>

/* =========================================================================
   HELPER MACROS FOR ANALYSIS
   ========================================================================= */

#define ERR_IS_RESERVED(err) \
    ((err) == ERR_SUCCESS || (err) == ERR_YIELDING || (err) == ERR_EXITING || (err) == ERR_ENDING || (err) == ERR_FINALIZED)

// Terminal roots: pure endpoints without internal structure
#define ERR_IS_ABSTRACT(err) \
    ((err) == ERR_ROOT_INIT || (err) == ERR_ROOT_RUN)

// Oscillatory roots: structured alternating primitives
#define ERR_IS_MOVEMENT(err) \
    ((err) == ERR_ROOT_BEFORE || (err) == ERR_ROOT_AFTER)

// 8-bit Plane: Split into two 4-bit nibbles
#define ERR_NIBBLE_LEFT(err) (((err) >> 4) & 0x0F)
#define ERR_NIBBLE_RIGHT(err) ((err) & 0x0F)

// Checks if the Left Nibble is equal to the Inverse of the Right Nibble
#define ERR_IS_SHADOW(err) \
    (ERR_NIBBLE_LEFT(err) == (~ERR_NIBBLE_RIGHT(err) & 0xF))

// The first half of a cluster is 4bits and must equal the other 4bits
// e.g. 0x00, 0x11, 0x22 ... 0xFF
#define ERR_IS_TWIN(err) (ERR_NIBBLE_LEFT(err) == ERR_NIBBLE_RIGHT(err))

#define ERR_IS_MIRROR(err) (err_op_reverse(err) == ((err) & 0xFF))

/**
 *  HIERARCHY FUNCTIONS
 *
 */

// The 4 primordial error classes of which every other error are its children.
// 0 (00000000), 255 (11111111), 85 (01010101), 170 (10101010)
#define ERR_IS_ROOT(err) \
    (ERR_IS_ABSTRACT(err) || ERR_IS_MOVEMENT(err))

// Direct 12 descendants of the primordial error classes
#define ERR_IS_DOMAIN(err) (!ERR_IS_ROOT(err) && (ERR_IS_ROOT(err_op_center(err))))

// Each domain has 4 sections and 12 per root
#define ERR_IS_SECTION(err) (err_op_depth(err) == 2)

// The direct descendants of the DOMAIN error codes
#define ERR_IS_LEAF(err) (err_op_depth(err) == 3)

/**
 * SYMMETRY CLASSES
 * 8-bit Matrix of 16x16 byte error codes
 */

// Balanced means half of the bits are set (4 out of 8)
#define ERR_IS_BALANCED(err) (err_op_one_count(err) == 4)
#define ERR_IS_UNBALANCED(err) (err_op_one_count(err) != 4)

// A balanced diagonal from right top to left bottom
// results in error codes 0x55 (85) and 0xAA (170) => ERR_IS_MOVEMENT(err)
#define ERR_IS_BALANCED_ROOT(err) (err_op_center(err) == err_op_inverse(err))

// A balanced diagonal from right top to left bottom
#define ERR_IS_BALANCED_SHADOW(err) (ERR_IS_BALANCED(err) && ERR_IS_SHADOW(err))

// A balanced circle in the middle of the 16x16 matrix
#define ERR_IS_BALANCED_EDGE(err) (!ERR_IS_BALANCED_ROOT(err) && ERR_IS_BALANCED(err) && !ERR_IS_SHADOW(err))

// An unbalanced diagonal from left top to right bottom
// Only 0x00 and 0xFF
#define ERR_IS_UNBALANCED_ROOT(err) (ERR_IS_ABSTRACT(err))

// Diagonal from left top to right bottom (twin but not 00 or FF)
#define ERR_IS_UNBALANCED_TWIN(err) ((!ERR_IS_ABSTRACT(err)) && ERR_IS_UNBALANCED(err) && ERR_IS_TWIN(err))

// Extreme imbalance (1 or 7 one's). Outer edge cirlce of the 16x16 matrix.
#define ERR_IS_UNBALANCED_EDGE(err) (err_op_one_count(err) == 1 || err_op_one_count(err) == 7)

// Everything else unbalanced
#define ERR_IS_UNBALANCED_OTHER(err) (ERR_IS_UNBALANCED(err) && !ERR_IS_TWIN(err) && !ERR_IS_UNBALANCED_EDGE(err))

static CC_ALWAYS_INLINE uint8_t err_op_inverse(uint8_t err) {
    // Inverse the cluster (EEEE DDDD)
    // In 8-bit, we invert the whole byte.
    return ~err;
}

static CC_ALWAYS_INLINE uint8_t err_op_reverse(uint8_t err) {
    // Reverse the cluster bits: 7-0
    // Standard SWAR bit reversal for 8 bits
    err = (err & 0xF0) >> 4 | (err & 0x0F) << 4;
    err = (err & 0xCC) >> 2 | (err & 0x33) << 2;
    err = (err & 0xAA) >> 1 | (err & 0x55) << 1;
    return err;
}

static CC_ALWAYS_INLINE uint8_t err_op_opposite(uint8_t err) {
    // Swap the nibbles (Left <-> Right)
    // (EEEE DDDD) -> (DDDD EEEE)
    return (ERR_NIBBLE_RIGHT(err) << 4) | ERR_NIBBLE_LEFT(err);
}

static CC_ALWAYS_INLINE uint8_t err_op_center(uint8_t err) {
    // Nuclear transformation for 8 bits.
    // Logic: Sliding window of size 4.
    // Right Nibble becomes bits 1,2,3,4 (shifted to 0,1,2,3)
    // Left Nibble becomes bits 3,4,5,6 (shifted to 4,5,6,7)
    // ((err >> 1) & 0x0F) extracts 1,2,3,4 -> 0,1,2,3
    // ((err << 1) & 0xF0) extracts 3,4,5,6 -> 4,5,6,7
    return ((err << 1) & 0xF0) | ((err >> 1) & 0x0F);
}

static CC_ALWAYS_INLINE uint8_t err_op_root(uint8_t err) {
    // Convergence tree iteration
    // Typically depth increases with bit width, 8-bit takes 4 steps max.
    while (!ERR_IS_ROOT(err)) err = err_op_center(err);
    return err;
}

/**
 * Get the depth in the nuclear convergence tree.
 * Returns how many center() operations needed to reach the root.
 */
static CC_ALWAYS_INLINE uint8_t err_op_depth(uint8_t err) {
  uint8_t d = 0;
  while (!ERR_IS_ROOT(err)) {
      err = err_op_center(err);
      d++;
  }
  return d;
}

static CC_ALWAYS_INLINE uint8_t err_op_one_count(uint8_t err) {
    // Hamming weight for 8 bits
    // This is a SWAR algorithm (SIMD Within A Register) for 32-bit capable CPUs,
    // adapted for 8-bit flow, or simply naive count if compiled without __builtin_popcount
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(err);
#else
    err = (err & 0x55) + ((err >> 1) & 0x55);
    err = (err & 0x33) + ((err >> 2) & 0x33);
    return (err + (err >> 4)) & 0x0F;
#endif
}

static CC_ALWAYS_INLINE uint8_t err_op_distance(uint8_t left, uint8_t right) {
    return err_op_one_count(left ^ right);
}

static CC_ALWAYS_INLINE very_fast_log2(float val) {
  union { float f; uint32_t i; } convert;
  convert.f = val;
  return (float)((convert.i >> 23) - 127);   // only integer part, error up to ~1
}

/*
 * Calculate entropy (Shannon information) of nibbles.
 * Measures balance/chaos: 0 = pure, 1 = perfectly balanced.
 *
 * Formula based on N=8 lines.
 */
static CC_ALWAYS_INLINE float err_op_entropy(uint8_t err) {
  uint8_t ones = err_op_one_count(err);
  uint8_t zeros = 8 - ones;

  if (zeros == 0 || ones == 0) return 0.0f;

  float p_ones = ones / 8.0f;
  float p_zeros = zeros / 8.0f;

  return -(p_ones * very_fast_log2(p_ones) + p_zeros * very_fast_log2(p_zeros));
}

/**
 * Calculate balance ratio (one/total lines).
 * Returns value from 0.0 to 1.0.
 */
static CC_ALWAYS_INLINE float err_op_balance(uint8_t err) {
  return err_op_one_count(err) / 8.0f;
}

// Identifies which transformation relates two errors
typedef enum {
    ERR_RELATION_DEFAULT = 0,
    ERR_RELATION_CENTER,
    ERR_RELATION_OPPOSITE,
    ERR_RELATION_REVERSED_EQUALS,
    ERR_RELATION_REVERSED,
    ERR_RELATION_INVERTED_EQUALS,
    ERR_RELATION_INVERTED,
} err_relation_t;

static err_relation_t err_op_relation(uint8_t left, uint8_t right) {
  if (err_op_center(left) == right)
    return ERR_RELATION_CENTER;
  else if (err_op_opposite(left) == right)
    return ERR_RELATION_OPPOSITE;
  else if (err_op_reverse(left) == right) {
    if (ERR_IS_TWIN(left))
      return ERR_RELATION_REVERSED_EQUALS;
    else
      return ERR_RELATION_REVERSED;
  }
  else if (err_op_inverse(left) == right) {
    if (ERR_IS_TWIN(left))
      return ERR_RELATION_INVERTED_EQUALS;
    else
      return ERR_RELATION_INVERTED;
  }
  else return ERR_RELATION_DEFAULT;
}


CC_EXTERN const char *error_to_string(uint8_t err);

#endif // __DBG_ERRORS_H__
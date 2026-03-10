/**
 * @file rune32.h
 * @brief 32-bit Unicode code-point type and emoji / SMP classification.
 *
 * Relationship to rune16.h
 * ========================
 * rune16_t covers U+0000–U+FFFF (the Basic Multilingual Plane, BMP).
 * rune32_t covers U+0000–U+10FFFF (all of Unicode, including emoji).
 *
 * For memory-constrained targets (Arduino Uno / AVR):
 *  - The lookup tables live in flash (CC_PROGMEM / PROGMEM).
 *  - SRAM cost: 0 bytes from this header.
 *  - Flash cost: ~200 bytes for the emoji tables.
 *
 * Classification strategy
 * =======================
 * For code points ≤ U+FFFF (BMP): delegate to the existing rune16_*
 * functions.  This avoids duplicating the large BMP case tables.
 *
 * For code points > U+FFFF (SMP): only emoji / symbol classification
 * is provided — alphabets in the SMP (Linear B, Gothic, etc.) are not
 * needed on typical microcontroller targets.  If you need full SMP
 * alpha tables, extend rune32_private.h.
 *
 * Special code points
 * ===================
 *  U+200D  Zero Width Joiner   – used in multi-person emoji sequences.
 *  U+FE0F  Variation Selector-16 – requests the emoji presentation of a
 *           BMP symbol (e.g. ☀ + VS16 = ☀️).
 *  U+D800–U+DFFF  Surrogate range – never valid in UTF-8; always an error.
 *
 * Author: Joham https://github.com/jklarenbeek
 */

#ifndef __RUNE32_H__
#define __RUNE32_H__

#include <cc.h>
#include <stdbool.h>
#include <stdint.h>
#include "rune16.h"

/* =========================================================================
 * Type and constants
 * ========================================================================= */

/**
 * @typedef rune32_t
 * @brief 32-bit Unicode code-point.
 *
 * Covers all of Unicode (U+0000–U+10FFFF) including emoji (U+1F300+).
 * The upper 11 bits (above bit 20) are always 0 for valid code points.
 */
typedef uint32_t rune32_t;

/** Highest valid Unicode code-point. */
#define RUNE32_MAX          ((rune32_t)0x10FFFFu)

/** Error / replacement code-point (mirrors the rune16 U+FFFD sentinel). */
#define RUNE32_DECODE_ERROR ((rune32_t)0xFFFFFFFDu)

/** First surrogate code unit (inclusive) – invalid in UTF-8. */
#define RUNE32_SURR_MIN     ((rune32_t)0xD800u)

/** Last surrogate code unit (inclusive) – invalid in UTF-8. */
#define RUNE32_SURR_MAX     ((rune32_t)0xDFFFu)

/** Zero Width Joiner – joins emoji into sequences (e.g. 👨‍👩‍👧). */
#define RUNE32_ZWJ          ((rune32_t)0x200Du)

/** Variation Selector-16: requests emoji presentation of a BMP symbol. */
#define RUNE32_VS16         ((rune32_t)0xFE0Fu)

/* =========================================================================
 * Inline predicate helpers (zero flash cost)
 * ========================================================================= */

/**
 * @brief True if the code-point lies in the Basic Multilingual Plane
 *        (U+0000–U+FFFF).  BMP code points fit in a rune16_t.
 */
static inline bool rune32_isbmp(const rune32_t c)
{
    return c <= 0xFFFFu;
}

/**
 * @brief True if the code-point lies in the Supplementary planes
 *        (U+10000–U+10FFFF) — i.e. requires a 4-byte UTF-8 sequence.
 */
static inline bool rune32_issmp(const rune32_t c)
{
    return c > 0xFFFFu && c <= RUNE32_MAX;
}

/**
 * @brief True if the code-point is a valid Unicode scalar value.
 *
 * Rejects surrogates and values above U+10FFFF.
 */
static inline bool rune32_isvalid(const rune32_t c)
{
    return c <= RUNE32_MAX
        && !(c >= RUNE32_SURR_MIN && c <= RUNE32_SURR_MAX);
}

/* =========================================================================
 * Classification  –  declarations
 * ========================================================================= */

/**
 * @brief True if the code-point is a Unicode whitespace character.
 *
 * Delegates to rune16_isspace for BMP; SMP code points are never space.
 *
 * @param c Code point to test.
 */
CC_EXTERN bool rune32_isspace(rune32_t c);

/**
 * @brief True if the code-point is an alphabetic character.
 *
 * Delegates to rune16_isalpha for BMP; SMP code points return false
 * (emoji and symbols are not alphabetic for typical microcontroller use).
 */
CC_EXTERN bool rune32_isalpha(rune32_t c);

/**
 * @brief True if the code-point is an uppercase letter.
 *
 * Delegates to rune16_isupper for BMP; SMP always returns false.
 */
CC_EXTERN bool rune32_isupper(rune32_t c);

/**
 * @brief True if the code-point is a lowercase letter.
 *
 * Delegates to rune16_islower for BMP; SMP always returns false.
 */
CC_EXTERN bool rune32_islower(rune32_t c);

/**
 * @brief True if the code-point is a titlecase letter.
 *
 * Delegates to rune16_istitle for BMP; SMP always returns false.
 */
CC_EXTERN bool rune32_istitle(rune32_t c);

/**
 * @brief True if the code-point is an emoji or emoji-modifier character.
 *
 * Covers:
 *  - BMP emoji (☀ U+2600 – ➿ U+27BF, miscellaneous symbols, dingbats …)
 *  - SMP emoji (😀 U+1F600, 🎉 U+1F389, 🌍 U+1F30D, …)
 *  - Zero Width Joiner (U+200D) used in emoji sequences
 *  - Variation Selector-16 (U+FE0F)
 *
 * This function uses compact range tables stored in flash (PROGMEM on AVR).
 */
CC_EXTERN bool rune32_isemoji(rune32_t c);

/* =========================================================================
 * Case conversion
 * ========================================================================= */

/**
 * @brief Return the uppercase equivalent of a code-point.
 *
 * Delegates to rune16_tolower (sic – original rune16.c naming convention)
 * for BMP code-points.  SMP code-points have no case; returned unchanged.
 *
 * @param c Code point to convert.
 * @return Uppercase equivalent, or @p c if none exists.
 */
CC_EXTERN rune32_t rune32_toupper(rune32_t c);

/**
 * @brief Return the lowercase equivalent of a code-point.
 *
 * Delegates to rune16_toupper (sic) for BMP.  SMP returned unchanged.
 *
 * @param c Code point to convert.
 * @return Lowercase equivalent, or @p c if none exists.
 */
CC_EXTERN rune32_t rune32_tolower(rune32_t c);

/**
 * @brief Return the titlecase equivalent of a code-point.
 *
 * Delegates to rune16_totitle for BMP.  SMP returned unchanged.
 */
CC_EXTERN rune32_t rune32_totitle(rune32_t c);

#endif /* __RUNE32_H__ */
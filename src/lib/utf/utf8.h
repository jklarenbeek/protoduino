/**
 * @file utf8.h
 * @brief UTF-8 codec and string operations for memory-constrained systems.
 *
 * This module provides:
 *  - 16-bit rune encode / decode  (utf8_torune16, utf8_fromrune16)
 *  - 32-bit rune encode / decode  (utf8_torune32, utf8_fromrune32)
 *    – handles 4-byte emoji (😀 U+1F600, 🎉 U+1F389, …)
 *  - String-level operations      (utf8_strcmp, utf8_strchr, utf8_strstr …)
 *  - 32-bit string operations     (utf8_strcmp32, utf8_strchr32, …)
 *  - Case conversion              (utf8_toupper, utf8_tolower) into a builder
 *  - Validity checking            (utf8_valid) – correctly accepts emoji
 *  - Safe bounded copy            (utf8_ecpy  –  pre-existing, kept)
 *
 * For zero-allocation iteration and buffered writing see utf8_iter.h.
 * For 32-bit (emoji-capable) iteration see utf8_iter32.h.
 *
 * Authors: Joham https://github.com/jklarenbeek
 *          (some functions taken from https://github.com/9fans/plan9port)
 */

#ifndef __UTF8_H__
#define __UTF8_H__

#include <cc.h>
#include <stdbool.h>
#include <stdint.h>
#include "rune16.h"
#include "rune32.h"   /* rune32_t, RUNE32_DECODE_ERROR, rune32_isemoji, … */

/* -------------------------------------------------------------------------
 * Sentinel / status values
 * ---------------------------------------------------------------------- */

/** Rune16 error sentinel: U+FFFD replacement character. */
#define UTF8_DECODE_ERROR    ((rune16_t)0xFFFDu)

/** Rune32 error sentinel: mirrors U+FFFD but in 32-bit space. */
#define UTF8_DECODE_ERROR32  RUNE32_DECODE_ERROR

#define UTF8_RET_SUCCESS    0
#define UTF8_RET_CORRUPT   -1
#define UTF8_RET_OVERFLOW  -2
#define UTF8_RET_INCOMPLETE -3

/* =========================================================================
 * Codec  –  encode / decode a single rune
 * ========================================================================= */

/**
 * @brief Decode the first UTF-8 rune from `str`.
 *
 * The function is intentionally stateless and allocation-free: it reads
 * at most 4 bytes from `str` and writes one rune16_t.
 *
 * Encoding errors advance by 1 byte and set *rune = UTF8_DECODE_ERROR.
 * End-of-string (str[0] == 0) returns 0 without writing *rune.
 * 4-byte sequences (emoji, SMP) return 4 and set *rune = UTF8_DECODE_ERROR
 * because rune16_t cannot represent code-points above U+FFFF.
 *
 * @param rune  Output: decoded 16-bit rune.
 * @param str   Input:  null-terminated or bounded UTF-8 byte stream.
 * @return Number of bytes consumed from `str`; 0 means end-of-string.
 */
CC_EXTERN uint8_t utf8_torune16(rune16_t *rune, const char *str);

/**
 * @brief Encode a single rune16_t into the UTF-8 byte buffer `str`.
 *
 * The caller must ensure `str` has at least 3 bytes available
 * (maximum encoding for a 16-bit code-point).
 * The output is NOT null-terminated.
 *
 * @param str   Destination buffer (at least 3 bytes).
 * @param rune  Unicode code point to encode.
 * @return Number of bytes written (1, 2, or 3); 0 on error.
 */
CC_EXTERN uint8_t utf8_fromrune16(char *str, rune16_t rune);

/**
 * @brief UTF-8 encoded byte-length of a single rune (0, 1, 2, or 3).
 *
 * Returns 0 for UTF8_DECODE_ERROR and for runes that cannot be
 * represented in UTF-8 (none for 16-bit runes in practice).
 */
CC_EXTERN uint8_t utf8_rune16len(rune16_t rune);

/**
 * @brief Total UTF-8 byte-length for an array of rune16_t values.
 *
 * Stops at the first 0 rune or after `nlen` elements.
 *
 * @param rune  Pointer to rune16_t array.
 * @param nlen  Maximum number of elements to examine.
 * @return Total encoded byte count (excludes null terminator).
 */
CC_EXTERN int utf8_rune16nlen(rune16_t *rune, int nlen);

/* =========================================================================
 * Validity
 * ========================================================================= */

/**
 * @brief Test whether `str` is valid UTF-8.
 *
 * Returns true for the empty string.  Returns false on any encoding
 * error, including overlong sequences and surrogate code-points.
 *
 * @param str   Null-terminated byte string.
 * @return true if every byte sequence is a legal UTF-8 encoding.
 */
CC_EXTERN bool utf8_valid(const char *str);

/**
 * @brief Test validity of a bounded byte region.
 *
 * @param str  Start of region.
 * @param len  Region length in bytes.
 */
CC_EXTERN bool utf8_validn(const char *str, uint16_t len);

/* =========================================================================
 * String length
 * ========================================================================= */

/**
 * @brief Count the number of Unicode runes in a null-terminated UTF-8 string.
 *
 * This replaces the broken utf8_len() that always returned 0.
 *
 * @param s  Null-terminated UTF-8 string.
 * @return   Rune count (not byte count).
 */
CC_EXTERN int utf8_strlen(const char *s);

/* =========================================================================
 * Bounded safe copy
 * ========================================================================= */

/**
 * @brief Safe UTF-8 string copy that always null-terminates.
 *
 * Copies bytes from `from` into `to` stopping before `e`, then ensures
 * the result is null-terminated even if truncation occurred at a
 * multi-byte boundary.
 *
 * @param to    Destination buffer start.
 * @param e     One past end of destination buffer.
 * @param from  Source null-terminated UTF-8 string.
 * @return Pointer to the terminating null byte in `to`.
 */
CC_EXTERN char *utf8_ecpy(char *to, char *e, char *from);

/* =========================================================================
 * String comparison
 * ========================================================================= */

/**
 * @brief Compare two null-terminated UTF-8 strings rune by rune.
 *
 * Comparison is by Unicode code-point value, not byte value, so
 * the result matches rune16_strcmp() on the decoded sequences.
 *
 * @return Negative, zero, or positive (like strcmp).
 */
CC_EXTERN int utf8_strcmp(const char *s1, const char *s2);

/**
 * @brief Compare up to `n` runes from two UTF-8 strings.
 *
 * @param s1  First string.
 * @param s2  Second string.
 * @param n   Maximum number of *runes* to compare.
 * @return Negative, zero, or positive.
 */
CC_EXTERN int utf8_strncmp(const char *s1, const char *s2, uint16_t n);

/* =========================================================================
 * Search
 * ========================================================================= */

/**
 * @brief Find the first occurrence of rune `c` in a UTF-8 string.
 *
 * @param s  Null-terminated UTF-8 string.
 * @param c  Rune to search for.
 * @return Pointer to the first byte of the matching rune, or NULL.
 */
CC_EXTERN const char *utf8_strchr(const char *s, rune16_t c);

/**
 * @brief Find the last occurrence of rune `c` in a UTF-8 string.
 *
 * @param s  Null-terminated UTF-8 string.
 * @param c  Rune to search for.
 * @return Pointer to the first byte of the last match, or NULL.
 */
CC_EXTERN const char *utf8_strrchr(const char *s, rune16_t c);

/**
 * @brief Find the first occurrence of UTF-8 substring `needle` in `haystack`.
 *
 * @param haystack  Null-terminated UTF-8 string to search in.
 * @param needle    Null-terminated UTF-8 string to search for.
 * @return Pointer to the start of the first match, or NULL.
 */
CC_EXTERN const char *utf8_strstr(const char *haystack, const char *needle);

/**
 * @brief Scan forward past any runes for which `predicate` returns true.
 *
 * Returns a pointer to the first rune that does NOT satisfy the predicate,
 * or a pointer to the null terminator if all runes satisfy it.
 *
 * Useful for trimming whitespace / skipping alphanumerics without allocation.
 *
 * @param s          UTF-8 string.
 * @param predicate  Function taking a rune16_t, returning bool.
 * @return Pointer into `s` past the matching prefix.
 */
CC_EXTERN const char *utf8_scan_while(const char *s,
                                      bool (*predicate)(rune16_t));

/**
 * @brief Convenience: skip leading whitespace in a UTF-8 string.
 *
 * @return Pointer to first non-whitespace rune (or null terminator).
 */
CC_EXTERN const char *utf8_ltrim(const char *s);

/* =========================================================================
 * Case conversion  –  writes into a utf8_builder_t
 * =========================================================================
 *
 * These functions do NOT operate in-place because uppercasing / lowercasing
 * a UTF-8 string can change its byte length (though rarely for the planes
 * covered by rune16_t).  Instead they iterate the source and write into the
 * caller-supplied builder, letting the caller decide the buffer strategy.
 *
 * Example:
 *
 *   char out[64];
 *   utf8_builder_t b;
 *   utf8_builder_init(&b, out, sizeof(out));
 *   utf8_toupper_build("héllo", &b);
 *   utf8_builder_finish(&b);
 *   // out now contains "HÉLLO"
 *
 * Include utf8_iter.h before utf8.h to use utf8_builder_t.
 * (Forward-declared here so utf8.h is self-contained.)
 */

struct utf8_builder;   /* forward declaration – defined in utf8_iter.h */

/**
 * @brief Write the uppercase form of UTF-8 string `s` into builder `b`.
 *
 * Stops when the builder is full or the string ends.
 *
 * @param s  Null-terminated UTF-8 source.
 * @param b  Initialised utf8_builder_t (from utf8_iter.h).
 * @return true if the entire string fit; false if truncated.
 */
CC_EXTERN bool utf8_toupper_build(const char *s, struct utf8_builder *b);

/**
 * @brief Write the lowercase form of UTF-8 string `s` into builder `b`.
 */
CC_EXTERN bool utf8_tolower_build(const char *s, struct utf8_builder *b);

/**
 * @brief Convenience: uppercase into a plain char buffer.
 *
 * Equivalent to init-builder + toupper_build + finish.
 *
 * @param dst   Destination buffer.
 * @param size  Size of `dst` in bytes (including null terminator).
 * @param src   Null-terminated UTF-8 source.
 * @return true if no truncation occurred.
 */
CC_EXTERN bool utf8_toupper(char *dst, uint16_t size, const char *src);

/**
 * @brief Convenience: lowercase into a plain char buffer.
 */
CC_EXTERN bool utf8_tolower(char *dst, uint16_t size, const char *src);

/* =========================================================================
 * 32-bit rune codec  –  emoji and full Unicode support
 * =========================================================================
 *
 * Unlike the rune16 codec, these functions handle all four UTF-8 sequence
 * lengths (1–4 bytes), covering every Unicode code-point from U+0000 to
 * U+10FFFF including emoji (😀 🎉 🌍 …).
 *
 * The utf8_torune16 / utf8_fromrune16 functions remain available for code
 * that only needs BMP characters and wants to avoid 32-bit arithmetic.
 * ========================================================================= */

/**
 * @brief Decode the first UTF-8 rune from @p str into a 32-bit code-point.
 *
 * Correctly handles 4-byte emoji sequences that utf8_torune16() cannot
 * represent.  Behaviour for 1-3 byte sequences is identical to
 * utf8_torune16().
 *
 * On encoding errors the function advances by 1 byte and sets
 * *rune = RUNE32_DECODE_ERROR.  Surrogate code-points (U+D800–U+DFFF)
 * and values above U+10FFFF are treated as errors.
 *
 * @param rune  Output: decoded 32-bit Unicode code-point.
 * @param str   Input: null-terminated or bounded UTF-8 stream.
 * @return Number of bytes consumed; 0 at end-of-string (str[0] == '\0').
 */
CC_EXTERN uint8_t utf8_torune32(rune32_t *rune, const char *str);

/**
 * @brief Encode a 32-bit Unicode code-point into a UTF-8 byte buffer.
 *
 * The caller must ensure @p str has at least 4 bytes available.
 * The output is NOT null-terminated.
 *
 * Returns 0 for RUNE32_DECODE_ERROR, surrogates, and values above
 * U+10FFFF.
 *
 * @param str   Destination buffer (at least 4 bytes).
 * @param rune  Unicode code-point to encode.
 * @return Number of bytes written (1, 2, 3, or 4); 0 on error.
 */
CC_EXTERN uint8_t utf8_fromrune32(char *str, rune32_t rune);

/**
 * @brief UTF-8 encoded byte-length of a 32-bit code-point (0–4).
 *
 * @return 1 for ASCII, 2–4 for multi-byte, 0 for invalid code-points.
 */
CC_EXTERN uint8_t utf8_rune32len(rune32_t rune);

/**
 * @brief Total UTF-8 byte-length for a rune32_t array.
 *
 * Stops at the first 0 rune or after @p nlen elements.
 *
 * @param rune  Pointer to rune32_t array.
 * @param nlen  Maximum number of elements to examine.
 * @return Total encoded byte count (excludes null terminator).
 */
CC_EXTERN int utf8_rune32nlen(rune32_t *rune, int nlen);

/* =========================================================================
 * 32-bit string operations
 * =========================================================================
 *
 * These mirror the rune16 string functions but use utf8_torune32 internally
 * so that emoji and other SMP characters are handled correctly.
 *
 * Rule of thumb:
 *  - Use the plain (rune16) variants for pure-ASCII or BMP-only strings.
 *  - Use the _32 variants whenever the string may contain emoji.
 * ========================================================================= */

/**
 * @brief Count runes (including emoji) in a null-terminated UTF-8 string.
 *
 * Each emoji counts as one rune regardless of its byte length.
 */
CC_EXTERN int utf8_strlen32(const char *s);

/**
 * @brief Compare two UTF-8 strings rune-by-rune (32-bit code-point order).
 *
 * Emoji compare by code-point value: 😀 (U+1F600) > ÿ (U+00FF) > A (U+0041).
 */
CC_EXTERN int utf8_strcmp32(const char *s1, const char *s2);

/**
 * @brief Compare up to @p n runes from two UTF-8 strings (32-bit variant).
 */
CC_EXTERN int utf8_strncmp32(const char *s1, const char *s2, uint16_t n);

/**
 * @brief Find the first occurrence of a 32-bit rune (e.g. an emoji) in @p s.
 *
 * @param s  Null-terminated UTF-8 string.
 * @param c  Code-point to search for.
 * @return Pointer to the first byte of the matching rune, or NULL.
 */
CC_EXTERN const char *utf8_strchr32(const char *s, rune32_t c);

/**
 * @brief Find the last occurrence of a 32-bit rune in @p s.
 */
CC_EXTERN const char *utf8_strrchr32(const char *s, rune32_t c);

/**
 * @brief Find the first occurrence of UTF-8 substring @p needle in
 *        @p haystack (32-bit / emoji-capable variant).
 */
CC_EXTERN const char *utf8_strstr32(const char *haystack,
                                    const char *needle);

/**
 * @brief Scan forward past runes for which @p predicate returns true
 *        (32-bit variant).
 *
 * Example – skip all emoji at the start of a string:
 *
 *   const char *p = utf8_scan_while32(text, rune32_isemoji);
 *
 * @param s          UTF-8 string.
 * @param predicate  Function taking a rune32_t and returning bool.
 * @return Pointer into @p s past the matching prefix.
 */
CC_EXTERN const char *utf8_scan_while32(const char *s,
                                        bool (*predicate)(rune32_t));

#endif /* __UTF8_H__ */
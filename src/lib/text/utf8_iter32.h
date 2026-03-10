/**
 * @file utf8_iter32.h
 * @brief Zero-allocation 32-bit UTF-8 cursor and builder functions.
 *
 * This header extends utf8_iter.h so that the same @c utf8_cursor_t and
 * @c utf8_builder_t structs — each costing exactly 4 bytes of SRAM on AVR —
 * can decode and encode 32-bit runes (including 4-byte emoji sequences).
 *
 * No new struct types are introduced.  The only difference from the 16-bit
 * API is the suffix @c 32 on the function names and the type of the rune
 * argument (@c rune32_t instead of @c rune16_t).
 *
 * Include order:
 *   #include "utf8_iter32.h"   // brings utf8_iter.h, utf8.h, rune32.h
 *
 * Typical use cases
 * =================
 *
 * 1. Iterate a string that may contain emoji (😀, 🎉, …):
 *
 *   utf8_cursor_t cur;
 *   utf8_cursor_init(&cur, my_text);
 *   rune32_t r;
 *   while (utf8_cursor32_next(&cur, &r)) {
 *       if (rune32_isemoji(r)) {  }   // handle emoji
 *   }
 *
 * 2. Build a string containing emoji:
 *
 *   char buf[16];
 *   utf8_builder_t b;
 *   utf8_builder_init(&b, buf, sizeof(buf));
 *   utf8_builder32_put(&b, 0x1F600u);  // 😀
 *   utf8_builder32_put(&b, 0x1F389u);  // 🎉
 *   utf8_builder_finish(&b);
 *
 * 3. Mix 16-bit and 32-bit iteration on the same cursor:
 *
 *   utf8_cursor_t cur;
 *   utf8_cursor_init(&cur, mixed_text);
 *   rune32_t r;
 *   while (utf8_cursor32_next(&cur, &r)) {
 *       if (rune32_isbmp(r)) {
 *           // r fits in rune16_t — use rune16 classification
 *           if (rune16_isalpha((rune16_t)r)) { ... }
 *       } else {
 *           if (rune32_isemoji(r)) { ... }
 *       }
 *   }
 *
 * 4. Peek ahead without consuming:
 *
 *   rune32_t next;
 *   if (utf8_cursor32_peek(&cur, &next) && rune32_isemoji(next)) {
 *       utf8_cursor_skip(&cur);   // skip it
 *   }
 *
 * Author: Joham https://github.com/jklarenbeek
 */

#ifndef __UTF8_ITER32_H__
#define __UTF8_ITER32_H__

#include "utf8_iter.h"   /* utf8_cursor_t, utf8_builder_t, and 16-bit API */
#include "rune32.h"      /* rune32_t, rune32_isemoji, rune32_toupper, … */

/*
 * NOTE: utf8_torune32, utf8_fromrune32, utf8_rune32len are declared in
 * utf8.h (included transitively through utf8_iter.h) and implemented in
 * utf8.c.  This header is intentionally header-only.
 */

/* =========================================================================
 * 32-bit cursor functions
 * =========================================================================
 * All operate on the existing utf8_cursor_t – no extra struct, no extra
 * SRAM.  Simply call utf8_cursor32_next instead of utf8_cursor_next.
 * ========================================================================= */

/**
 * @brief Advance the cursor and decode the next rune as a rune32_t.
 *
 * Unlike utf8_cursor_next(), this function correctly decodes 4-byte
 * emoji sequences (U+1F300 and above).  For BMP code-points the result
 * is numerically identical.
 *
 * @param cur   Cursor (updated in place after the call).
 * @param rune  Receives the decoded 32-bit code-point, or
 *              RUNE32_DECODE_ERROR on a malformed byte sequence.
 * @return true if a rune was produced, false at end-of-stream.
 */
static inline bool utf8_cursor32_next(utf8_cursor_t *cur, rune32_t *rune)
{
    uint8_t n;
    if (utf8_cursor_done(cur))
        return false;
    n = utf8_torune32(rune, cur->ptr);
    if (n == 0u)
        return false;
    /* Bounded mode: reject a rune whose bytes extend past cur->end.
     * Without this check, a 4-byte emoji starting 2 bytes before end
     * would be decoded and the pointer would jump past the boundary. */
    if (cur->end && cur->ptr + n > cur->end)
        return false;
    cur->ptr += n;
    return true;
}

/**
 * @brief Peek at the next 32-bit rune without advancing the cursor.
 *
 * @param cur   Cursor (NOT modified).
 * @param rune  Receives the decoded rune.
 * @return true if a rune is available, false at end-of-stream.
 */
static inline bool utf8_cursor32_peek(const utf8_cursor_t *cur,
                                      rune32_t *rune)
{
    if (utf8_cursor_done(cur))
        return false;
    utf8_torune32(rune, cur->ptr);
    return true;
}

/* =========================================================================
 * 32-bit builder functions
 * ========================================================================= */

/**
 * @brief Encode and append a 32-bit rune (including emoji) to the builder.
 *
 * Handles all valid Unicode code-points from U+0000 to U+10FFFF.
 * 4-byte emoji (U+1F300+) are written as 4-byte UTF-8 sequences.
 *
 * @param b     Builder (from utf8_iter.h).
 * @param rune  Unicode code-point to append.
 * @return true if written, false if the buffer is full or rune is invalid.
 */
static inline bool utf8_builder32_put(utf8_builder_t *b, rune32_t rune)
{
    uint8_t needed = utf8_rune32len(rune);
    if (!needed || b->ptr + needed > b->end)
        return false;
    b->ptr += utf8_fromrune32(b->ptr, rune);
    return true;
}

/* =========================================================================
 * Inline peek helpers for 32-bit runes
 * ========================================================================= */

/**
 * @brief True if the first rune of the UTF-8 string at @p s is an emoji.
 *
 * This decodes exactly one rune from @p s (no allocation) and tests it
 * with rune32_isemoji().
 */
static inline bool utf8_peekemoji(const char *s)
{
    rune32_t r;
    utf8_torune32(&r, s);
    return rune32_isemoji(r);
}

/**
 * @brief True if the first rune of the UTF-8 string at @p s is a
 *        Supplementary-plane code-point (i.e. a 4-byte sequence).
 */
static inline bool utf8_peeksmp(const char *s)
{
    rune32_t r;
    utf8_torune32(&r, s);
    return rune32_issmp(r);
}

/* =========================================================================
 * rune32 count helpers
 * ========================================================================= */

/**
 * @brief Count the number of runes (including emoji) in a null-terminated
 *        UTF-8 string.
 *
 * Equivalent to utf8_rune_count() but uses the 32-bit codec so 4-byte
 * emoji count as one rune each rather than being skipped.
 */
static inline uint16_t utf8_rune32_count(const char *s)
{
    uint16_t  n = 0;
    rune32_t  r;
    uint8_t   adv;

    while (*s) {
        adv = utf8_torune32(&r, s);
        if (!adv) break;
        s += adv;
        n++;
    }
    return n;
}

/**
 * @brief Count runes in a bounded UTF-8 byte region (32-bit variant).
 *
 * @param s    Start of region.
 * @param len  Length in bytes.
 */
static inline uint16_t utf8_rune32_countn(const char *s, uint16_t len)
{
    const char *end = s + len;
    uint16_t    n   = 0;
    rune32_t    r;
    uint8_t     adv;

    while (s < end && *s) {
        adv = utf8_torune32(&r, s);
        if (!adv || s + adv > end) break;
        s += adv;
        n++;
    }
    return n;
}

#endif /* __UTF8_ITER32_H__ */
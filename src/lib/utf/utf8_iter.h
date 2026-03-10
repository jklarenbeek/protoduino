/**
 * @file utf8_iter.h
 * @brief Zero-allocation UTF-8 cursor, builder and scanner for
 *        memory-constrained systems (Arduino Uno / AVR).
 *
 * Design goals
 * ============
 * - O(1) SRAM: cursor and builder each occupy exactly 2 pointers
 *   (4 bytes on AVR, 16 bytes on 64-bit hosts).
 * - No heap, no VLA, no dynamic allocation of any kind.
 * - All hot-path operations are `static inline` – the compiler can
 *   fold them down to a handful of AVR instructions with -Os.
 * - Both null-terminated *and* bounded (ptr+len) string modes.
 * - Optional PROGMEM (AVR flash) read support via utf8_cursor_initP.
 *
 * Typical use cases
 * =================
 *
 * 1. Forward iteration (read-only, O(1) SRAM):
 *
 *   utf8_cursor_t cur;
 *   utf8_cursor_init(&cur, my_utf8_string);
 *   rune16_t r;
 *   while (utf8_cursor_next(&cur, &r)) {
 *       if (rune16_isalpha(r)) { ... }
 *   }
 *
 * 2. Building a UTF-8 string into a stack buffer:
 *
 *   char buf[32];
 *   utf8_builder_t b;
 *   utf8_builder_init(&b, buf, sizeof(buf));
 *   utf8_builder_put(&b, 0x00E9);   // é
 *   utf8_builder_put(&b, 0x0041);   // A
 *   utf8_builder_finish(&b);        // null-terminate
 *
 * 3. In-place classification without full decode:
 *
 *   if (utf8_peekspace(my_ptr))  { ... }  // test first rune = whitespace?
 *
 * 4. Scanning for a rune (returns byte pointer, no allocation):
 *
 *   const char *p = utf8_scan(haystack, needle_rune);
 *
 * Author: generated from jklarenbeek/protoduino utf8 base
 */

#ifndef __UTF8_ITER_H__
#define __UTF8_ITER_H__

#include "utf8.h"
#include "rune16.h"
#include <stdbool.h>
#include <stdint.h>

/* =========================================================================
 * utf8_cursor_t  –  forward, read-only iterator
 * ========================================================================= */

/**
 * @brief Forward cursor over a UTF-8 byte stream.
 *
 * Stack cost: 4 bytes on AVR (2 × 2-byte pointer),
 *             16 bytes on a 64-bit host.
 *
 * When `end` is NULL the string is treated as null-terminated.
 * When `end` is non-NULL the cursor stops at that byte address
 * regardless of null bytes in the data.
 */
typedef struct utf8_cursor {
    const char *ptr;  /**< Next byte to decode.                          */
    const char *end;  /**< Exclusive end, or NULL for null-terminated.   */
} utf8_cursor_t;

/**
 * @brief Initialise cursor over a null-terminated UTF-8 string.
 * @param cur  Cursor to initialise (must not be NULL).
 * @param str  Null-terminated UTF-8 input.
 */
static inline void utf8_cursor_init(utf8_cursor_t *cur, const char *str)
{
    cur->ptr = str;
    cur->end = (const char *)0;
}

/**
 * @brief Initialise cursor over a bounded UTF-8 byte region.
 * @param cur  Cursor to initialise.
 * @param str  Start of byte region.
 * @param len  Number of *bytes* (not runes) in the region.
 */
static inline void utf8_cursor_initn(utf8_cursor_t *cur,
                                     const char *str, uint16_t len)
{
    cur->ptr = str;
    cur->end = str + len;
}

#ifdef __AVR__
#ifdef __AVR__
#include <avr/pgmspace.h>
#endif /* __AVR__ */
/**
 * @brief Initialise cursor over a PROGMEM (flash) null-terminated string.
 *
 * On AVR the cursor stores a flash address; utf8_cursor_next_P must be
 * used instead of utf8_cursor_next when reading from flash.
 */
static inline void utf8_cursor_initP(utf8_cursor_t *cur,
                                     const char *str_P)
{
    cur->ptr = str_P;
    cur->end = (const char *)0;
}
#endif /* __AVR__ */

/**
 * @brief Return true when no more runes are available.
 */
static inline bool utf8_cursor_done(const utf8_cursor_t *cur)
{
    if (cur->end)
        return cur->ptr >= cur->end;
    return *(cur->ptr) == '\0';
}

/**
 * @brief Advance the cursor and decode the next rune.
 *
 * @param cur   Cursor (updated in place).
 * @param rune  Receives the decoded rune, or UTF8_DECODE_ERROR.
 * @return true if a rune was produced, false at end-of-stream.
 */
static inline bool utf8_cursor_next(utf8_cursor_t *cur, rune16_t *rune)
{
    uint8_t n;
    if (utf8_cursor_done(cur))
        return false;
    n = utf8_torune16(rune, cur->ptr);
    if (n == 0)
        return false;
    cur->ptr += n;
    return true;
}

/**
 * @brief Peek at the next rune without advancing the cursor.
 *
 * @param cur   Cursor (NOT modified).
 * @param rune  Receives the decoded rune.
 * @return true if a rune is available, false at end-of-stream.
 */
static inline bool utf8_cursor_peek(const utf8_cursor_t *cur, rune16_t *rune)
{
    if (utf8_cursor_done(cur))
        return false;
    utf8_torune16(rune, cur->ptr);
    return true;
}

/**
 * @brief Skip (consume) the next rune without returning it.
 * @return true if a rune was skipped, false at end-of-stream.
 */
static inline bool utf8_cursor_skip(utf8_cursor_t *cur)
{
    rune16_t dummy;
    return utf8_cursor_next(cur, &dummy);
}

/**
 * @brief Skip up to `n` runes.
 * @return Number of runes actually skipped.
 */
static inline uint16_t utf8_cursor_skipn(utf8_cursor_t *cur, uint16_t n)
{
    uint16_t skipped = 0;
    while (n-- && utf8_cursor_skip(cur))
        skipped++;
    return skipped;
}

/**
 * @brief Byte offset of the cursor from the original start pointer.
 *
 * Useful to convert a cursor position back to an index for display
 * or to restart iteration from a saved position.
 *
 * @param cur    Current cursor state.
 * @param start  Original `str` pointer passed to utf8_cursor_init*.
 * @return Byte offset (not rune count).
 */
static inline uint16_t utf8_cursor_byte_offset(const utf8_cursor_t *cur,
                                                const char *start)
{
    return (uint16_t)(cur->ptr - start);
}

/**
 * @brief Save and restore cursor positions (e.g. for look-ahead).
 *
 * Example:
 *   utf8_cursor_t saved = cur;   // save
 *   ...look ahead...
 *   cur = saved;                 // restore
 *
 * (A cursor is a plain struct – assignment copies it by value.)
 */

/* =========================================================================
 * utf8_builder_t  –  write UTF-8 runes into a fixed stack/static buffer
 * ========================================================================= */

/**
 * @brief Streaming UTF-8 output builder into a caller-owned buffer.
 *
 * Stack cost: 4 bytes on AVR, 16 bytes on a 64-bit host.
 *
 * The buffer must remain valid for the lifetime of the builder.
 * Always call utf8_builder_finish() to null-terminate before use.
 */
typedef struct utf8_builder {
    char       *ptr;  /**< Next byte to write.              */
    const char *end;  /**< First byte past the usable area. */
} utf8_builder_t;

/**
 * @brief Initialise a builder targeting a caller-supplied buffer.
 *
 * One byte of `size` is reserved for the null terminator, so the
 * maximum payload is `size - 1` bytes.
 *
 * @param b     Builder to initialise.
 * @param buf   Buffer to write into (must not be NULL).
 * @param size  Total size of `buf` in bytes (including terminator).
 */
static inline void utf8_builder_init(utf8_builder_t *b,
                                     char *buf, uint16_t size)
{
    b->ptr = buf;
    b->end = buf + size - 1u;  /* -1 reserves space for '\0' */
}

/**
 * @brief Encode and append a single rune to the builder.
 *
 * @param b     Builder.
 * @param rune  Unicode code point to append.
 * @return true if the rune was written, false if the buffer is full.
 */
static inline bool utf8_builder_put(utf8_builder_t *b, rune16_t rune)
{
    uint8_t needed = utf8_rune16len(rune);
    if (!needed || b->ptr + needed > b->end)
        return false;
    b->ptr += utf8_fromrune16(b->ptr, rune);
    return true;
}

/**
 * @brief Append a raw ASCII byte (0x00–0x7F) without decoding overhead.
 *
 * Slightly cheaper than utf8_builder_put() when the caller knows the
 * value is plain ASCII.
 *
 * @return true if written, false if full.
 */
static inline bool utf8_builder_putc(utf8_builder_t *b, char c)
{
    if (b->ptr >= b->end)
        return false;
    *(b->ptr++) = c;
    return true;
}

/**
 * @brief Null-terminate the builder output.
 *
 * Must be called before passing the buffer to any C string function.
 * Does NOT advance the write pointer; subsequent put() calls continue
 * from the same position (overwriting the terminator).
 */
static inline void utf8_builder_finish(utf8_builder_t *b)
{
    *(b->ptr) = '\0';
}

/**
 * @brief True if no more bytes can be written.
 */
static inline bool utf8_builder_full(const utf8_builder_t *b)
{
    return b->ptr >= b->end;
}

/**
 * @brief Number of bytes written so far (excluding null terminator).
 *
 * @param b      Builder.
 * @param start  The `buf` pointer originally passed to utf8_builder_init().
 */
static inline uint16_t utf8_builder_len(const utf8_builder_t *b,
                                        const char *start)
{
    return (uint16_t)(b->ptr - start);
}

/**
 * @brief Remaining writable bytes (excluding null terminator slot).
 */
static inline uint16_t utf8_builder_remaining(const utf8_builder_t *b)
{
    return (uint16_t)(b->end - b->ptr);
}

/* =========================================================================
 * Inline character-class tests operating on raw UTF-8 bytes
 * (decode one rune, test, discard – no allocation)
 * ========================================================================= */

/**
 * @brief True if the first rune of the UTF-8 string at `s` is whitespace.
 *
 * Avoids storing the rune anywhere; purely tests and discards.
 */
static inline bool utf8_peekspace(const char *s)
{
    rune16_t r;
    utf8_torune16(&r, s);
    return rune16_isspace(r);
}

/**
 * @brief True if the first rune of the UTF-8 string at `s` is alphabetic.
 */
static inline bool utf8_peekalpha(const char *s)
{
    rune16_t r;
    utf8_torune16(&r, s);
    return rune16_isalpha(r);
}

/**
 * @brief True if the first rune at `s` is uppercase.
 */
static inline bool utf8_peekupper(const char *s)
{
    rune16_t r;
    utf8_torune16(&r, s);
    return rune16_isupper(r);
}

/**
 * @brief True if the first rune at `s` is lowercase.
 */
static inline bool utf8_peeklower(const char *s)
{
    rune16_t r;
    utf8_torune16(&r, s);
    return rune16_islower(r);
}

/* =========================================================================
 * Rune-count and byte-length helpers
 * ========================================================================= */

/**
 * @brief Count the number of runes in a null-terminated UTF-8 string.
 *
 * O(n) in bytes. Returns 0 for an empty string.
 * Replaces the broken utf8_len() in utf8.c.
 */
static inline uint16_t utf8_rune_count(const char *s)
{
    uint16_t  n = 0;
    rune16_t  r;
    uint8_t   adv;

    while (*s) {
        adv = utf8_torune16(&r, s);
        if (!adv) break;
        s += adv;
        n++;
    }
    return n;
}

/**
 * @brief Count runes in a bounded UTF-8 byte region.
 *
 * @param s    Start of region.
 * @param len  Length in bytes.
 */
static inline uint16_t utf8_rune_countn(const char *s, uint16_t len)
{
    const char *end = s + len;
    uint16_t    n   = 0;
    rune16_t    r;
    uint8_t     adv;

    while (s < end && *s) {
        adv = utf8_torune16(&r, s);
        /* Stop if no advance (null) OR the sequence extends past end
         * (incomplete multi-byte sequence at the boundary). */
        if (!adv || s + adv > end) break;
        s += adv;
        n++;
    }
    return n;
}

#endif /* __UTF8_ITER_H__ */
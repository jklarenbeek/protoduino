/**
 * @file utf8.c
 * @brief UTF-8 codec and string operations for memory-constrained systems.
 *
 * Bug fixes vs. original utf8.c
 * ==============================
 * 1. utf8_torune16: the original code accumulated raw s0 into `r` without
 *    masking the leading sequence-marker bits, producing wrong code-points
 *    for every multi-byte rune.  Fixed by masking s0 before each shift.
 *
 * 2. utf8_len: the original returned 0 at the null terminator instead of
 *    the accumulated count n.  Renamed utf8_strlen and fixed.
 *
 * New additions
 * =============
 * utf8_valid, utf8_validn, utf8_strlen,
 * utf8_strcmp, utf8_strncmp,
 * utf8_strchr, utf8_strrchr, utf8_strstr,
 * utf8_scan_while, utf8_ltrim,
 * utf8_toupper_build, utf8_tolower_build,
 * utf8_toupper, utf8_tolower.
 */

#include "utf8_iter32.h"  /* brings utf8_iter.h, utf8.h, rune32.h, rune32_t */
#include <string.h>

/* =========================================================================
 * Internal helper
 * ========================================================================= */

/**
 * Read one byte from a normal RAM string.
 * On AVR this can be swapped for pgm_read_byte() for PROGMEM variants.
 */
#define _U8_RD(p)  (*(const unsigned char *)(p))

/* =========================================================================
 * utf8_torune16  –  decode one rune
 * ========================================================================= */

uint8_t utf8_torune16(rune16_t *rune, const char *str)
{
    unsigned char s0, s1, s2, s3;
    rune16_t r;

    /* Initialise to the error sentinel; overwritten on success. */
    *rune = UTF8_DECODE_ERROR;

    s0 = _U8_RD(str);
    if (s0 == 0u)
        return 0u;  /* end-of-string: caller must NOT advance */

    /* ---- 1-byte sequence: U+0000 – U+007F ---- */
    if (s0 < 0x80u) {
        *rune = (rune16_t)s0;
        return 1u;
    }

    /* ---- 2-byte sequence: U+0080 – U+07FF ----
     *  Encoding:  110xxxxx  10xxxxxx
     *  mask s0 with 0x1F before shifting so the "110" prefix
     *  bits do not corrupt the assembled code-point.
     */
    s1 = _U8_RD(str + 1);
    if ((s1 & 0xC0u) != 0x80u)
        return 1u;  /* invalid continuation – skip s0 */

    if ((s0 & 0xE0u) == 0xC0u) {
        r = (rune16_t)(((s0 & 0x1Fu) << 6) | (s1 & 0x3Fu));
        if (r < 0x80u)
            return 2u;   /* overlong: consumed, *rune stays DECODE_ERROR */
        *rune = r;
        return 2u;
    }

    /* ---- 3-byte sequence: U+0800 – U+FFFF ----
     *  Encoding:  1110xxxx  10xxxxxx  10xxxxxx
     *  mask s0 with 0x0F.
     */
    s2 = _U8_RD(str + 2);
    if ((s2 & 0xC0u) != 0x80u)
        return 1u;

    if ((s0 & 0xF0u) == 0xE0u) {
        r = (rune16_t)(((rune16_t)(s0 & 0x0Fu) << 12)
                     | ((rune16_t)(s1 & 0x3Fu) <<  6)
                     |  (rune16_t)(s2 & 0x3Fu));
        if (r < 0x800u)
            return 3u;   /* overlong */
        if (r >= 0xD800u && r <= 0xDFFFu)
            return 3u;   /* surrogate: never valid in UTF-8 */
        *rune = r;
        return 3u;
    }

    /* ---- 4-byte sequence: U+10000 – U+10FFFF ----
     *  rune16_t is only 16 bits so we cannot represent these.
     *  Consume the bytes so the caller does not get stuck, but
     *  leave *rune = UTF8_DECODE_ERROR.
     */
    s3 = _U8_RD(str + 3);
    if ((s3 & 0xC0u) != 0x80u)
        return 1u;

    if ((s0 & 0xF8u) == 0xF0u)
        return 4u;  /* valid 4-byte, but unrepresentable */

    /* Anything else is illegal; skip one byte. */
    return 1u;
}

/* =========================================================================
 * utf8_fromrune16  –  encode one rune (unchanged logic, kept for reference)
 * ========================================================================= */

uint8_t utf8_fromrune16(char *str, rune16_t rune)
{
    /* 1-byte: U+0000 – U+007F */
    if (rune < 0x80) {
        str[0] = (char)rune;
        return 1u;
    }

    /* 2-byte: U+0080 – U+07FF */
    if (rune < 0x800) {
        str[0] = (char)(0xC0u | (unsigned)(rune >> 6));
        str[1] = (char)(0x80u | (unsigned)(rune & 0x3Fu));
        return 2u;
    }

    /* Reject surrogates (invalid scalar values) and the error sentinel. */
    if ((rune >= 0xD800u && rune <= 0xDFFFu) || rune == UTF8_DECODE_ERROR)
        return 0u;

    /* 3-byte: U+0800 – U+FFFF */
    str[0] = (char)(0xE0u | (unsigned)(rune >> 12));
    str[1] = (char)(0x80u | (unsigned)((rune >> 6) & 0x3Fu));
    str[2] = (char)(0x80u | (unsigned)(rune & 0x3Fu));
    return 3u;
}

/* =========================================================================
 * utf8_rune16len  –  encoded byte count of a single rune
 * ========================================================================= */

uint8_t utf8_rune16len(rune16_t rune)
{
    if (rune < 0x80)            return 1u;
    if (rune < 0x800)           return 2u;
    if ((rune >= 0xD800u && rune <= 0xDFFFu)
        || rune == UTF8_DECODE_ERROR) return 0u;
    return 3u;
}

/* =========================================================================
 * utf8_rune16nlen  –  total encoded byte count for a rune16_t array
 * ========================================================================= */

int utf8_rune16nlen(rune16_t *rune, int nlen)
{
    int      total = 0;
    rune16_t r;

    while (nlen--) {
        r = *rune++;
        if (r == 0) break;
        total += utf8_rune16len(r);
    }
    return total;
}

/* =========================================================================
 * utf8_ecpy  –  safe bounded copy (original, kept, minor style cleanup)
 * ========================================================================= */

char *utf8_ecpy(char *to, char *e, char *from)
{
    char *end;

    if (to >= e)
        return to;

    /* portable bounded copy (memccpy not available on all C11 targets) */
    {
        size_t avail = (size_t)(e - to);
        const char *fp = from;
        char *tp = to;
        end = (char *)0;
        while (avail--) {
            if ((*tp++ = *fp++) == '\0') { end = tp; break; }
        }
    }
    if (!end) {
        /* Truncated: walk back past any partial multi-byte sequence. */
        end = e - 1;
        while (end > to && ((_U8_RD(end)) & 0xC0u) == 0x80u)
            end--;
        *end = '\0';
    } else {
        end--;   /* memccpy returns one past '\0'; we want the '\0'. */
    }
    return end;
}

/* =========================================================================
 * utf8_valid / utf8_validn
 * =========================================================================
 * Previously these used utf8_torune16(), which incorrectly returned
 * DECODE_ERROR for valid 4-byte emoji sequences, causing utf8_valid()
 * to reject strings containing any emoji.
 *
 * Fixed: use utf8_torune32() so that every valid Unicode code-point
 * (U+0000–U+10FFFF, excluding surrogates) is accepted.
 * ========================================================================= */

bool utf8_valid(const char *str)
{
    rune32_t r;
    uint8_t  n;

    while (*str) {
        n = utf8_torune32(&r, str);
        if (!n)                        return false;
        if (r == RUNE32_DECODE_ERROR)  return false;
        str += n;
    }
    return true;
}

bool utf8_validn(const char *str, uint16_t len)
{
    const char *end = str + len;
    rune32_t    r;
    uint8_t     n;

    while (str < end && *str) {
        n = utf8_torune32(&r, str);
        if (!n)                       return false;
        if (r == RUNE32_DECODE_ERROR) return false;
        str += n;
    }
    return true;
}

/* =========================================================================
 * utf8_strlen  –  rune count
 * ========================================================================= */

int utf8_strlen(const char *s)
{
    int      n = 0;
    rune16_t r;
    uint8_t  adv;

    while (*s) {
        adv = utf8_torune16(&r, s);
        if (!adv) break;          /* malformed; stop rather than loop */
        s += adv;
        n++;
    }
    return n;
}

/* =========================================================================
 * utf8_strcmp / utf8_strncmp
 * ========================================================================= */

int utf8_strcmp(const char *s1, const char *s2)
{
    rune16_t  r1, r2;
    uint8_t   n1, n2;
    rune16_t  c1, c2;

    for (;;) {
        n1 = utf8_torune16(&r1, s1);
        n2 = utf8_torune16(&r2, s2);

        /*
         * Treat end-of-string (n==0) as code-point 0 for comparison so
         * that "abc" < "abcd" and "" < "a" behave correctly.
         */
        c1 = (n1 == 0) ? (rune16_t)0 : r1;
        c2 = (n2 == 0) ? (rune16_t)0 : r2;

        if (c1 != c2)
            return (c1 > c2) ? 1 : -1;
        if (c1 == 0)
            return 0;   /* both strings ended at the same position */

        s1 += n1;
        s2 += n2;
    }
}

int utf8_strncmp(const char *s1, const char *s2, uint16_t n)
{
    rune16_t  r1, r2, c1, c2;
    uint8_t   a1, a2;

    while (n--) {
        a1 = utf8_torune16(&r1, s1);
        a2 = utf8_torune16(&r2, s2);

        c1 = (a1 == 0) ? (rune16_t)0 : r1;
        c2 = (a2 == 0) ? (rune16_t)0 : r2;

        if (c1 != c2)
            return (c1 > c2) ? 1 : -1;
        if (c1 == 0)
            return 0;

        s1 += a1;
        s2 += a2;
    }
    return 0;
}

/* =========================================================================
 * utf8_strchr / utf8_strrchr
 * ========================================================================= */

const char *utf8_strchr(const char *s, rune16_t c)
{
    rune16_t r;
    uint8_t  n;

    /* Special case: find the null terminator */
    if (c == 0) {
        while (*s) s++;
        return s;
    }

    while (*s) {
        n = utf8_torune16(&r, s);
        if (!n) break;
        if (r == c)
            return s;
        s += n;
    }
    return (const char *)0;
}

const char *utf8_strrchr(const char *s, rune16_t c)
{
    const char *found = (const char *)0;
    const char *match;

    if (c == 0)
        return utf8_strchr(s, 0);  /* end of string */

    while ((match = utf8_strchr(s, c)) != (const char *)0) {
        found = match;
        s     = match + 1;         /* advance past this match */
    }
    return found;
}

/* =========================================================================
 * utf8_strstr  –  substring search (rune-level, no allocation)
 * ========================================================================= */

const char *utf8_strstr(const char *haystack, const char *needle)
{
    rune16_t   c0;
    uint8_t    n0;
    rune16_t   rh, rn;
    uint8_t    ah, an;
    const char *ph;

    /*
     * An empty needle matches at the start (POSIX strstr behaviour).
     * Check the RETURN VALUE of utf8_torune16, not the rune: at null
     * terminator the rune is DECODE_ERROR, not 0.
     */
    n0 = utf8_torune16(&c0, needle);
    if (n0 == 0)                        /* needle is empty string */
        return haystack;

    /* Walk haystack looking for the first rune of needle. */
    for (ph = utf8_strchr(haystack, c0);
         ph;
         ph = utf8_strchr(ph + n0, c0))
    {
        const char *h2 = ph + n0;
        const char *n2 = needle + n0;

        for (;;) {
            an = utf8_torune16(&rn, n2);
            if (an == 0) return ph;          /* end of needle → full match */

            ah = utf8_torune16(&rh, h2);
            if (ah == 0 || rh != rn) break;  /* end of haystack or mismatch */

            h2 += ah;
            n2 += an;
        }
    }
    return (const char *)0;
}

/* =========================================================================
 * utf8_scan_while / utf8_ltrim
 * ========================================================================= */

const char *utf8_scan_while(const char *s, bool (*predicate)(rune16_t))
{
    rune16_t r;
    uint8_t  n;

    while (*s) {
        n = utf8_torune16(&r, s);
        if (!n || !predicate(r))
            break;
        s += n;
    }
    return s;
}

/**
 * Thin wrapper over utf8_scan_while using rune16_isspace.
 */
const char *utf8_ltrim(const char *s)
{
    return utf8_scan_while(s, rune16_isspace);
}

/* =========================================================================
 * Case conversion helpers
 * ========================================================================= */

bool utf8_toupper_build(const char *s, struct utf8_builder *b)
{
    utf8_cursor_t  cur;
    rune16_t       r;
    bool           ok = true;

    utf8_cursor_init(&cur, s);
    while (utf8_cursor_next(&cur, &r)) {
        if (!utf8_builder_put(b, rune16_toupper(r))) {
            ok = false;
            break;
        }
    }
    return ok;
}

bool utf8_tolower_build(const char *s, struct utf8_builder *b)
{
    utf8_cursor_t  cur;
    rune16_t       r;
    bool           ok = true;

    utf8_cursor_init(&cur, s);
    while (utf8_cursor_next(&cur, &r)) {
        if (!utf8_builder_put(b, rune16_tolower(r))) {
            ok = false;
            break;
        }
    }
    return ok;
}

bool utf8_toupper(char *dst, uint16_t size, const char *src)
{
    utf8_builder_t b;
    bool           ok;

    utf8_builder_init(&b, dst, size);
    ok = utf8_toupper_build(src, (struct utf8_builder *)&b);
    utf8_builder_finish(&b);
    return ok;
}

bool utf8_tolower(char *dst, uint16_t size, const char *src)
{
    utf8_builder_t b;
    bool           ok;

    utf8_builder_init(&b, dst, size);
    ok = utf8_tolower_build(src, (struct utf8_builder *)&b);
    utf8_builder_finish(&b);
    return ok;
}

/* =========================================================================
 * 32-bit codec  –  utf8_torune32, utf8_fromrune32, utf8_rune32len
 * =========================================================================
 * These functions handle all four UTF-8 sequence lengths and produce
 * rune32_t values covering the full Unicode range U+0000–U+10FFFF.
 * ========================================================================= */

uint8_t utf8_torune32(rune32_t *rune, const char *str)
{
    unsigned char s0, s1, s2, s3;
    rune32_t      r;

    *rune = RUNE32_DECODE_ERROR;

    s0 = _U8_RD(str);
    if (s0 == 0u)
        return 0u;   /* end-of-string */

    /* ---- 1-byte: U+0000 – U+007F ---- */
    if (s0 < 0x80u) {
        *rune = (rune32_t)s0;
        return 1u;
    }

    /* ---- 2-byte: U+0080 – U+07FF ---- */
    s1 = _U8_RD(str + 1);
    if ((s1 & 0xC0u) != 0x80u)
        return 1u;   /* bad continuation */

    if ((s0 & 0xE0u) == 0xC0u) {
        r = ((rune32_t)(s0 & 0x1Fu) << 6) | (rune32_t)(s1 & 0x3Fu);
        if (r < 0x80u)
            return 2u;   /* overlong: consumed, *rune stays DECODE_ERROR */
        *rune = r;
        return 2u;
    }

    /* ---- 3-byte: U+0800 – U+FFFF ---- */
    s2 = _U8_RD(str + 2);
    if ((s2 & 0xC0u) != 0x80u)
        return 1u;

    if ((s0 & 0xF0u) == 0xE0u) {
        r = ((rune32_t)(s0 & 0x0Fu) << 12)
          | ((rune32_t)(s1 & 0x3Fu) <<  6)
          |  (rune32_t)(s2 & 0x3Fu);

        /* Reject overlong encodings and surrogates U+D800–U+DFFF. */
        if (r < 0x800u)
            return 3u;   /* consumed, *rune stays DECODE_ERROR */
        if (r >= RUNE32_SURR_MIN && r <= RUNE32_SURR_MAX)
            return 3u;

        *rune = r;
        return 3u;
    }

    /* ---- 4-byte: U+10000 – U+10FFFF  (emoji live here) ---- */
    s3 = _U8_RD(str + 3);
    if ((s3 & 0xC0u) != 0x80u)
        return 1u;

    if ((s0 & 0xF8u) == 0xF0u) {
        r = ((rune32_t)(s0 & 0x07u) << 18)
          | ((rune32_t)(s1 & 0x3Fu) << 12)
          | ((rune32_t)(s2 & 0x3Fu) <<  6)
          |  (rune32_t)(s3 & 0x3Fu);

        /* Must be in valid Unicode range and not an overlong encoding. */
        if (r < 0x10000u || r > RUNE32_MAX) {
            return 4u;   /* consumed, *rune stays DECODE_ERROR */
        }

        *rune = r;
        return 4u;
    }

    return 1u;   /* unknown sequence leader – skip one byte */
}

uint8_t utf8_fromrune32(char *str, rune32_t rune)
{
    /* 1-byte: U+0000 – U+007F */
    if (rune < 0x80u) {
        str[0] = (char)rune;
        return 1u;
    }

    /* 2-byte: U+0080 – U+07FF */
    if (rune < 0x800u) {
        str[0] = (char)(0xC0u | (rune >> 6));
        str[1] = (char)(0x80u | (rune & 0x3Fu));
        return 2u;
    }

    /* Reject surrogates and the rune16 error sentinel. */
    if ((rune >= RUNE32_SURR_MIN && rune <= RUNE32_SURR_MAX)
        || rune == RUNE32_DECODE_ERROR)
        return 0u;

    /* 3-byte: U+0800 – U+FFFF */
    if (rune <= 0xFFFFu) {
        str[0] = (char)(0xE0u | (rune >> 12));
        str[1] = (char)(0x80u | ((rune >> 6) & 0x3Fu));
        str[2] = (char)(0x80u | (rune & 0x3Fu));
        return 3u;
    }

    /* 4-byte: U+10000 – U+10FFFF  (emoji!) */
    if (rune <= RUNE32_MAX) {
        str[0] = (char)(0xF0u | (rune >> 18));
        str[1] = (char)(0x80u | ((rune >> 12) & 0x3Fu));
        str[2] = (char)(0x80u | ((rune >>  6) & 0x3Fu));
        str[3] = (char)(0x80u | (rune & 0x3Fu));
        return 4u;
    }

    return 0u;   /* above U+10FFFF – not representable */
}

uint8_t utf8_rune32len(rune32_t rune)
{
    if (rune < 0x80u)    return 1u;
    if (rune < 0x800u)   return 2u;
    if (rune == RUNE32_DECODE_ERROR
        || (rune >= RUNE32_SURR_MIN && rune <= RUNE32_SURR_MAX))
        return 0u;
    if (rune <= 0xFFFFu) return 3u;
    if (rune <= RUNE32_MAX) return 4u;
    return 0u;
}

int utf8_rune32nlen(rune32_t *rune, int nlen)
{
    int      total = 0;
    rune32_t r;

    while (nlen--) {
        r = *rune++;
        if (r == 0u) break;
        total += (int)utf8_rune32len(r);
    }
    return total;
}

/* =========================================================================
 * 32-bit string operations
 * ========================================================================= */

int utf8_strlen32(const char *s)
{
    int      n = 0;
    rune32_t r;
    uint8_t  adv;

    while (*s) {
        adv = utf8_torune32(&r, s);
        if (!adv) break;
        s += adv;
        n++;
    }
    return n;
}

int utf8_strcmp32(const char *s1, const char *s2)
{
    rune32_t r1, r2, c1, c2;
    uint8_t  n1, n2;

    for (;;) {
        n1 = utf8_torune32(&r1, s1);
        n2 = utf8_torune32(&r2, s2);

        c1 = (n1 == 0u) ? 0u : r1;
        c2 = (n2 == 0u) ? 0u : r2;

        if (c1 != c2)
            return (c1 > c2) ? 1 : -1;
        if (c1 == 0u)
            return 0;

        s1 += n1;
        s2 += n2;
    }
}

int utf8_strncmp32(const char *s1, const char *s2, uint16_t n)
{
    rune32_t r1, r2, c1, c2;
    uint8_t  a1, a2;

    while (n--) {
        a1 = utf8_torune32(&r1, s1);
        a2 = utf8_torune32(&r2, s2);

        c1 = (a1 == 0u) ? 0u : r1;
        c2 = (a2 == 0u) ? 0u : r2;

        if (c1 != c2)
            return (c1 > c2) ? 1 : -1;
        if (c1 == 0u)
            return 0;

        s1 += a1;
        s2 += a2;
    }
    return 0;
}

const char *utf8_strchr32(const char *s, rune32_t c)
{
    rune32_t r;
    uint8_t  n;

    if (c == 0u) {
        while (*s) s++;
        return s;   /* pointer to null terminator */
    }

    while (*s) {
        n = utf8_torune32(&r, s);
        if (!n) break;
        if (r == c)
            return s;
        s += n;
    }
    return (const char *)0;
}

const char *utf8_strrchr32(const char *s, rune32_t c)
{
    const char *found = (const char *)0;
    const char *match;

    if (c == 0u)
        return utf8_strchr32(s, 0u);

    while ((match = utf8_strchr32(s, c)) != (const char *)0) {
        found = match;
        s     = match + 1;
    }
    return found;
}

const char *utf8_strstr32(const char *haystack, const char *needle)
{
    rune32_t   c0;
    uint8_t    n0;
    rune32_t   rh, rn;
    uint8_t    ah, an;
    const char *ph;

    n0 = utf8_torune32(&c0, needle);
    if (n0 == 0u)
        return haystack;   /* empty needle */

    for (ph = utf8_strchr32(haystack, c0);
         ph;
         ph = utf8_strchr32(ph + n0, c0))
    {
        const char *h2 = ph + n0;
        const char *n2 = needle + n0;

        for (;;) {
            an = utf8_torune32(&rn, n2);
            if (an == 0u) return ph;          /* full match */

            ah = utf8_torune32(&rh, h2);
            if (ah == 0u || rh != rn) break;  /* mismatch or end */

            h2 += ah;
            n2 += an;
        }
    }
    return (const char *)0;
}

const char *utf8_scan_while32(const char *s, bool (*predicate)(rune32_t))
{
    rune32_t r;
    uint8_t  n;

    while (*s) {
        n = utf8_torune32(&r, s);
        if (!n || !predicate(r))
            break;
        s += n;
    }
    return s;
}
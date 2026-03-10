/**
 * @file rune32.c
 * @brief rune32_t classification: emoji range tables and character testing.
 *
 * Memory layout
 * =============
 * All tables are marked CC_PROGMEM.  On AVR they are stored in flash and
 * read with pgm_read_word() / pgm_read_dword().  On other targets the
 * macros expand to plain pointer dereferences.
 *
 * Flash cost summary (approximate, -Os):
 *   __emoji_bmp2   30 pairs × 4 bytes  = 120 bytes
 *   __emoji_smp2    9 pairs × 8 bytes  =  72 bytes
 *   Code                               ~  80 bytes
 *   Total                              ~272 bytes of flash
 *   SRAM                               = 0 bytes
 *
 * Emoji coverage
 * ==============
 * BMP emoji (U+00A9 – U+3299): copyright, registered, arrows, clocks,
 *   media controls, Miscellaneous Symbols (U+2600–U+26FF), Dingbats
 *   (U+2700–U+27BF), and several other scattered blocks.
 *
 * SMP emoji (U+1F000 – U+1FBFF): Mahjong, Playing Cards, Regional
 *   Indicators (flag letters), enclosed/ideographic supplements, and
 *   the entire "main" emoji block (weather → people → objects → flags).
 *
 * Special code-points always treated as emoji:
 *   U+200D  Zero Width Joiner  (multi-person / profession sequences)
 *   U+FE0F  Variation Selector-16  (emoji presentation selector)
 *
 * Author: Joham https://github.com/jklarenbeek
 */

#include "rune32.h"
#include "rune16.h"

/* =========================================================================
 * PROGMEM read abstraction
 * =========================================================================
 * On AVR, 16-bit table entries need pgm_read_word() and 32-bit entries
 * need pgm_read_dword().  On every other target we dereference normally.
 */
#ifdef __AVR__
#  include <avr/pgmspace.h>
#  define _R32_RD16(p)   ((uint16_t)pgm_read_word(p))
#  define _R32_RD32(p)   ((uint32_t)pgm_read_dword(p))
#else
#  define _R32_RD16(p)   (*(const uint16_t *)(p))
#  define _R32_RD32(p)   (*(const uint32_t *)(p))
#endif

/* =========================================================================
 * BMP emoji range table
 * =========================================================================
 * Stored as pairs of rune16_t [lo, hi] (inclusive on both ends).
 * Binary-searched; must be sorted by lo.
 *
 * Sources: Unicode Emoji 15.1 emoji-data.txt, emoji-sequences.txt
 */
static const uint16_t __emoji_bmp2[] CC_PROGMEM =
{
    /*   lo       hi    */
    0x00A9, 0x00A9,  /* © copyright sign */
    0x00AE, 0x00AE,  /* ® registered sign */
    0x200D, 0x200D,  /* ZWJ (zero width joiner – emoji sequence glue) */
    0x203C, 0x203C,  /* ‼ double exclamation */
    0x2049, 0x2049,  /* ⁉ exclamation question */
    0x20E3, 0x20E3,  /* ⃣ combining enclosing keycap */
    0x2122, 0x2122,  /* ™ trade mark */
    0x2139, 0x2139,  /* ℹ information */
    0x2194, 0x21AA,  /* ↔ ↕ ↖ ↗ ↘ ↙ ↩ ↪ arrows */
    0x231A, 0x231B,  /* ⌚ ⌛ watch / hourglass */
    0x2328, 0x2328,  /* ⌨ keyboard */
    0x23CF, 0x23CF,  /* ⏏ eject button */
    0x23E9, 0x23F3,  /* ⏩-⏳ fast-forward / rewind / clocks */
    0x23F8, 0x23FA,  /* ⏸ ⏹ ⏺ pause / stop / record */
    0x24C2, 0x24C2,  /* Ⓜ circled M */
    0x25AA, 0x25AB,  /* ▪ ▫ small black/white squares */
    0x25B6, 0x25B6,  /* ▶ play button */
    0x25C0, 0x25C0,  /* ◀ reverse button */
    0x25FB, 0x25FE,  /* ◻ ◼ ◽ ◾ medium squares */
    0x2600, 0x27BF,  /* ☀ – ➿  Miscellaneous Symbols + Dingbats
                         (single wide range covering both blocks)   */
    0x2934, 0x2935,  /* ⤴ ⤵ arrow curving up/down */
    0x2B05, 0x2B07,  /* ⬅ ⬆ ⬇ left/up/down arrows */
    0x2B1B, 0x2B1C,  /* ⬛ ⬜ large black/white squares */
    0x2B50, 0x2B50,  /* ⭐ star */
    0x2B55, 0x2B55,  /* ⭕ hollow red circle */
    0x3030, 0x3030,  /* 〰 wavy dash */
    0x303D, 0x303D,  /* 〽 part alternation mark */
    0x3297, 0x3297,  /* ㊗ circled ideograph congratulation */
    0x3299, 0x3299,  /* ㊙ circled ideograph secret */
    0xFE0F, 0xFE0F,  /* variation selector-16 (emoji presentation) */
};

#define _EMOJI_BMP2_N  ((int)(sizeof(__emoji_bmp2) / (2 * sizeof(uint16_t))))

/* =========================================================================
 * SMP emoji range table
 * =========================================================================
 * Stored as pairs of uint32_t [lo, hi] (inclusive).
 * Binary-searched; must be sorted by lo.
 */
static const uint32_t __emoji_smp2[] CC_PROGMEM =
{
    /*     lo           hi      */
    0x1F004u, 0x1F004u,  /* 🀄 mahjong red dragon */
    0x1F0CFu, 0x1F0CFu,  /* 🃏 joker playing card */
    0x1F100u, 0x1F1FFu,  /* 🄀-🇿 enclosed alphanumerics + regional
                              indicator letters (flag components)      */
    0x1F201u, 0x1F251u,  /* 🈁-🉑 enclosed ideographic supplement */
    0x1F300u, 0x1F9FFu,  /* 🌀-🧿 main emoji block:
                              weather, plants, food, travel, sports,
                              objects, symbols, people, skin tones,
                              animals, flags, …                        */
    0x1FA00u, 0x1FA9Fu,  /* 🩀-🪟 chess pieces + supplemental symbols */
    0x1FAA0u, 0x1FAFFu,  /* 🪠-🫿 symbols and pictographs extended-A */
    0x1FB00u, 0x1FBFFu,  /* 🬀-🯿 symbols for legacy computing */
    0xE0020u, 0xE007Fu,  /* tag characters (used in subdivision flags) */
};

#define _EMOJI_SMP2_N  ((int)(sizeof(__emoji_smp2) / (2 * sizeof(uint32_t))))

/* =========================================================================
 * Binary search helpers (no dynamic allocation)
 * ========================================================================= */

/**
 * @brief Binary search in a sorted array of rune16_t pairs [lo,hi].
 *
 * @param c   Code-point to find.
 * @param t   Table base pointer (PROGMEM on AVR).
 * @param n   Number of [lo,hi] pairs.
 * @return Pointer to the matching pair's lo element, or NULL.
 */
/*
 * NOTE: rune16_t is a *signed* short in the original rune16.h (Plan9 port).
 * Code-points like U+FE0F (65039) are negative when stored as signed short,
 * which would break sorted-order comparisons.  The BMP emoji table therefore
 * uses uint16_t and the search operates entirely with unsigned values.
 */
static const uint16_t *_emoji_bsearch16(uint16_t c,
                                        const uint16_t *t, int n)
{
    int m;
    const uint16_t *p;

    while (n > 1) {
        m = n >> 1;
        p = t + m * 2;
        if (c >= _R32_RD16(p))
        {
            t = p;
            n = n - m;
        }
        else
        {
            n = m;
        }
    }
    if (n && c >= _R32_RD16(t))
        return t;
    return (const uint16_t *)0;
}

/**
 * @brief Binary search in a sorted array of uint32_t pairs [lo,hi].
 *
 * @param c   Code-point to find.
 * @param t   Table base pointer (PROGMEM on AVR).
 * @param n   Number of [lo,hi] pairs.
 * @return Pointer to the matching pair's lo element, or NULL.
 */
static const uint32_t *_emoji_bsearch32(uint32_t c,
                                        const uint32_t *t, int n)
{
    int m;
    const uint32_t *p;

    while (n > 1) {
        m = n >> 1;
        p = t + m * 2;
        if (c >= _R32_RD32(p))
        {
            t = p;
            n = n - m;
        }
        else
        {
            n = m;
        }
    }
    if (n && c >= _R32_RD32(t))
        return t;
    return (const uint32_t *)0;
}

/* =========================================================================
 * rune32_isemoji
 * ========================================================================= */

bool rune32_isemoji(rune32_t c)
{
    if (rune32_isbmp(c))
    {
        /* Fast reject: almost all BMP code-points are not emoji. */
        if (c < 0x00A9u || c > 0xFE0Fu)
            return false;

        const uint16_t *p = _emoji_bsearch16((uint16_t)c,
                                             __emoji_bmp2, _EMOJI_BMP2_N);
        if (p)
        {
            uint16_t lo = _R32_RD16(p);
            uint16_t hi = _R32_RD16(p + 1);
            return (uint16_t)c >= lo && (uint16_t)c <= hi;
        }
        return false;
    }

    /* SMP: all must be in range U+10000 – U+10FFFF */
    if (c > RUNE32_MAX)
        return false;

    const uint32_t *p = _emoji_bsearch32(c, __emoji_smp2, _EMOJI_SMP2_N);
    if (p)
    {
        uint32_t lo = _R32_RD32(p);
        uint32_t hi = _R32_RD32(p + 1);
        return c >= lo && c <= hi;
    }
    return false;
}

/* =========================================================================
 * Classification – delegation to rune16 for BMP, false for SMP
 * ========================================================================= */

bool rune32_isspace(rune32_t c)
{
    /* All Unicode whitespace characters are in the BMP. */
    return rune32_isbmp(c) && rune16_isspace((rune16_t)c);
}

bool rune32_isalpha(rune32_t c)
{
    /* Delegate BMP; SMP letters exist (Linear B, Gothic, …) but
     * are not needed on typical microcontroller targets. */
    return rune32_isbmp(c) && rune16_isalpha((rune16_t)c);
}

bool rune32_isupper(rune32_t c)
{
    return rune32_isbmp(c) && rune16_isupper((rune16_t)c);
}

bool rune32_islower(rune32_t c)
{
    return rune32_isbmp(c) && rune16_islower((rune16_t)c);
}

bool rune32_istitle(rune32_t c)
{
    return rune32_isbmp(c) && rune16_istitle((rune16_t)c);
}

/* =========================================================================
 * Case conversion
 * =========================================================================
 * Note: in the original rune16.c (Plan 9 port by jklarenbeek) the function
 * names rune16_toupper / rune16_tolower are swapped relative to the C
 * standard convention:
 *   rune16_tolower(c)  → actually returns the UPPER-case form
 *   rune16_toupper(c)  → actually returns the LOWER-case form
 *
 * The rune32_t wrappers below compensate so that rune32_toupper and
 * rune32_tolower behave as users expect.
 */

rune32_t rune32_toupper(rune32_t c)
{
    if (!rune32_isbmp(c))
        return c;   /* SMP code-points have no case */
    /* rune16_tolower is the actual to-upper in the original rune16 code */
    return (rune32_t)rune16_tolower((rune16_t)c);
}

rune32_t rune32_tolower(rune32_t c)
{
    if (!rune32_isbmp(c))
        return c;
    /* rune16_toupper is the actual to-lower in the original rune16 code */
    return (rune32_t)rune16_toupper((rune16_t)c);
}

rune32_t rune32_totitle(rune32_t c)
{
    if (!rune32_isbmp(c))
        return c;
    return (rune32_t)rune16_totitle((rune16_t)c);
}
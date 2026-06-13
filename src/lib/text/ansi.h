/**
 * @file ansi.h
 * @brief Near-full ANSI / VT input parser for protoduino.
 *
 * A byte-fed, non-blocking, zero-malloc, instance-parameterised state machine
 * modelled on Paul Williams' DEC-compatible VT500 parser
 * (https://vt100.net/emu/dec_ansi_parser) — the same design used by libvterm,
 * vte, and most modern terminal emulators.  It is the INPUT-side counterpart
 * to mcurses (which is OUTPUT-side): feed it the raw bytes arriving from a
 * terminal / host program and it classifies them into structured actions:
 *
 *   - printable runes            (UTF-8 decoded to a codepoint)
 *   - C0 control bytes           (execute)
 *   - CSI sequences              ESC [ params; intermediates final
 *   - ESC / SS2 / SS3 sequences  ESC <inter> final   /   ESC O x   /   ESC N x
 *   - OSC strings                ESC ] ... (BEL | ST)
 *   - DCS strings                ESC P ... (hook / put / unhook)
 *
 * The application supplies a handler table; the parser invokes the matching
 * handler synchronously as each complete action is recognised.  A small
 * helper (`ansi_key_from_csi` / `ansi_key_from_ss3`) maps cursor/navigation/
 * function-key sequences onto the project's KEY_* codes (vterm.h) with
 * modifier extraction — everything a shell needs to read the keyboard.
 *
 * Design notes
 * ============
 *  - UTF-8 native: in the GROUND state bytes 0x80-0xFF are decoded as UTF-8
 *    (NOT treated as 8-bit C1 controls — those collide with UTF-8 and are
 *    not used by UTF-8 terminals).  Only the 7-bit escape forms are parsed.
 *  - Zero malloc / no globals: all state lives in a caller-allocated
 *    ansi_parser_t.  OSC/DCS string bytes are collected into a caller-owned
 *    buffer (may be NULL if strings are not needed).
 *  - A pending lone ESC (the bare ESC key) is delivered by ansi_parser_flush()
 *    when the input source is drained — the usual escape-timeout pattern.
 *
 * Copyright (c) 2026 protoduino contributors.
 */

#ifndef __ANSI_H__
#define __ANSI_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <cc.h>
#include "vterm.h"   /* KEY_* codes reused by the key decoder */

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Compile-time limits
 * ========================================================================= */

/** Max numeric parameters captured per CSI/DCS sequence (extra are dropped). */
#ifndef ANSI_MAX_PARAMS
#define ANSI_MAX_PARAMS        8
#endif

/** Max intermediate bytes (0x20-0x2F) captured per sequence. */
#ifndef ANSI_MAX_INTERMEDIATE
#define ANSI_MAX_INTERMEDIATE  2
#endif

/* =========================================================================
 * Control-sequence descriptor (shared by CSI and DCS)
 * ========================================================================= */

/**
 * @brief Parsed parameters/intermediates of a CSI (or DCS) sequence.
 *
 * Numeric parameters use -1 to mean "absent / use default", so a dispatcher
 * can apply command-specific defaults (use ansi_csi_param()).  Sub-parameter
 * separators ':' are treated like ';' (a common simplification that keeps
 * SGR truecolour parsing working).
 */
typedef struct {
    int16_t  params[ANSI_MAX_PARAMS];          /**< Numeric params (-1 = default). */
    uint8_t  nparams;                          /**< Count of stored params.        */
    uint8_t  intermediate[ANSI_MAX_INTERMEDIATE]; /**< Intermediate bytes 0x20-0x2F. */
    uint8_t  nintermediate;                    /**< Count of intermediates.        */
    uint8_t  private_marker;                   /**< '<' '=' '>' '?' or 0.          */
    uint8_t  final;                            /**< Final byte (the command).      */
} ansi_csi_t;

/** Resolve param @p i, returning @p def when absent or out of range. */
static inline int16_t ansi_csi_param(const ansi_csi_t *c, uint8_t i, int16_t def)
{
    if (i >= c->nparams || c->params[i] < 0)
        return def;
    return c->params[i];
}

/* =========================================================================
 * Handler table
 * =========================================================================
 * Any handler may be NULL (the corresponding action is then ignored).
 * The void *ctx is the opaque pointer passed to ansi_parser_init().
 */
typedef struct {
    /** A printable character (Unicode codepoint, UTF-8 already decoded). */
    void (*on_print)(void *ctx, uint32_t codepoint);

    /** A C0 control byte (0x00-0x1F, e.g. CR, LF, TAB, BS) was executed. */
    void (*on_execute)(void *ctx, uint8_t ctrl);

    /** A complete CSI sequence (ESC [ ... final). */
    void (*on_csi)(void *ctx, const ansi_csi_t *csi);

    /**
     * An ESC sequence completed.  @p intermediate is the first intermediate
     * byte (e.g. '(' for charset designation) or, for SS2/SS3, the introducer
     * 'N'/'O'; 0 if none.  @p final is the final byte.
     */
    void (*on_esc)(void *ctx, uint8_t intermediate, uint8_t final);

    /** A complete OSC string.  @p data is NUL-terminated (len bytes, may be
     *  truncated if the caller buffer was too small). */
    void (*on_osc)(void *ctx, const char *data, uint16_t len);

    /** DCS started: params/intermediates/final available like a CSI. */
    void (*on_dcs_hook)(void *ctx, const ansi_csi_t *csi);
    /** One DCS data byte (between hook and unhook). */
    void (*on_dcs_put)(void *ctx, uint8_t byte);
    /** DCS terminated (ST). */
    void (*on_dcs_unhook)(void *ctx);
} ansi_handlers_t;

/* =========================================================================
 * Parser instance
 * ========================================================================= */

/** Internal parser state (do not access fields directly). */
typedef struct ansi_parser {
    uint8_t     state;          /* current state machine state            */

    ansi_csi_t  csi;            /* accumulating CSI/DCS descriptor         */
    int32_t     cur;            /* current numeric parameter accumulator   */
    uint8_t     cur_set;        /* a digit was seen for the current param  */

    char       *strbuf;         /* caller buffer for OSC/DCS strings (or NULL) */
    uint16_t    strcap;         /* capacity of strbuf                      */
    uint16_t    strpos;         /* bytes collected so far                  */

    uint32_t    u_cp;           /* UTF-8 codepoint accumulator             */
    uint8_t     u_need;         /* UTF-8 continuation bytes still expected */

    const ansi_handlers_t *h;   /* handler table                          */
    void       *ctx;            /* opaque context for handlers            */
} ansi_parser_t;

/* =========================================================================
 * API
 * ========================================================================= */

/**
 * @brief Initialise a parser.
 *
 * @param p        Caller-allocated parser.
 * @param h        Handler table (may live in flash; not copied).
 * @param ctx      Opaque pointer passed back to every handler.
 * @param strbuf   Buffer for OSC/DCS string bytes, or NULL to drop them.
 * @param strcap   Capacity of @p strbuf in bytes (room for a NUL is reserved).
 */
CC_EXTERN void ansi_parser_init(ansi_parser_t *p,
                                const ansi_handlers_t *h, void *ctx,
                                char *strbuf, uint16_t strcap);

/** Reset the parser to GROUND and discard any partial sequence. */
CC_EXTERN void ansi_parser_reset(ansi_parser_t *p);

/** Feed one byte. */
CC_EXTERN void ansi_parse(ansi_parser_t *p, uint8_t byte);

/** Feed a buffer of bytes. */
CC_EXTERN void ansi_parse_buf(ansi_parser_t *p, const uint8_t *buf, uint16_t len);

/**
 * @brief Flush a pending lone ESC.
 *
 * Call when the input source is drained (no more bytes available right now).
 * If the parser is sitting in the ESCAPE state with nothing collected, the
 * bare ESC key is delivered via on_execute(0x1B) and the parser returns to
 * GROUND.  Otherwise this is a no-op.  This is the standard escape-timeout
 * pattern that lets a shell distinguish the ESC key from the start of a
 * longer sequence.
 */
CC_EXTERN void ansi_parser_flush(ansi_parser_t *p);

/* =========================================================================
 * Key decoder  (maps sequences onto vterm.h KEY_* codes)
 * ========================================================================= */

/** Modifier bits returned by the key decoder (xterm encoding, minus 1). */
#define ANSI_MOD_SHIFT  0x01u
#define ANSI_MOD_ALT    0x02u
#define ANSI_MOD_CTRL   0x04u
#define ANSI_MOD_META   0x08u

/**
 * @brief Map a CSI sequence to a KEY_* code (arrows, nav, F-keys).
 *
 * @param csi       The dispatched CSI descriptor.
 * @param out_mods  Receives the modifier bitfield (ANSI_MOD_*), or 0.
 * @return KEY_* code, or 0 if the CSI is not a recognised key (e.g. a cursor
 *         position report `CSI r;c R`, which is intentionally not a key).
 */
CC_EXTERN uint16_t ansi_key_from_csi(const ansi_csi_t *csi, uint8_t *out_mods);

/**
 * @brief Map an SS3 final (from ESC O <final>) to a KEY_* code.
 *
 * Use from your on_esc handler when @p intermediate == 'O'.  Covers
 * application-cursor-mode arrows (ESC O A..D), Home/End (ESC O H/F), and
 * F1-F4 (ESC O P/Q/R/S).
 *
 * @param final     The final byte following ESC O.
 * @param out_mods  Receives 0 (SS3 carries no modifiers); kept for symmetry.
 * @return KEY_* code, or 0 if unrecognised.
 */
CC_EXTERN uint16_t ansi_key_from_ss3(uint8_t final, uint8_t *out_mods);

#ifdef __cplusplus
}
#endif

#endif /* __ANSI_H__ */

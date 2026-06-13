/**
 * @file ansi.c
 * @brief Near-full ANSI / VT input parser — implementation.
 *
 * Hand-coded DEC VT500 (Williams) state machine.  See ansi.h for the model.
 * No globals, no malloc, integer-only; fits comfortably on an ATmega328P.
 */

#include "ansi.h"

#include <string.h>   /* memset */

/* =========================================================================
 * States
 * ========================================================================= */
enum {
    S_GROUND = 0,
    S_ESCAPE,
    S_ESCAPE_INTER,
    S_SS2_SS3,      /* ESC N / ESC O : expect exactly one final byte        */
    S_CSI_ENTRY,
    S_CSI_PARAM,
    S_CSI_INTER,
    S_CSI_IGNORE,
    S_OSC,
    S_DCS_ENTRY,
    S_DCS_PARAM,
    S_DCS_INTER,
    S_DCS_PASS,     /* DCS passthrough: data bytes delivered via on_dcs_put */
    S_DCS_IGNORE,
    S_STR_IGNORE    /* SOS / PM / APC : consumed and discarded              */
};

#define PARAM_MAX_VALUE  32767  /* clamp accumulator to int16_t range */

/* =========================================================================
 * Small helpers
 * ========================================================================= */

static void _clear(ansi_parser_t *p)
{
    p->csi.nparams        = 0;
    p->csi.nintermediate  = 0;
    p->csi.private_marker = 0;
    p->csi.final          = 0;
    p->cur                = 0;
    p->cur_set            = 0;
}

static void _enter_escape(ansi_parser_t *p)
{
    _clear(p);
    p->state = S_ESCAPE;
}

static void _collect_inter(ansi_parser_t *p, uint8_t b)
{
    if (p->csi.nintermediate < ANSI_MAX_INTERMEDIATE)
        p->csi.intermediate[p->csi.nintermediate++] = b;
}

static void _param_digit(ansi_parser_t *p, uint8_t d)
{
    int32_t v = (p->cur_set ? p->cur : 0) * 10 + d;
    if (v > PARAM_MAX_VALUE)
        v = PARAM_MAX_VALUE;
    p->cur     = v;
    p->cur_set = 1;
}

/* End the current parameter on a separator (';' or ':'). */
static void _param_next(ansi_parser_t *p)
{
    if (p->csi.nparams < ANSI_MAX_PARAMS)
        p->csi.params[p->csi.nparams++] = p->cur_set ? (int16_t)p->cur : -1;
    p->cur     = 0;
    p->cur_set = 0;
}

/* Push the trailing parameter (if any activity) just before dispatch. */
static void _param_finish(ansi_parser_t *p)
{
    if (p->cur_set || p->csi.nparams > 0) {
        if (p->csi.nparams < ANSI_MAX_PARAMS)
            p->csi.params[p->csi.nparams++] = p->cur_set ? (int16_t)p->cur : -1;
    }
}

/* ---- string (OSC / DCS) collection ---- */

static void _str_reset(ansi_parser_t *p)
{
    p->strpos = 0;
}

static void _str_put(ansi_parser_t *p, uint8_t b)
{
    /* Reserve one byte for the NUL terminator. */
    if (p->strbuf && p->strpos + 1u < p->strcap)
        p->strbuf[p->strpos++] = (char)b;
}

static void _osc_end(ansi_parser_t *p)
{
    if (p->strbuf && p->strcap > 0)
        p->strbuf[p->strpos < p->strcap ? p->strpos : (p->strcap - 1)] = '\0';
    if (p->h && p->h->on_osc)
        p->h->on_osc(p->ctx, p->strbuf ? p->strbuf : "", p->strpos);
}

/* ---- dispatch wrappers ---- */

static void _csi_dispatch(ansi_parser_t *p, uint8_t final)
{
    _param_finish(p);
    p->csi.final = final;
    if (p->h && p->h->on_csi)
        p->h->on_csi(p->ctx, &p->csi);
}

static void _esc_dispatch(ansi_parser_t *p, uint8_t intermediate, uint8_t final)
{
    if (p->h && p->h->on_esc)
        p->h->on_esc(p->ctx, intermediate, final);
}

static void _dcs_hook(ansi_parser_t *p, uint8_t final)
{
    _param_finish(p);
    p->csi.final = final;
    if (p->h && p->h->on_dcs_hook)
        p->h->on_dcs_hook(p->ctx, &p->csi);
}

static void _execute(ansi_parser_t *p, uint8_t b)
{
    if (p->h && p->h->on_execute)
        p->h->on_execute(p->ctx, b);
}

static void _print(ansi_parser_t *p, uint32_t cp)
{
    if (p->h && p->h->on_print)
        p->h->on_print(p->ctx, cp);
}

/* =========================================================================
 * UTF-8 decoding (GROUND printable path)
 * ========================================================================= */

static void _ground_byte(ansi_parser_t *p, uint8_t b)
{
    /* ASCII fast path */
    if (b < 0x80u) {
        p->u_need = 0;
        _print(p, b);
        return;
    }

    if (p->u_need == 0) {
        /* Lead byte */
        if (b >= 0xC0u && b <= 0xDFu)      { p->u_cp = b & 0x1Fu; p->u_need = 1; }
        else if (b >= 0xE0u && b <= 0xEFu) { p->u_cp = b & 0x0Fu; p->u_need = 2; }
        else if (b >= 0xF0u && b <= 0xF7u) { p->u_cp = b & 0x07u; p->u_need = 3; }
        else {
            /* Stray continuation (0x80-0xBF) or invalid lead (0xF8-0xFF) */
            _print(p, 0xFFFDu); /* U+FFFD REPLACEMENT CHARACTER */
        }
        return;
    }

    /* Continuation byte expected */
    if (b >= 0x80u && b <= 0xBFu) {
        p->u_cp = (p->u_cp << 6) | (uint32_t)(b & 0x3Fu);
        if (--p->u_need == 0)
            _print(p, p->u_cp);
    } else {
        /* Malformed: abort the partial sequence, then reprocess this byte. */
        p->u_need = 0;
        _print(p, 0xFFFDu);
        _ground_byte(p, b);
    }
}

/* =========================================================================
 * Public API
 * ========================================================================= */

void ansi_parser_init(ansi_parser_t *p, const ansi_handlers_t *h, void *ctx,
                      char *strbuf, uint16_t strcap)
{
    if (!p)
        return;
    memset(p, 0, sizeof(*p));
    p->h      = h;
    p->ctx    = ctx;
    p->strbuf = strbuf;
    p->strcap = strcap;
    p->state  = S_GROUND;
}

void ansi_parser_reset(ansi_parser_t *p)
{
    if (!p)
        return;
    p->state  = S_GROUND;
    p->u_need = 0;
    _clear(p);
    _str_reset(p);
}

void ansi_parser_flush(ansi_parser_t *p)
{
    if (!p)
        return;
    /* A bare ESC with nothing after it = the ESC key. */
    if (p->state == S_ESCAPE &&
        p->csi.nintermediate == 0) {
        p->state = S_GROUND;
        _execute(p, KEY_ESCAPE);
    }
}

void ansi_parse_buf(ansi_parser_t *p, const uint8_t *buf, uint16_t len)
{
    if (!p || !buf)
        return;
    while (len--)
        ansi_parse(p, *buf++);
}

void ansi_parse(ansi_parser_t *p, uint8_t b)
{
    if (!p)
        return;

    /* ---- "anywhere" transitions ---- */

    /* CAN / SUB : abort any sequence and return to GROUND (no dispatch). */
    if (b == 0x18u || b == 0x1Au) {
        p->u_need = 0;
        p->state  = S_GROUND;
        return;
    }

    /* ESC : begin a new escape sequence.  Close any open string first. */
    if (b == 0x1Bu) {
        if (p->state == S_OSC)
            _osc_end(p);
        else if (p->state == S_DCS_PASS && p->h && p->h->on_dcs_unhook)
            p->h->on_dcs_unhook(p->ctx);
        p->u_need = 0;
        _enter_escape(p);
        return;
    }

    switch (p->state) {

    /* --------------------------------------------------------------- GROUND */
    case S_GROUND:
        if (b < 0x20u || b == 0x7Fu)
            _execute(p, b);          /* C0 control, or DEL/Backspace key */
        else
            _ground_byte(p, b);      /* printable (0x20-0x7E) / UTF-8 */
        break;

    /* --------------------------------------------------------------- ESCAPE */
    case S_ESCAPE:
        if (b < 0x20u) {
            _execute(p, b);
        } else if (b >= 0x20u && b <= 0x2Fu) {
            _collect_inter(p, b);
            p->state = S_ESCAPE_INTER;
        } else if (b == 0x5Bu) {     /* '[' */
            _clear(p);
            p->state = S_CSI_ENTRY;
        } else if (b == 0x5Du) {     /* ']' OSC */
            _str_reset(p);
            p->state = S_OSC;
        } else if (b == 0x50u) {     /* 'P' DCS */
            _clear(p);
            p->state = S_DCS_ENTRY;
        } else if (b == 0x4Eu || b == 0x4Fu) { /* 'N' SS2 / 'O' SS3 */
            p->csi.private_marker = b;          /* remember introducer */
            p->state = S_SS2_SS3;
        } else if (b == 0x58u || b == 0x5Eu || b == 0x5Fu) { /* X / ^ / _ */
            p->state = S_STR_IGNORE;
        } else if (b == 0x7Fu) {
            /* ignore DEL */
        } else {                     /* 0x30-0x7E final */
            uint8_t inter = (p->csi.nintermediate > 0) ? p->csi.intermediate[0] : 0;
            _esc_dispatch(p, inter, b);
            p->state = S_GROUND;
        }
        break;

    /* -------------------------------------------------------- ESCAPE_INTER */
    case S_ESCAPE_INTER:
        if (b < 0x20u) {
            _execute(p, b);
        } else if (b >= 0x20u && b <= 0x2Fu) {
            _collect_inter(p, b);
        } else if (b == 0x7Fu) {
            /* ignore */
        } else {                     /* 0x30-0x7E final */
            uint8_t inter = (p->csi.nintermediate > 0) ? p->csi.intermediate[0] : 0;
            _esc_dispatch(p, inter, b);
            p->state = S_GROUND;
        }
        break;

    /* ------------------------------------------------------------- SS2/SS3 */
    case S_SS2_SS3:
        if (b < 0x20u) {
            _execute(p, b);
            p->state = S_GROUND;
        } else if (b == 0x7Fu) {
            /* ignore */
        } else {                     /* the single final byte */
            _esc_dispatch(p, p->csi.private_marker, b);
            p->state = S_GROUND;
        }
        break;

    /* ----------------------------------------------------------- CSI_ENTRY */
    case S_CSI_ENTRY:
        if (b < 0x20u) {
            _execute(p, b);
        } else if (b >= 0x30u && b <= 0x39u) {        /* digit */
            _param_digit(p, (uint8_t)(b - '0'));
            p->state = S_CSI_PARAM;
        } else if (b == 0x3Bu || b == 0x3Au) {        /* ';' or ':' */
            _param_next(p);
            p->state = S_CSI_PARAM;
        } else if (b >= 0x3Cu && b <= 0x3Fu) {        /* '<' '=' '>' '?' */
            p->csi.private_marker = b;
            p->state = S_CSI_PARAM;
        } else if (b >= 0x20u && b <= 0x2Fu) {        /* intermediate */
            _collect_inter(p, b);
            p->state = S_CSI_INTER;
        } else if (b == 0x7Fu) {
            /* ignore */
        } else if (b >= 0x40u && b <= 0x7Eu) {        /* final */
            _csi_dispatch(p, b);
            p->state = S_GROUND;
        } else {
            p->state = S_CSI_IGNORE;
        }
        break;

    /* ----------------------------------------------------------- CSI_PARAM */
    case S_CSI_PARAM:
        if (b < 0x20u) {
            _execute(p, b);
        } else if (b >= 0x30u && b <= 0x39u) {
            _param_digit(p, (uint8_t)(b - '0'));
        } else if (b == 0x3Bu || b == 0x3Au) {
            _param_next(p);
        } else if (b >= 0x20u && b <= 0x2Fu) {
            _collect_inter(p, b);
            p->state = S_CSI_INTER;
        } else if (b == 0x7Fu) {
            /* ignore */
        } else if (b >= 0x40u && b <= 0x7Eu) {
            _csi_dispatch(p, b);
            p->state = S_GROUND;
        } else {                       /* 0x3C-0x3F mid-param: invalid */
            p->state = S_CSI_IGNORE;
        }
        break;

    /* ----------------------------------------------------------- CSI_INTER */
    case S_CSI_INTER:
        if (b < 0x20u) {
            _execute(p, b);
        } else if (b >= 0x20u && b <= 0x2Fu) {
            _collect_inter(p, b);
        } else if (b == 0x7Fu) {
            /* ignore */
        } else if (b >= 0x40u && b <= 0x7Eu) {
            _csi_dispatch(p, b);
            p->state = S_GROUND;
        } else {                       /* 0x30-0x3F: invalid here */
            p->state = S_CSI_IGNORE;
        }
        break;

    /* ---------------------------------------------------------- CSI_IGNORE */
    case S_CSI_IGNORE:
        if (b < 0x20u) {
            _execute(p, b);
        } else if (b >= 0x40u && b <= 0x7Eu) {
            p->state = S_GROUND;       /* swallow the final, no dispatch */
        }
        /* else ignore 0x20-0x3F */
        break;

    /* ---------------------------------------------------------------- OSC */
    case S_OSC:
        if (b == 0x07u) {              /* BEL terminator (xterm) */
            _osc_end(p);
            p->state = S_GROUND;
        } else if (b < 0x20u) {
            /* ignore other C0 inside OSC */
        } else {
            _str_put(p, b);            /* collect (incl. UTF-8 bytes) */
        }
        break;

    /* ---------------------------------------------------------- DCS_ENTRY */
    case S_DCS_ENTRY:
        if (b < 0x20u) {
            /* ignore */
        } else if (b >= 0x30u && b <= 0x39u) {
            _param_digit(p, (uint8_t)(b - '0'));
            p->state = S_DCS_PARAM;
        } else if (b == 0x3Bu || b == 0x3Au) {
            _param_next(p);
            p->state = S_DCS_PARAM;
        } else if (b >= 0x3Cu && b <= 0x3Fu) {
            p->csi.private_marker = b;
            p->state = S_DCS_PARAM;
        } else if (b >= 0x20u && b <= 0x2Fu) {
            _collect_inter(p, b);
            p->state = S_DCS_INTER;
        } else if (b == 0x7Fu) {
            /* ignore */
        } else if (b >= 0x40u && b <= 0x7Eu) {
            _dcs_hook(p, b);
            p->state = S_DCS_PASS;
        } else {
            p->state = S_DCS_IGNORE;
        }
        break;

    /* ---------------------------------------------------------- DCS_PARAM */
    case S_DCS_PARAM:
        if (b < 0x20u) {
            /* ignore */
        } else if (b >= 0x30u && b <= 0x39u) {
            _param_digit(p, (uint8_t)(b - '0'));
        } else if (b == 0x3Bu || b == 0x3Au) {
            _param_next(p);
        } else if (b >= 0x20u && b <= 0x2Fu) {
            _collect_inter(p, b);
            p->state = S_DCS_INTER;
        } else if (b == 0x7Fu) {
            /* ignore */
        } else if (b >= 0x40u && b <= 0x7Eu) {
            _dcs_hook(p, b);
            p->state = S_DCS_PASS;
        } else {
            p->state = S_DCS_IGNORE;
        }
        break;

    /* ---------------------------------------------------------- DCS_INTER */
    case S_DCS_INTER:
        if (b < 0x20u) {
            /* ignore */
        } else if (b >= 0x20u && b <= 0x2Fu) {
            _collect_inter(p, b);
        } else if (b == 0x7Fu) {
            /* ignore */
        } else if (b >= 0x40u && b <= 0x7Eu) {
            _dcs_hook(p, b);
            p->state = S_DCS_PASS;
        } else {
            p->state = S_DCS_IGNORE;
        }
        break;

    /* ----------------------------------------------------------- DCS_PASS */
    case S_DCS_PASS:
        /* Data bytes until ST (handled by the anywhere ESC branch). */
        if (p->h && p->h->on_dcs_put)
            p->h->on_dcs_put(p->ctx, b);
        break;

    /* --------------------------------------------- DCS_IGNORE / STR_IGNORE */
    case S_DCS_IGNORE:
    case S_STR_IGNORE:
        /* Consume everything; ST (ESC) ends it via the anywhere branch. */
        break;

    default:
        p->state = S_GROUND;
        break;
    }
}

/* =========================================================================
 * Key decoder
 * ========================================================================= */

uint16_t ansi_key_from_csi(const ansi_csi_t *c, uint8_t *out_mods)
{
    uint16_t key  = 0;
    uint8_t  mods = 0;

    switch (c->final) {
    case 'A': key = KEY_UP;    break;
    case 'B': key = KEY_DOWN;  break;
    case 'C': key = KEY_RIGHT; break;
    case 'D': key = KEY_LEFT;  break;
    case 'H': key = KEY_HOME;  break;
    case 'F': key = KEY_END;   break;
    case 'Z': key = KEY_BTAB;  break;

    /* Modified F1/F2/F4 arrive as CSI 1 ; <mod> P/Q/S.  Unmodified F1-F4 come
     * via SS3 (ESC O P..S); a bare CSI P/Q/S is an OUTPUT command, not a key. */
    case 'P': case 'Q': case 'S':
        if (c->nparams >= 2 && c->params[0] == 1) {
            key = (c->final == 'P') ? KEY_F(1)
                : (c->final == 'Q') ? KEY_F(2)
                                    : KEY_F(4);
            if (c->params[1] > 1)
                mods = (uint8_t)(c->params[1] - 1);
        } else {
            if (out_mods) *out_mods = 0;
            return 0;
        }
        if (out_mods) *out_mods = mods;
        return key;

    /* 'R' is the cursor-position report (CSI r;c R) — deliberately not a key. */
    case 'R':
        if (out_mods) *out_mods = 0;
        return 0;

    case '~': {
        int16_t n = (c->nparams >= 1 && c->params[0] > 0) ? c->params[0] : 0;
        switch (n) {
        case 1: case 7: key = KEY_HOME;  break;
        case 2:         key = KEY_IC;    break;
        case 3:         key = KEY_DC;    break;
        case 4: case 8: key = KEY_END;   break;
        case 5:         key = KEY_PPAGE; break;
        case 6:         key = KEY_NPAGE; break;
        case 11:        key = KEY_F(1);  break;
        case 12:        key = KEY_F(2);  break;
        case 13:        key = KEY_F(3);  break;
        case 14:        key = KEY_F(4);  break;
        case 15:        key = KEY_F(5);  break;
        case 17:        key = KEY_F(6);  break;
        case 18:        key = KEY_F(7);  break;
        case 19:        key = KEY_F(8);  break;
        case 20:        key = KEY_F(9);  break;
        case 21:        key = KEY_F(10); break;
        case 23:        key = KEY_F(11); break;
        case 24:        key = KEY_F(12); break;
        default:
            if (out_mods) *out_mods = 0;
            return 0;
        }
        if (c->nparams >= 2 && c->params[1] > 1)
            mods = (uint8_t)(c->params[1] - 1);
        if (out_mods) *out_mods = mods;
        return key;
    }

    default:
        if (out_mods) *out_mods = 0;
        return 0;
    }

    /* Letter-final cursor/nav keys (arrows, Home, End, BTab).  Distinguish the
     * keyboard form from a cursor-movement OUTPUT command:
     *   input :  CSI X            (no params)
     *            CSI 1 ; <mod> X  (modified)
     *   output:  CSI <row>;<col> H = CUP,   CSI <n> A = CUU, ...
     * A leading parameter other than 1 means it is a movement command, not a
     * key, so it must not be misread (e.g. CSI 10;20 H is NOT the Home key). */
    if (c->nparams >= 1 && c->params[0] != 1) {
        if (out_mods) *out_mods = 0;
        return 0;
    }
    if (c->nparams >= 2 && c->params[1] > 1)
        mods = (uint8_t)(c->params[1] - 1);
    if (out_mods) *out_mods = mods;
    return key;
}

uint16_t ansi_key_from_ss3(uint8_t final, uint8_t *out_mods)
{
    if (out_mods) *out_mods = 0;
    switch (final) {
    case 'A': return KEY_UP;
    case 'B': return KEY_DOWN;
    case 'C': return KEY_RIGHT;
    case 'D': return KEY_LEFT;
    case 'H': return KEY_HOME;
    case 'F': return KEY_END;
    case 'P': return KEY_F(1);
    case 'Q': return KEY_F(2);
    case 'R': return KEY_F(3);
    case 'S': return KEY_F(4);
    default:  return 0;
    }
}

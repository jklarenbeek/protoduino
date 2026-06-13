/**
 * @file mcurses.c
 * @brief Lean mcurses terminal library – implementation.
 *
 * All output is written via ipc_pipe_write() into the TX pipe supplied
 * at init time.  The caller's transport layer (UART ISR, USB-CDC, …)
 * drains that pipe independently.
 *
 * All input is read from the RX pipe with ipc_pipe_read().  The caller's
 * receive ISR fills the RX pipe so getch_ex() never blocks the MCU.
 *
 * No globals.  No malloc.  Fits on an Arduino Uno.
 */

#include "mcurses.h"

#include <string.h>    /* memset */

#ifdef __AVR__
#  include <avr/pgmspace.h>
#  include <util/delay.h>   /* _delay_ms for halfdelay */
#else
   /* Host / ARM stubs */
#  ifndef PSTR
#    define PSTR(s)          (s)
#  endif
#  ifndef pgm_read_byte
#    define pgm_read_byte(p) (*(const uint8_t *)(p))
#  endif
#  ifndef _delay_ms
#    include <unistd.h>
#    define _delay_ms(ms)    /* no-op on host */
#  endif
#endif

/* =========================================================================
 * Internal helpers – write primitives
 * =========================================================================
 * All output goes through these two primitives so we have a single choke
 * point.  ipc_pipe_write() is already atomic-safe (its ISR story is in
 * ipc.h), so no additional locking is needed here.
 */

/**
 * Write @p len bytes to the TX pipe, accounting for ipc_pipe_write()'s
 * partial-write contract so output is never *silently* truncated.
 *
 *  - If a drain hook is installed (mcurses_set_drain), it is invoked when the
 *    pipe is full so the transport can make room; the write then resumes.
 *    With a working drain, no bytes are lost (back-pressure).
 *  - If no drain hook is available (or it makes no headway), the unwritten
 *    bytes are added to scr->tx_dropped — a visible loss counter — and the
 *    call returns rather than blocking the cooperative scheduler.
 */
static void _emit(mcurses_t *scr, const char *buf, size_t len)
{
    size_t  off    = 0;
    uint8_t stalls = 0;

    while (off < len) {
        size_t w = ipc_pipe_write(scr->txpipe, (const uint8_t *)buf + off, len - off);
        off += w;
        if (off >= len)
            return;

        if (w == 0) {
            /* Pipe full, no progress this round. */
            if (scr->tx_drain && stalls < 8u) {
                scr->tx_drain(scr->tx_drain_ctx);   /* try to make space */
                ++stalls;
                continue;
            }
            /* Give up without blocking; record the loss so it is not silent. */
            uint32_t total = (uint32_t)scr->tx_dropped + (uint32_t)(len - off);
            scr->tx_dropped = (total > 0xFFFFu) ? 0xFFFFu : (uint16_t)total;
            return;
        }
        stalls = 0;   /* made progress */
    }
}

/** Write a single byte into the TX pipe (with back-pressure / loss counting). */
static void _putc(mcurses_t *scr, uint8_t c)
{
    _emit(scr, (const char *)&c, 1);
}

/** Write a RAM byte string of known length into the TX pipe. */
static void _puts_n(mcurses_t *scr, const char *s, uint8_t len)
{
    _emit(scr, s, len);
}

/** Write a NUL-terminated RAM string into the TX pipe. */
static void _puts(mcurses_t *scr, const char *s)
{
    while (*s) {
        _putc(scr, (uint8_t)*s);
        ++s;
    }
}

#ifdef __AVR__
/** Write a NUL-terminated PROGMEM string into the TX pipe. */
static void _puts_P(mcurses_t *scr, const char *s_P)
{
    uint8_t c;
    while ((c = pgm_read_byte(s_P)) != 0) {
        _putc(scr, c);
        ++s_P;
    }
}
#else
#  define _puts_P(scr, s)  _puts((scr), (s))
#endif

/* =========================================================================
 * Internal helpers – VT sequence emission
 * =========================================================================
 * We compose numeric VT sequences into a small stack buffer to avoid
 * snprintf / printf overhead (not available on all AVR configs).
 *
 * Format helpers produce strings like "\x1b[<n>A" directly.
 */

/* Append a decimal uint8 to buf[], return new position. */
static uint8_t _u8toa(char *buf, uint8_t pos, uint8_t v)
{
    if (v >= 100) { buf[pos++] = (char)('0' + v / 100); v %= 100; }
    if (v >= 10)  { buf[pos++] = (char)('0' + v / 10);  v %= 10;  }
    buf[pos++] = (char)('0' + v);
    return pos;
}

/**
 * Emit  ESC [ <row+1> ; <col+1> H  (VT cursor-position, 1-based).
 * Uses a 12-byte stack buffer; no heap.
 */
static void _emit_cup(mcurses_t *scr, uint8_t row, uint8_t col)
{
    char buf[12];
    uint8_t n = 0;
    buf[n++] = '\x1b';
    buf[n++] = '[';
    n = _u8toa(buf, n, (uint8_t)(row + 1));
    buf[n++] = ';';
    n = _u8toa(buf, n, (uint8_t)(col + 1));
    buf[n++] = 'H';
    _puts_n(scr, buf, n);
}

/** Emit  ESC [ <n> <cmd>  e.g. "\x1b[3A" (cursor up 3). */
static void _emit_csi_n(mcurses_t *scr, uint8_t n, char cmd)
{
    char buf[8];
    uint8_t pos = 0;
    buf[pos++] = '\x1b';
    buf[pos++] = '[';
    pos = _u8toa(buf, pos, n);
    buf[pos++] = cmd;
    _puts_n(scr, buf, pos);
}

/** Emit  ESC [ <top+1> ; <bot+1> r  (DECSTBM – set scroll region). */
static void _emit_stbm(mcurses_t *scr, uint8_t top, uint8_t bot)
{
    char buf[14];
    uint8_t n = 0;
    buf[n++] = '\x1b';
    buf[n++] = '[';
    n = _u8toa(buf, n, (uint8_t)(top + 1));
    buf[n++] = ';';
    n = _u8toa(buf, n, (uint8_t)(bot + 1));
    buf[n++] = 'r';
    _puts_n(scr, buf, n);
}

/* =========================================================================
 * Internal helper – SGR attribute emission
 * =========================================================================
 * Only re-sends the SGR sequence when attributes actually changed to
 * minimise TX pipe pressure.
 *
 * SGR colour mapping: MCURSES colour index 0=default, 1-8 → ANSI 30-37/40-47.
 */

static void _emit_sgr(mcurses_t *scr, uint16_t attr)
{
    if (attr == scr->last_attr)
        return;

    /* Reset first */
    _puts(scr, "\x1b[0");

    /* Style bits */
    if (attr & MCURSES_ATTR_BOLD)      _puts(scr, ";1");
    if (attr & MCURSES_ATTR_DIM)       _puts(scr, ";2");
    if (attr & MCURSES_ATTR_UNDERLINE) _puts(scr, ";4");
    if (attr & MCURSES_ATTR_BLINK)     _puts(scr, ";5");
    if (attr & MCURSES_ATTR_REVERSE)   _puts(scr, ";7");
    if (attr & MCURSES_ATTR_INVISIBLE) _puts(scr, ";8");

    /* Foreground colour (index 0 = default = skip) */
    uint8_t fg = (uint8_t)((attr & MCURSES_FG_MASK) >> MCURSES_FG_SHIFT);
    if (fg > 0) {
        char buf[5]; /* ";3X" + NUL */
        uint8_t n = 0;
        buf[n++] = ';';
        buf[n++] = '3';
        buf[n++] = (char)('0' + (fg - 1)); /* maps 1-8 → '0'-'7' */
        _puts_n(scr, buf, n);
    }

    /* Background colour */
    uint8_t bg = (uint8_t)((attr & MCURSES_BG_MASK) >> MCURSES_BG_SHIFT);
    if (bg > 0) {
        char buf[5];
        uint8_t n = 0;
        buf[n++] = ';';
        buf[n++] = '4';
        buf[n++] = (char)('0' + (bg - 1));
        _puts_n(scr, buf, n);
    }

    _putc(scr, 'm');
    scr->last_attr = attr;
}

/* =========================================================================
 * Lifecycle
 * ========================================================================= */

void mcurses_init(mcurses_t *scr,
                  ipc_pipe_t *tx, ipc_pipe_t *rx,
                  uint8_t rows, uint8_t cols)
{
    memset(scr, 0, sizeof(*scr));
    scr->txpipe     = tx;
    scr->rxpipe     = rx;
    scr->rows       = rows;
    scr->cols       = cols;
    scr->scroll_bot = (uint8_t)(rows - 1u);
    scr->cursor_vis = 1;
    /* last_attr = 0 → "unknown", forces first SGR flush */
    scr->last_attr  = 0xFFFFu;
}

void mcurses_set_drain(mcurses_t *scr, void (*cb)(void *), void *ctx)
{
    if (!scr)
        return;
    scr->tx_drain     = cb;
    scr->tx_drain_ctx = ctx;
}

uint_fast8_t initscr_ex(mcurses_t *scr)
{
    if (!scr || !scr->txpipe)
        return 0;

    scr->cury       = 0;
    scr->curx       = 0;
    scr->scroll_top = 0;
    scr->scroll_bot = (uint8_t)(scr->rows - 1u);
    scr->attr       = MCURSES_ATTR_NORMAL;
    scr->last_attr  = 0xFFFFu;
    scr->nodelay    = 0;
    scr->halfdelay  = 0;
    scr->cursor_vis = 1;
    scr->initialized = 1;

    /* Switch to alternate screen, reset attributes, clear, home */
    _puts(scr, "\x1b[?1049h");  /* DECSET alt screen   */
    _puts(scr, "\x1b[0m");      /* SGR reset           */
    scr->last_attr = MCURSES_ATTR_NORMAL;
    _puts(scr, "\x1b[2J");      /* ED – erase display  */
    _puts(scr, "\x1b[H");       /* CUP home            */
    _puts(scr, "\x1b[?25h");    /* DECTCEM show cursor */

    return 1;
}

void endwin_ex(mcurses_t *scr)
{
    if (!scr || !scr->initialized)
        return;

    _emit_sgr(scr, MCURSES_ATTR_NORMAL);
    _puts(scr, "\x1b[?25h");    /* show cursor         */
    _puts(scr, "\x1b[r");       /* reset scroll region */
    _puts(scr, "\x1b[?1049l");  /* leave alt screen    */

    scr->initialized = 0;
}

/* =========================================================================
 * Cursor & attributes
 * ========================================================================= */

void move_ex(mcurses_t *scr, uint_fast8_t y, uint_fast8_t x)
{
    if (!scr) return;
    scr->cury = (uint8_t)y;
    scr->curx = (uint8_t)x;
    _emit_cup(scr, (uint8_t)y, (uint8_t)x);
}

void attrset_ex(mcurses_t *scr, uint_fast16_t attr)
{
    if (!scr) return;
    scr->attr = (uint16_t)attr;
    _emit_sgr(scr, scr->attr);
}

void curs_set_ex(mcurses_t *scr, uint_fast8_t vis)
{
    if (!scr) return;
    scr->cursor_vis = (uint8_t)(vis & 0x03u);
    if (vis == 0)
        _puts(scr, "\x1b[?25l");   /* DECTCEM hide         */
    else if (vis == 2)
        _puts(scr, "\x1b[?25h\x1b[1 q"); /* show + block blink */
    else
        _puts(scr, "\x1b[?25h\x1b[2 q"); /* show + block steady */
}

/* =========================================================================
 * Output
 * ========================================================================= */

void addch_ex(mcurses_t *scr, uint_fast8_t ch)
{
    if (!scr) return;
    _emit_sgr(scr, scr->attr);
    _putc(scr, (uint8_t)ch);
    if (++scr->curx >= scr->cols) {
        scr->curx = 0;
        if (scr->cury < scr->rows - 1u)
            ++scr->cury;
    }
}

void addch_utf8_ex(mcurses_t *scr, const char *utf8, uint_fast8_t len)
{
    if (!scr || !utf8) return;

    /* Auto-detect byte length from lead byte if not provided. */
    if (len == 0) {
        uint8_t lead = (uint8_t)utf8[0];
        if      (lead < 0x80u) len = 1;
        else if (lead < 0xE0u) len = 2;
        else if (lead < 0xF0u) len = 3;
        else                   len = 4;
    }

    _emit_sgr(scr, scr->attr);

    /* Write the raw UTF-8 bytes directly to the TX pipe – zero-copy. */
    _puts_n(scr, utf8, (uint8_t)len);

    /* Advance cursor by 1 display column.
     * TODO: CJK double-width detection would add 2 here. */
    if (++scr->curx >= scr->cols) {
        scr->curx = 0;
        if (scr->cury < scr->rows - 1u)
            ++scr->cury;
    }
}

void addstr_ex(mcurses_t *scr, const char *s)
{
    if (!scr || !s) return;
    _emit_sgr(scr, scr->attr);

    /* Stream the UTF-8 bytes directly to the TX pipe.
     * We iterate byte-by-byte only to track the display column count;
     * the raw bytes go out unmodified. */
    const char *run_start = s;
    while (*s) {
        uint8_t lead = (uint8_t)*s;
        uint8_t seq_len;
        if      (lead < 0x80u) seq_len = 1;
        else if (lead < 0xE0u) seq_len = 2;
        else if (lead < 0xF0u) seq_len = 3;
        else                   seq_len = 4;

        /* Consume up to seq_len bytes but NEVER step past the NUL
         * terminator – a truncated/malformed multi-byte sequence must
         * not cause an out-of-bounds read or emit bytes past the string. */
        uint8_t i = 0;
        while (i < seq_len && s[i] != '\0')
            ++i;
        s += i;

        /* Advance cursor column (each rune = 1 display cell) */
        if (++scr->curx >= scr->cols) {
            scr->curx = 0;
            if (scr->cury < scr->rows - 1u)
                ++scr->cury;
        }
    }
    /* Flush the entire string in one write.  _emit() honours the pipe's
     * partial-write contract (back-pressure via the drain hook, or a visible
     * tx_dropped count) so output >255 bytes or larger than free TX space is
     * not silently truncated. */
    size_t total = (size_t)(s - run_start);
    if (total > 0)
        _emit(scr, run_start, total);
}

void addnstr_ex(mcurses_t *scr, const char *s, uint_fast8_t byte_len)
{
    if (!scr || !s || byte_len == 0) return;
    _emit_sgr(scr, scr->attr);

    /* Write the byte range directly to the TX pipe – zero-copy. */
    _puts_n(scr, s, (uint8_t)byte_len);

    /* Count display columns: iterate the UTF-8 sequence headers. */
    const char *p = s;
    const char *end = s + byte_len;
    while (p < end) {
        uint8_t lead = (uint8_t)*p;
        uint8_t seq_len;
        if      (lead < 0x80u) seq_len = 1;
        else if (lead < 0xE0u) seq_len = 2;
        else if (lead < 0xF0u) seq_len = 3;
        else                   seq_len = 4;
        p += seq_len;

        if (++scr->curx >= scr->cols) {
            scr->curx = 0;
            if (scr->cury < scr->rows - 1u)
                ++scr->cury;
        }
    }
}

void addstr_P_ex(mcurses_t *scr, const char *s_P)
{
    if (!scr || !s_P) return;
    _emit_sgr(scr, scr->attr);

#ifdef __AVR__
    /* On AVR the flash bytes are already UTF-8 encoded.
     * We read byte-by-byte from PROGMEM and stream them out.
     * The utf8_cursor_initP iterator decodes to rune16 which we don't need;
     * instead, just read raw bytes and track display columns by inspecting
     * lead bytes. */
    uint8_t c;
    while ((c = pgm_read_byte(s_P)) != 0) {
        _putc(scr, c);
        /* Only count display column on lead bytes, not continuation bytes */
        if ((c & 0xC0u) != 0x80u) {
            if (++scr->curx >= scr->cols) {
                scr->curx = 0;
                if (scr->cury < scr->rows - 1u)
                    ++scr->cury;
            }
        }
        ++s_P;
    }
#else
    addstr_ex(scr, s_P);
#endif
}

void refresh_ex(mcurses_t *scr)
{
    /*
     * The TX pipe is drained by the caller's transport (UART TX ISR or
     * DMA).  refresh() is a semantic flush – on a bare-metal UART the
     * pipe drains automatically; no extra work is needed here.
     *
     * If the platform needs a software kick (e.g. enable UDRE interrupt),
     * do it via the wake_cb registered with ipc_pipe_init().
     */
    (void)scr;
}

/* =========================================================================
 * Screen / line editing
 * ========================================================================= */

void setscrreg_ex(mcurses_t *scr, uint_fast8_t top, uint_fast8_t bot)
{
    if (!scr) return;
    scr->scroll_top = (uint8_t)top;
    scr->scroll_bot = (uint8_t)bot;
    _emit_stbm(scr, (uint8_t)top, (uint8_t)bot);
}

void deleteln_ex(mcurses_t *scr)
{
    if (!scr) return;
    /* Move to start of current line, then DL 1 */
    _emit_cup(scr, scr->cury, 0);
    _puts(scr, "\x1b[M");   /* DL – delete line */
}

void insertln_ex(mcurses_t *scr)
{
    if (!scr) return;
    _emit_cup(scr, scr->cury, 0);
    _puts(scr, "\x1b[L");   /* IL – insert line */
}

void scroll_ex(mcurses_t *scr)
{
    if (!scr) return;
    /* Ensure scroll region is active, then send IND (ESC D = index down). */
    _emit_stbm(scr, scr->scroll_top, scr->scroll_bot);
    /* Move to bottom of scroll region and emit LF to scroll. */
    _emit_cup(scr, scr->scroll_bot, 0);
    _putc(scr, '\n');
    /* Restore cursor */
    _emit_cup(scr, scr->cury, scr->curx);
}

void clear_ex(mcurses_t *scr)
{
    if (!scr) return;
    _puts(scr, "\x1b[2J");  /* ED – erase all */
    _puts(scr, "\x1b[H");   /* CUP home        */
    scr->cury = 0;
    scr->curx = 0;
}

void clrtobot_ex(mcurses_t *scr)
{
    if (!scr) return;
    _puts(scr, "\x1b[J");   /* ED – erase from cursor to end */
}

void clrtoeol_ex(mcurses_t *scr)
{
    if (!scr) return;
    _puts(scr, "\x1b[K");   /* EL – erase from cursor to end of line */
}

void delch_ex(mcurses_t *scr)
{
    if (!scr) return;
    _puts(scr, "\x1b[P");   /* DCH 1 – delete character */
}

void insch_ex(mcurses_t *scr, uint_fast8_t ch)
{
    if (!scr) return;
    _puts(scr, "\x1b[@");   /* ICH 1 – insert character cell */
    _emit_sgr(scr, scr->attr);
    _putc(scr, (uint8_t)ch);
    /* Return cursor to insertion point (ICH leaves it after the new char) */
    _emit_cup(scr, scr->cury, scr->curx);
}

/* =========================================================================
 * Input – nodelay / halfdelay
 * ========================================================================= */

void nodelay_ex(mcurses_t *scr, uint_fast8_t onoff)
{
    if (scr) scr->nodelay = onoff ? 1u : 0u;
}

void halfdelay_ex(mcurses_t *scr, uint_fast8_t tenths)
{
    if (scr) scr->halfdelay = tenths;
}

/* =========================================================================
 * Input – getch_ex
 * =========================================================================
 *
 * The RX pipe contains raw bytes from the terminal (UART RX ISR fills it).
 * VT escape sequences are parsed with vt_esc_add16() / vt_esc_match16()
 * from vterm.h.
 *
 * Timing model (cooperative scheduler friendly):
 *  - nodelay=1 : return MCURSES_KEY_ERR immediately if nothing available.
 *  - halfdelay  : busy-poll for up to halfdelay×100 ms (coarse; uses
 *                 _delay_ms on AVR).  Not protothread-safe on its own –
 *                 wrap in PROCESS_WAIT_EVENT_UNTIL for proper yielding.
 *  - blocking   : spin until a byte appears (only for simple apps where
 *                 a single process owns the terminal).
 */

uint_fast8_t getch_ex(mcurses_t *scr)
{
    if (!scr || !scr->rxpipe)
        return MCURSES_KEY_ERR;

    /* ---- Wait policy ---- */
    uint16_t ms_left = (uint16_t)scr->halfdelay * 100u;

    for (;;) {
        if (ipc_pipe_available(scr->rxpipe) == 0) {
            if (scr->nodelay)
                return MCURSES_KEY_ERR;
            if (scr->halfdelay) {
                if (ms_left == 0)
                    return MCURSES_KEY_ERR;
                _delay_ms(10);
                ms_left = (ms_left > 10u) ? (ms_left - 10u) : 0u;
            }
            /* else: fully blocking – keep spinning */
            continue;
        }
        break; /* byte available */
    }

    /* ---- Read first byte ---- */
    uint8_t c;
    ipc_pipe_read(scr->rxpipe, &c, 1);

    if (c != KEY_ESCAPE)
        return (uint_fast8_t)c;

    /* ---- Escape sequence handling (vterm.h) ---- */
    char esc_buf[VT_ESCAPE_BUFLEN];
    uint8_t esc_idx = 0;

    int8_t res = vt_esc_add16(esc_buf, &esc_idx, (rune16_t)c);
    if (res != 0)
        return (uint_fast8_t)c; /* error – return raw ESC */

    /* Collect sequence bytes with a short inter-byte timeout */
    for (;;) {
        /* Wait briefly for next byte (escape sequence is typically sent
         * as a burst; 20 ms is generous for local serial). */
        uint8_t wait = 20; /* 20 × 1 ms = 20 ms max */
        while (ipc_pipe_available(scr->rxpipe) == 0) {
            if (--wait == 0)
                goto seq_done;  /* timeout → partial / standalone ESC */
            _delay_ms(1);
        }

        uint8_t next;
        ipc_pipe_read(scr->rxpipe, &next, 1);

        res = vt_esc_add16(esc_buf, &esc_idx, (rune16_t)next);
        if (res < 0)
            goto seq_done; /* buffer full or invalid */
        if (res == 0)
            continue; /* accumulating */
        /* res > 0: sequence complete, try to match */
        break;
    }

seq_done:;
    rune16_t key = vt_esc_match16(esc_buf, esc_idx);
    if (key == 0)
        return KEY_ESCAPE; /* unrecognised sequence – return bare ESC */

    /* vt_esc_match16 returns rune16_t KEY_* codes (0x80-based from vterm.h).
     * These fit in uint8_t for the KEY_C(n) values defined there. */
    return (uint_fast8_t)(key & 0xFFu);
}

/* =========================================================================
 * Input – getnstr_ex
 * =========================================================================
 *
 * Mini line editor:
 *   - Printable ASCII  → echo and append to buffer.
 *   - KEY_BACKSPACE    → delete last character, erase on terminal.
 *   - KEY_ENTER / CR   → finish and NUL-terminate.
 *   - KEY_ESCAPE       → cancel; buf[0] = '\0'.
 */

void getnstr_ex(mcurses_t *scr, char *buf, uint_fast8_t maxlen)
{
    if (!scr || !buf || maxlen == 0)
        return;

    uint8_t len = 0;
    buf[0] = '\0';

    for (;;) {
        uint_fast8_t ch = getch_ex(scr);

        if (ch == MCURSES_KEY_ERR)
            continue;

        if (ch == KEY_ENTER || ch == KEY_CR) {
            buf[len] = '\0';
            return;
        }

        if (ch == KEY_ESCAPE) {
            buf[0] = '\0';
            return;
        }

        if (ch == KEY_BACKSPACE || ch == KEY_DELETE) {
            if (len > 0) {
                --len;
                buf[len] = '\0';
                /* Erase character on terminal: BS SPC BS */
                _putc(scr, '\b');
                _putc(scr, ' ');
                _putc(scr, '\b');
                if (scr->curx > 0)
                    --scr->curx;
            }
            continue;
        }

        /* Accept printable ASCII only (UTF-8 multibyte would need
         * extra buffering for the continuation bytes – keep it simple
         * for an embedded line editor). */
        if (ch >= 0x20u && ch < 0x7Fu && len < maxlen) {
            buf[len++] = (char)ch;
            buf[len]   = '\0';
            _emit_sgr(scr, scr->attr);
            _putc(scr, (uint8_t)ch);
            if (scr->curx < scr->cols - 1u)
                ++scr->curx;
        }
    }
}

/* =========================================================================
 * Box drawing
 * =========================================================================
 * Uses Unicode box-drawing characters from vterm.h (ACS_* constants),
 * encoded as UTF-8 and written directly to the TX pipe.
 */

/* Box-drawing character sets: single, double, round */
static const uint16_t _box_chars[][6] CC_PROGMEM = {
    /* single:  UL     UR     LL     LR     HLINE  VLINE  */
    { 0x250Cu, 0x2510u, 0x2514u, 0x2518u, 0x2500u, 0x2502u },
    /* double:  UL     UR     LL     LR     HLINE  VLINE  */
    { 0x2554u, 0x2557u, 0x255Au, 0x255Du, 0x2550u, 0x2551u },
    /* round:   UL     UR     LL     LR     HLINE  VLINE  */
    { 0x256Du, 0x256Eu, 0x2570u, 0x256Fu, 0x2500u, 0x2502u },
};

#define _BOX_UL 0
#define _BOX_UR 1
#define _BOX_LL 2
#define _BOX_LR 3
#define _BOX_HL 4
#define _BOX_VL 5

static void _emit_box_char(mcurses_t *scr, uint16_t codepoint)
{
    char buf[3];
    uint8_t n = utf8_fromrune16(buf, (rune16_t)codepoint);
    if (n)
        _puts_n(scr, buf, n);
}

void addbox_ex(mcurses_t *scr, uint_fast8_t y, uint_fast8_t x,
               uint_fast8_t h, uint_fast8_t w, uint_fast8_t style)
{
    if (!scr || h < 2 || w < 2) return;
    /* Style numbering matches tui_border_style_t: 0=none, 1=single, 2=double,
     * 3=round.  Map onto the single/double/round rows (index style-1). */
    if (style < 1u || style > 3u) return;   /* none / invalid: draw nothing */

    _emit_sgr(scr, scr->attr);

    const uint16_t *chars = _box_chars[style - 1u];
    uint16_t ul, ur, ll, lr, hl, vl;
#ifdef __AVR__
    ul = pgm_read_word(&chars[_BOX_UL]);
    ur = pgm_read_word(&chars[_BOX_UR]);
    ll = pgm_read_word(&chars[_BOX_LL]);
    lr = pgm_read_word(&chars[_BOX_LR]);
    hl = pgm_read_word(&chars[_BOX_HL]);
    vl = pgm_read_word(&chars[_BOX_VL]);
#else
    ul = chars[_BOX_UL];
    ur = chars[_BOX_UR];
    ll = chars[_BOX_LL];
    lr = chars[_BOX_LR];
    hl = chars[_BOX_HL];
    vl = chars[_BOX_VL];
#endif

    /* Top edge */
    _emit_cup(scr, (uint8_t)y, (uint8_t)x);
    _emit_box_char(scr, ul);
    for (uint8_t i = 0; i < w - 2u; i++)
        _emit_box_char(scr, hl);
    _emit_box_char(scr, ur);

    /* Side edges */
    for (uint8_t row = 1; row < h - 1u; row++) {
        _emit_cup(scr, (uint8_t)(y + row), (uint8_t)x);
        _emit_box_char(scr, vl);
        _emit_cup(scr, (uint8_t)(y + row), (uint8_t)(x + w - 1u));
        _emit_box_char(scr, vl);
    }

    /* Bottom edge */
    _emit_cup(scr, (uint8_t)(y + h - 1u), (uint8_t)x);
    _emit_box_char(scr, ll);
    for (uint8_t i = 0; i < w - 2u; i++)
        _emit_box_char(scr, hl);
    _emit_box_char(scr, lr);

    /* Leave the cursor in a defined, in-range state that matches the
     * terminal.  The box edges were emitted with bare CUP sequences that do
     * not update scr->curx/cury, so re-home via move_ex() (which emits CUP
     * and syncs the tracked cursor).  Park at the box's bottom-left corner. */
    move_ex(scr, (uint8_t)(y + h - 1u), (uint8_t)x);
}

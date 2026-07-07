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

/** Write a NUL-terminated RAM string into the TX pipe (single write). */
static void _puts(mcurses_t *scr, const char *s)
{
    _emit(scr, s, strlen(s));
}

/** Write a string literal (e.g. a vtout.h VT_* macro) into the TX pipe.
 *  The length is computed at compile time — no strlen, one _emit call. */
#define _puts_lit(scr, lit)  _emit((scr), (lit), sizeof(lit) - 1u)

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
    /* Compose the whole sequence in a stack buffer and emit it with ONE
     * pipe write.  Worst case: ESC [ 0 ;1;2;4;5;7;8 ;3X ;4X m = 22 bytes. */
    char    buf[22];
    uint8_t n = 0;

    if (attr == scr->last_attr)
        return;

    buf[n++] = '\x1b';
    buf[n++] = '[';
    buf[n++] = '0';   /* reset first */

    /* Style bits */
    if (attr & MCURSES_ATTR_BOLD)      { buf[n++] = ';'; buf[n++] = '1'; }
    if (attr & MCURSES_ATTR_DIM)       { buf[n++] = ';'; buf[n++] = '2'; }
    if (attr & MCURSES_ATTR_UNDERLINE) { buf[n++] = ';'; buf[n++] = '4'; }
    if (attr & MCURSES_ATTR_BLINK)     { buf[n++] = ';'; buf[n++] = '5'; }
    if (attr & MCURSES_ATTR_REVERSE)   { buf[n++] = ';'; buf[n++] = '7'; }
    if (attr & MCURSES_ATTR_INVISIBLE) { buf[n++] = ';'; buf[n++] = '8'; }

    /* Foreground colour (index 0 = default = skip); maps 1-8 → 30-37 */
    uint8_t fg = (uint8_t)((attr & MCURSES_FG_MASK) >> MCURSES_FG_SHIFT);
    if (fg > 0) {
        buf[n++] = ';';
        buf[n++] = '3';
        buf[n++] = (char)('0' + (fg - 1));
    }

    /* Background colour; maps 1-8 → 40-47 */
    uint8_t bg = (uint8_t)((attr & MCURSES_BG_MASK) >> MCURSES_BG_SHIFT);
    if (bg > 0) {
        buf[n++] = ';';
        buf[n++] = '4';
        buf[n++] = (char)('0' + (bg - 1));
    }

    buf[n++] = 'm';
    _emit(scr, buf, n);
    scr->last_attr = attr;
}

/* =========================================================================
 * Internal helpers – keyboard event capture (ansi.h handler bridge)
 * =========================================================================
 * The embedded ansi_parser_t classifies RX bytes; these handlers translate
 * each recognised action into a single pending key on the mcurses_t.
 * getch_ex() stops feeding bytes as soon as a key is pending, so no key is
 * ever lost — remaining bytes stay in the RX pipe for the next call.
 */

static void _key_set(mcurses_t *scr, uint8_t code, uint8_t mods)
{
    if (scr->key_pending)
        return;               /* keep the first event of this byte */
    scr->key_code    = code;
    scr->key_mods    = mods;
    scr->key_pending = 1u;
}

static void _key_on_print(void *ctx, uint32_t cp)
{
    /* getch_ex() returns a single byte: pass ASCII and Latin-1 through,
     * fold wider runes (and the unused C1 range that would collide with
     * KEY_* codes) to '?'.  Apps needing full rune input should feed the
     * RX bytes to their own ansi_parser_t instead. */
    if (cp >= 0x100u || (cp >= 0x80u && cp <= 0x9Fu))
        cp = ASC_UNKNOWN;
    _key_set((mcurses_t *)ctx, (uint8_t)cp, 0u);
}

static void _key_on_execute(void *ctx, uint8_t ctrl)
{
    /* C0 controls: CR, TAB, BS, … and the bare ESC delivered by flush. */
    _key_set((mcurses_t *)ctx, ctrl, 0u);
}

static void _key_on_csi(void *ctx, const ansi_csi_t *csi)
{
    uint8_t  mods = 0;
    uint16_t key  = ansi_key_from_csi(csi, &mods);
    if (key)
        _key_set((mcurses_t *)ctx, (uint8_t)key, mods);
    /* non-key CSI (cursor reports, mode acks) are ignored by getch */
}

static void _key_on_esc(void *ctx, uint8_t intermediate, uint8_t final)
{
    if (intermediate == 'O') {          /* SS3: app-cursor arrows, F1-F4 */
        uint8_t  mods = 0;
        uint16_t key  = ansi_key_from_ss3(final, &mods);
        if (key)
            _key_set((mcurses_t *)ctx, (uint8_t)key, mods);
    }
}

static const ansi_handlers_t _key_handlers = {
    _key_on_print,
    _key_on_execute,
    _key_on_csi,
    _key_on_esc,
    /* on_osc */ 0, /* dcs_hook */ 0, /* dcs_put */ 0, /* dcs_unhook */ 0,
};

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
    /* terminal cursor position unknown until the first CUP */
    scr->cursor_dirty = 1;

    ansi_parser_init(&scr->key_parser, &_key_handlers, scr,
                     /* no OSC/DCS string buffer needed for keyboard */
                     (char *)0, 0u);
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

    /* Switch to alternate screen, reset attributes, clear, home.
     * One compile-time-concatenated literal → one pipe write. */
    _puts_lit(scr, VT_DECSET_ALT_SCREEN   /* DECSET alt screen   */
                   VT_SGR_RESET0          /* SGR reset           */
                   VT_ED_ALL              /* ED – erase display  */
                   VT_HOME                /* CUP home            */
                   VT_DECSET_DECTCEM);    /* DECTCEM show cursor */
    scr->last_attr    = MCURSES_ATTR_NORMAL;
    scr->cursor_dirty = 0;   /* terminal is homed and in sync */

    return 1;
}

void endwin_ex(mcurses_t *scr)
{
    if (!scr || !scr->initialized)
        return;

    _emit_sgr(scr, MCURSES_ATTR_NORMAL);
    _puts_lit(scr, VT_DECSET_DECTCEM      /* show cursor         */
                   VT_DECSTBM_FULL        /* reset scroll region */
                   VT_DECRST_ALT_SCREEN); /* leave alt screen    */

    scr->initialized = 0;
}

/* =========================================================================
 * Cursor & attributes
 * ========================================================================= */

void move_ex(mcurses_t *scr, uint_fast8_t y, uint_fast8_t x)
{
    if (!scr) return;

    /* Lazy cursor: while the tracked position is known to match the
     * terminal (cursor_dirty == 0) we can skip or shorten the move.
     * A full CUP is 6-8 bytes; at UART speeds every byte is latency. */
    if (!scr->cursor_dirty && y == scr->cury) {
        if (x == scr->curx)
            return;                          /* already there: 0 bytes */

        if (x == 0u) {
            _putc(scr, '\r');                /* CR: 1 byte             */
            scr->curx = 0;
            return;
        }
        if (x < scr->curx && (uint_fast8_t)(scr->curx - x) <= 3u) {
            _emit(scr, "\b\b\b",             /* BS run: 1-3 bytes      */
                  (size_t)(scr->curx - x));
            scr->curx = (uint8_t)x;
            return;
        }
    }

    scr->cury = (uint8_t)y;
    scr->curx = (uint8_t)x;
    _emit_cup(scr, (uint8_t)y, (uint8_t)x);
    scr->cursor_dirty = 0;
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
        _puts_lit(scr, VT_DECRST_DECTCEM);                        /* hide */
    else if (vis == 2)
        _puts_lit(scr, VT_DECSET_DECTCEM VT_CURSOR_BLOCK_BLINK);
    else
        _puts_lit(scr, VT_DECSET_DECTCEM VT_CURSOR_BLOCK_STEADY);
}

/* =========================================================================
 * Output
 * ========================================================================= */

/**
 * Advance the tracked cursor by one display column.  When the terminal's
 * autowrap takes over at the right margin the real cursor position becomes
 * uncertain (pending-wrap semantics differ between terminals), so the
 * tracked state is marked dirty and the next move_ex() emits a full CUP.
 */
static void _advance_col(mcurses_t *scr)
{
    if (++scr->curx >= scr->cols) {
        scr->curx = 0;
        if (scr->cury < scr->rows - 1u)
            ++scr->cury;
        scr->cursor_dirty = 1;
    }
}

void addch_ex(mcurses_t *scr, uint_fast8_t ch)
{
    if (!scr) return;
    _emit_sgr(scr, scr->attr);
    _putc(scr, (uint8_t)ch);
    _advance_col(scr);
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
    _advance_col(scr);
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
        _advance_col(scr);
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

        _advance_col(scr);
    }
}

void addstr_P_ex(mcurses_t *scr, const char *s_P)
{
    if (!scr || !s_P) return;
    _emit_sgr(scr, scr->attr);

#ifdef __AVR__
    /* On AVR the flash bytes are already UTF-8 encoded.  Copy them out of
     * PROGMEM into a small stack chunk and emit each chunk with a single
     * pipe write (instead of one write per byte).  Display columns are
     * tracked by counting lead bytes only. */
    char    chunk[16];
    uint8_t n = 0;
    uint8_t c;
    while ((c = pgm_read_byte(s_P)) != 0) {
        chunk[n++] = (char)c;
        /* Only count display column on lead bytes, not continuation bytes */
        if ((c & 0xC0u) != 0x80u)
            _advance_col(scr);
        if (n == (uint8_t)sizeof(chunk)) {
            _emit(scr, chunk, n);
            n = 0;
        }
        ++s_P;
    }
    if (n)
        _emit(scr, chunk, n);
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
    /* DECSTBM homes the terminal cursor; the tracked position is stale. */
    scr->cursor_dirty = 1;
}

void deleteln_ex(mcurses_t *scr)
{
    if (!scr) return;
    /* Move to start of current line, then DL 1 */
    _emit_cup(scr, scr->cury, 0);
    _puts_lit(scr, VT_CSI "M");   /* DL – delete line */
    scr->cursor_dirty = 1;        /* terminal now at col 0, tracked isn't */
}

void insertln_ex(mcurses_t *scr)
{
    if (!scr) return;
    _emit_cup(scr, scr->cury, 0);
    _puts_lit(scr, VT_CSI "L");   /* IL – insert line */
    scr->cursor_dirty = 1;
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
    scr->cursor_dirty = 0;        /* restored to the tracked position */
}

void clear_ex(mcurses_t *scr)
{
    if (!scr) return;
    _puts_lit(scr, VT_ED_ALL      /* ED – erase all */
                   VT_HOME);      /* CUP home       */
    scr->cury = 0;
    scr->curx = 0;
    scr->cursor_dirty = 0;
}

void clrtobot_ex(mcurses_t *scr)
{
    if (!scr) return;
    _puts_lit(scr, VT_ED_BELOW);  /* ED – erase from cursor to end */
}

void clrtoeol_ex(mcurses_t *scr)
{
    if (!scr) return;
    _puts_lit(scr, VT_EL_RIGHT);  /* EL – erase from cursor to end of line */
}

void delch_ex(mcurses_t *scr)
{
    if (!scr) return;
    _puts_lit(scr, VT_CSI "P");   /* DCH 1 – delete character */
}

void insch_ex(mcurses_t *scr, uint_fast8_t ch)
{
    if (!scr) return;
    _puts_lit(scr, VT_CSI "@");   /* ICH 1 – insert character cell */
    _emit_sgr(scr, scr->attr);
    _putc(scr, (uint8_t)ch);
    /* Return cursor to insertion point (ICH leaves it after the new char) */
    _emit_cup(scr, scr->cury, scr->curx);
    scr->cursor_dirty = 0;
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
 * Bytes are fed to the embedded ansi.h state machine (scr->key_parser);
 * its handlers (top of this file) capture one decoded key per call.
 *
 * Because the parser keeps its own state across calls, a sequence split
 * over several getch_ex() invocations still decodes correctly — nothing
 * is buffered on the stack and nothing is lost.
 *
 * Timing model (cooperative scheduler friendly):
 *  - While bytes are available, the call NEVER waits: O(1) work per byte.
 *  - Only when the pipe drains in the middle of an escape sequence does
 *    the call pause up to MCURSES_ESC_TIMEOUT_MS for the rest of the
 *    burst; on timeout a lone ESC keypress is delivered (parser flush).
 *  - nodelay=1 : return MCURSES_KEY_ERR immediately when no key decodes.
 *  - halfdelay  : busy-poll for up to halfdelay×100 ms (coarse; uses
 *                 _delay_ms on AVR).  Not protothread-safe on its own –
 *                 wrap in PROCESS_WAIT_EVENT_UNTIL for proper yielding.
 *  - blocking   : spin until a key arrives (only for simple apps where
 *                 a single process owns the terminal).
 */

uint_fast8_t getch_ex(mcurses_t *scr)
{
    if (!scr || !scr->rxpipe)
        return MCURSES_KEY_ERR;

    uint16_t ms_left = (uint16_t)scr->halfdelay * 100u;

    for (;;) {
        /* ---- Feed available bytes until a key pops out ---- */
        while (!scr->key_pending && ipc_pipe_available(scr->rxpipe) > 0) {
            uint8_t b;
            if (ipc_pipe_read(scr->rxpipe, &b, 1) == 0)
                break;
            ansi_parse(&scr->key_parser, b);
        }

        if (scr->key_pending) {
            scr->key_pending = 0;
            return (uint_fast8_t)scr->key_code;
        }

        /* ---- Pipe drained mid-sequence: short inter-byte timeout ---- */
        if (!ansi_parser_idle(&scr->key_parser)) {
            uint8_t wait = MCURSES_ESC_TIMEOUT_MS;
            while (ipc_pipe_available(scr->rxpipe) == 0 && wait > 0) {
                _delay_ms(1);
                --wait;
            }
            if (ipc_pipe_available(scr->rxpipe) > 0)
                continue;                        /* burst continues */

            /* Timeout: a bare ESC keypress is delivered via the flush
             * (on_execute(0x1B)); any other stalled partial sequence is
             * not keyboard input — drop it so it cannot eat later keys. */
            ansi_parser_flush(&scr->key_parser);
            if (!ansi_parser_idle(&scr->key_parser))
                ansi_parser_reset(&scr->key_parser);
            if (scr->key_pending) {
                scr->key_pending = 0;
                return (uint_fast8_t)scr->key_code;
            }
        }

        /* ---- No key available: apply the wait policy ---- */
        if (scr->nodelay)
            return MCURSES_KEY_ERR;

        if (scr->halfdelay) {
            if (ms_left == 0)
                return MCURSES_KEY_ERR;
            _delay_ms(10);
            ms_left = (ms_left > 10u) ? (uint16_t)(ms_left - 10u) : 0u;
        }
        /* else: fully blocking – keep spinning */
    }
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
 * Uses Unicode box-drawing characters from vtkeys.h (ACS_* constants),
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

/**
 * Emit @p count repetitions of @p codepoint.  The rune is UTF-8 encoded
 * ONCE and replicated into a stack chunk, so a full horizontal box edge
 * costs a handful of pipe writes instead of one encode+write per cell.
 */
static void _emit_box_run(mcurses_t *scr, uint16_t codepoint, uint8_t count)
{
    char    seq[3];
    uint8_t sl = utf8_fromrune16(seq, (rune16_t)codepoint);
    char    chunk[30];
    uint8_t per, i;

    if (!sl || !count)
        return;

    per = (uint8_t)(sizeof(chunk) / sl);   /* cells per chunk */
    if (per > count)
        per = count;

    for (i = 0; i < per; i++) {
        chunk[i * sl] = seq[0];
        if (sl > 1) chunk[i * sl + 1] = seq[1];
        if (sl > 2) chunk[i * sl + 2] = seq[2];
    }

    while (count) {
        uint8_t take = (count < per) ? count : per;
        _emit(scr, chunk, (size_t)take * sl);
        count = (uint8_t)(count - take);
    }
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
    _emit_box_run(scr, hl, (uint8_t)(w - 2u));
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
    _emit_box_run(scr, hl, (uint8_t)(w - 2u));
    _emit_box_char(scr, lr);

    /* Leave the cursor in a defined, in-range state that matches the
     * terminal.  The box edges were emitted with bare CUP sequences that do
     * not update scr->curx/cury, so mark the tracked state dirty and re-home
     * via move_ex() (which then emits a real CUP and re-syncs).  Park at the
     * box's bottom-left corner. */
    scr->cursor_dirty = 1;
    move_ex(scr, (uint8_t)(y + h - 1u), (uint8_t)x);
}

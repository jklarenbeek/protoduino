/**
 * @file mcurses.h
 * @brief Lean mcurses terminal library for protoduino / AVR embedded systems.
 *
 * Design principles
 * =================
 *  - Zero globals: all state lives in mcurses_t, caller-allocated.
 *  - Multiple instances: each screen has its own mcurses_t.
 *  - I/O via ipc_pipe_t: decoupled from any physical UART/serial device.
 *    Connect the pipes to whatever transport you like (UART ISR, USB-CDC, …).
 *  - Minimal flash/SRAM footprint; targets Arduino Uno (ATmega328P).
 *  - Uses utf8.h / utf8_iter.h / vterm.h / vtout.h where possible (DRY).
 *
 * Quick-start
 * ===========
 *
 *   // 1. Allocate buffers
 *   static uint8_t  tx_buf[64];
 *   static uint8_t  rx_buf[16];
 *   static ipc_pipe_t tx_pipe, rx_pipe;
 *   static mcurses_t  scr;
 *
 *   // 2. Wire up pipes (wake callbacks tie into your scheduler)
 *   ipc_pipe_init(&tx_pipe, tx_buf, sizeof(tx_buf), tx_wake_cb, NULL);
 *   ipc_pipe_init(&rx_pipe, rx_buf, sizeof(rx_buf), NULL,       NULL);
 *
 *   // 3. Init
 *   mcurses_init(&scr, &tx_pipe, &rx_pipe, 24, 80);
 *   if (initscr_ex(&scr)) { ... ready ... }
 *
 *   // 4. Use the API through the mcurses_t pointer:
 *   move_ex(&scr, 0, 0);
 *   addstr_ex(&scr, "Hello, world!");
 *   refresh_ex(&scr);
 *
 *   // 5. Optionally use the single-instance convenience wrappers
 *   //    (mcurses_default must be set; see MCURSES_USE_DEFAULT_INSTANCE).
 */

#ifndef __MCURSES_H__
#define __MCURSES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <protoduino.h>
#include "../../sys/ipc.h"
#include "vterm.h"
#include "vtout.h"
#include "utf8.h"
#include "utf8_iter.h"

#ifdef __AVR__
#  include <avr/pgmspace.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Attribute flags  (fits in uint16_t, same width as ncurses attr_t)
 * ========================================================================= */

#define MCURSES_ATTR_NORMAL         0x0000u
#define MCURSES_ATTR_BOLD           0x0001u  /* SGR 1  */
#define MCURSES_ATTR_DIM            0x0002u  /* SGR 2  */
#define MCURSES_ATTR_UNDERLINE      0x0004u  /* SGR 4  */
#define MCURSES_ATTR_BLINK          0x0008u  /* SGR 5  */
#define MCURSES_ATTR_REVERSE        0x0010u  /* SGR 7  */
#define MCURSES_ATTR_INVISIBLE      0x0020u  /* SGR 8  */

/* Foreground colour: bits [8..11]  (value 0 = default) */
#define MCURSES_FG_SHIFT            8u
#define MCURSES_FG_MASK             0x0F00u
#define MCURSES_FG(c)               (((uint16_t)(c) & 0x0Fu) << MCURSES_FG_SHIFT)
#define MCURSES_FG_DEFAULT          MCURSES_FG(0)
#define MCURSES_FG_BLACK            MCURSES_FG(1)
#define MCURSES_FG_RED              MCURSES_FG(2)
#define MCURSES_FG_GREEN            MCURSES_FG(3)
#define MCURSES_FG_YELLOW           MCURSES_FG(4)
#define MCURSES_FG_BLUE             MCURSES_FG(5)
#define MCURSES_FG_MAGENTA          MCURSES_FG(6)
#define MCURSES_FG_CYAN             MCURSES_FG(7)
#define MCURSES_FG_WHITE            MCURSES_FG(8)

/* Background colour: bits [12..15] */
#define MCURSES_BG_SHIFT            12u
#define MCURSES_BG_MASK             0xF000u
#define MCURSES_BG(c)               (((uint16_t)(c) & 0x0Fu) << MCURSES_BG_SHIFT)
#define MCURSES_BG_DEFAULT          MCURSES_BG(0)
#define MCURSES_BG_BLACK            MCURSES_BG(1)
#define MCURSES_BG_RED              MCURSES_BG(2)
#define MCURSES_BG_GREEN            MCURSES_BG(3)
#define MCURSES_BG_YELLOW           MCURSES_BG(4)
#define MCURSES_BG_BLUE             MCURSES_BG(5)
#define MCURSES_BG_MAGENTA          MCURSES_BG(6)
#define MCURSES_BG_CYAN             MCURSES_BG(7)
#define MCURSES_BG_WHITE            MCURSES_BG(8)

/* =========================================================================
 * nodelay / halfdelay constants
 * ========================================================================= */

#define MCURSES_NODELAY_OFF         0u
#define MCURSES_NODELAY_ON          1u

/* halfdelay: number of 100 ms tenths to wait; 0 = blocking */
#define MCURSES_HALFDELAY_MAX       255u

/* getch return codes */
#define MCURSES_KEY_ERR             0xFFu   /* no key (nodelay/timeout) */

/* =========================================================================
 * mcurses_t  –  all instance state in one caller-allocated struct
 * =========================================================================
 *
 * Callers must NOT access fields directly; use the API functions.
 * The struct is declared here (not opaque) so it can live on the stack
 * or in a static array without dynamic allocation.
 */
typedef struct mcurses {
    /* ---- I/O pipes ---- */
    ipc_pipe_t *txpipe;         /**< output to terminal              */
    ipc_pipe_t *rxpipe;         /**< input  from terminal            */

    /* ---- Terminal dimensions ---- */
    uint8_t     rows;           /**< number of rows    (e.g. 24)     */
    uint8_t     cols;           /**< number of columns (e.g. 80)     */

    /* ---- Cursor ---- */
    uint8_t     cury;           /**< current row    (0-based)        */
    uint8_t     curx;           /**< current column (0-based)        */

    /* ---- Scroll region ---- */
    uint8_t     scroll_top;     /**< top    of scroll region (0-based) */
    uint8_t     scroll_bot;     /**< bottom of scroll region (0-based) */

    /* ---- Attributes ---- */
    uint16_t    attr;           /**< current SGR attribute set       */
    uint16_t    last_attr;      /**< last attribute actually sent     */

    /* ---- Input timing ---- */
    uint8_t     nodelay;        /**< non-zero = non-blocking getch   */
    uint8_t     halfdelay;      /**< 0 = block, N = N*100 ms timeout */

    /* ---- State flags ---- */
    uint8_t     initialized : 1;
    uint8_t     cursor_vis  : 2; /**< 0=invis 1=normal 2=very vis   */

    /* ---- TX back-pressure / loss tracking ---- */
    uint16_t    tx_dropped;      /**< Saturating count of TX bytes dropped
                                  *   because the pipe was full and no drain
                                  *   hook was available (0 = none lost).     */
    void      (*tx_drain)(void *ctx); /**< Optional: invoked when the TX pipe
                                  *   is full so the transport can make space
                                  *   (e.g. flush a polled UART).  NULL = drop
                                  *   + count instead of block.               */
    void       *tx_drain_ctx;    /**< Context passed to tx_drain.             */
} mcurses_t;

/* =========================================================================
 * Lifecycle
 * ========================================================================= */

/**
 * @brief Populate an mcurses_t with pipe references and screen dimensions.
 *
 * Call this before initscr_ex().  txpipe must be pre-initialised via
 * ipc_pipe_init().  rxpipe may be NULL if input is not required.
 *
 * @param scr   Caller-allocated instance to initialise.
 * @param tx    Output pipe (terminal TX direction).
 * @param rx    Input pipe  (terminal RX direction), or NULL.
 * @param rows  Screen height in character rows.
 * @param cols  Screen width  in character columns.
 */
void mcurses_init(mcurses_t *scr,
                  ipc_pipe_t *tx, ipc_pipe_t *rx,
                  uint8_t rows, uint8_t cols);

/**
 * @brief Send terminal setup sequence and reset internal state.
 *
 * Equivalent to ncurses initscr().
 *
 * @return Non-zero on success, 0 if scr is invalid.
 */
uint_fast8_t initscr_ex(mcurses_t *scr);

/**
 * @brief Restore terminal to normal mode and clean up.
 */
void endwin_ex(mcurses_t *scr);

/**
 * @brief Install a TX back-pressure drain hook.
 *
 * When an output call cannot fit all its bytes into the TX pipe, the hook is
 * invoked (up to a few times) to give the transport a chance to make space —
 * e.g. transmit queued bytes on a polled UART, or run a synchronous
 * pipe→UART drain.  With a hook installed, output is never lost.  Without
 * one, the shortfall is counted in `tx_dropped` (visible, not silent) and
 * the call returns rather than blocking.
 *
 * @param scr  Screen instance.
 * @param cb   Drain callback, or NULL to disable (drop + count).
 * @param ctx  Opaque pointer passed to @p cb.
 */
void mcurses_set_drain(mcurses_t *scr, void (*cb)(void *), void *ctx);

/* =========================================================================
 * Cursor & attributes
 * ========================================================================= */

/** Move cursor to (y, x); top-left is (0,0). */
void move_ex(mcurses_t *scr, uint_fast8_t y, uint_fast8_t x);

/** Set SGR attribute(s); use MCURSES_ATTR_* and MCURSES_FG/BG macros. */
void attrset_ex(mcurses_t *scr, uint_fast16_t attr);

/** Set cursor visibility: 0=invisible, 1=normal, 2=very visible. */
void curs_set_ex(mcurses_t *scr, uint_fast8_t vis);

/* =========================================================================
 * Output
 * ========================================================================= */

/** Write a single ASCII character at the current cursor position. */
void addch_ex(mcurses_t *scr, uint_fast8_t ch);

/**
 * @brief Write a single UTF-8 encoded character at the current cursor position.
 *
 * Zero-copy: the UTF-8 bytes at `utf8` are written directly to the TX pipe
 * without intermediate decode/re-encode.  The caller retains ownership of
 * the pointer – no data is copied beyond the pipe write.
 *
 * @param scr   Screen instance.
 * @param utf8  Pointer to the first byte of the UTF-8 sequence.
 * @param len   Byte length of the sequence (1–4).  If 0, the length is
 *              auto-detected from the lead byte.
 */
void addch_utf8_ex(mcurses_t *scr, const char *utf8, uint_fast8_t len);

/** Write a null-terminated RAM UTF-8 string at the current cursor position.
 *  Bytes are streamed directly to the TX pipe (zero-copy where possible). */
void addstr_ex(mcurses_t *scr, const char *s);

/**
 * @brief Write a bounded RAM UTF-8 string (pointer + byte length).
 *
 * The byte range `s[0..byte_len-1]` is written directly to the TX pipe.
 * The range must contain complete UTF-8 sequences (no partial multi-byte
 * characters at the boundary).
 *
 * @param scr       Screen instance.
 * @param s         Pointer to UTF-8 byte range (need not be null-terminated).
 * @param byte_len  Number of bytes to write.
 */
void addnstr_ex(mcurses_t *scr, const char *s, uint_fast8_t byte_len);

/** Write a PROGMEM string at the current cursor position (AVR flash).
 *  On AVR the raw flash bytes are UTF-8 encoded; they are streamed directly. */
void addstr_P_ex(mcurses_t *scr, const char *s_P);

/** Flush any pending output from the TX pipe to the transport layer. */
void refresh_ex(mcurses_t *scr);

/**
 * @brief Draw a box (border rectangle) using Unicode box-drawing characters.
 *
 * The border is drawn at (y,x) with dimensions (h,w).  Interior cells are
 * left untouched.  After the call the cursor is parked at the box's
 * bottom-left corner (row y+h-1, column x) and the tracked cursor state
 * matches the terminal.
 *
 * @param scr    Screen instance.
 * @param y      Top row (0-based).
 * @param x      Left column (0-based).
 * @param h      Height in rows (including border, minimum 2).
 * @param w      Width in columns (including border, minimum 2).
 * @param style  Border style, matching tui_border_style_t so the value is
 *               portable across the mcurses and TUI APIs:
 *                 0 = none (nothing drawn),
 *                 1 = single ┌─┐│ │└─┘,
 *                 2 = double ╔═╗║ ║╚═╝,
 *                 3 = round  ╭─╮│ │╰─╯.
 */
void addbox_ex(mcurses_t *scr, uint_fast8_t y, uint_fast8_t x,
               uint_fast8_t h, uint_fast8_t w, uint_fast8_t style);

/* =========================================================================
 * Screen / line editing
 * ========================================================================= */

/** Set scrolling region [top..bot] (0-based row indices). */
void setscrreg_ex(mcurses_t *scr, uint_fast8_t top, uint_fast8_t bot);

/** Delete line at current row; lines below scroll up. */
void deleteln_ex(mcurses_t *scr);

/** Insert blank line at current row; lines below scroll down. */
void insertln_ex(mcurses_t *scr);

/** Scroll the scroll region up by one line. */
void scroll_ex(mcurses_t *scr);

/** Clear the entire screen and home the cursor. */
void clear_ex(mcurses_t *scr);

/** Clear from current row to bottom of screen. */
void clrtobot_ex(mcurses_t *scr);

/** Clear from current column to end of line. */
void clrtoeol_ex(mcurses_t *scr);

/** Delete character at current position; line shifts left. */
void delch_ex(mcurses_t *scr);

/** Insert character at current position; rest of line shifts right. */
void insch_ex(mcurses_t *scr, uint_fast8_t ch);

/* =========================================================================
 * Input
 * ========================================================================= */

/** Enable (1) or disable (0) non-blocking getch. */
void nodelay_ex(mcurses_t *scr, uint_fast8_t onoff);

/**
 * @brief Set half-delay: getch blocks for at most tenths×100 ms.
 *        Pass 0 to revert to fully blocking.
 */
void halfdelay_ex(mcurses_t *scr, uint_fast8_t tenths);

/**
 * @brief Read one key from the RX pipe.
 *
 * Translates VT escape sequences to KEY_* constants (vterm.h).
 * Returns MCURSES_KEY_ERR when nodelay is on and no byte is available,
 * or when halfdelay expires.
 *
 * @return KEY_* constant or raw ASCII byte.
 */
uint_fast8_t getch_ex(mcurses_t *scr);

/**
 * @brief Read a string into buf with simple line-editing.
 *
 * Supports: printable chars, KEY_BACKSPACE, KEY_ENTER to finish,
 * KEY_ESCAPE to cancel (buf[0]='\0').  Echoes characters to tx.
 *
 * @param scr     Screen instance.
 * @param buf     Destination buffer (maxlen+1 bytes minimum).
 * @param maxlen  Maximum characters to accept (excluding NUL).
 */
void getnstr_ex(mcurses_t *scr, char *buf, uint_fast8_t maxlen);

/* =========================================================================
 * Optional single-instance convenience layer
 * =========================================================================
 *
 * Define MCURSES_USE_DEFAULT_INSTANCE before including this header to
 * compile in the thin wrappers that match the classic mcurses API.
 * You must also define / declare:
 *
 *   extern mcurses_t *mcurses_default;
 *
 * and point it at your instance before calling initscr().
 */
#ifdef MCURSES_USE_DEFAULT_INSTANCE

extern mcurses_t *mcurses_default;

/* Cursor position read-back (mirrors classic mcurses_cury/mcurses_curx) */
#define mcurses_cury  (mcurses_default->cury)
#define mcurses_curx  (mcurses_default->curx)

static inline uint_fast8_t  initscr(void)                        { return initscr_ex(mcurses_default); }
static inline void          endwin(void)                          { endwin_ex(mcurses_default); }
static inline void          move(uint_fast8_t y, uint_fast8_t x) { move_ex(mcurses_default, y, x); }
static inline void          attrset(uint_fast16_t a)              { attrset_ex(mcurses_default, a); }
static inline void          curs_set(uint_fast8_t v)              { curs_set_ex(mcurses_default, v); }
static inline void          addch(uint_fast8_t c)                 { addch_ex(mcurses_default, c); }
static inline void          addch_utf8(const char *u, uint_fast8_t n) { addch_utf8_ex(mcurses_default, u, n); }
static inline void          addstr(const char *s)                 { addstr_ex(mcurses_default, s); }
static inline void          addnstr(const char *s, uint_fast8_t n){ addnstr_ex(mcurses_default, s, n); }
static inline void          addstr_P(const char *s)               { addstr_P_ex(mcurses_default, s); }
static inline void          refresh(void)                         { refresh_ex(mcurses_default); }
static inline void          setscrreg(uint_fast8_t t, uint_fast8_t b) { setscrreg_ex(mcurses_default, t, b); }
static inline void          deleteln(void)                        { deleteln_ex(mcurses_default); }
static inline void          insertln(void)                        { insertln_ex(mcurses_default); }
static inline void          scroll(void)                          { scroll_ex(mcurses_default); }
static inline void          clear(void)                           { clear_ex(mcurses_default); }
static inline void          clrtobot(void)                        { clrtobot_ex(mcurses_default); }
static inline void          clrtoeol(void)                        { clrtoeol_ex(mcurses_default); }
static inline void          delch(void)                           { delch_ex(mcurses_default); }
static inline void          insch(uint_fast8_t c)                 { insch_ex(mcurses_default, c); }
static inline void          nodelay(uint_fast8_t v)               { nodelay_ex(mcurses_default, v); }
static inline void          halfdelay(uint_fast8_t t)             { halfdelay_ex(mcurses_default, t); }
static inline uint_fast8_t  getch(void)                           { return getch_ex(mcurses_default); }
static inline void          getnstr(char *s, uint_fast8_t n)      { getnstr_ex(mcurses_default, s, n); }

#endif /* MCURSES_USE_DEFAULT_INSTANCE */

/* =========================================================================
 * Macros  (instance-aware versions)
 * ========================================================================= */

#define erase_ex(scr)                  clear_ex(scr)
#define mvaddch_ex(scr,y,x,c)          move_ex((scr),(y),(x)), addch_ex((scr),(c))
#define mvaddch_utf8_ex(scr,y,x,u,n)   move_ex((scr),(y),(x)), addch_utf8_ex((scr),(u),(n))
#define mvaddstr_ex(scr,y,x,s)         move_ex((scr),(y),(x)), addstr_ex((scr),(s))
#define mvaddnstr_ex(scr,y,x,s,n)      move_ex((scr),(y),(x)), addnstr_ex((scr),(s),(n))
#define mvaddstr_P_ex(scr,y,x,s)       move_ex((scr),(y),(x)), addstr_P_ex((scr),(s))
#define mvinsch_ex(scr,y,x,c)          move_ex((scr),(y),(x)), insch_ex((scr),(c))
#define mvdelch_ex(scr,y,x)            move_ex((scr),(y),(x)), delch_ex((scr))
#define mvgetnstr_ex(scr,y,x,s,n)      move_ex((scr),(y),(x)), getnstr_ex((scr),(s),(n))
#define mvaddbox_ex(scr,y,x,h,w,s)     addbox_ex((scr),(y),(x),(h),(w),(s))
#define getyx_ex(scr,y,x)              ((y) = (scr)->cury, (x) = (scr)->curx)

/* Classic (single-instance) macros – available when default instance is used */
#ifdef MCURSES_USE_DEFAULT_INSTANCE
#  define erase()                       clear()
#  define mvaddch(y,x,c)                move((y),(x)), addch((c))
#  define mvaddch_utf8(y,x,u,n)         move((y),(x)), addch_utf8((u),(n))
#  define mvaddstr(y,x,s)               move((y),(x)), addstr((s))
#  define mvaddnstr(y,x,s,n)            move((y),(x)), addnstr((s),(n))
#  define mvaddstr_P(y,x,s)             move((y),(x)), addstr_P((s))
#  define mvinsch(y,x,c)                move((y),(x)), insch((c))
#  define mvdelch(y,x)                  move((y),(x)), delch()
#  define mvgetnstr(y,x,s,n)            move((y),(x)), getnstr((s),(n))
#  define mvaddbox(y,x,h,w,s)           addbox_ex(mcurses_default,(y),(x),(h),(w),(s))
#  define getyx(y,x)                    ((y) = mcurses_cury, (x) = mcurses_curx)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __MCURSES_H__ */
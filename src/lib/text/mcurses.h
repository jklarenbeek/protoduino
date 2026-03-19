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

/** Write a single character at the current cursor position. */
void addch_ex(mcurses_t *scr, uint_fast8_t ch);

/** Write a RAM string at the current cursor position. */
void addstr_ex(mcurses_t *scr, const char *s);

/** Write a PROGMEM string at the current cursor position (AVR flash). */
void addstr_P_ex(mcurses_t *scr, const char *s_P);

/** Flush any pending output from the TX pipe to the transport layer. */
void refresh_ex(mcurses_t *scr);

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
static inline void          addstr(const char *s)                 { addstr_ex(mcurses_default, s); }
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
#define mvaddstr_ex(scr,y,x,s)         move_ex((scr),(y),(x)), addstr_ex((scr),(s))
#define mvaddstr_P_ex(scr,y,x,s)       move_ex((scr),(y),(x)), addstr_P_ex((scr),(s))
#define mvinsch_ex(scr,y,x,c)          move_ex((scr),(y),(x)), insch_ex((scr),(c))
#define mvdelch_ex(scr,y,x)            move_ex((scr),(y),(x)), delch_ex((scr))
#define mvgetnstr_ex(scr,y,x,s,n)      move_ex((scr),(y),(x)), getnstr_ex((scr),(s),(n))
#define getyx_ex(scr,y,x)              ((y) = (scr)->cury, (x) = (scr)->curx)

/* Classic (single-instance) macros – available when default instance is used */
#ifdef MCURSES_USE_DEFAULT_INSTANCE
#  define erase()                       clear()
#  define mvaddch(y,x,c)                move((y),(x)), addch((c))
#  define mvaddstr(y,x,s)               move((y),(x)), addstr((s))
#  define mvaddstr_P(y,x,s)             move((y),(x)), addstr_P((s))
#  define mvinsch(y,x,c)                move((y),(x)), insch((c))
#  define mvdelch(y,x)                  move((y),(x)), delch()
#  define mvgetnstr(y,x,s,n)            move((y),(x)), getnstr((s),(n))
#  define getyx(y,x)                    ((y) = mcurses_cury, (x) = mcurses_curx)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __MCURSES_H__ */
/**
 * @file tui_scroll.h
 * @brief Scroll buffer for TUI scrollable text areas.
 *
 * Provides a fixed-size ring buffer of UTF-8 text lines, suitable for
 * shell output, log windows, or any scrollable text region.
 *
 * Design:
 *  - Zero malloc.  The entire buffer lives inside tui_scrollbuf_t,
 *    which the caller allocates (static or stack).
 *  - Ring-buffer storage: oldest lines are silently dropped when full.
 *  - Viewport model: view_offset=0 means "show latest lines at bottom".
 *    view_offset>0 scrolls backwards into history.
 *  - Per-line default attribute (colour / bold) for easy log colouring.
 *  - UTF-8 safe copy: truncation respects multibyte boundaries.
 *
 * Tune TUI_SCROLL_LINES and TUI_SCROLL_LINE_BYTES at compile time to
 * balance SRAM usage vs. history depth.
 *
 * Copyright (c) 2026 protoduino contributors.
 */

#ifndef __TUI_SCROLL_H__
#define __TUI_SCROLL_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <protoduino.h>
#if defined(__AVR__)
#  include <avr/io.h>
#endif

/* A part with 2 KB of RAM or less (ATmega328P, 168, 88) gets small TUI
 * defaults that fit it. The library sources and the sketch see the same
 * defaults, which a #define in a sketch cannot achieve in the Arduino IDE:
 * there a sketch's #define does not reach the library's .c files. */
#ifndef TUI_SMALL_RAM
#  if defined(RAMEND) && (RAMEND < 0x900)
#    define TUI_SMALL_RAM 1
#  else
#    define TUI_SMALL_RAM 0
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Compile-time tunables
 * ========================================================================= */

/** Number of lines stored in the scroll buffer (ring depth). */
#ifndef TUI_SCROLL_LINES
#  if TUI_SMALL_RAM
#    define TUI_SCROLL_LINES     6
#  else
#    define TUI_SCROLL_LINES    16
#  endif
#endif

/** Maximum bytes per line including NUL terminator.
 *  80 ASCII columns + NUL = 81.  UTF-8 multibyte may use more. */
#ifndef TUI_SCROLL_LINE_BYTES
#  if TUI_SMALL_RAM
#    define TUI_SCROLL_LINE_BYTES 42   /* 40 ASCII columns, NUL, one spare */
#  else
#    define TUI_SCROLL_LINE_BYTES 81
#  endif
#endif

/* =========================================================================
 * Scroll buffer structure
 * ========================================================================= */

/**
 * @brief Ring buffer of UTF-8 text lines for a scrollable viewport.
 *
 * Allocate this struct statically or on the stack – no heap needed.
 * Call tui_scroll_init() before first use.
 */
typedef struct {
    /** Line storage: each slot holds a NUL-terminated UTF-8 string. */
    char     lines[TUI_SCROLL_LINES][TUI_SCROLL_LINE_BYTES];

    /** Per-line default attribute (packed mcurses attr_t). */
    uint16_t line_attrs[TUI_SCROLL_LINES];

    uint8_t  head;         /**< Index of oldest line in the ring.          */
    uint8_t  count;        /**< Number of lines stored (0..TUI_SCROLL_LINES). */
    uint8_t  view_offset;  /**< 0 = latest at bottom, >0 = scrolled back. */
    uint8_t  dirty;        /**< Non-zero if content changed since render.  */

    /* What the renderer last drew of this buffer, row by row, so that a new
     * line costs the rows that changed and not the whole area.  Kept by
     * tui_render.c; a buffer is drawn in one scroll area at a time (another
     * area, or the same one moved, is drawn in full). */
    uint16_t shown_at;                      /**< The area it was drawn in; 0: none. */
    uint8_t  shown_rows;                    /**< Its height; 0: not drawn yet.      */
    uint16_t shown_sig[TUI_SCROLL_LINES];   /**< Signature of each row's text.      */
    uint8_t  shown_len[TUI_SCROLL_LINES];   /**< Columns each row's text took.      */
} tui_scrollbuf_t;

/* =========================================================================
 * Lifecycle
 * ========================================================================= */

/**
 * @brief Initialise (or reset) a scroll buffer.
 *
 * Clears all lines, resets head/count/view_offset, marks dirty.
 *
 * @param sb  Caller-allocated scroll buffer.
 */
CC_EXTERN void tui_scroll_init(tui_scrollbuf_t *sb);

/* =========================================================================
 * Content management
 * ========================================================================= */

/**
 * @brief Append a UTF-8 text line to the scroll buffer.
 *
 * If the buffer is full the oldest line is silently dropped.
 * The text is copied into the ring; the caller retains ownership
 * of utf8_text.  Truncation respects UTF-8 multibyte boundaries.
 *
 * @param sb         Scroll buffer.
 * @param utf8_text  NUL-terminated UTF-8 source line.
 * @param attr       Default attribute for this line.
 */
CC_EXTERN void tui_scroll_add_line(tui_scrollbuf_t *sb,
                                   const char *utf8_text,
                                   uint16_t attr);

/**
 * @brief Clear all stored lines and reset the viewport.
 *
 * @param sb  Scroll buffer.
 */
CC_EXTERN void tui_scroll_clear(tui_scrollbuf_t *sb);

/* =========================================================================
 * Viewport queries
 * ========================================================================= */

/**
 * @brief Number of visible lines for a given viewport height.
 *
 * Returns min(stored_lines, viewport_height), accounting for
 * view_offset (scrolled-back lines are still "visible").
 *
 * @param sb               Scroll buffer.
 * @param viewport_height  Height of the viewport in rows.
 * @return Number of lines to render (0..viewport_height).
 */
CC_EXTERN uint8_t tui_scroll_visible_count(const tui_scrollbuf_t *sb,
                                           uint8_t viewport_height);

/**
 * @brief Get a pointer to a visible line by viewport index.
 *
 * viewport_idx 0 is the topmost visible line in the viewport.
 * Returns NULL if the index is out of range.
 *
 * The returned pointer is valid until the next tui_scroll_add_line()
 * call that overwrites that ring slot.
 *
 * @param sb               Scroll buffer.
 * @param viewport_idx     Line index within the viewport (0 = top).
 * @param viewport_height  Viewport height in rows.
 * @return Pointer to NUL-terminated UTF-8 line, or NULL.
 */
CC_EXTERN const char *tui_scroll_get_line(const tui_scrollbuf_t *sb,
                                          uint8_t viewport_idx,
                                          uint8_t viewport_height);

/**
 * @brief Get the attribute for a visible line.
 *
 * @param sb               Scroll buffer.
 * @param viewport_idx     Line index within the viewport (0 = top).
 * @param viewport_height  Viewport height in rows.
 * @return Packed attribute, or MCURSES_ATTR_NORMAL on error.
 */
CC_EXTERN uint16_t tui_scroll_get_attr(const tui_scrollbuf_t *sb,
                                       uint8_t viewport_idx,
                                       uint8_t viewport_height);

/* =========================================================================
 * Scroll navigation
 * ========================================================================= */

/**
 * @brief Scroll the viewport one line back into the history.
 *
 * Stops when the oldest line is at the top of the area the buffer was last
 * drawn in: a log with no more lines than rows has nothing to scroll back
 * to.  Marks the buffer dirty when the view moved.
 */
CC_EXTERN void tui_scroll_up(tui_scrollbuf_t *sb);

/**
 * @brief Scroll the viewport one line towards newer content.
 *
 * view_offset is clamped at zero (cannot scroll past the newest line).
 *
 * @param sb  Scroll buffer.
 */
CC_EXTERN void tui_scroll_down(tui_scrollbuf_t *sb);

/**
 * @brief Jump the viewport to the newest content (view_offset = 0).
 *
 * @param sb  Scroll buffer.
 */
CC_EXTERN void tui_scroll_to_bottom(tui_scrollbuf_t *sb);

#ifdef __cplusplus
}
#endif

#endif /* __TUI_SCROLL_H__ */

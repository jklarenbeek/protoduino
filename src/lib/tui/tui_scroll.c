/**
 * @file tui_scroll.c
 * @brief TUI scroll buffer – ring buffer of UTF-8 lines.
 *
 * Implements a fixed-size circular buffer of NUL-terminated UTF-8 text
 * lines with per-line attributes and a viewport offset for scrolling.
 *
 * Key invariants:
 *  - head points to the oldest line.
 *  - Lines are stored at index  (head + i) % TUI_SCROLL_LINES
 *    for i in [0 .. count-1], where i=0 is oldest, i=count-1 is newest.
 *  - view_offset=0 means the viewport bottom aligns with the newest line.
 *  - view_offset>0 shifts the viewport backwards into history.
 *
 * No globals.  No malloc.  UTF-8 safe truncation via utf8_ecpy.
 *
 * Copyright (c) 2026 protoduino contributors.
 */

#include "tui_scroll.h"

#include "../text/utf8.h"

#include <string.h>   /* memset, strlen */

/* =========================================================================
 * Lifecycle
 * ========================================================================= */

void tui_scroll_init(tui_scrollbuf_t *sb)
{
    if (!sb) return;
    memset(sb->lines, 0, sizeof(sb->lines));
    memset(sb->line_attrs, 0, sizeof(sb->line_attrs));
    sb->head        = 0;
    sb->count       = 0;
    sb->view_offset = 0;
    sb->dirty       = 1;
}

/* =========================================================================
 * Content management
 * ========================================================================= */

void tui_scroll_add_line(tui_scrollbuf_t *sb,
                         const char *utf8_text,
                         uint16_t attr)
{
    if (!sb) return;

    uint8_t slot;

    if (sb->count < TUI_SCROLL_LINES) {
        /* Buffer not yet full – append after the last used slot */
        slot = (uint8_t)((sb->head + sb->count) % TUI_SCROLL_LINES);
        ++sb->count;
    } else {
        /* Buffer full – overwrite oldest (head) and advance head */
        slot = sb->head;
        sb->head = (uint8_t)((sb->head + 1u) % TUI_SCROLL_LINES);
    }

    /* Copy text into the slot with UTF-8-safe truncation.
     * utf8_ecpy(to, end, from) copies bytes respecting multibyte boundaries
     * and always NUL-terminates. */
    if (utf8_text) {
        char *dst     = sb->lines[slot];
        /* utf8_ecpy's `e` is one-past-the-last-writable byte; pass the true
         * buffer end so the full TUI_SCROLL_LINE_BYTES is usable (the NUL
         * may land in the last slot).  Using LINE_BYTES-1 wasted a byte. */
        char *dst_end = dst + TUI_SCROLL_LINE_BYTES;
        utf8_ecpy(dst, dst_end, (char *)utf8_text);
    } else {
        sb->lines[slot][0] = '\0';
    }

    sb->line_attrs[slot] = attr;
    sb->dirty = 1;
}

void tui_scroll_clear(tui_scrollbuf_t *sb)
{
    if (!sb) return;
    sb->head        = 0;
    sb->count       = 0;
    sb->view_offset = 0;
    sb->dirty       = 1;
}

/* =========================================================================
 * Internal helpers
 * ========================================================================= */

/**
 * @brief Map a viewport line index to a ring-buffer slot.
 *
 * The viewport shows `viewport_height` lines.  viewport_idx 0 is the
 * topmost visible line.  With view_offset the window shifts backwards.
 *
 * Logical line ordering (newest at highest logical index):
 *   logical[0]          = oldest   (ring slot = head)
 *   logical[count-1]    = newest   (ring slot = head + count - 1)
 *
 * The viewport bottom is aligned at logical[count - 1 - view_offset].
 * The viewport top is at logical[count - 1 - view_offset - (vh-1)]
 *   = logical[count - view_offset - vh].
 *
 * So viewport_idx i maps to logical[count - view_offset - vh + i].
 *
 * @param sb               Scroll buffer.
 * @param viewport_idx     0 = top of viewport.
 * @param viewport_height  Number of rows in the viewport.
 * @param out_slot         Receives ring-buffer index.
 * @return true if the mapping is valid.
 */
static bool _viewport_to_slot(const tui_scrollbuf_t *sb,
                              uint8_t viewport_idx,
                              uint8_t viewport_height,
                              uint8_t *out_slot)
{
    if (sb->count == 0)
        return false;

    /* Number of visible lines: min(count, viewport_height) */
    uint8_t visible = sb->count;
    if (visible > viewport_height)
        visible = viewport_height;

    if (viewport_idx >= visible)
        return false;

    /*
     * Logical index of the bottom-most visible line:
     *   bot_logical = count - 1 - view_offset
     *
     * Logical index of the top-most visible line:
     *   top_logical = bot_logical - (visible - 1)
     *
     * Logical index for viewport_idx:
     *   logical = top_logical + viewport_idx
     */
    int16_t bot_logical = (int16_t)sb->count - 1 - (int16_t)sb->view_offset;
    int16_t top_logical = bot_logical - (int16_t)(visible - 1);
    int16_t logical     = top_logical + (int16_t)viewport_idx;

    if (logical < 0 || logical >= (int16_t)sb->count)
        return false;

    *out_slot = (uint8_t)((sb->head + (uint8_t)logical) % TUI_SCROLL_LINES);
    return true;
}

/* =========================================================================
 * Viewport queries
 * ========================================================================= */

uint8_t tui_scroll_visible_count(const tui_scrollbuf_t *sb,
                                 uint8_t viewport_height)
{
    if (!sb)
        return 0;
    uint8_t n = sb->count;
    if (n > viewport_height)
        n = viewport_height;
    return n;
}

const char *tui_scroll_get_line(const tui_scrollbuf_t *sb,
                                uint8_t viewport_idx,
                                uint8_t viewport_height)
{
    if (!sb)
        return (const char *)0;

    uint8_t slot;
    if (!_viewport_to_slot(sb, viewport_idx, viewport_height, &slot))
        return (const char *)0;

    return sb->lines[slot];
}

uint16_t tui_scroll_get_attr(const tui_scrollbuf_t *sb,
                             uint8_t viewport_idx,
                             uint8_t viewport_height)
{
    if (!sb)
        return 0; /* MCURSES_ATTR_NORMAL == 0 */

    uint8_t slot;
    if (!_viewport_to_slot(sb, viewport_idx, viewport_height, &slot))
        return 0;

    return sb->line_attrs[slot];
}

/* =========================================================================
 * Scroll navigation
 * ========================================================================= */

void tui_scroll_up(tui_scrollbuf_t *sb)
{
    if (!sb)
        return;

    /*
     * view_offset can be at most count - 1 (viewing only the oldest line
     * at the bottom of the viewport).  In practice this is conservative;
     * a tighter bound would account for viewport_height, but we don't
     * store that in the scroll buffer.  The renderer just won't find
     * valid lines for viewport indices past the stored content.
     *
     * Guard against an empty buffer: `sb->count - 1u` promotes to unsigned
     * and would wrap to a huge value when count == 0, letting view_offset
     * drift.  Use an addition form that is safe for count 0 and 1.
     */
    if ((uint8_t)(sb->view_offset + 1u) < sb->count) {
        ++sb->view_offset;
        sb->dirty = 1;
    }
}

void tui_scroll_down(tui_scrollbuf_t *sb)
{
    if (!sb)
        return;

    if (sb->view_offset > 0) {
        --sb->view_offset;
        sb->dirty = 1;
    }
}

void tui_scroll_to_bottom(tui_scrollbuf_t *sb)
{
    if (!sb)
        return;

    if (sb->view_offset != 0) {
        sb->view_offset = 0;
        sb->dirty = 1;
    }
}

/**
 * @file tui_render.c
 * @brief TUI renderer – damage-aware VT output implementation.
 *
 * Drains a tui_render_queue_t by emitting VT escape sequences through
 * the mcurses_t screen instance.  Each command type maps to a specific
 * output pattern:
 *
 *  - FILL:       move + spaces with background attribute per row.
 *  - TEXT:       move + attrset + raw UTF-8 bytes via ipc_pipe_write().
 *  - BORDER:    box-drawing corners/edges encoded as UTF-8 from ACS_*.
 *  - HLINE:     repeated horizontal character.
 *  - VLINE:     repeated vertical character.
 *  - CLIP_START: save clip rect.
 *  - CLIP_END:   restore clip rect.
 *
 * No globals.  No malloc.  Fits on an Arduino Uno.
 *
 * Copyright (c) 2026 protoduino contributors.
 */

/* Include the layout header FIRST so its authoritative tui_border_style_t
 * enum is in scope before tui_render.h decides whether to emit its fallback
 * TUI_BORDER_* macros (it only does so when __TUI_LAYOUT_H__ is absent).
 * The frame walker and scroll composition also need the full layout/scroll
 * types. */
#if __has_include("tui_layout.h")
#  include "tui_layout.h"
#endif
#include "tui_render.h"
#include "tui_scroll.h"

#include <string.h>   /* memset */

/* =========================================================================
 * Box-drawing character sets (Unicode code-points), selected by border style
 * =========================================================================
 * Rows are indexed by (tui_border_style_t - 1): SINGLE, DOUBLE, ROUND.
 * Columns: UL, UR, LL, LR, HLINE, VLINE.  These mirror the table in
 * mcurses.c so the two border paths render identically. */
#define _TBOX_UL 0
#define _TBOX_UR 1
#define _TBOX_LL 2
#define _TBOX_LR 3
#define _TBOX_HL 4
#define _TBOX_VL 5

static const uint16_t _tui_box_chars[3][6] = {
    /* SINGLE: UL      UR      LL      LR      HLINE   VLINE  */
    { 0x250Cu, 0x2510u, 0x2514u, 0x2518u, 0x2500u, 0x2502u },
    /* DOUBLE: */
    { 0x2554u, 0x2557u, 0x255Au, 0x255Du, 0x2550u, 0x2551u },
    /* ROUND:  */
    { 0x256Du, 0x256Eu, 0x2570u, 0x256Fu, 0x2500u, 0x2502u },
};

/* =========================================================================
 * Internal helpers – clip rectangle
 * ========================================================================= */

/**
 * @brief Test whether a cell (cx, cy) is inside the active clip rect.
 *
 * Returns true (visible) when clipping is disabled.
 */
static bool _in_clip(const tui_render_queue_t *rq, uint8_t cx, uint8_t cy)
{
    if (!rq->has_clip)
        return true;
    if (cx < rq->clip.x || cx >= rq->clip.x + rq->clip.w)
        return false;
    if (cy < rq->clip.y || cy >= rq->clip.y + rq->clip.h)
        return false;
    return true;
}

/**
 * @brief Test whether an entire row-span is fully outside the clip rect.
 *
 * Quick reject for FILL / HLINE rows.
 */
static bool _row_clipped(const tui_render_queue_t *rq, uint8_t row)
{
    if (!rq->has_clip)
        return false;
    return (row < rq->clip.y || row >= rq->clip.y + rq->clip.h);
}

/* =========================================================================
 * Internal helpers – ACS box-drawing to UTF-8
 * ========================================================================= */

/**
 * @brief Encode a 16-bit ACS_* Unicode code-point to UTF-8.
 *
 * The ACS_* values in vterm.h are uint16_t Unicode code-points in the
 * box-drawing block (U+2500–U+257F, 3-byte UTF-8).  This helper encodes
 * one code-point into buf[] and returns the byte count.
 *
 * @param acs  Unicode code-point (e.g. ACS_ULCORNER = 0x250C).
 * @param buf  Output buffer (at least 3 bytes).
 * @return Number of UTF-8 bytes written (1, 2, or 3).
 */
static uint8_t _encode_acs(uint16_t acs, char *buf)
{
    return utf8_fromrune16(buf, (rune16_t)acs);
}

/**
 * @brief Write a single ACS character at the current cursor position.
 *
 * Encodes to UTF-8 on the stack and writes the bytes to the TX pipe.
 */
static void _put_acs(mcurses_t *scr, uint16_t acs)
{
    char buf[3];
    uint8_t n = _encode_acs(acs, buf);
    if (n > 0)
        ipc_pipe_write(scr->txpipe, (const uint8_t *)buf, n);
}

/* =========================================================================
 * Command handlers
 * ========================================================================= */

/**
 * @brief Handle TUI_CMD_FILL: fill bounding box with a character + attr.
 *
 * For each row in [y .. y+h), move to (y, x), set attribute, then write
 * `w` copies of the fill character (default space).
 */
static void _flush_fill(const tui_render_cmd_t *cmd,
                         tui_render_queue_t *rq,
                         mcurses_t *scr)
{
    uint8_t ch = cmd->data.fill.ch;
    if (ch == 0) ch = ' ';

    attrset_ex(scr, cmd->style);

    for (uint8_t row = cmd->y; row < cmd->y + cmd->h; ++row) {
        if (_row_clipped(rq, row))
            continue;

        /* Find visible column range within this row */
        uint8_t x0 = cmd->x;
        uint8_t x1 = cmd->x + cmd->w;

        if (rq->has_clip) {
            if (x0 < rq->clip.x) x0 = rq->clip.x;
            uint8_t clip_x1 = rq->clip.x + rq->clip.w;
            if (x1 > clip_x1) x1 = clip_x1;
        }
        if (x0 >= x1)
            continue;

        move_ex(scr, row, x0);

        uint8_t w = x1 - x0;
        for (uint8_t i = 0; i < w; ++i)
            addch_ex(scr, ch);
    }
}

/**
 * @brief Handle TUI_CMD_TEXT: place UTF-8 text at position.
 *
 * Emits via addnstr_ex(), which streams the bytes through the back-pressure
 * path (no silent truncation) AND keeps mcurses_t's cursor column/row in
 * sync — important for any code that later queries the cursor or relies on
 * wrap tracking.  Horizontal extent is already clamped to the element width
 * by the frame walker (`_clip_cols`), so the text stays inside its rect.
 *
 * If clipping is active the command is skipped when its origin row/column
 * falls outside the clip rect (coarse, command-level clip).
 */
static void _flush_text(const tui_render_cmd_t *cmd,
                         tui_render_queue_t *rq,
                         mcurses_t *scr)
{
    if (!cmd->data.text.str || cmd->data.text.byte_len == 0)
        return;

    /* Coarse clip: skip if origin row is outside clip rect */
    if (_row_clipped(rq, cmd->y))
        return;
    if (rq->has_clip && cmd->x >= rq->clip.x + rq->clip.w)
        return;

    move_ex(scr, cmd->y, cmd->x);
    attrset_ex(scr, cmd->style);
    addnstr_ex(scr, cmd->data.text.str, cmd->data.text.byte_len);
}

/**
 * @brief Handle TUI_CMD_BORDER: draw a box-drawing border.
 *
 * Layout:
 *   ┌──────────┐   row y
 *   │          │   rows y+1 .. y+h-2
 *   └──────────┘   row y+h-1
 *
 * Uses ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER,
 * ACS_HLINE, ACS_VLINE from vterm.h.
 */
static void _flush_border(const tui_render_cmd_t *cmd,
                           tui_render_queue_t *rq,
                           mcurses_t *scr)
{
    if (cmd->w < 2 || cmd->h < 2)
        return;

    /* Select the box-drawing character set from the requested style.
     * Style values follow tui_border_style_t (1=SINGLE,2=DOUBLE,3=ROUND);
     * anything out of range falls back to SINGLE. */
    uint8_t style = cmd->data.border.border_style;
    if (style < 1u || style > 3u)
        style = 1u;
    const uint16_t *bc = _tui_box_chars[style - 1u];

    attrset_ex(scr, cmd->style);

    /* ---- Top row ---- */
    if (!_row_clipped(rq, cmd->y)) {
        if (_in_clip(rq, cmd->x, cmd->y)) {
            move_ex(scr, cmd->y, cmd->x);
            _put_acs(scr, bc[_TBOX_UL]);
        }
        for (uint8_t col = 1; col < cmd->w - 1u; ++col) {
            uint8_t cx = cmd->x + col;
            if (_in_clip(rq, cx, cmd->y)) {
                move_ex(scr, cmd->y, cx);
                _put_acs(scr, bc[_TBOX_HL]);
            }
        }
        {
            uint8_t cx = cmd->x + cmd->w - 1u;
            if (_in_clip(rq, cx, cmd->y)) {
                move_ex(scr, cmd->y, cx);
                _put_acs(scr, bc[_TBOX_UR]);
            }
        }
    }

    /* ---- Middle rows (vertical lines on left and right edges) ---- */
    for (uint8_t row = cmd->y + 1u; row < cmd->y + cmd->h - 1u; ++row) {
        if (_row_clipped(rq, row))
            continue;

        /* Left edge */
        if (_in_clip(rq, cmd->x, row)) {
            move_ex(scr, row, cmd->x);
            _put_acs(scr, bc[_TBOX_VL]);
        }

        /* Right edge */
        {
            uint8_t cx = cmd->x + cmd->w - 1u;
            if (_in_clip(rq, cx, row)) {
                move_ex(scr, row, cx);
                _put_acs(scr, bc[_TBOX_VL]);
            }
        }
    }

    /* ---- Bottom row ---- */
    {
        uint8_t bot = cmd->y + cmd->h - 1u;
        if (!_row_clipped(rq, bot)) {
            if (_in_clip(rq, cmd->x, bot)) {
                move_ex(scr, bot, cmd->x);
                _put_acs(scr, bc[_TBOX_LL]);
            }
            for (uint8_t col = 1; col < cmd->w - 1u; ++col) {
                uint8_t cx = cmd->x + col;
                if (_in_clip(rq, cx, bot)) {
                    move_ex(scr, bot, cx);
                    _put_acs(scr, bc[_TBOX_HL]);
                }
            }
            {
                uint8_t cx = cmd->x + cmd->w - 1u;
                if (_in_clip(rq, cx, bot)) {
                    move_ex(scr, bot, cx);
                    _put_acs(scr, bc[_TBOX_LR]);
                }
            }
        }
    }
}

/**
 * @brief Handle TUI_CMD_HLINE: draw a horizontal line.
 *
 * Draws `w` copies of the line character starting at (x, y).
 * If the character is 0, ACS_HLINE is used.
 */
static void _flush_hline(const tui_render_cmd_t *cmd,
                          tui_render_queue_t *rq,
                          mcurses_t *scr)
{
    if (_row_clipped(rq, cmd->y))
        return;

    attrset_ex(scr, cmd->style);

    uint16_t ch = cmd->data.line.ch;
    if (ch == 0) ch = ACS_HLINE;

    for (uint8_t col = 0; col < cmd->w; ++col) {
        uint8_t cx = cmd->x + col;
        if (!_in_clip(rq, cx, cmd->y))
            continue;
        move_ex(scr, cmd->y, cx);
        if (ch > 0x7Fu) {
            _put_acs(scr, ch);
        } else {
            addch_ex(scr, (uint_fast8_t)ch);
        }
    }
}

/**
 * @brief Handle TUI_CMD_VLINE: draw a vertical line.
 *
 * Draws `h` copies of the line character starting at (x, y).
 * If the character is 0, ACS_VLINE is used.
 */
static void _flush_vline(const tui_render_cmd_t *cmd,
                          tui_render_queue_t *rq,
                          mcurses_t *scr)
{
    attrset_ex(scr, cmd->style);

    uint16_t ch = cmd->data.line.ch;
    if (ch == 0) ch = ACS_VLINE;

    for (uint8_t row = 0; row < cmd->h; ++row) {
        uint8_t ry = cmd->y + row;
        if (_row_clipped(rq, ry))
            continue;
        if (!_in_clip(rq, cmd->x, ry))
            continue;
        move_ex(scr, ry, cmd->x);
        if (ch > 0x7Fu) {
            _put_acs(scr, ch);
        } else {
            addch_ex(scr, (uint_fast8_t)ch);
        }
    }
}

/* =========================================================================
 * Public API
 * ========================================================================= */

void tui_render_init(tui_render_queue_t *rq)
{
    if (!rq) return;
    rq->count    = 0;
    rq->has_clip = 0;
    rq->clip.x   = 0;
    rq->clip.y   = 0;
    rq->clip.w   = 0;
    rq->clip.h   = 0;
}

void tui_render_push(tui_render_queue_t *rq, const tui_render_cmd_t *cmd)
{
    if (!rq || !cmd)
        return;
    if (rq->count >= TUI_MAX_RENDER_CMDS)
        return;  /* queue full – drop silently */

    rq->cmds[rq->count] = *cmd;
    ++rq->count;
}

void tui_render_flush(tui_render_queue_t *rq, mcurses_t *scr)
{
    if (!rq || !scr)
        return;

    /* Saved clip state for CLIP_START / CLIP_END nesting (1 level). */
    struct { uint8_t x, y, w, h; } saved_clip = {0, 0, 0, 0};
    uint8_t saved_has_clip = 0;

    for (uint8_t i = 0; i < rq->count; ++i) {
        const tui_render_cmd_t *cmd = &rq->cmds[i];

        switch ((tui_cmd_type_t)cmd->type) {

        case TUI_CMD_FILL:
            _flush_fill(cmd, rq, scr);
            break;

        case TUI_CMD_TEXT:
            _flush_text(cmd, rq, scr);
            break;

        case TUI_CMD_BORDER:
            _flush_border(cmd, rq, scr);
            break;

        case TUI_CMD_HLINE:
            _flush_hline(cmd, rq, scr);
            break;

        case TUI_CMD_VLINE:
            _flush_vline(cmd, rq, scr);
            break;

        case TUI_CMD_CLIP_START:
            /* Save current clip, install new one from the command bbox */
            saved_clip.x = rq->clip.x;
            saved_clip.y = rq->clip.y;
            saved_clip.w = rq->clip.w;
            saved_clip.h = rq->clip.h;
            saved_has_clip = rq->has_clip;

            rq->clip.x   = cmd->x;
            rq->clip.y   = cmd->y;
            rq->clip.w   = cmd->w;
            rq->clip.h   = cmd->h;
            rq->has_clip  = 1;
            break;

        case TUI_CMD_CLIP_END:
            /* Restore previous clip state */
            rq->clip.x   = saved_clip.x;
            rq->clip.y   = saved_clip.y;
            rq->clip.w   = saved_clip.w;
            rq->clip.h   = saved_clip.h;
            rq->has_clip  = saved_has_clip;
            break;
        }
    }

    /* Ensure output reaches the transport */
    refresh_ex(scr);
}

/* =========================================================================
 * Frame-level rendering
 * =========================================================================
 *
 * tui_render_frame() walks the layout context's element tree (already laid
 * out and damage-diffed by tui_compute_layout) and, for every dirty
 * element, pushes the matching render commands, then flushes once.
 *
 * Z-order is implicit: elements are stored in declaration order, so a
 * parent always precedes its children in the array.  Pushing commands in
 * index order therefore emits a container's FILL/BORDER before its
 * children's TEXT, so background fills never clobber foreground content.
 * The damage diff guarantees that any dirty child also has a dirty parent,
 * so partial repaints stay internally consistent.
 */

#ifdef __TUI_LAYOUT_H__

/**
 * @brief Byte length of @p str fitting within @p max_cols display columns
 *        without splitting a UTF-8 sequence.
 *
 * One rune counts as one cell (the project's 1-rune ≈ 1-cell model).  A
 * trailing incomplete multi-byte sequence is excluded.
 */
static uint8_t _clip_cols(const char *str, uint8_t byte_len, uint8_t max_cols)
{
    uint8_t i = 0, cols = 0;
    if (!str)
        return 0;
    while (i < byte_len && cols < max_cols) {
        uint8_t lead = (uint8_t)str[i];
        uint8_t n = (lead < 0x80u) ? 1u : (lead < 0xE0u) ? 2u
                  : (lead < 0xF0u) ? 3u : 4u;
        if ((uint16_t)i + n > byte_len)
            break;                  /* incomplete sequence at the end: stop */
        i = (uint8_t)(i + n);
        ++cols;
    }
    return i;
}

/**
 * Flush-and-reset the queue when it is full, so the next push never drops a
 * command (and dirty flags are only cleared for output that actually ships).
 * Safe between any two frame-walker commands: each is self-positioning
 * (carries its own move + attr) and the walker never leaves a clip region
 * open across commands. */
static void _flush_if_full(tui_render_queue_t *rq, mcurses_t *scr)
{
    if (rq->count >= TUI_MAX_RENDER_CMDS) {
        tui_render_flush(rq, scr);
        tui_render_init(rq);
    }
}

/** Push a FILL/box command, flushing first if the queue is full. */
static void _push_box(tui_render_queue_t *rq, mcurses_t *scr, uint8_t type,
                      uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                      uint16_t style)
{
    tui_render_cmd_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type  = type;
    cmd.x     = x;
    cmd.y     = y;
    cmd.w     = w;
    cmd.h     = h;
    cmd.style = style;
    _flush_if_full(rq, scr);
    tui_render_push(rq, &cmd);
}

/** Push a TEXT command (byte_len already column-clamped), flush if full. */
static void _push_text(tui_render_queue_t *rq, mcurses_t *scr,
                       uint8_t x, uint8_t y, uint16_t style,
                       const char *str, uint8_t byte_len)
{
    if (!str || byte_len == 0)
        return;
    tui_render_cmd_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type               = TUI_CMD_TEXT;
    cmd.x                  = x;
    cmd.y                  = y;
    cmd.style              = style;
    cmd.data.text.str      = str;
    cmd.data.text.byte_len = byte_len;
    _flush_if_full(rq, scr);
    tui_render_push(rq, &cmd);
}

/**
 * Render a scroll-area element: fill the rect, then one TEXT per visible row.
 * No clip region is used — each line is clamped to the element width in
 * display columns and the row loop is bounded by the element height, so the
 * output stays inside the rect.  Keeping every command independent lets the
 * queue be flushed between any of them (even a very tall scroll area) without
 * losing a clip or dropping commands.
 */
static void _frame_scroll(tui_render_queue_t *rq, mcurses_t *scr,
                          const tui_element_t *el)
{
    tui_scrollbuf_t *sb = (tui_scrollbuf_t *)el->data.scroll.sb;
    if (!sb || el->w == 0 || el->h == 0)
        return;

    _push_box(rq, scr, TUI_CMD_FILL, el->x, el->y, el->w, el->h, el->style);

    for (uint8_t row = 0; row < el->h; ++row) {
        const char *line = tui_scroll_get_line(sb, row, el->h);
        if (!line || line[0] == '\0')
            continue;

        uint8_t  len  = _clip_cols(line, (uint8_t)strlen(line), el->w);
        uint16_t attr = tui_scroll_get_attr(sb, row, el->h);
        _push_text(rq, scr, el->x, (uint8_t)(el->y + row), attr, line, len);
    }

    sb->dirty = 0;  /* content now reflected on screen */
}

void tui_render_frame(struct tui_context *ctx,
                      tui_render_queue_t *rq,
                      mcurses_t *scr)
{
    if (!ctx || !rq || !scr)
        return;

    tui_render_init(rq);

    for (uint8_t i = 0; i < ctx->elem_count; ++i) {
        tui_element_t *el = &ctx->elements[i];

        if (!(el->flags & TUI_FLAG_VISIBLE))
            continue;

        /* Scroll content can change without any layout/style change, so a
         * scroll area is also dirty when its buffer signals new content. */
        uint8_t dirty = (el->flags & TUI_FLAG_DIRTY) ? 1u : 0u;
        if (!dirty && el->type == TUI_ELEM_SCROLL_AREA && el->data.scroll.sb) {
            tui_scrollbuf_t *sb = (tui_scrollbuf_t *)el->data.scroll.sb;
            if (sb->dirty)
                dirty = 1u;
        }
        if (!dirty)
            continue;

        switch (el->type) {
        case TUI_ELEM_CONTAINER:
            _push_box(rq, scr, TUI_CMD_FILL, el->x, el->y, el->w, el->h, el->style);
            if ((el->flags & TUI_FLAG_HAS_BORDER) &&
                el->border != TUI_BORDER_NONE) {
                tui_render_cmd_t cmd;
                memset(&cmd, 0, sizeof(cmd));
                cmd.type                     = TUI_CMD_BORDER;
                cmd.x                        = el->x;
                cmd.y                        = el->y;
                cmd.w                        = el->w;
                cmd.h                        = el->h;
                cmd.style                    = el->style;
                cmd.data.border.border_style = el->border;
                _flush_if_full(rq, scr);
                tui_render_push(rq, &cmd);
            }
            break;

        case TUI_ELEM_TEXT: {
            /* Clamp the text to the element width in display columns so it
             * cannot overflow its rect or split a UTF-8 sequence. */
            uint8_t len = _clip_cols(el->data.text.str, el->data.text.len, el->w);
            _push_text(rq, scr, el->x, el->y, el->style, el->data.text.str, len);
            break;
        }

        case TUI_ELEM_SCROLL_AREA:
            _frame_scroll(rq, scr, el);
            break;

        default:
            break;
        }

        el->flags &= (uint8_t)~TUI_FLAG_DIRTY;  /* consumed */
    }

    tui_render_flush(rq, scr);
}

#else  /* __TUI_LAYOUT_H__ not available – keep a linkable no-op */

void tui_render_frame(struct tui_context *ctx,
                      tui_render_queue_t *rq,
                      mcurses_t *scr)
{
    (void)ctx;
    (void)rq;
    (void)scr;
}

#endif /* __TUI_LAYOUT_H__ */

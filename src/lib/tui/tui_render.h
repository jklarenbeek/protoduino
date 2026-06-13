/**
 * @file tui_render.h
 * @brief TUI renderer – damage-aware VT output through mcurses.
 *
 * The renderer consumes a queue of render commands (produced by the
 * layout engine or by hand) and emits VT escape sequences via the
 * mcurses_t screen instance.  Only dirty elements generate commands;
 * the queue is flushed once per frame.
 *
 * UTF-8 text is passed by pointer+length directly to the TX pipe –
 * no decode/re-encode round-trip.
 *
 * Design:
 *  - Zero malloc.  The render queue is caller-allocated.
 *  - Clip-rect support for scrollable regions.
 *  - Box-drawing via Unicode ACS_* constants (vterm.h), encoded as
 *    UTF-8 on the fly with a small stack buffer.
 *
 * Copyright (c) 2026 protoduino contributors.
 */

#ifndef __TUI_RENDER_H__
#define __TUI_RENDER_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <protoduino.h>
#include "../text/mcurses.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Render command types
 * ========================================================================= */

/**
 * @brief Type tag for each render command in the queue.
 */
typedef enum {
    TUI_CMD_FILL,        /**< Fill area with spaces + background attr.     */
    TUI_CMD_TEXT,        /**< Place UTF-8 text at position.                */
    TUI_CMD_BORDER,      /**< Draw box-drawing border around bounding box. */
    TUI_CMD_HLINE,       /**< Horizontal line with repeated character.     */
    TUI_CMD_VLINE,       /**< Vertical line with repeated character.       */
    TUI_CMD_CLIP_START,  /**< Begin clipping region.                       */
    TUI_CMD_CLIP_END,    /**< End clipping region (restore previous clip). */
} tui_cmd_type_t;

/* =========================================================================
 * Border style constants
 * =========================================================================
 *
 * When tui_layout.h is present it defines the authoritative
 * tui_border_style_t enum (NONE=0, SINGLE=1, DOUBLE=2, ROUND=3) and the
 * render path interprets cmd.data.border.border_style using those values.
 * Only provide fallback object-like macros for standalone use of the
 * renderer (without the layout module), and NEVER when the enum is in
 * scope – defining macros with the same names would silently shadow the
 * enumerators and remap SINGLE->NONE. */
#ifndef __TUI_LAYOUT_H__
#define TUI_BORDER_NONE     0   /**< No border.                          */
#define TUI_BORDER_SINGLE   1   /**< Single-line box (┌─┐│ │└─┘).        */
#define TUI_BORDER_DOUBLE   2   /**< Double-line box (╔═╗║ ║╚═╝).        */
#define TUI_BORDER_ROUND    3   /**< Rounded-corner box (╭─╮│ │╰─╯).     */
#endif

/* =========================================================================
 * Render command
 * ========================================================================= */

/**
 * @brief A single render command consumed by the flush stage.
 *
 * Produced by the layout walker (tui_render_frame) or pushed manually.
 * The bounding box (x,y,w,h) is in character-cell coordinates.
 */
typedef struct {
    uint8_t  x;          /**< Left column   (0-based). */
    uint8_t  y;          /**< Top row        (0-based). */
    uint8_t  w;          /**< Width  in cells.          */
    uint8_t  h;          /**< Height in rows.           */
    uint8_t  type;       /**< tui_cmd_type_t tag.       */
    uint16_t style;      /**< Packed attribute (matches mcurses_t.attr). */
    union {
        struct {
            const char *str;       /**< Pointer into caller's UTF-8 data. */
            uint8_t     byte_len;  /**< Byte length of the UTF-8 span.    */
        } text;
        struct {
            uint8_t ch;            /**< ASCII fill character (' ' common). */
        } fill;
        struct {
            uint8_t border_style;  /**< TUI_BORDER_SINGLE, etc.           */
        } border;
        struct {
            uint8_t ch;            /**< Line character (ASCII or ACS_*).  */
        } line;
    } data;
} tui_render_cmd_t;

/* =========================================================================
 * Render queue
 * ========================================================================= */

/** Maximum render commands per frame (tuneable at compile time). */
#ifndef TUI_MAX_RENDER_CMDS
#define TUI_MAX_RENDER_CMDS 64
#endif

/**
 * @brief Caller-allocated command queue and clip state for one frame.
 */
typedef struct {
    tui_render_cmd_t cmds[TUI_MAX_RENDER_CMDS]; /**< Command ring. */
    uint8_t count;                               /**< Commands queued. */

    /** Active clip rectangle (character cells). */
    struct {
        uint8_t x;
        uint8_t y;
        uint8_t w;
        uint8_t h;
    } clip;
    uint8_t has_clip;  /**< Non-zero when a clip region is active. */
} tui_render_queue_t;

/* =========================================================================
 * Queue API
 * ========================================================================= */

/**
 * @brief Reset the render queue (zero commands, no clip).
 *
 * Call once before building each frame.
 *
 * @param rq  Caller-allocated render queue.
 */
CC_EXTERN void tui_render_init(tui_render_queue_t *rq);

/**
 * @brief Append a render command to the queue.
 *
 * Silently drops the command if the queue is full.
 *
 * @param rq   Render queue.
 * @param cmd  Command to copy into the queue.
 */
CC_EXTERN void tui_render_push(tui_render_queue_t *rq,
                               const tui_render_cmd_t *cmd);

/**
 * @brief Flush all queued commands to the mcurses VT output layer.
 *
 * Iterates the queue, applies clip rectangles, and emits VT sequences
 * (cursor moves, SGR attributes, UTF-8 text, box-drawing characters)
 * via the mcurses_t instance.
 *
 * The queue is NOT automatically cleared – call tui_render_init()
 * before the next frame if you want a fresh queue.
 *
 * @param rq   Render queue to drain.
 * @param scr  mcurses screen instance for VT output.
 */
CC_EXTERN void tui_render_flush(tui_render_queue_t *rq, mcurses_t *scr);

/* =========================================================================
 * Frame-level API  (requires tui_layout.h – forward declared)
 * ========================================================================= */

/*
 * tui_render_frame() walks the layout context's element tree, generates
 * render commands for dirty elements, flushes them, and clears the
 * dirty flags.  It is declared here but only usable once tui_layout.h
 * is available (tui.h pulls both in).
 */
struct tui_context;  /* forward declaration – defined in tui_layout.h */

/**
 * @brief Render all dirty elements from a layout context.
 *
 * For each element with TUI_FLAG_DIRTY set:
 *  - CONTAINER → FILL (background) + optional BORDER
 *  - TEXT      → TEXT command
 *  - SCROLL    → FILL + visible lines from scroll buffer
 *
 * After rendering, dirty flags are cleared.
 *
 * @param ctx  Layout context (element tree owner).
 * @param rq   Caller-allocated render queue (will be init'd internally).
 * @param scr  mcurses screen for VT output.
 */
CC_EXTERN void tui_render_frame(struct tui_context *ctx,
                                tui_render_queue_t *rq,
                                mcurses_t *scr);

#ifdef __cplusplus
}
#endif

#endif /* __TUI_RENDER_H__ */

/**
 * @file tui_layout.h
 * @brief Clay-inspired TUI layout engine for protoduino / AVR embedded systems.
 *
 * Design principles
 * =================
 *  - Zero globals: all state lives in tui_context_t, caller-allocated.
 *  - Zero malloc: element arena is fixed-size (TUI_MAX_ELEMENTS).
 *  - Integer-only math: all coordinates are uint8_t (max 255 cells).
 *  - I/O via mcurses_t / ipc_pipe_t: decoupled from any transport.
 *  - Clay-compatible naming: TUI() macro, TUI_SIZING_GROW(), etc.
 *
 * Quick-start
 * ===========
 *
 *   static tui_context_t ctx;
 *   tui_init(&ctx, 24, 80);
 *
 *   tui_begin_layout(&ctx);
 *   TUI_CTX(&ctx) {
 *       TUI(TUI_ID("root"),
 *           &(tui_layout_config_t){ .width  = TUI_SIZING_GROW(),
 *                                   .height = TUI_SIZING_GROW(),
 *                                   .direction = TUI_DIR_LEFT_TO_RIGHT },
 *           &(tui_style_config_t){ .border = TUI_BORDER_SINGLE })
 *       {
 *           TUI_TEXT("Hello!", MCURSES_ATTR_BOLD);
 *       }
 *   }
 *   tui_end_layout(&ctx);
 *   tui_compute_layout(&ctx);
 */

#ifndef __TUI_LAYOUT_H__
#define __TUI_LAYOUT_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <protoduino.h>

#ifdef __AVR__
#  include <avr/pgmspace.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Sizing modes
 * ========================================================================= */

/**
 * @brief How a dimension (width or height) is determined during layout.
 */
typedef enum {
    TUI_SIZE_FIXED   = 0,  /**< Exact cell count.                         */
    TUI_SIZE_GROW    = 1,  /**< Expand to fill parent (with optional min). */
    TUI_SIZE_FIT     = 2,  /**< Shrink to content (with optional max).     */
    TUI_SIZE_PERCENT = 3,  /**< Percentage of parent.                      */
} tui_size_mode_t;

/**
 * @brief A single axis (width or height) sizing descriptor.
 *
 * 4 bytes.  Packed into element layout config.
 */
typedef struct {
    uint8_t mode;   /**< tui_size_mode_t                              */
    uint8_t value;  /**< Fixed cell count, or percent (1..100).       */
    uint8_t min;    /**< Minimum cells (GROW/FIT), 0 = no minimum.    */
    uint8_t max;    /**< Maximum cells (GROW/FIT), 255 = unlimited.   */
} tui_size_t;

/* =========================================================================
 * Convenience sizing macros  (Clay-compatible names with TUI_ prefix)
 * ========================================================================= */

/** Fixed size: exactly @p n cells. */
#define TUI_SIZING_FIXED(n) \
    ((tui_size_t){ TUI_SIZE_FIXED, (uint8_t)(n), 0, 0 })

/** Grow to fill available space.  Optional min_val (default 0). */
#define TUI_SIZING_GROW_MIN(min_val) \
    ((tui_size_t){ TUI_SIZE_GROW, 0, (uint8_t)(min_val), 255 })

/** Grow to fill – zero minimum. */
#define TUI_SIZING_GROW() \
    ((tui_size_t){ TUI_SIZE_GROW, 0, 0, 255 })

/** Fit to content.  Optional max_val (default 255 = unlimited). */
#define TUI_SIZING_FIT_MAX(max_val) \
    ((tui_size_t){ TUI_SIZE_FIT, 0, 0, (uint8_t)(max_val) })

/** Fit to content – no maximum. */
#define TUI_SIZING_FIT() \
    ((tui_size_t){ TUI_SIZE_FIT, 0, 0, 255 })

/** Percentage of parent. */
#define TUI_SIZING_PERCENT(p) \
    ((tui_size_t){ TUI_SIZE_PERCENT, (uint8_t)(p), 0, 0 })

/* =========================================================================
 * Direction & border enumerations
 * ========================================================================= */

/**
 * @brief Child layout direction within a container.
 */
typedef enum {
    TUI_DIR_TOP_TO_BOTTOM = 0,  /**< Vertical stack (default).   */
    TUI_DIR_LEFT_TO_RIGHT = 1,  /**< Horizontal row.             */
} tui_direction_t;

/**
 * @brief Box border style.  Each style maps to a set of six Unicode
 *        box-drawing characters: ─│┌┐└┘  ═║╔╗╚╝  ─│╭╮╰╯
 */
typedef enum {
    TUI_BORDER_NONE   = 0,  /**< No border.                     */
    TUI_BORDER_SINGLE = 1,  /**< ─│┌┐└┘  (single lines).       */
    TUI_BORDER_DOUBLE = 2,  /**< ═║╔╗╚╝  (double lines).       */
    TUI_BORDER_ROUND  = 3,  /**< ─│╭╮╰╯  (rounded corners).    */
} tui_border_style_t;

/* =========================================================================
 * Element types
 * ========================================================================= */

/**
 * @brief Discriminator for the element union data.
 */
typedef enum {
    TUI_ELEM_CONTAINER  = 0,  /**< Container with children.           */
    TUI_ELEM_TEXT       = 1,  /**< UTF-8 text leaf.                   */
    TUI_ELEM_SCROLL_AREA = 2, /**< Scrollable container (future).     */
} tui_elem_type_t;

/* =========================================================================
 * Layout & style configuration (passed to tui_open_element)
 * ========================================================================= */

/**
 * @brief Layout configuration for a container element.
 *
 * Matches Clay's CLAY_LAYOUT() concept.  10 bytes.
 */
typedef struct {
    tui_size_t     width;      /**< Horizontal sizing.                  */
    tui_size_t     height;     /**< Vertical sizing.                    */
    tui_direction_t direction; /**< Child layout direction.             */
    uint8_t        padding;    /**< Inner padding – all sides, in cells.*/
    uint8_t        gap;        /**< Gap between children, in cells.     */
} tui_layout_config_t;

/**
 * @brief Style configuration for an element.
 *
 * Matches Clay's CLAY_RECTANGLE() concept.  3 bytes.
 */
typedef struct {
    uint16_t attr;    /**< MCURSES_ATTR_* | MCURSES_FG() | MCURSES_BG(). */
    uint8_t  border;  /**< tui_border_style_t.                            */
} tui_style_config_t;

/* =========================================================================
 * Padding helper  (like CLAY_PADDING_ALL)
 * ========================================================================= */

/** Shorthand for uniform padding on all four sides. */
#define TUI_PADDING_ALL(n)  ((uint8_t)(n))

/* =========================================================================
 * Element flags
 * ========================================================================= */

#define TUI_FLAG_DIRTY       0x01u  /**< Element needs repaint.          */
#define TUI_FLAG_VISIBLE     0x02u  /**< Element is visible.             */
#define TUI_FLAG_FOCUSABLE   0x04u  /**< Element can receive focus.      */
#define TUI_FLAG_HAS_BORDER  0x08u  /**< Element draws a border.         */

/* =========================================================================
 * tui_element_t  –  retained element node
 * ========================================================================= */

/** Sentinel index meaning "no element" (root has no parent, etc.) */
#define TUI_INDEX_NONE  0xFFu

/**
 * @brief One node in the retained element tree.
 *
 * Designed for minimal SRAM.  The union at the end carries type-specific
 * payload without increasing the struct size for simple element types.
 */
typedef struct tui_element {
    /* ---- Tree linkage (4 bytes) ---- */
    uint8_t  id;             /**< Stable ID (hash of user string).        */
    uint8_t  parent;         /**< Parent index in array (0xFF = root).    */
    uint8_t  first_child;    /**< First child index (0xFF = none).        */
    uint8_t  next_sibling;   /**< Next sibling index (0xFF = none).       */

    /* ---- Type & flags (2 bytes) ---- */
    uint8_t  type;           /**< tui_elem_type_t.                        */
    uint8_t  flags;          /**< TUI_FLAG_DIRTY | VISIBLE | FOCUSABLE …  */

    /* ---- Computed bounding box (4 bytes) ---- */
    uint8_t  x;              /**< Left column (0-based, character cells). */
    uint8_t  y;              /**< Top row     (0-based, character cells). */
    uint8_t  w;              /**< Width  in character cells.              */
    uint8_t  h;              /**< Height in character cells.              */

    /* ---- Visual style (3+1 bytes) ---- */
    uint16_t style;          /**< Packed attr (same format as mcurses_t). */
    uint8_t  border;         /**< tui_border_style_t.                     */
    uint8_t  _pad;           /**< Alignment padding.                      */

    /* ---- Type-specific payload ---- */
    union {
        /** Text leaf: pointer into caller-owned string. */
        struct {
            const char *str;    /**< UTF-8 text (not owned).             */
            uint8_t     len;    /**< Byte length (0 = use strlen).       */
        } text;

        /** Scroll area: borrowed pointer to a tui_scrollbuf_t.
         *  Stored as void* so the layout module stays decoupled from
         *  tui_scroll.h; the renderer casts it back. */
        struct {
            void *sb;           /**< Pointer to tui_scrollbuf_t (not owned). */
        } scroll;

        /** Container: inline copy of the layout config. */
        struct {
            tui_layout_config_t layout;
        } container;
    } data;
} tui_element_t;

/* =========================================================================
 * Arena / context
 * ========================================================================= */

/** Maximum elements in a single layout tree (tune per-platform). */
#ifndef TUI_MAX_ELEMENTS
#define TUI_MAX_ELEMENTS  48
#endif

/** Maximum nesting depth of the build stack. */
#define TUI_MAX_DEPTH  8

/**
 * @brief All state for one layout tree.
 *
 * Caller-allocated, no globals.  Contains the element arena, the build
 * stack used during declaration, and current terminal dimensions.
 */
typedef struct tui_context {
    tui_element_t elements[TUI_MAX_ELEMENTS];

    uint8_t  elem_count;              /**< Current number of elements.     */
    uint8_t  build_stack[TUI_MAX_DEPTH]; /**< Parent indices during build. */
    uint8_t  stack_depth;             /**< Current nesting depth.          */

    uint8_t  root_w;                  /**< Terminal width  (columns).      */
    uint8_t  root_h;                  /**< Terminal height (rows).         */

    uint8_t  focused;                 /**< Focused element (0xFF = none).  */
    uint8_t  dirty_any;               /**< Non-zero if any dirty element.  */

    /* ---- Damage-diff shadow (previous frame) ----
     * Per-element appearance signature from the last computed frame.
     * tui_compute_layout() compares the new tree against this to decide
     * which elements actually changed, so unchanged regions produce no
     * VT output.  Compared by array position (declaration order is
     * deterministic frame-to-frame); structural changes conservatively
     * dirty the affected elements. */
    uint16_t prev_sig[TUI_MAX_ELEMENTS]; /**< Signature per element index. */
    uint8_t  prev_count;                 /**< Element count last frame.    */
} tui_context_t;

/* =========================================================================
 * ID hashing
 * ========================================================================= */

/**
 * @brief Hash a short string to a uint8_t element ID.
 *
 * Uses a truncated FNV-1a hash.  Collisions are rare for typical TUI
 * element names (< 50 elements).
 *
 * @param name  Null-terminated ASCII string.
 * @return 8-bit hash.
 */
CC_EXTERN uint8_t tui_hash_id(const char *name);

/** Convenience: hash a compile-time string literal to an element ID. */
#define TUI_ID(name)  tui_hash_id(name)

/* =========================================================================
 * Lifecycle API
 * ========================================================================= */

/**
 * @brief Initialise a layout context.
 *
 * Zeroes all elements, sets terminal dimensions.  Call once at start-up
 * or when the terminal is resized.
 *
 * @param ctx   Caller-allocated context.
 * @param rows  Terminal height in rows.
 * @param cols  Terminal width  in columns.
 */
CC_EXTERN void tui_init(tui_context_t *ctx, uint8_t rows, uint8_t cols);

/**
 * @brief Begin a new layout declaration pass.
 *
 * Resets the build stack and element count.  Preserves dirty flags of
 * elements that already existed so the renderer can diff.
 */
CC_EXTERN void tui_begin_layout(tui_context_t *ctx);

/**
 * @brief End the layout declaration pass.
 *
 * Marks any newly-added or structurally-changed elements as dirty.
 * After this call the tree is immutable until the next tui_begin_layout().
 */
CC_EXTERN void tui_end_layout(tui_context_t *ctx);

/* =========================================================================
 * Element declaration API
 * ========================================================================= */

/**
 * @brief Open a container element (push onto build stack).
 *
 * @param ctx     Layout context.
 * @param id      Stable element ID (use TUI_ID("name")).
 * @param layout  Layout configuration (may be NULL for defaults).
 * @param style   Style configuration  (may be NULL for defaults).
 * @return Index of the created element, or TUI_INDEX_NONE on overflow.
 */
CC_EXTERN uint8_t tui_open_element(tui_context_t *ctx,
                                    uint8_t id,
                                    const tui_layout_config_t *layout,
                                    const tui_style_config_t *style);

/**
 * @brief Close the most recently opened element (pop build stack).
 */
CC_EXTERN void tui_close_element(tui_context_t *ctx);

/**
 * @brief Add a text leaf element (no children).
 *
 * @param ctx   Layout context.
 * @param text  UTF-8 text (caller-owned, must remain valid until render).
 * @param len   Byte length of text (0 = use strlen internally).
 * @param attr  MCURSES_ATTR_* packed attributes for the text.
 */
CC_EXTERN void tui_text_element(tui_context_t *ctx,
                                 const char *text,
                                 uint8_t len,
                                 uint16_t attr);

/**
 * @brief Add a scrollable text-area leaf bound to a scroll buffer.
 *
 * The element renders the visible window of @p scrollbuf within its
 * computed bounding box (background fill + one TEXT line per visible row,
 * clipped to the box).  The scroll buffer is borrowed, not owned, and
 * must outlive the render call.
 *
 * @param ctx        Layout context.
 * @param scrollbuf  Pointer to a tui_scrollbuf_t (passed as void* to keep
 *                   the layout module decoupled from tui_scroll.h).
 * @param attr       Background attribute for the area fill.
 */
CC_EXTERN void tui_scroll_element(tui_context_t *ctx,
                                  void *scrollbuf,
                                  uint16_t attr);

/* =========================================================================
 * Dirty tracking
 * ========================================================================= */

/**
 * @brief Mark a specific element as needing repaint.
 *
 * @param ctx  Layout context.
 * @param id   Element ID (hash).
 */
CC_EXTERN void tui_mark_dirty(tui_context_t *ctx, uint8_t id);

/**
 * @brief Mark every visible element as dirty (full repaint).
 */
CC_EXTERN void tui_mark_all_dirty(tui_context_t *ctx);

/* =========================================================================
 * Layout computation
 * ========================================================================= */

/**
 * @brief Run the integer flexbox layout algorithm.
 *
 * Two passes:
 *   1. Bottom-up: compute intrinsic sizes for FIT elements.
 *   2. Top-down:  distribute available space to GROW elements.
 *
 * Accounts for borders (±2 cells), padding, and inter-child gaps.
 * All arithmetic is uint8_t (max 255 cols/rows).  Elements that
 * overflow their parent are clipped to the parent bounding box.
 */
CC_EXTERN void tui_compute_layout(tui_context_t *ctx);

/* =========================================================================
 * Macro API  (Clay-compatible declaration style)
 * =========================================================================
 *
 * Usage:
 *
 *   TUI_CTX(&ctx) {
 *       TUI(TUI_ID("sidebar"),
 *           &(tui_layout_config_t){ .width = TUI_SIZING_FIXED(20),
 *                                   .height = TUI_SIZING_GROW() },
 *           &(tui_style_config_t){ .border = TUI_BORDER_SINGLE })
 *       {
 *           TUI_TEXT("Menu", MCURSES_ATTR_BOLD);
 *       }
 *   }
 *
 * The TUI() macro uses Clay's for-loop trick: open element, execute
 * body (children), then close element – all without goto or cleanup.
 */

/**
 * @brief Set the implicit _tui_ctx pointer used by TUI() / TUI_TEXT().
 *
 * Opens a block scope; use with braces:
 *   TUI_CTX(&my_ctx) { ... }
 */
#define TUI_CTX(ctx_ptr) \
    for (tui_context_t *_tui_ctx = (ctx_ptr), *_tui_once_ctx = _tui_ctx; \
         _tui_once_ctx; \
         _tui_once_ctx = (tui_context_t *)0)

/**
 * @brief Declare a container element with children in the body.
 *
 * @param id_expr   Element ID (use TUI_ID("name")).
 * @param ...       The layout-config pointer and style-config pointer, e.g.
 *                  &(tui_layout_config_t){...}, &(tui_style_config_t){...}
 *                  (either may be NULL).
 *
 * The macro is variadic so that compound-literal config arguments — whose
 * internal commas the preprocessor would otherwise treat as macro-argument
 * separators — pass through untouched into the tui_open_element() call,
 * where the surrounding parentheses protect them.
 */
#define TUI(id_expr, ...) \
    for (uint8_t _tui_once = (tui_open_element(_tui_ctx, (id_expr), \
                                                __VA_ARGS__), 1); \
         _tui_once; \
         _tui_once = (tui_close_element(_tui_ctx), 0))

/**
 * @brief Add a text leaf element.
 *
 * @param text_str  UTF-8 string literal or pointer.
 * @param attr_val  MCURSES_ATTR_* packed attributes.
 */
#define TUI_TEXT(text_str, attr_val) \
    tui_text_element(_tui_ctx, (text_str), 0, (attr_val))

/**
 * @brief Attach a scroll buffer to the current scope as a scrollable area.
 *
 * @param sb_ptr    Pointer to a tui_scrollbuf_t.
 * @param attr_val  Background attribute for the area fill.
 */
#define TUI_SCROLL(sb_ptr, attr_val) \
    tui_scroll_element(_tui_ctx, (sb_ptr), (attr_val))

#ifdef __cplusplus
}
#endif

#endif /* __TUI_LAYOUT_H__ */

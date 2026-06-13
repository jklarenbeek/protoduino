/**
 * @file tui_layout.c
 * @brief Clay-inspired TUI layout engine – implementation.
 *
 * All layout computation is integer-only (uint8_t arithmetic, max 255).
 * No globals.  No malloc.  Fits on an Arduino Uno (ATmega328P, 2 KB SRAM).
 *
 * Layout algorithm (two-pass integer flexbox):
 *   Pass 1 (bottom-up):  compute intrinsic sizes for FIT elements.
 *   Pass 2 (top-down):   distribute space for GROW elements, resolve
 *                         PERCENT, apply padding/gap/border, clip.
 */

#include "tui_layout.h"

#include <string.h>    /* memset, memcpy, strlen */

#ifdef __AVR__
#  include <avr/pgmspace.h>
#else
#  ifndef PSTR
#    define PSTR(s)          (s)
#  endif
#  ifndef pgm_read_byte
#    define pgm_read_byte(p) (*(const uint8_t *)(p))
#  endif
#endif

/* =========================================================================
 * Internal helpers
 * ========================================================================= */

/** Saturating uint8_t addition: min(a + b, 255). */
static inline uint8_t _sat_add8(uint8_t a, uint8_t b)
{
    uint16_t s = (uint16_t)a + (uint16_t)b;
    return (s > 255u) ? 255u : (uint8_t)s;
}

/** Saturating uint8_t subtraction: max(a - b, 0). */
static inline uint8_t _sat_sub8(uint8_t a, uint8_t b)
{
    return (a > b) ? (uint8_t)(a - b) : 0u;
}

/** Clamp value to [lo, hi]. */
static inline uint8_t _clamp8(uint8_t v, uint8_t lo, uint8_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/**
 * @brief Byte-length of a RAM UTF-8 string, limited to 255.
 *
 * Used to determine the display width of text elements.
 * NOTE: assumes 1 rune ≈ 1 cell for simplicity on embedded targets.
 *       A proper wcwidth table could be added later.
 */
static uint8_t _text_display_width(const char *str, uint8_t byte_len)
{
    if (!str)
        return 0;

    if (byte_len == 0)
        byte_len = (uint8_t)(strlen(str) & 0xFFu);

    /*
     * Walk the UTF-8 string counting runes (= columns).
     * Each lead byte increments the count; continuation bytes (0x80-0xBF)
     * are skipped.  This gives rune count, which we treat as cell width.
     */
    uint8_t cols = 0;
    for (uint8_t i = 0; i < byte_len; ++i) {
        uint8_t c = (uint8_t)str[i];
        if (c == 0)
            break;
        /* Skip continuation bytes */
        if ((c & 0xC0u) != 0x80u)
            ++cols;
    }
    return cols;
}

/* =========================================================================
 * tui_hash_id  –  FNV-1a truncated to uint8_t
 * ========================================================================= */

uint8_t tui_hash_id(const char *name)
{
    /*
     * 8-bit FNV-1a.  The classic FNV multiply uses the 32-bit prime
     * 0x01000193; truncated to 8 bits its only set low byte is 0x93, so we
     * mix with 0x93 (an odd multiplier, which is invertible mod 256 and
     * therefore loses no information per step).  This replaces the previous
     * multiply-by-0x01 (a no-op that left the hash order-/avalanche-weak).
     *
     * NOTE: an 8-bit id has only 256 buckets, so collisions are inherent
     * once a tree approaches a few dozen elements (birthday bound).  The id
     * is used only by tui_mark_dirty() for targeted repaints; the damage
     * diff in tui_compute_layout() detects changes independently of it.
     */
    uint8_t h = 0xC1u;  /* FNV-1a offset basis (2166136261) mod 256 */
    if (!name)
        return h;
    while (*name) {
        h ^= (uint8_t)*name;
        h  = (uint8_t)(h * 0x93u);  /* FNV prime, low byte */
        ++name;
    }
    return h;
}

/* =========================================================================
 * Lifecycle
 * ========================================================================= */

void tui_init(tui_context_t *ctx, uint8_t rows, uint8_t cols)
{
    if (!ctx)
        return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->root_w   = cols;
    ctx->root_h   = rows;
    ctx->focused  = TUI_INDEX_NONE;
}

void tui_begin_layout(tui_context_t *ctx)
{
    if (!ctx)
        return;

    /*
     * Preserve dirty flags of existing elements so the renderer can
     * detect what changed after the new tree is built.
     *
     * We save the old element IDs and their dirty bits in the unused
     * upper region of the build_stack (which is only 8 bytes).
     * For a proper diff we'd need a shadow copy – but on AVR we keep
     * it simple: clear all elements and force a full dirty pass in
     * tui_end_layout() for any structural change.
     */
    ctx->elem_count  = 0;
    ctx->stack_depth = 0;
    ctx->dirty_any   = 0;
}

/* =========================================================================
 * Element declaration
 * ========================================================================= */

/**
 * @brief Allocate a new element slot from the arena.
 * @return Index, or TUI_INDEX_NONE if full.
 */
static uint8_t _alloc_element(tui_context_t *ctx)
{
    if (ctx->elem_count >= TUI_MAX_ELEMENTS)
        return TUI_INDEX_NONE;
    uint8_t idx = ctx->elem_count++;
    memset(&ctx->elements[idx], 0, sizeof(tui_element_t));
    ctx->elements[idx].parent       = TUI_INDEX_NONE;
    ctx->elements[idx].first_child  = TUI_INDEX_NONE;
    ctx->elements[idx].next_sibling = TUI_INDEX_NONE;
    ctx->elements[idx].flags        = TUI_FLAG_VISIBLE;
    return idx;
}

/**
 * @brief Attach element @p child_idx as the last child of @p parent_idx.
 */
static void _attach_child(tui_context_t *ctx, uint8_t parent_idx, uint8_t child_idx)
{
    tui_element_t *parent = &ctx->elements[parent_idx];
    tui_element_t *child  = &ctx->elements[child_idx];

    child->parent = parent_idx;

    if (parent->first_child == TUI_INDEX_NONE) {
        parent->first_child = child_idx;
    } else {
        /* Walk to the last sibling */
        uint8_t sib = parent->first_child;
        while (ctx->elements[sib].next_sibling != TUI_INDEX_NONE)
            sib = ctx->elements[sib].next_sibling;
        ctx->elements[sib].next_sibling = child_idx;
    }
}

/**
 * @brief Parent index for a child declared at the current depth, or
 *        TUI_INDEX_NONE if there is none (top level, depth overflow, or a
 *        dropped ancestor) — in which case the child must be dropped rather
 *        than attached to a bogus parent.
 */
static uint8_t _current_parent(const tui_context_t *ctx)
{
    if (ctx->stack_depth == 0 || ctx->stack_depth > TUI_MAX_DEPTH)
        return TUI_INDEX_NONE;
    return ctx->build_stack[ctx->stack_depth - 1u];
}

uint8_t tui_open_element(tui_context_t *ctx,
                          uint8_t id,
                          const tui_layout_config_t *layout,
                          const tui_style_config_t *style)
{
    if (!ctx)
        return TUI_INDEX_NONE;

    /*
     * Determine our parent.  The root (stack_depth == 0) has none and is
     * allowed; any deeper level needs a valid stored parent.  If there is no
     * valid parent (arena/depth overflow earlier in the tree) the element is
     * dropped — but we STILL push a slot and bump the depth so that the
     * matching tui_close_element() stays balanced and never pops the wrong
     * parent.  A dropped element pushes TUI_INDEX_NONE, so its own children
     * also see "no parent" and are dropped cleanly (no orphans, no
     * out-of-bounds attach).
     */
    uint8_t parent_idx = _current_parent(ctx);
    uint8_t can_alloc  = (ctx->stack_depth == 0) || (parent_idx != TUI_INDEX_NONE);

    uint8_t idx = TUI_INDEX_NONE;
    if (can_alloc)
        idx = _alloc_element(ctx);     /* TUI_INDEX_NONE if arena full */

    if (idx != TUI_INDEX_NONE) {
        tui_element_t *el = &ctx->elements[idx];
        el->id   = id;
        el->type = TUI_ELEM_CONTAINER;

        /* Copy layout config */
        if (layout) {
            memcpy(&el->data.container.layout, layout, sizeof(tui_layout_config_t));
        } else {
            /* Default: GROW in both axes, top-to-bottom */
            el->data.container.layout.width  = (tui_size_t){ TUI_SIZE_GROW, 0, 0, 255 };
            el->data.container.layout.height = (tui_size_t){ TUI_SIZE_GROW, 0, 0, 255 };
            el->data.container.layout.direction = TUI_DIR_TOP_TO_BOTTOM;
            el->data.container.layout.padding = 0;
            el->data.container.layout.gap     = 0;
        }

        /* Apply style */
        if (style) {
            el->style  = style->attr;
            el->border = style->border;
            if (style->border != TUI_BORDER_NONE)
                el->flags |= TUI_FLAG_HAS_BORDER;
        }

        if (parent_idx != TUI_INDEX_NONE)
            _attach_child(ctx, parent_idx, idx);
    }

    /* Always record this level (idx, possibly NONE) and bump the depth so
     * open/close stay balanced.  Levels beyond TUI_MAX_DEPTH are counted but
     * not stored (no room); their children see parent == NONE and drop. */
    if (ctx->stack_depth < TUI_MAX_DEPTH)
        ctx->build_stack[ctx->stack_depth] = idx;
    if (ctx->stack_depth < 0xFFu)
        ++ctx->stack_depth;

    return idx;
}

void tui_close_element(tui_context_t *ctx)
{
    if (!ctx || ctx->stack_depth == 0)
        return;
    --ctx->stack_depth;
}

void tui_text_element(tui_context_t *ctx,
                       const char *text,
                       uint8_t len,
                       uint16_t attr)
{
    if (!ctx || !text)
        return;

    /* A leaf needs a valid parent; drop it if there is none (top level,
     * depth overflow, or under a dropped ancestor) — never attach to a
     * bogus parent index. */
    uint8_t parent_idx = _current_parent(ctx);
    if (parent_idx == TUI_INDEX_NONE)
        return;

    uint8_t idx = _alloc_element(ctx);
    if (idx == TUI_INDEX_NONE)
        return;

    tui_element_t *el = &ctx->elements[idx];
    el->type = TUI_ELEM_TEXT;
    el->style = attr;

    el->data.text.str = text;
    el->data.text.len = (len != 0) ? len : (uint8_t)(strlen(text) & 0xFFu);

    _attach_child(ctx, parent_idx, idx);
}

void tui_scroll_element(tui_context_t *ctx, void *scrollbuf, uint16_t attr)
{
    if (!ctx || !scrollbuf)
        return;

    uint8_t parent_idx = _current_parent(ctx);
    if (parent_idx == TUI_INDEX_NONE)
        return;

    uint8_t idx = _alloc_element(ctx);
    if (idx == TUI_INDEX_NONE)
        return;

    tui_element_t *el = &ctx->elements[idx];
    el->type  = TUI_ELEM_SCROLL_AREA;
    el->style = attr;
    el->data.scroll.sb = scrollbuf;

    _attach_child(ctx, parent_idx, idx);
}

/* =========================================================================
 * End layout & dirty tracking
 * ========================================================================= */

void tui_end_layout(tui_context_t *ctx)
{
    if (!ctx)
        return;

    /*
     * The tree is already linked during tui_open_element/tui_close_element,
     * so there is no structural work to do here.  Crucially we do NOT
     * force every element dirty: damage tracking lives in
     * tui_compute_layout(), which diffs each element's appearance against
     * the previous frame so unchanged regions emit no VT output.
     *
     * (Any dirty bits set manually via tui_mark_dirty() before the diff
     * runs are preserved and OR-ed in by the diff.)
     */
    (void)ctx;
}

void tui_mark_dirty(tui_context_t *ctx, uint8_t id)
{
    if (!ctx)
        return;
    for (uint8_t i = 0; i < ctx->elem_count; ++i) {
        if (ctx->elements[i].id == id) {
            ctx->elements[i].flags |= TUI_FLAG_DIRTY;
            ctx->dirty_any = 1;
            return;
        }
    }
}

void tui_mark_all_dirty(tui_context_t *ctx)
{
    if (!ctx)
        return;
    for (uint8_t i = 0; i < ctx->elem_count; ++i) {
        if (ctx->elements[i].flags & TUI_FLAG_VISIBLE)
            ctx->elements[i].flags |= TUI_FLAG_DIRTY;
    }
    ctx->dirty_any = 1;
}

/* =========================================================================
 * Layout computation – integer flexbox
 * =========================================================================
 *
 * Algorithm overview:
 *
 * Pass 1 – intrinsic sizes (bottom-up, post-order):
 *   For each element, compute the "desired" width/height:
 *     FIXED   → value
 *     PERCENT → parent_dim * value / 100  (resolved in pass 2)
 *     FIT     → sum of children's intrinsic sizes (+ gaps + padding + border)
 *     GROW    → 0 (will be resolved in pass 2)
 *   Text elements have intrinsic width = rune count, height = 1.
 *
 * Pass 2 – space distribution (top-down, pre-order):
 *   Root gets x=0, y=0, w=root_w, h=root_h.
 *   For each container, compute the inner content area:
 *     inner_w = w - border*2 - padding*2
 *     inner_h = h - border*2 - padding*2
 *   Then distribute inner space to children along the layout axis:
 *     1. Sum up fixed / percent / fit sizes.
 *     2. Remaining space is split among GROW children.
 *     3. Position children with gap between them.
 *     4. Clip children to parent bounds.
 */

/* ---- Pass 1 helpers ---- */

/** Return the intrinsic width of a single element (before GROW/PERCENT). */
static uint8_t _intrinsic_w(tui_context_t *ctx, uint8_t idx);
/** Return the intrinsic height of a single element. */
static uint8_t _intrinsic_h(tui_context_t *ctx, uint8_t idx);

/** Count direct children of element at @p idx. */
static uint8_t _child_count(const tui_context_t *ctx, uint8_t idx)
{
    uint8_t n = 0;
    uint8_t c = ctx->elements[idx].first_child;
    while (c != TUI_INDEX_NONE) {
        ++n;
        c = ctx->elements[c].next_sibling;
    }
    return n;
}

/** Border overhead: 2 cells if bordered, else 0. */
static inline uint8_t _border_overhead(const tui_element_t *el)
{
    return (el->flags & TUI_FLAG_HAS_BORDER) ? 2u : 0u;
}

static uint8_t _intrinsic_w(tui_context_t *ctx, uint8_t idx)
{
    tui_element_t *el = &ctx->elements[idx];

    /* Text leaf: display width */
    if (el->type == TUI_ELEM_TEXT)
        return _text_display_width(el->data.text.str, el->data.text.len);

    /* Scroll area: a flexible viewport with no intrinsic size.  It has no
     * layout config in its union, so it must NOT fall through to the
     * container path below (that would reinterpret the scroll pointer as a
     * layout struct).  Treated as GROW; pass 2 fills it. */
    if (el->type == TUI_ELEM_SCROLL_AREA)
        return 0;

    /* Container */
    const tui_layout_config_t *lay = &el->data.container.layout;

    if (lay->width.mode == TUI_SIZE_FIXED)
        return lay->width.value;

    if (lay->width.mode == TUI_SIZE_GROW)
        return lay->width.min;  /* minimum for now; pass 2 expands */

    /* FIT: accumulate children */
    if (lay->width.mode == TUI_SIZE_FIT) {
        uint8_t total = 0;
        uint8_t nchildren = 0;
        uint8_t c = el->first_child;

        while (c != TUI_INDEX_NONE) {
            uint8_t cw = _intrinsic_w(ctx, c);
            if (lay->direction == TUI_DIR_LEFT_TO_RIGHT) {
                total = _sat_add8(total, cw);
            } else {
                /* Vertical stacking: width = max of children */
                if (cw > total) total = cw;
            }
            ++nchildren;
            c = ctx->elements[c].next_sibling;
        }

        /* Add gaps (horizontal direction only) */
        if (lay->direction == TUI_DIR_LEFT_TO_RIGHT && nchildren > 1)
            total = _sat_add8(total, (uint8_t)((nchildren - 1u) * lay->gap));

        /* Add border + padding */
        total = _sat_add8(total, _border_overhead(el));
        total = _sat_add8(total, (uint8_t)(lay->padding * 2u));

        return _clamp8(total, lay->width.min, lay->width.max);
    }

    /* PERCENT: cannot resolve yet (needs parent), return min */
    return lay->width.min;
}

static uint8_t _intrinsic_h(tui_context_t *ctx, uint8_t idx)
{
    tui_element_t *el = &ctx->elements[idx];

    /* Text leaf: 1 row */
    if (el->type == TUI_ELEM_TEXT)
        return 1;

    /* Scroll area: flexible viewport, no intrinsic size (see _intrinsic_w). */
    if (el->type == TUI_ELEM_SCROLL_AREA)
        return 0;

    /* Container */
    const tui_layout_config_t *lay = &el->data.container.layout;

    if (lay->height.mode == TUI_SIZE_FIXED)
        return lay->height.value;

    if (lay->height.mode == TUI_SIZE_GROW)
        return lay->height.min;

    /* FIT */
    if (lay->height.mode == TUI_SIZE_FIT) {
        uint8_t total = 0;
        uint8_t nchildren = 0;
        uint8_t c = el->first_child;

        while (c != TUI_INDEX_NONE) {
            uint8_t ch = _intrinsic_h(ctx, c);
            if (lay->direction == TUI_DIR_TOP_TO_BOTTOM) {
                total = _sat_add8(total, ch);
            } else {
                /* Horizontal row: height = max of children */
                if (ch > total) total = ch;
            }
            ++nchildren;
            c = ctx->elements[c].next_sibling;
        }

        /* Add gaps (vertical direction only) */
        if (lay->direction == TUI_DIR_TOP_TO_BOTTOM && nchildren > 1)
            total = _sat_add8(total, (uint8_t)((nchildren - 1u) * lay->gap));

        /* Border + padding */
        total = _sat_add8(total, _border_overhead(el));
        total = _sat_add8(total, (uint8_t)(lay->padding * 2u));

        return _clamp8(total, lay->height.min, lay->height.max);
    }

    /* PERCENT */
    return lay->height.min;
}

/* ---- Pass 2: top-down space distribution ---- */

/**
 * @brief Resolve the final size for one axis given parent's available size.
 *
 * @param sz           The sizing descriptor.
 * @param parent_dim   Parent's available cells on this axis.
 * @param intrinsic    Intrinsic size from pass 1.
 * @return Resolved cell count.
 */
static uint8_t _resolve_size(const tui_size_t *sz,
                              uint8_t parent_dim,
                              uint8_t intrinsic)
{
    switch (sz->mode) {
    case TUI_SIZE_FIXED:
        return sz->value;

    case TUI_SIZE_PERCENT:
        {
            uint16_t v = (uint16_t)parent_dim * (uint16_t)sz->value / 100u;
            return (v > 255u) ? 255u : (uint8_t)v;
        }

    case TUI_SIZE_FIT:
        return _clamp8(intrinsic, sz->min, sz->max);

    case TUI_SIZE_GROW:
        /* GROW is handled specially by the distribution loop.
         * Fallback: return the minimum. */
        return sz->min;

    default:
        return 0;
    }
}

/** Primary-axis sizing descriptor for a child, or NULL if it has none
 *  (text and scroll leaves carry no layout config). */
static const tui_size_t *_primary_size(const tui_element_t *child,
                                        uint8_t is_horizontal)
{
    if (child->type == TUI_ELEM_CONTAINER)
        return is_horizontal ? &child->data.container.layout.width
                             : &child->data.container.layout.height;
    return (const tui_size_t *)0;
}

/** Does this child expand to fill the primary axis?
 *  True for GROW containers and for scroll areas (which always fill). */
static uint8_t _is_grow_primary(const tui_element_t *child, uint8_t is_horizontal)
{
    if (child->type == TUI_ELEM_SCROLL_AREA)
        return 1u;
    const tui_size_t *sz = _primary_size(child, is_horizontal);
    return (sz && sz->mode == TUI_SIZE_GROW) ? 1u : 0u;
}

/**
 * @brief Distribute space among children of a container.
 *
 * Computes each child's x, y, w, h within the parent's content area.
 */
static void _distribute_children(tui_context_t *ctx, uint8_t parent_idx)
{
    tui_element_t *parent = &ctx->elements[parent_idx];

    /* Containers only */
    if (parent->type != TUI_ELEM_CONTAINER)
        return;
    if (parent->first_child == TUI_INDEX_NONE)
        return;

    const tui_layout_config_t *lay = &parent->data.container.layout;

    /* Compute inner content area */
    uint8_t bord = _border_overhead(parent);
    uint8_t pad2 = (uint8_t)(lay->padding * 2u);
    uint8_t inner_w = _sat_sub8(parent->w, _sat_add8(bord, pad2));
    uint8_t inner_h = _sat_sub8(parent->h, _sat_add8(bord, pad2));
    uint8_t content_x = _sat_add8(parent->x, (uint8_t)(bord / 2u + lay->padding));
    uint8_t content_y = _sat_add8(parent->y, (uint8_t)(bord / 2u + lay->padding));

    /* Count children and total gap overhead */
    uint8_t nchildren = _child_count(ctx, parent_idx);
    uint8_t total_gap = (nchildren > 1u) ? (uint8_t)((nchildren - 1u) * lay->gap) : 0u;

    /* Primary axis is the layout direction */
    uint8_t is_horizontal = (lay->direction == TUI_DIR_LEFT_TO_RIGHT);
    uint8_t primary_available = is_horizontal ? inner_w : inner_h;
    uint8_t cross_available   = is_horizontal ? inner_h : inner_w;

    /* --- Phase A: determine non-GROW sizes and count GROW children --- */
    uint8_t fixed_total  = 0;
    uint8_t grow_count   = 0;

    uint8_t c = parent->first_child;
    while (c != TUI_INDEX_NONE) {
        tui_element_t *child = &ctx->elements[c];
        const tui_size_t *sz = _primary_size(child, is_horizontal);

        if (_is_grow_primary(child, is_horizontal)) {
            ++grow_count;
        } else {
            /* Resolve non-GROW primary size (FIXED/PERCENT/FIT container, or
             * a text leaf sized to its content). */
            uint8_t intrinsic = is_horizontal ? _intrinsic_w(ctx, c)
                                              : _intrinsic_h(ctx, c);
            uint8_t child_primary;
            if (sz) {
                child_primary = _resolve_size(sz, primary_available, intrinsic);
            } else {
                child_primary = intrinsic;
            }
            fixed_total = _sat_add8(fixed_total, child_primary);

            /* Temporarily stash in w/h so we don't recompute */
            if (is_horizontal)
                child->w = child_primary;
            else
                child->h = child_primary;
        }

        c = child->next_sibling;
    }

    /* --- Phase B: distribute remaining space among GROW children --- */
    uint8_t grow_space = _sat_sub8(primary_available,
                                    _sat_add8(fixed_total, total_gap));
    uint8_t grow_each  = (grow_count > 0) ? (uint8_t)(grow_space / grow_count) : 0u;
    uint8_t grow_remainder = (grow_count > 0) ? (uint8_t)(grow_space % grow_count) : 0u;

    uint8_t grow_idx = 0;
    c = parent->first_child;
    while (c != TUI_INDEX_NONE) {
        tui_element_t *child = &ctx->elements[c];

        if (_is_grow_primary(child, is_horizontal)) {
            const tui_size_t *sz = _primary_size(child, is_horizontal);
            uint8_t this_grow = grow_each;
            /* Last GROW child absorbs the remainder */
            if (++grow_idx == grow_count)
                this_grow = _sat_add8(this_grow, grow_remainder);
            /* Clamp to min (scroll areas have no config, so min = 0) */
            uint8_t minv = sz ? sz->min : 0u;
            this_grow = (this_grow < minv) ? minv : this_grow;

            if (is_horizontal)
                child->w = this_grow;
            else
                child->h = this_grow;
        }

        c = child->next_sibling;
    }

    /* --- Phase C: resolve cross-axis sizes --- */
    c = parent->first_child;
    while (c != TUI_INDEX_NONE) {
        tui_element_t *child = &ctx->elements[c];

        /* Scroll areas fill the cross axis (no intrinsic cross size). */
        if (child->type == TUI_ELEM_SCROLL_AREA) {
            if (is_horizontal)
                child->h = cross_available;
            else
                child->w = cross_available;
            c = child->next_sibling;
            continue;
        }

        const tui_size_t *cross_sz = NULL;

        if (child->type == TUI_ELEM_CONTAINER)
            cross_sz = is_horizontal ? &child->data.container.layout.height
                                     : &child->data.container.layout.width;

        uint8_t intrinsic_cross = is_horizontal ? _intrinsic_h(ctx, c)
                                                : _intrinsic_w(ctx, c);

        if (cross_sz) {
            uint8_t resolved;
            if (cross_sz->mode == TUI_SIZE_GROW) {
                /* GROW on cross axis = fill parent */
                resolved = cross_available;
                resolved = (resolved < cross_sz->min) ? cross_sz->min : resolved;
            } else {
                resolved = _resolve_size(cross_sz, cross_available, intrinsic_cross);
            }
            if (is_horizontal)
                child->h = resolved;
            else
                child->w = resolved;
        } else {
            /* Text: use intrinsic */
            if (is_horizontal)
                child->h = intrinsic_cross;
            else
                child->w = intrinsic_cross;
        }

        c = child->next_sibling;
    }

    /* --- Phase D: position children along primary axis --- */
    uint8_t pos = is_horizontal ? content_x : content_y;

    /* Children must stay inside the parent's *content* area, not its outer
     * box: clipping to parent->x+parent->w would let a child overrun the
     * parent's own border/padding cells.  Clip to the inner rectangle. */
    uint8_t content_r = _sat_add8(content_x, inner_w);  /* content right edge  */
    uint8_t content_b = _sat_add8(content_y, inner_h);  /* content bottom edge */

    c = parent->first_child;
    while (c != TUI_INDEX_NONE) {
        tui_element_t *child = &ctx->elements[c];

        if (is_horizontal) {
            child->x = pos;
            child->y = content_y;
            pos = _sat_add8(pos, _sat_add8(child->w, lay->gap));
        } else {
            child->x = content_x;
            child->y = pos;
            pos = _sat_add8(pos, _sat_add8(child->h, lay->gap));
        }

        /* Clip to parent content area */
        if (_sat_add8(child->x, child->w) > content_r)
            child->w = _sat_sub8(content_r, child->x);
        if (_sat_add8(child->y, child->h) > content_b)
            child->h = _sat_sub8(content_b, child->y);

        c = child->next_sibling;
    }
}

/* ---- Recursive top-down layout ---- */

static void _layout_subtree(tui_context_t *ctx, uint8_t idx)
{
    _distribute_children(ctx, idx);

    /* Recurse into children */
    uint8_t c = ctx->elements[idx].first_child;
    while (c != TUI_INDEX_NONE) {
        _layout_subtree(ctx, c);
        c = ctx->elements[c].next_sibling;
    }
}

/* =========================================================================
 * Damage tracking – per-element appearance diff
 * =========================================================================
 *
 * After geometry is computed we condense each element's visible appearance
 * (id, type, position, size, style, border, text/scroll payload) into a
 * 16-bit signature and compare it, by array position, against the previous
 * frame's signatures.  Only elements whose signature changed (or that were
 * marked dirty manually) are repainted; everything else emits no VT output.
 */

/** Condense an element's visible appearance to a 16-bit signature. */
static uint16_t _element_sig(const tui_element_t *el)
{
    uint16_t h = 0xA53Cu;  /* arbitrary non-zero seed */

#define _SIG_MIX(v)                                              \
    do {                                                         \
        h ^= (uint16_t)(v);                                      \
        h  = (uint16_t)((h << 5) | (h >> 11));  /* rotl 5 */     \
        h  = (uint16_t)(h * 0x9E37u + 1u);      /* mix       */  \
    } while (0)

    _SIG_MIX(el->id);
    _SIG_MIX(el->type);
    _SIG_MIX(((uint16_t)el->x << 8) | el->y);
    _SIG_MIX(((uint16_t)el->w << 8) | el->h);
    _SIG_MIX(el->style);
    _SIG_MIX(el->border);

    if (el->type == TUI_ELEM_TEXT) {
        uintptr_t p = (uintptr_t)el->data.text.str;
        _SIG_MIX((uint16_t)(p & 0xFFFFu));
        _SIG_MIX((uint16_t)((p >> 16) & 0xFFFFu));
        _SIG_MIX(el->data.text.len);
    } else if (el->type == TUI_ELEM_SCROLL_AREA) {
        uintptr_t p = (uintptr_t)el->data.scroll.sb;
        _SIG_MIX((uint16_t)(p & 0xFFFFu));
        _SIG_MIX((uint16_t)((p >> 16) & 0xFFFFu));
        /* Scroll *content* changes are detected by the renderer via the
         * scroll buffer's own dirty flag, not here. */
    }

#undef _SIG_MIX
    return h;
}

/** Compute dirty flags by diffing the new frame against the previous one. */
static void _compute_damage(tui_context_t *ctx)
{
    uint8_t n = ctx->elem_count;

    /*
     * Pass 1 – seed: an element is a "seed" if its signature changed since
     * last frame, it is new this frame, or it was marked dirty manually
     * before compute.  Each seed also dirties its immediate parent so the
     * parent's background FILL clears any cells the child vacated (e.g.
     * shrinking text) on repaint.  Parents always have a lower array index
     * than their children, so dirtying the parent here can never re-seed an
     * element (its own flag at index i reflects only manual marks).
     */
    for (uint8_t i = 0; i < n; ++i) {
        uint16_t sig = _element_sig(&ctx->elements[i]);
        uint8_t seed = (i >= ctx->prev_count)
                     || (sig != ctx->prev_sig[i])
                     || (ctx->elements[i].flags & TUI_FLAG_DIRTY);
        ctx->prev_sig[i] = sig;

        if (seed) {
            ctx->elements[i].flags |= TUI_FLAG_DIRTY;
            uint8_t p = ctx->elements[i].parent;
            if (p != TUI_INDEX_NONE)
                ctx->elements[p].flags |= TUI_FLAG_DIRTY;
        }
    }

    /*
     * Pass 2 – propagate down: re-filling a dirty container erases its
     * children, so every descendant of a dirty element must repaint.  A
     * single forward sweep cascades through all depths because a parent's
     * index always precedes its children's.
     */
    for (uint8_t i = 0; i < n; ++i) {
        uint8_t p = ctx->elements[i].parent;
        if (p != TUI_INDEX_NONE && (ctx->elements[p].flags & TUI_FLAG_DIRTY))
            ctx->elements[i].flags |= TUI_FLAG_DIRTY;
    }

    /* Remember this frame's element count and summarise. */
    ctx->prev_count = n;
    ctx->dirty_any  = 0;
    for (uint8_t i = 0; i < n; ++i) {
        if (ctx->elements[i].flags & TUI_FLAG_DIRTY) {
            ctx->dirty_any = 1;
            break;
        }
    }
}

/* =========================================================================
 * Public: tui_compute_layout
 * ========================================================================= */

void tui_compute_layout(tui_context_t *ctx)
{
    if (!ctx || ctx->elem_count == 0)
        return;

    /*
     * The first element (index 0) is treated as the root.
     * Set its bounding box to the full terminal area.
     */
    tui_element_t *root = &ctx->elements[0];
    root->x = 0;
    root->y = 0;
    root->w = ctx->root_w;
    root->h = ctx->root_h;

    /* Pass 1 is implicit: _intrinsic_w / _intrinsic_h are called lazily
     * by the distribution code when it encounters FIT elements. */

    /* Pass 2: top-down distribution starting from root */
    _layout_subtree(ctx, 0);

    /* Damage diff: decide which elements actually changed this frame. */
    _compute_damage(ctx);
}

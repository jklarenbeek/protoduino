/**
 * @file tui.h
 * @brief Unified include header for the protoduino TUI subsystem.
 *
 * Pulls in all TUI modules – layout, renderer, and scroll buffer – and
 * defines convenience macros for the declarative TUI DSL.
 *
 * Typical usage:
 *
 *   #include "tui/tui.h"
 *
 *   static tui_render_queue_t rq;
 *   static tui_scrollbuf_t   shell_buf;
 *   static mcurses_t         scr;
 *
 *   // ... init scr, shell_buf ...
 *
 *   tui_render_init(&rq);
 *   // push commands ...
 *   tui_render_flush(&rq, &scr);
 *
 * Once tui_layout.h is available, the TUI_BEGIN / TUI_END macros enable
 * a declarative style:
 *
 *   TUI_BEGIN(ctx) {
 *       // ... element declarations ...
 *   } TUI_END(ctx);
 *
 * Copyright (c) 2026 protoduino contributors.
 */

#ifndef __TUI_H__
#define __TUI_H__

/* ---- Core TUI modules ---- */
/* tui_layout.h is not yet available; guard the include so the umbrella
 * header compiles even before the layout module is written. */
#if __has_include("tui_layout.h")
#  include "tui_layout.h"
#endif

#include "tui_render.h"
#include "tui_scroll.h"

/* =========================================================================
 * Declarative layout API
 * =========================================================================
 *
 * There is a single, canonical way to declare a frame (defined in
 * tui_layout.h):
 *
 *     tui_begin_layout(&ctx);
 *     TUI_CTX(&ctx) {
 *         TUI(TUI_ID("root"), &(tui_layout_config_t){ ... },
 *                             &(tui_style_config_t){ ... }) {
 *             TUI_TEXT("hello", MCURSES_ATTR_NORMAL);
 *             TUI_SCROLL(&sb, MCURSES_ATTR_NORMAL);
 *         }
 *     }
 *     tui_end_layout(&ctx);
 *     tui_compute_layout(&ctx);
 *     tui_render_frame(&ctx, &rq, &scr);
 *
 * (Earlier drafts also defined TUI_BEGIN/TUI_END wrappers here; they were
 * removed to avoid two competing declaration styles — use the form above,
 * which is what the examples and docs/tui.md describe.)
 */

#endif /* __TUI_H__ */

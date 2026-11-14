// file: examples/7-text-ui/22-tui-render/22-tui-render.ino

/*
 * 22-tui-render.ino  –  end-to-end smoke test for the TUI layout/render stack
 * ===========================================================================
 *
 * Exercises the full declarative pipeline that 21-process-mcurses does NOT:
 *
 *     tui_begin_layout -> TUI()/TUI_TEXT/TUI_SCROLL -> tui_end_layout
 *  -> tui_compute_layout (flexbox + damage diff)
 *  -> tui_render_frame    (FILL / BORDER / TEXT / scroll  via mcurses)
 *
 * It is written to be run head-less under protosim with deterministic UART
 * capture (no terminal needed):
 *
 *     protosim 22-tui-render.elf -m atmega328p -f 16000000 --warp \
 *         --uart0-out out.txt --exit-on-uart "<DONE>" --max-steps 600000000
 *
 * tools/test_tui.sh runs it, and once more built with -DTUI22_FULL_REDRAW,
 * which redraws everything in every frame: each screen of the first has to
 * be the screen of the second, cell for cell, in far fewer bytes.
 *
 * TX transport: instead of the interrupt-driven uart0 driver we drain the
 * mcurses TX pipe SYNCHRONOUSLY inside the pipe's wake callback (blocking
 * UART writes).  Because ipc_pipe_write() fires the wake on every
 * empty->non-empty transition, the pipe is emptied after every write and
 * can never overflow regardless of frame size.
 *
 * Markers in the stream let a test harness assert behaviour:
 *   <A>  emitted after the FIRST full render        (expect VT output before it)
 *   <B>  emitted after an IDENTICAL second render   (expect NOTHING between A and B
 *                                                     -> damage diff works)
 *   <C>  emitted after changing the status text and re-rendering
 *                                                    (expect output between B and C)
 *   <D>  one more line in the log                    (expect that one row)
 *   <E>  five more: the ring is full and wraps, every row of the log moves
 *   <F>  the log scrolled back two lines
 *   <G>  the log cleared
 *   <H>  the status text rewritten in place: same buffer, same length
 *                                                    (expect the status bar)
 *   <DONE> final sentinel
 */

#include <protoduino.h>
#include <autostart.h>
#include <sys/uart.h>

#include <lib/text/mcurses.h>
#include <lib/tui/tui_layout.h>
#include <lib/tui/tui_render.h>
#include <lib/tui/tui_scroll.h>

#include <string.h>

/* The TUI sizes follow the part: on one with 2 KB of RAM (the Uno) the
 * defaults are 12 elements, a render queue of 8 commands and 6 scroll lines
 * of 42 bytes, which this sketch fits. To change them, set them for the
 * whole build, not in the sketch: a #define here does not reach the
 * library's .c files, and both have to agree on the size of the structs.
 *
 *   arduino-cli compile --fqbn arduino:avr:uno --library . \
 *     --build-property "build.extra_flags=-DTUI_MAX_ELEMENTS=16" \
 *     examples/7-text-ui/22-tui-render
 */

#define SCR_ROWS 12
#define SCR_COLS 40

/* ---- TX/RX pipes ---- */
static uint8_t    tx_buf[128];
static ipc_pipe_t tx_pipe;
static uint8_t    rx_buf[16];
static ipc_pipe_t rx_pipe;

/* ---- mcurses + TUI instances ---- */
static mcurses_t          scr;
static tui_context_t      tui;
static tui_render_queue_t rq;
static tui_scrollbuf_t    logbuf;

/* The status line text is swapped between renders to prove damage tracking. */
static const char *status_text = " Ready";
static char        status_buf[]  = " Busy";   /* rewritten in place in frame H */

/* =========================================================================
 * Polled blocking UART0 output (9600 8N1) via the platform uart API
 * (sys/uart.h).  Polled TX keeps the captured stream exactly what mcurses
 * emitted, in order, with no ISR interleaving — and no raw registers.
 * ========================================================================= */

static void uart_init_9600(void)
{
    uart0_open(UART_BAUD_9600);
}

static void uart_putc_blocking(uint8_t b)
{
    while (!uart0_tx_is_available()) { }
    uart0_tx_write8(b);
}

/* Wake callback: synchronously drain the whole TX pipe to the UART. */
static void tx_drain_cb(void *ctx)
{
    (void)ctx;
    uint8_t b;
    while (ipc_pipe_read(&tx_pipe, &b, 1) == 1)
        uart_putc_blocking(b);
}

/* Emit a plain marker straight through the pipe (so it is captured in order). */
static void emit_marker(const char *s)
{
    ipc_pipe_write(&tx_pipe, (const uint8_t *)s, strlen(s));
}

/* =========================================================================
 * Layout declaration
 * ========================================================================= */

static void build_ui(void)
{
    tui_begin_layout(&tui);

    TUI_CTX(&tui) {
        TUI(TUI_ID("root"),
            &(tui_layout_config_t){ .width     = TUI_SIZING_GROW(),
                                    .height    = TUI_SIZING_GROW(),
                                    .direction = TUI_DIR_TOP_TO_BOTTOM },
            NULL)
        {
            /* Header: fixed 1-row reverse bar */
            TUI(TUI_ID("hdr"),
                &(tui_layout_config_t){ .width  = TUI_SIZING_GROW(),
                                        .height = TUI_SIZING_FIXED(1) },
                &(tui_style_config_t){ .attr = MCURSES_ATTR_REVERSE })
            {
                TUI_TEXT(" Protoduino TUI", MCURSES_ATTR_REVERSE);
            }

            /* Body: grows, single border, one line of text + a scroll log */
            TUI(TUI_ID("body"),
                &(tui_layout_config_t){ .width     = TUI_SIZING_GROW(),
                                        .height    = TUI_SIZING_GROW(),
                                        .direction = TUI_DIR_TOP_TO_BOTTOM,
                                        .padding   = 0 },
                &(tui_style_config_t){ .attr   = MCURSES_ATTR_NORMAL,
                                       .border = TUI_BORDER_SINGLE })
            {
                TUI_TEXT("Hello, layout!", MCURSES_ATTR_NORMAL);
                TUI_SCROLL(&logbuf, MCURSES_ATTR_NORMAL);
            }

            /* Footer/status: fixed 1-row reverse bar (text swaps between frames) */
            TUI(TUI_ID("status"),
                &(tui_layout_config_t){ .width  = TUI_SIZING_GROW(),
                                        .height = TUI_SIZING_FIXED(1) },
                &(tui_style_config_t){ .attr = MCURSES_ATTR_REVERSE })
            {
                TUI_TEXT(status_text, MCURSES_ATTR_REVERSE);
            }
        }
    }

    tui_end_layout(&tui);
    tui_compute_layout(&tui);
}

static void render(void)
{
#ifdef TUI22_FULL_REDRAW
    tui_mark_all_dirty(&tui);  /* the reference: everything, every frame */
#endif
    tui_render_frame(&tui, &rq, &scr);
}

/* =========================================================================
 * setup / loop
 * ========================================================================= */

void setup(void)
{
    protoduino_start();

    uart_init_9600();

    ipc_pipe_init(&tx_pipe, tx_buf, sizeof(tx_buf), tx_drain_cb, NULL);
    ipc_pipe_init(&rx_pipe, rx_buf, sizeof(rx_buf), NULL, NULL);

    mcurses_init(&scr, &tx_pipe, &rx_pipe, SCR_ROWS, SCR_COLS);
    /* Back-pressure: if a write ever fills the 128-byte TX pipe, mcurses
     * calls this hook to drain it synchronously, so no output is lost even
     * for a full-screen redraw larger than the pipe. */
    mcurses_set_drain(&scr, tx_drain_cb, NULL);
    initscr_ex(&scr);

    tui_init(&tui, SCR_ROWS, SCR_COLS);
    tui_render_init(&rq);
    tui_scroll_init(&logbuf);

    tui_scroll_add_line(&logbuf, "boot ok",   MCURSES_ATTR_NORMAL);
    tui_scroll_add_line(&logbuf, "tui ready",  MCURSES_ATTR_NORMAL);

    /* Frame 1: first full render. */
    build_ui();
    render();
    emit_marker("<A>");

    /* Frame 2: identical tree, no changes -> damage diff should emit nothing. */
    build_ui();
    render();
    emit_marker("<B>");

    /* Frame 3: change the status text -> expect localized output. */
    status_text = status_buf;
    build_ui();
    render();
    emit_marker("<C>");

    /* Frame 4: one more line in the log -> one row. */
    tui_scroll_add_line(&logbuf, "sensor 42", MCURSES_ATTR_NORMAL);
    build_ui(); render(); emit_marker("<D>");

    /* Frame 5: past the capacity of the ring: every row of the log moves. */
    tui_scroll_add_line(&logbuf, "a much longer line than the others", MCURSES_ATTR_BOLD);
    tui_scroll_add_line(&logbuf, "short", MCURSES_ATTR_NORMAL);
    tui_scroll_add_line(&logbuf, "x", MCURSES_ATTR_NORMAL);
    tui_scroll_add_line(&logbuf, "the ring wraps here", MCURSES_ATTR_NORMAL);
    tui_scroll_add_line(&logbuf, "and here", MCURSES_ATTR_NORMAL);
    build_ui(); render(); emit_marker("<E>");

    /* Frame 6: back into the history. */
    tui_scroll_up(&logbuf);
    tui_scroll_up(&logbuf);
    build_ui(); render(); emit_marker("<F>");

    /* Frame 7: the log emptied. */
    tui_scroll_clear(&logbuf);
    build_ui(); render(); emit_marker("<G>");

    /* Frame 8: the status rewritten in place: the same pointer and length. */
    memcpy(status_buf, " Done", 5);
    build_ui(); render(); emit_marker("<H>");

    emit_marker("<DONE>");
}

void loop(void)
{
    /* All work happened in setup(); nothing to do. */
}

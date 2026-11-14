// file: examples/7-text-ui/25-tui-robust/25-tui-robust.ino

/*
 * 25-tui-robust.ino  –  robustness tests for the TUI/mcurses fixes
 * ================================================================
 *
 * Exercises the failure-handling that earlier code lacked, so the fixes are
 * verified rather than latent:
 *
 *   TEST A  TX back-pressure (mcurses)
 *           A write larger than the TX pipe must NOT silently lose bytes.
 *           With a drain hook installed -> tx_dropped == 0 and every byte
 *           reaches the sink.  With no hook -> the shortfall is COUNTED in
 *           tx_dropped (visible loss, not silent corruption).
 *
 *   TEST B  Element / depth overflow balance (tui_layout)
 *           Declaring a subtree that overflows the arena/depth must keep the
 *           open/close build stack balanced, so a sibling declared AFTER the
 *           overflow still attaches to the correct parent (no tree corruption).
 *
 *   TEST C  Border glyphs go through mcurses (tui_render)
 *           Every byte of a bordered frame either reaches the sink or is
 *           counted in tx_dropped, glyphs included; and a row of border costs
 *           one cursor move, not one per glyph.
 *
 *   TEST D  Text in flash (TUI_TEXT_F / TUI_TEXT_P)
 *           Measured and drawn from PROGMEM like text in RAM; an unchanged
 *           frame draws nothing.
 *
 *   TEST E  Queries and drawing one element
 *           tui_find / tui_get_rect / tui_element_count, and
 *           tui_render_element() draws inside its element only.
 *
 * Run head-less under protosim:
 *   protosim 25-tui-robust.elf -m atmega328p -f 16000000 \
 *       --uart0-out out.txt --exit-on-uart "<DONE>" --max-steps 20000000
 *
 * The defaults are enough: TEST B uses ten elements, and on a part with
 * 2 KB of RAM the default arena holds twelve.
 */

#include <protoduino.h>
#include <sys/uart.h>

#include <lib/text/mcurses.h>
#include <lib/tui/tui_layout.h>
#include <lib/tui/tui_render.h>

#include <string.h>

/* ---- polled blocking UART0 report output (platform uart API) ---- */
static void uart_init_9600(void)
{
    uart0_open(UART_BAUD_9600);
}
static void up(uint8_t b) { while (!uart0_tx_is_available()) { } uart0_tx_write8(b); }
static void us(const char *s) { while (*s) up((uint8_t)*s++); }
static void udec(uint32_t v)
{
    char buf[11]; uint8_t i = sizeof(buf); buf[--i] = '\0';
    if (!v) { up('0'); return; }
    while (v && i) { buf[--i] = (char)('0' + v % 10); v /= 10; }
    us(&buf[i]);
}
static void report(const char *label, uint8_t pass)
{
    us(pass ? "PASS " : "FAIL "); us(label); up('\n');
}

/* ========================================================================
 * TEST A — TX back-pressure
 * ======================================================================== */

static ipc_pipe_t test_pipe;
static uint8_t    test_buf[8];        /* deliberately tiny */
static mcurses_t  tscr;
static uint16_t   sink_count;

/* Drain hook: pull everything out of the pipe (to a counter), making room. */
static void sink_drain(void *ctx)
{
    (void)ctx;
    uint8_t b;
    while (ipc_pipe_read(&test_pipe, &b, 1) == 1)
        ++sink_count;
}

static void test_backpressure(void)
{
    /* A line much larger than the 8-byte pipe. */
    static const char LINE[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";   /* 46 bytes */
    uint8_t len = (uint8_t)strlen(LINE);

    ipc_pipe_init(&test_pipe, test_buf, sizeof(test_buf), NULL, NULL);
    mcurses_init(&tscr, &test_pipe, NULL, 24, 80);

    /* --- with drain hook: no loss --- */
    mcurses_set_drain(&tscr, sink_drain, NULL);
    sink_count = 0;
    tscr.tx_dropped = 0;
    addnstr_ex(&tscr, LINE, len);     /* writes 46 bytes through an 8-byte pipe */
    sink_drain(NULL);                 /* flush any tail left in the pipe */

    us("A1 with-drain: tx_dropped="); udec(tscr.tx_dropped);
    us(" sink="); udec(sink_count); up('\n');
    report("A1 back-pressure loses nothing",
           tscr.tx_dropped == 0 && sink_count >= len);

    /* --- without drain hook: loss is counted, not silent --- */
    mcurses_set_drain(&tscr, NULL, NULL);
    tscr.tx_dropped = 0;
    /* same attributes -> no SGR re-emitted; exactly `len` text bytes attempted
     * into an 8-byte pipe that already holds the previous tail. */
    addnstr_ex(&tscr, LINE, len);
    us("A2 no-drain:   tx_dropped="); udec(tscr.tx_dropped); up('\n');
    report("A2 overflow is counted (not silent)", tscr.tx_dropped > 0);
}

/* ========================================================================
 * TEST B — element / depth overflow balance
 * ======================================================================== */

static tui_context_t tctx;

/* Find an element by id hash; return index or 0xFF. */
static uint8_t find_by_id(uint8_t id)
{
    for (uint8_t i = 0; i < tctx.elem_count; ++i)
        if (tctx.elements[i].id == id)
            return i;
    return 0xFFu;
}

/* Is `child` reachable as a direct child of `parent`? */
static uint8_t is_child_of(uint8_t parent, uint8_t child)
{
    if (parent == 0xFFu || child == 0xFFu)
        return 0;
    uint8_t c = tctx.elements[parent].first_child;
    while (c != TUI_INDEX_NONE) {
        if (c == child) return 1;
        c = tctx.elements[c].next_sibling;
    }
    return 0;
}

static void test_overflow(void)
{
    tui_init(&tctx, 24, 80);
    tui_begin_layout(&tctx);

    TUI_CTX(&tctx) {
        TUI(TUI_ID("root"),
            &(tui_layout_config_t){ .width = TUI_SIZING_GROW(),
                                    .height = TUI_SIZING_GROW(),
                                    .direction = TUI_DIR_TOP_TO_BOTTOM }, NULL)
        {
            /* Nest WAY past TUI_MAX_DEPTH (8): the deep levels are dropped,
             * but the build stack must stay balanced. */
            TUI(TUI_ID("d1"), NULL, NULL) {
             TUI(TUI_ID("d2"), NULL, NULL) {
              TUI(TUI_ID("d3"), NULL, NULL) {
               TUI(TUI_ID("d4"), NULL, NULL) {
                TUI(TUI_ID("d5"), NULL, NULL) {
                 TUI(TUI_ID("d6"), NULL, NULL) {
                  TUI(TUI_ID("d7"), NULL, NULL) {
                   TUI(TUI_ID("d8"), NULL, NULL) {
                    TUI(TUI_ID("d9"), NULL, NULL) {
                     TUI(TUI_ID("d10"), NULL, NULL) {
                       TUI_TEXT("deep", MCURSES_ATTR_NORMAL);
                     }}}}}}}}}}

            /* Sibling of d1, declared AFTER the overflow subtree closes.
             * With a balanced stack it must attach to root. */
            TUI(TUI_ID("tail"),
                &(tui_layout_config_t){ .width = TUI_SIZING_GROW(),
                                        .height = TUI_SIZING_FIXED(1) }, NULL)
            {
                TUI_TEXT("tail", MCURSES_ATTR_NORMAL);
            }
        }
    }

    tui_end_layout(&tctx);
    tui_compute_layout(&tctx);

    uint8_t root = find_by_id(TUI_ID("root"));
    uint8_t d1   = find_by_id(TUI_ID("d1"));
    uint8_t tail = find_by_id(TUI_ID("tail"));

    us("B elem_count="); udec(tctx.elem_count);
    us(" stack_depth="); udec(tctx.stack_depth); up('\n');

    /* Balanced stack returns to 0; arena never exceeded; the post-overflow
     * sibling 'tail' is correctly a child of root (not corrupted/lost). */
    report("B stack balanced (depth==0)", tctx.stack_depth == 0);
    report("B arena capped",            tctx.elem_count <= TUI_MAX_ELEMENTS);
    report("B root has d1 child",       is_child_of(root, d1));
    report("B tail attached to root after overflow", is_child_of(root, tail));
}

/* ========================================================================
 * A sink that looks at what a frame sends: bytes, cursor moves and the rows
 * they go to, box-drawing glyphs, and whether a given text went by.
 * ======================================================================== */

static struct {
    uint16_t bytes, cups, glyphs;
    uint8_t  state, row, num, row_lo, row_hi;
    const char *needle;   /* text to look for */
    uint8_t  matched, found;
} look;

static void look_reset(const char *needle)
{
    memset(&look, 0, sizeof(look));
    look.row_lo = 0xFF;
    look.needle = needle;
}

static void look_byte(uint8_t b)
{
    ++look.bytes;
    if (b == 0xE2u)
        ++look.glyphs;   /* box drawing: E2 94 xx / E2 95 xx */

    /* ESC [ row ; col H */
    if (look.state == 0 && b == 0x1B) look.state = 1;
    else if (look.state == 1) { look.state = (b == '[') ? 2 : 0; look.num = 0; }
    else if (look.state == 2 || look.state == 3) {
        if (b >= '0' && b <= '9') look.num = (uint8_t)(look.num * 10 + (b - '0'));
        else if (b == ';' && look.state == 2) { look.row = look.num; look.num = 0; look.state = 3; }
        else {
            if (b == 'H' && look.state == 3) {
                ++look.cups;
                if (look.row - 1 < look.row_lo) look.row_lo = (uint8_t)(look.row - 1);
                if (look.row - 1 > look.row_hi) look.row_hi = (uint8_t)(look.row - 1);
            }
            look.state = 0;
        }
    }

    if (look.needle && !look.found) {
        if ((char)b == look.needle[look.matched]) {
            if (look.needle[++look.matched] == '\0') look.found = 1;
        } else {
            look.matched = ((char)b == look.needle[0]) ? 1 : 0;
        }
    }
}

static void look_drain(void *ctx)
{
    (void)ctx;
    uint8_t b;
    while (ipc_pipe_read(&test_pipe, &b, 1) == 1)
        look_byte(b);
}

static tui_render_queue_t trq;

/* a 6 x 20 screen: a bordered panel, a line of text in flash below it */
static void build_frame(void)
{
    tui_begin_layout(&tctx);
    TUI_CTX(&tctx) {
        TUI(TUI_ID("root"),
            &(tui_layout_config_t){ .width = TUI_SIZING_GROW(),
                                    .height = TUI_SIZING_GROW(),
                                    .direction = TUI_DIR_TOP_TO_BOTTOM }, NULL)
        {
            TUI(TUI_ID("panel"),
                &(tui_layout_config_t){ .width = TUI_SIZING_GROW(),
                                        .height = TUI_SIZING_FIXED(4) },
                &(tui_style_config_t){ .attr = MCURSES_ATTR_NORMAL, .border = TUI_BORDER_SINGLE })
            {
                TUI_TEXT("ram text", MCURSES_ATTR_NORMAL);
            }
            TUI(TUI_ID("label"),
                &(tui_layout_config_t){ .width = TUI_SIZING_FIT(),
                                        .height = TUI_SIZING_FIXED(1) }, NULL)
            {
                TUI_TEXT_F("Flash text", MCURSES_ATTR_BOLD);
            }
        }
    }
    tui_end_layout(&tctx);
    tui_compute_layout(&tctx);
}

/* one frame into the 8-byte pipe; with or without the drain hook */
static void frame_through(uint8_t with_drain, const char *needle)
{
    ipc_pipe_init(&test_pipe, test_buf, sizeof(test_buf), NULL, NULL);
    mcurses_init(&tscr, &test_pipe, NULL, 6, 20);
    mcurses_set_drain(&tscr, with_drain ? look_drain : NULL, NULL);
    look_reset(needle);
    tscr.tx_dropped = 0;
    tui_init(&tctx, 6, 20);
    tui_render_init(&trq);
    build_frame();
    tui_render_frame(&tctx, &trq, &tscr);
    look_drain(NULL);
}

static void test_borders(void)
{
    uint16_t all, cups;

    frame_through(1, NULL);
    all = look.bytes;
    cups = look.cups;
    us("C1 with-drain: bytes="); udec(all); us(" glyphs="); udec(look.glyphs);
    us(" cursor moves="); udec(cups); us(" tx_dropped="); udec(tscr.tx_dropped); up('\n');
    /* a 6 x 20 frame, a panel of 4 rows x 20 with a border: 2*20 + 2*2 glyphs */
    report("C1 every border glyph arrives", look.glyphs == 44 && tscr.tx_dropped == 0);
    /* the fills: one move per row (root 6, panel 4, label 1); the border:
     * one for the top row, one per side of the two middle rows, one for the
     * bottom row; the two texts one each: 19. A move per glyph would be 44
     * for the border alone. */
    report("C2 a row of border is one cursor move", cups <= 19);

    frame_through(0, NULL);
    us("C3 no-drain: arrived="); udec(look.bytes); us(" tx_dropped="); udec(tscr.tx_dropped); up('\n');
    report("C3 what did not arrive is counted, glyphs too", look.bytes + tscr.tx_dropped == all);
}

static void test_flash_text(void)
{
    tui_rect_t r;

    frame_through(1, "Flash text");
    report("D1 flash text is drawn", look.found);
    report("D2 flash text is measured: FIT gives 10 columns",
           tui_get_rect(&tctx, TUI_ID("label"), &r) && r.w == 10);

    /* the same frame again: nothing changed, nothing sent */
    look_reset(NULL);
    build_frame();
    tui_render_frame(&tctx, &trq, &tscr);
    look_drain(NULL);
    us("D3 unchanged frame: bytes="); udec(look.bytes); up('\n');
    report("D3 an unchanged frame with flash text sends nothing", look.bytes == 0);
}

static void test_queries(void)
{
    tui_rect_t r;
    uint8_t panel;

    frame_through(1, NULL);
    panel = tui_find(&tctx, TUI_ID("panel"));
    /* root, panel, its text, label, its text */
    report("E1 tui_element_count", tui_element_count(&tctx) == 5);
    report("E2 tui_find", panel == 1 && tui_find(&tctx, TUI_ID("nobody")) == TUI_INDEX_NONE);
    report("E3 tui_get_rect", tui_get_rect(&tctx, TUI_ID("panel"), &r)
                              && r.x == 0 && r.y == 0 && r.w == 20 && r.h == 4
                              && !tui_get_rect(&tctx, TUI_ID("nobody"), &r));
    report("E4 tui_is_inside", tui_is_inside(&tctx, 2, panel) && !tui_is_inside(&tctx, 3, panel));

    /* draw the panel alone: its border and its text, and nothing below it */
    look_reset("ram text");
    report("E5 tui_render_element finds the panel", tui_render_element(&tctx, &trq, &tscr, TUI_ID("panel")));
    look_drain(NULL);
    us("E6 panel alone: bytes="); udec(look.bytes); us(" rows "); udec(look.row_lo); up('-'); udec(look.row_hi); up('\n');
    report("E6 it draws the panel and its text, inside rows 0-3",
           look.glyphs == 44 && look.found && look.row_hi <= 3);
    report("E7 an unknown element is not drawn", !tui_render_element(&tctx, &trq, &tscr, TUI_ID("nobody")));
}

/* ========================================================================
 * setup / loop
 * ======================================================================== */

void setup(void)
{
    uart_init_9600();
    us("== TUI robustness tests ==\n");
    test_backpressure();
    test_overflow();
    test_borders();
    test_flash_text();
    test_queries();
    us("<DONE>\n");
}

void loop(void) { }

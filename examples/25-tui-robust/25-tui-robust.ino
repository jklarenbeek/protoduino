// file: examples/25-tui-robust/25-tui-robust.ino

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
 * Run head-less under protosim:
 *   protosim 25-tui-robust.elf -m atmega328p -f 16000000 \
 *       --uart0-out out.txt --exit-on-uart "<DONE>" --max-steps 20000000
 *
 * Build with a roomy element arena so TEST B hits DEPTH overflow before arena
 * exhaustion:  -DTUI_MAX_ELEMENTS=30
 */

#include <protoduino.h>
#include <avr/io.h>

#include <lib/text/mcurses.h>
#include <lib/tui/tui_layout.h>
#include <lib/tui/tui_render.h>

#include <string.h>

/* ---- raw blocking UART0 report output ---- */
static void uart_init_9600(void)
{
    UBRR0H = 0; UBRR0L = 103;
    UCSR0B = (uint8_t)(1u << TXEN0);
    UCSR0C = (uint8_t)((1u << UCSZ01) | (1u << UCSZ00));
}
static void up(uint8_t b) { while (!(UCSR0A & (uint8_t)(1u << UDRE0))) { } UDR0 = b; }
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
 * setup / loop
 * ======================================================================== */

void setup(void)
{
    uart_init_9600();
    us("== TUI robustness tests ==\n");
    test_backpressure();
    test_overflow();
    us("<DONE>\n");
}

void loop(void) { }

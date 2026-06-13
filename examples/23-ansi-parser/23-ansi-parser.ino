// file: examples/23-ansi-parser/23-ansi-parser.ino

/*
 * 23-ansi-parser.ino  –  ANSI / VT input parser conformance trace
 * ===============================================================
 *
 * Feeds a crafted byte stream through the ansi_parser_t state machine and
 * prints a human-readable decode of every action it recognises.  This is
 * the INPUT-side counterpart to the TUI/mcurses OUTPUT examples.
 *
 * Run head-less under protosim (no terminal needed):
 *
 *   protosim 23-ansi-parser.elf -m atmega328p -f 16000000 \
 *       --uart0-out out.txt --exit-on-uart "<DONE>" --max-steps 20000000
 *
 * Output transport is raw blocking UART0 TX (captured by --uart0-out), so the
 * decode trace appears in order with no pipe/ISR plumbing.
 */

#include <protoduino.h>
#include <avr/io.h>

#include <lib/text/ansi.h>

#include <string.h>

/* =========================================================================
 * Raw blocking UART0 (9600 8N1)
 * ========================================================================= */

static void uart_init_9600(void)
{
    UBRR0H = 0;
    UBRR0L = 103;                                     /* 16MHz/(16*9600)-1 */
    UCSR0B = (uint8_t)(1u << TXEN0);
    UCSR0C = (uint8_t)((1u << UCSZ01) | (1u << UCSZ00));
}

static void up(uint8_t b)
{
    while (!(UCSR0A & (uint8_t)(1u << UDRE0))) { }
    UDR0 = b;
}

static void us(const char *s)        { while (*s) up((uint8_t)*s++); }

static void uhex(uint32_t v, uint8_t digits)
{
    static const char H[] = "0123456789ABCDEF";
    for (int8_t i = (int8_t)((digits - 1) * 4); i >= 0; i -= 4)
        up((uint8_t)H[(v >> i) & 0xF]);
}

static void udec(uint32_t v)
{
    char buf[11];
    uint8_t i = sizeof(buf);
    buf[--i] = '\0';
    if (v == 0) { up('0'); return; }
    while (v && i) { buf[--i] = (char)('0' + (v % 10)); v /= 10; }
    us(&buf[i]);
}

/* =========================================================================
 * KEY_* -> short name (for the decode trace)
 * ========================================================================= */

static void emit_key_name(uint16_t key)
{
    switch (key) {
    case KEY_UP:    us("UP");    return;
    case KEY_DOWN:  us("DOWN");  return;
    case KEY_LEFT:  us("LEFT");  return;
    case KEY_RIGHT: us("RIGHT"); return;
    case KEY_HOME:  us("HOME");  return;
    case KEY_END:   us("END");   return;
    case KEY_IC:    us("INS");   return;
    case KEY_DC:    us("DEL");   return;
    case KEY_PPAGE: us("PGUP");  return;
    case KEY_NPAGE: us("PGDN");  return;
    case KEY_BTAB:  us("BTAB");  return;
    default:
        if (key >= KEY_F(1) && key <= KEY_F(12)) {
            us("F"); udec((uint32_t)(key - KEY_F(1) + 1));
            return;
        }
        us("KEY:0x"); uhex(key, 2);
    }
}

static void emit_mods(uint8_t mods)
{
    if (!mods) return;
    us(" [");
    if (mods & ANSI_MOD_CTRL)  us("Ctrl ");
    if (mods & ANSI_MOD_ALT)   us("Alt ");
    if (mods & ANSI_MOD_SHIFT) us("Shift ");
    if (mods & ANSI_MOD_META)  us("Meta ");
    us("]");
}

/* =========================================================================
 * Parser handlers — print one decode line per action
 * ========================================================================= */

static void on_print(void *ctx, uint32_t cp)
{
    (void)ctx;
    us("PRINT  U+"); uhex(cp, 4);
    if (cp >= 0x20 && cp < 0x7F) { us(" '"); up((uint8_t)cp); us("'"); }
    up('\n');
}

static void on_execute(void *ctx, uint8_t ctrl)
{
    (void)ctx;
    us("CTRL   0x"); uhex(ctrl, 2);
    if (ctrl == '\r') us(" CR");
    else if (ctrl == '\n') us(" LF");
    else if (ctrl == '\t') us(" TAB");
    else if (ctrl == KEY_ESCAPE) us(" ESC");
    up('\n');
}

static void on_csi(void *ctx, const ansi_csi_t *csi)
{
    (void)ctx;
    us("CSI    ");
    if (csi->private_marker) { up('('); up(csi->private_marker); us(") "); }
    us("params=");
    if (csi->nparams == 0) {
        us("-");
    } else {
        for (uint8_t i = 0; i < csi->nparams; ++i) {
            if (i) up(';');
            if (csi->params[i] < 0) up('?'); else udec((uint32_t)csi->params[i]);
        }
    }
    us(" final='"); up(csi->final); us("'");

    uint8_t mods = 0;
    uint16_t key = ansi_key_from_csi(csi, &mods);
    if (key) { us("  -> KEY "); emit_key_name(key); emit_mods(mods); }
    else if (csi->final == 'R' && csi->nparams == 2) {
        us("  -> CPR row="); udec((uint32_t)ansi_csi_param(csi, 0, 1));
        us(" col=");        udec((uint32_t)ansi_csi_param(csi, 1, 1));
    }
    up('\n');
}

static void on_esc(void *ctx, uint8_t inter, uint8_t final)
{
    (void)ctx;
    us("ESC    ");
    if (inter) { us("inter='"); up(inter); us("' "); }
    us("final='"); up(final); us("'");
    if (inter == 'O') {
        uint8_t mods = 0;
        uint16_t key = ansi_key_from_ss3(final, &mods);
        if (key) { us("  -> SS3 KEY "); emit_key_name(key); }
    }
    up('\n');
}

static void on_osc(void *ctx, const char *data, uint16_t len)
{
    (void)ctx;
    us("OSC    len="); udec(len); us(" \""); us(data); us("\"\n");
}

static const ansi_handlers_t HANDLERS = {
    on_print, on_execute, on_csi, on_esc, on_osc,
    /* dcs_hook */ 0, /* dcs_put */ 0, /* dcs_unhook */ 0,
};

/* =========================================================================
 * Crafted input stream covering the major ANSI categories
 * ========================================================================= */

static const uint8_t STREAM[] = {
    'H','i',                                  /* printable ASCII            */
    0xC3, 0xA9,                               /* UTF-8 'é' (U+00E9)         */
    0xE2, 0x94, 0x8C,                         /* UTF-8 '┌' (U+250C)         */
    '\r',                                     /* C0 control (CR)            */
    0x1B,'[','1','0',';','2','0','H',         /* CUP to (10,20)             */
    0x1B,'[','1',';','3','1','m',             /* SGR bold red               */
    0x1B,'[','?','2','5','h',                 /* DECSET show cursor         */
    0x1B,'[','A',                             /* arrow Up                   */
    0x1B,'[','1',';','5','C',                 /* Ctrl + arrow Right         */
    0x1B,'[','1','5','~',                     /* F5                         */
    0x1B,'O','P',                             /* SS3 F1                     */
    0x1B,']','0',';','t','i','t','l','e',0x07,/* OSC set title (BEL term)   */
    0x1B,']','2',';','S','T',0x1B,'\\',       /* OSC set title (ST term)    */
    0x1B,'[','6',';','7','R',                 /* cursor position report     */
};

/* =========================================================================
 * setup / loop
 * ========================================================================= */

void setup(void)
{
    uart_init_9600();

    static ansi_parser_t parser;
    static char          oscbuf[32];
    ansi_parser_init(&parser, &HANDLERS, NULL, oscbuf, sizeof(oscbuf));

    us("== ANSI parser trace ==\n");
    ansi_parse_buf(&parser, STREAM, (uint16_t)sizeof(STREAM));
    ansi_parser_flush(&parser);   /* deliver a trailing lone ESC, if any */

    us("<DONE>\n");
}

void loop(void)
{
    /* everything happened in setup() */
}

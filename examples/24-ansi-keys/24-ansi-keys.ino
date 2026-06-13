// file: examples/24-ansi-keys/24-ansi-keys.ino

/*
 * 24-ansi-keys.ino  –  shell-style keyboard input via the ANSI parser
 * ====================================================================
 *
 * Demonstrates the INPUT path a shell needs: raw terminal bytes are fed to
 * ansi_parser_t, which classifies them into printable characters, control
 * keys, and decoded special keys (arrows, F-keys, navigation, modifiers).
 * A tiny line editor accumulates printable input and "submits" the line on
 * Enter — exactly the loop a command shell runs.
 *
 * To stay fully deterministic, the keystrokes are supplied from a hard-coded
 * buffer via get_key_byte() (which models draining an RX pipe).  In a real
 * build you would instead feed bytes arriving from the UART RX ISR / a
 * protoduino ipc_pipe_t, then call ansi_parser_flush() when the pipe drains
 * (so a lone ESC keypress is delivered).  See the comment in loop().
 *
 * Run head-less under protosim:
 *
 *   protosim 24-ansi-keys.elf -m atmega328p -f 16000000 \
 *       --uart0-out out.txt --exit-on-uart "<DONE>" --max-steps 20000000
 */

#include <protoduino.h>
#include <avr/io.h>

#include <lib/text/ansi.h>

#include <string.h>

/* =========================================================================
 * Raw blocking UART0 (9600 8N1) output
 * ========================================================================= */

static void uart_init_9600(void)
{
    UBRR0H = 0;
    UBRR0L = 103;
    UCSR0B = (uint8_t)(1u << TXEN0);
    UCSR0C = (uint8_t)((1u << UCSZ01) | (1u << UCSZ00));
}

static void up(uint8_t b) { while (!(UCSR0A & (uint8_t)(1u << UDRE0))) { } UDR0 = b; }
static void us(const char *s) { while (*s) up((uint8_t)*s++); }

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
            us("F");
            uint8_t n = (uint8_t)(key - KEY_F(1) + 1);
            if (n >= 10) up((uint8_t)('0' + n / 10));
            up((uint8_t)('0' + n % 10));
            return;
        }
        us("?");
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
 * Tiny line editor (the "shell" state the parser drives)
 * ========================================================================= */

typedef struct {
    char    line[40];
    uint8_t len;
} editor_t;

static void on_print(void *ctx, uint32_t cp)
{
    editor_t *ed = (editor_t *)ctx;
    /* Append printable ASCII to the line buffer (UTF-8 > 0x7F omitted here
     * for brevity; a real editor would store the bytes). */
    if (cp >= 0x20 && cp < 0x7F && ed->len < sizeof(ed->line) - 1) {
        ed->line[ed->len++] = (char)cp;
        us("CHAR '"); up((uint8_t)cp); us("'\n");
    }
}

static void on_execute(void *ctx, uint8_t ctrl)
{
    editor_t *ed = (editor_t *)ctx;
    switch (ctrl) {
    case '\r':                            /* Enter / KEY_ENTER == '\r' */
    case '\n':
        ed->line[ed->len] = '\0';
        us("SUBMIT \""); us(ed->line); us("\"\n");
        ed->len = 0;
        break;
    case '\b':
    case KEY_DELETE:                      /* Backspace key (0x7F) erases */
        if (ed->len > 0) ed->len--;
        us("BACKSPACE\n");
        break;
    case '\t':
        us("TAB\n");
        break;
    case KEY_ESCAPE:
        us("ESCAPE\n");
        break;
    default:
        us("CTRL\n");
        break;
    }
}

static void on_csi(void *ctx, const ansi_csi_t *csi)
{
    (void)ctx;
    uint8_t mods = 0;
    uint16_t key = ansi_key_from_csi(csi, &mods);
    if (key) { us("KEY "); emit_key_name(key); emit_mods(mods); up('\n'); }
    /* Non-key CSI (reports, mode changes) would be handled here in a shell. */
}

static void on_esc(void *ctx, uint8_t inter, uint8_t final)
{
    (void)ctx;
    if (inter == 'O') {
        uint8_t mods = 0;
        uint16_t key = ansi_key_from_ss3(final, &mods);
        if (key) { us("KEY "); emit_key_name(key); us(" (SS3)\n"); }
    }
}

static const ansi_handlers_t HANDLERS = {
    on_print, on_execute, on_csi, on_esc,
    /* osc */ 0, /* dcs_hook */ 0, /* dcs_put */ 0, /* dcs_unhook */ 0,
};

/* =========================================================================
 * Simulated keyboard input (models draining an RX pipe)
 * =========================================================================
 * Types "echo hi", backspaces the 'i', types "ello" -> "echo hello", fires a
 * few special keys, presses Enter, then a lone ESC.
 */
static const uint8_t KEYS[] = {
    'e','c','h','o',' ','h','i',
    0x7F,                                 /* Backspace -> "echo h"          */
    'e','l','l','o',                      /* -> "echo hello"                */
    0x1B,'[','D',                         /* Left arrow                     */
    0x1B,'[','1',';','5','D',             /* Ctrl+Left                      */
    0x1B,'O','Q',                         /* SS3 F2                         */
    0x1B,'[','3','~',                     /* Delete key (KEY_DC)            */
    '\r',                                 /* Enter -> submit                */
    0x1B,                                 /* lone ESC (delivered on flush)  */
};

static uint8_t key_idx = 0;

static int16_t get_key_byte(void)
{
    if (key_idx < sizeof(KEYS))
        return KEYS[key_idx++];
    return -1;   /* input exhausted (like an empty RX pipe) */
}

/* =========================================================================
 * setup / loop
 * ========================================================================= */

void setup(void)
{
    uart_init_9600();

    static ansi_parser_t parser;
    static editor_t      editor;
    editor.len = 0;
    ansi_parser_init(&parser, &HANDLERS, &editor, NULL, 0); /* no OSC buffer */

    us("== ANSI key reader ==\n");

    /* The shell input loop: drain available bytes into the parser, then flush
     * so a pending lone ESC is delivered.  In a real build, replace
     * get_key_byte() with PROCESS_PIPE_READ()/getch from the UART RX pipe and
     * call ansi_parser_flush() whenever the pipe is empty. */
    int16_t b;
    while ((b = get_key_byte()) >= 0)
        ansi_parse(&parser, (uint8_t)b);
    ansi_parser_flush(&parser);

    us("<DONE>\n");
}

void loop(void)
{
    /* everything happened in setup() */
}

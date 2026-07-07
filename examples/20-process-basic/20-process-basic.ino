// file: examples/20-process-basic/20-process-basic.ino

/*
 * 20-process-basic.ino  –  basic process demo for protoduino
 * ================================================================
 */

#include <protoduino.h>
#include <autostart.h>
#include <sys/process.h>
#include <sys/uart.h>
#include <lib/text/ansi.h>

static uint8_t   rx_buf[64];
static ipc_pipe_t rx_pipe;

static uint8_t   tx_buf[64];
static ipc_pipe_t tx_pipe;

static void on_rx_complete(uint_fast8_t ch)
{
  uint8_t b = (uint8_t)ch;
  ipc_pipe_write(&rx_pipe, &b, 1);
}

static uint32_t rx_error_count = 0;
static void on_rx_error(uint_fast8_t err)
{
  (void)err;
  rx_error_count++;
  uart0_rx_clear_errors();
}

static int_fast16_t on_tx_complete(void)
{
  uint8_t data;
  return (ipc_pipe_read(&tx_pipe, &data, 1) > 0) ? (int_fast16_t)data : -1;
}

static void tx_wake_cb(void *ctx)
{
  (void)ctx;
  uart0_tx_enable_int();
}

PROCESS_DEFINE(shell, "shell", 2,
  uint8_t  input_buf[32];
  uint8_t  input_idx;
  ansi_parser_t parser;   /* byte-fed VT500 escape decoder (ansi.h) */
);

PROCESS_INSTANCE(shell, shell_proc);

static const uint8_t prompt[] CC_PROGMEM = "shell> ";

/* =========================================================================
 * ANSI parser handlers – the parser classifies each RX byte and calls one
 * of these; escape sequences (arrow keys, F-keys, …) arrive fully decoded
 * instead of being collected in a fixed escape buffer.
 * ========================================================================= */

static void shell_on_print(void *ctx, uint32_t cp)
{
  process_shell_t *self = (process_shell_t *)ctx;

  if (cp >= 0x20u && cp <= 0x7Eu
      && self->input_idx < sizeof(self->input_buf) - 1) {
    uint8_t ch = (uint8_t)cp;
    self->input_buf[self->input_idx++] = ch;
    ipc_pipe_write(&tx_pipe, &ch, 1);   /* echo */
  }
}

static void shell_on_execute(void *ctx, uint8_t ctrl)
{
  process_shell_t *self = (process_shell_t *)ctx;
  static const uint8_t crlf[] = "\r\n";

  if (ctrl == '\r' || ctrl == '\n') {
    ipc_pipe_write(&tx_pipe, crlf, 2);

    if (self->input_idx > 0) {
      static const uint8_t prefix[] = "Command: ";
      ipc_pipe_write(&tx_pipe, prefix, sizeof(prefix) - 1);
      ipc_pipe_write(&tx_pipe, self->input_buf, self->input_idx);
      ipc_pipe_write(&tx_pipe, crlf, 2);
      self->input_idx = 0;
    }

    static const uint8_t prompt_cmd[] = "shell> ";
    ipc_pipe_write(&tx_pipe, prompt_cmd, sizeof(prompt_cmd) - 1);

  } else if (ctrl == '\b' || ctrl == KEY_DELETE) {
    if (self->input_idx > 0) {
      self->input_idx--;
      static const uint8_t bksp[] = "\b \b";
      ipc_pipe_write(&tx_pipe, bksp, 3);
    }
  }
  /* other C0 controls (and a flushed bare ESC) are ignored here */
}

static void shell_on_csi(void *ctx, const ansi_csi_t *csi)
{
  /* Special keys arrive decoded; this basic shell ignores them. */
  (void)ctx;
  uint8_t  mods = 0;
  (void)ansi_key_from_csi(csi, &mods);
}

static const ansi_handlers_t SHELL_HANDLERS = {
  shell_on_print, shell_on_execute, shell_on_csi, /* on_esc */ 0,
  /* osc */ 0, /* dcs_hook */ 0, /* dcs_put */ 0, /* dcs_unhook */ 0,
};

PROCESS_THREAD(shell, ev, data)
{
  process_shell_t *self = PROCESS_SELF(shell);

  PROCESS_BEGIN();

  PROCESS_SET_PIPEIN(&rx_pipe);
  PROCESS_SET_PIPEOUT(&tx_pipe);

  self->input_idx = 0;
  ansi_parser_init(&self->parser, &SHELL_HANDLERS, self, NULL, 0);

  PROCESS_PIPE_WRITE_ATOMIC(prompt, sizeof(prompt) - 1);

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(
        ev == PROCESS_EVENT_POLL || ev == PROCESS_EVENT_INIT);

    while (PROCESS_PIPE_AVAILABLE()) {
      uint8_t ch;
      size_t  nread;
      PROCESS_PIPE_READ(&ch, 1, &nread);
      if (nread == 0)
        break;

      /* The parser invokes SHELL_HANDLERS synchronously per action;
       * escape sequences never reach the line buffer. */
      ansi_parse(&self->parser, ch);
    }

    /* NOTE: no ansi_parser_flush() here.  The RX ISR wakes this process
     * per byte, so the pipe regularly drains *between* the bytes of one
     * escape burst — flushing now would misread every arrow key as a
     * bare ESC + text.  This basic shell has no timer, so a lone ESC
     * keypress is simply absorbed; see 13-pt-basic-echo for the
     * clock-based escape-gap pattern, or use mcurses getch_ex() which
     * handles the timeout internally. */
  }

  PROCESS_END();
}

void setup(void)
{
  protoduino_start();

  ipc_pipe_init(&rx_pipe, rx_buf, sizeof(rx_buf),
                process_ipc_wake, &shell_proc.base);
  ipc_pipe_init(&tx_pipe, tx_buf, sizeof(tx_buf),
                tx_wake_cb, NULL);

  uart0_on_rx_complete(on_rx_complete);
  uart0_on_rx_error(on_rx_error);
  uart0_on_tx_complete(on_tx_complete);
  uart0_open(9600);

  process_start(&shell_proc.base, NULL);
}

void loop(void)
{
  process_run();
}

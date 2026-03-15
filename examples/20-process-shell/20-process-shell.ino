// file: 20-process-shell.ino
/*
 * 20-process-shell.ino  –  interactive UART shell demo for protoduino
 * ================================================================
 */

#include <protoduino.h>
#include <autostart.h>
#include <sys/process.h>
#include <sys/uart.h>
#include <lib/text/vterm.h>

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

/* VT escape sequence parser state */
enum shell_parse_state {
  SHELL_NORMAL = 0,
  SHELL_ESCAPE = 1
};

PROCESS_DEFINE(shell, "shell", 2,
  uint8_t  input_buf[32];
  uint8_t  input_idx;
  enum shell_parse_state parse_state;
  uint8_t  esc_buf[VT_ESCAPE_BUFLEN];
  uint8_t  esc_idx;
);

PROCESS_INSTANCE(shell, shell_proc);

static const uint8_t prompt[] CC_PROGMEM = "shell> ";

PROCESS_THREAD(shell, ev, data)
{
  process_shell_t *self = PROCESS_SELF(shell);

  PROCESS_BEGIN();

  PROCESS_SET_PIPEIN(&rx_pipe);
  PROCESS_SET_PIPEOUT(&tx_pipe);

  self->input_idx   = 0;
  self->parse_state = SHELL_NORMAL;
  self->esc_idx     = 0;

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

      if (self->parse_state == SHELL_ESCAPE) {
        int8_t ret = vt_esc_add16((char *)self->esc_buf,
                                   &self->esc_idx, ch);
        if (ret == ERR_SUCCESS) {
          vt_esc_match16((const char *)self->esc_buf, self->esc_idx);
          self->parse_state = SHELL_NORMAL;
        } else if (ret != ERR_YIELDING) {
          self->parse_state = SHELL_NORMAL;
        }
        continue;
      }

      if (ch == KEY_ESCAPE) {
        self->parse_state = SHELL_ESCAPE;
        self->esc_idx     = 0;
        vt_esc_add16((char *)self->esc_buf, &self->esc_idx, ch);
      } else if (ch == KEY_BACKSPACE || ch == KEY_DELETE
                 || ch == '\b'       || ch == 127) {
        if (self->input_idx > 0) {
          self->input_idx--;
          static const uint8_t bksp[] = "\b \b";
          PROCESS_PIPE_WRITE_ATOMIC(bksp, 3);
        }
      } else if (ch == KEY_ENTER || ch == '\r' || ch == '\n') {
        static const uint8_t crlf[] = "\r\n";
        PROCESS_PIPE_WRITE_ATOMIC(crlf, 2);

        uint8_t end = (self->input_idx < sizeof(self->input_buf) - 1)
                      ? self->input_idx
                      : (uint8_t)(sizeof(self->input_buf) - 1);
        self->input_buf[end] = '\0';

        if (self->input_idx > 0) {
          static const uint8_t prefix[] = "Command: ";
          PROCESS_PIPE_WRITE_ATOMIC(prefix, sizeof(prefix) - 1);
          PROCESS_PIPE_WRITE_ATOMIC(self->input_buf, self->input_idx);
          PROCESS_PIPE_WRITE_ATOMIC(crlf, 2);
          self->input_idx = 0;
        }

        static const uint8_t prompt_cmd[] = "shell> ";
        PROCESS_PIPE_WRITE_ATOMIC(prompt_cmd, sizeof(prompt_cmd) - 1);

      } else if (ch >= 32 && ch <= 126) {
        if (self->input_idx < sizeof(self->input_buf) - 1) {
          self->input_buf[self->input_idx++] = ch;
          PROCESS_PIPE_WRITE_ATOMIC(&ch, 1);
        }
      }
    }
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
  
  process_start(&shell_proc.base);
}

void loop(void)
{
  process_run();
}

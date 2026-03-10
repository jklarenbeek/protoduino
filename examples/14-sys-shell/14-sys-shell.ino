// file: 14-sys-shell.ino

#include <protoduino.h>
#include <autostart.h>
#include <sys/process.h>
#include <sys/uart.h>
#include <lib/text/vterm.h>


static uint8_t rx_buf[64];
static ipc_pipe_t rx_pipe;

static uint8_t tx_buf[64];
static ipc_pipe_t tx_pipe;

static void on_rx_complete(uint_fast8_t data) {
  uint8_t b = data;
  ipc_pipe_write(&rx_pipe, &b, 1);
}

static uint32_t errcnt = 0;
static void on_rx_error(uint_fast8_t err) {
  errcnt++;
  uart0_rx_clear_errors();
}

static int_fast16_t on_tx_complete(void) {
  uint8_t data;
  if (ipc_pipe_read(&tx_pipe, &data, 1) > 0) {
    return data;
  }
  return -1;
}

PROCESS(shell_process, "shell", 2);

static void tx_wake_cb(void *ctx) { uart0_tx_enable_int(); }

PROCESS_THREAD(shell_process, ev, data) {
  PROCESS_BEGIN();

  PROCESS_SET_PIPEIN(&rx_pipe);
  PROCESS_SET_PIPEOUT(&tx_pipe);

  static uint8_t input_buf[32];
  static uint8_t input_idx = 0;

  static enum { NORMAL, ESCAPE } state = NORMAL;
  static uint8_t esc_buf[VT_ESCAPE_BUFLEN];
  static uint8_t esc_idx = 0;

  static const uint8_t prompt[] = "shell> ";
  PROCESS_PIPE_WRITE_ATOMIC(prompt, sizeof(prompt) - 1);

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL ||
                             ev == PROCESS_EVENT_INIT);

    static uint8_t ch;
    static size_t read_len;

    while (PROCESS_PIPE_AVAILABLE()) {
      PROCESS_PIPE_READ(&ch, 1, &read_len);
      if (read_len == 0)
        continue;

      if (state == ESCAPE) {
        int8_t ret = vt_esc_add16((char *)esc_buf, &esc_idx, ch);
        if (ret == ERR_SUCCESS) {
          rune16_t rune = vt_esc_match16((const char *)esc_buf, esc_idx);
          state = NORMAL;
          // Optionally handle VT100 keys (e.g. arrows) here
        } else if (ret != ERR_YIELDING) {
          state = NORMAL; // invalid escape sequence
        }
        continue;
      }

      if (ch == KEY_ESCAPE) {
        state = ESCAPE;
        esc_idx = 0;
        vt_esc_add16((char *)esc_buf, &esc_idx, ch);
      } else if (ch == KEY_BACKSPACE || ch == KEY_DELETE || ch == '\b' ||
                 ch == 127) {
        if (input_idx > 0) {
          input_idx--;
          static const uint8_t bksp[] = "\b \b";
          PROCESS_PIPE_WRITE_ATOMIC(bksp, 3);
        }
      } else if (ch == KEY_ENTER || ch == '\r' || ch == '\n') {
        static const uint8_t crlf[] = "\r\n";
        PROCESS_PIPE_WRITE_ATOMIC(crlf, 2);

        // ensure null termination and evaluate command
        input_buf[input_idx < sizeof(input_buf) ? input_idx
                                                : sizeof(input_buf) - 1] = '\0';

        if (input_idx > 0) {
          static const uint8_t cmd_msg[] = "Command: ";
          PROCESS_PIPE_WRITE_ATOMIC(cmd_msg, sizeof(cmd_msg) - 1);
          PROCESS_PIPE_WRITE_ATOMIC(input_buf, input_idx);
          PROCESS_PIPE_WRITE_ATOMIC(crlf, 2);
          input_idx = 0;
        }

        PROCESS_PIPE_WRITE_ATOMIC(prompt, sizeof(prompt) - 1);
      } else if (ch >= 32 && ch <= 126) {
        if (input_idx < sizeof(input_buf) - 1) {
          input_buf[input_idx++] = ch;
          PROCESS_PIPE_WRITE_ATOMIC(&ch, 1);
        }
      }
    }
  }

  PROCESS_END();
}

void setup() {

  protoduino_start();

  ipc_pipe_init(&rx_pipe, rx_buf, sizeof(rx_buf), process_ipc_wake,
                &shell_process);
  ipc_pipe_init(&tx_pipe, tx_buf, sizeof(tx_buf), tx_wake_cb, NULL);

  uart0_on_rx_complete(on_rx_complete);
  uart0_on_rx_error(on_rx_error);
  uart0_on_tx_complete(on_tx_complete);
  uart0_open(9600);

  process_start(&shell_process);
}

void loop() { process_run(); }

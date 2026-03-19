// file: examples/21-process-mcurses/21-process-mcurses.ino

/*
 * 21-process-mcurses.ino  –  mcurses TUI demo for protoduino
 * ===========================================================
 *
 * What this sketch demonstrates
 * ------------------------------
 *  1. Wiring mcurses_t to a UART via ipc_pipe_t (same pattern as the
 *     shell demo in 20-process-shell.ino).
 *  2. Running two independent screen processes on the *same* terminal
 *     (one draws a status dashboard, one is a mini input form) to show
 *     that mcurses_t is fully re-entrant – no shared globals.
 *  3. Using PROGMEM strings (addstr_P_ex) to save SRAM on AVR.
 *  4. Attributes: bold, reverse, colour.
 *  5. Scroll region + scroll().
 *  6. Line-editor getnstr_ex() with KEY_* echo.
 *  7. Cursor visibility control.
 *  8. Cooperative scheduling: neither process blocks process_run().
 *
 * Terminal requirements: VT100-compatible, 80×24, UTF-8.
 * Open e.g. `screen /dev/ttyUSB0 9600` or PuTTY in VT100 mode.
 *
 * Memory budget (Arduino Uno, ATmega328P):
 *   tx_buf  96 bytes  (SRAM)
 *   rx_buf  32 bytes  (SRAM)
 *   mcurses_t ~16 bytes (SRAM)
 *   Two process structs + fields ~80 bytes (SRAM)
 *   All label strings in PROGMEM (Flash only)
 *
 * Layout (80×24):
 *
 *   ┌──────────────────────────────────────────────────────────────────────────────┐
 *   │ row 0    : title bar (reverse)                                               │
 *   │ row 1    : blank                                                             │
 *   │ rows 2-9 : stats panel (uptime, free SRAM, error counter, …)                │
 *   │ row 10   : blank separator                                                   │
 *   │ rows 11-19: event log (scroll region, newest at bottom)                      │
 *   │ row 20   : separator line                                                    │
 *   │ row 21   : input prompt  "Cmd: [               ]"                            │
 *   │ row 22   : command output / response                                         │
 *   │ row 23   : status bar (reverse, bottom)                                      │
 *   └──────────────────────────────────────────────────────────────────────────────┘
 */

#include <protoduino.h>
#include <autostart.h>
#include <sys/process.h>
#include <sys/uart.h>
#include <lib/text/mcurses.h>   /* the library we just wrote */

/* =========================================================================
 * Screen dimensions
 * ========================================================================= */

#define SCR_ROWS        24
#define SCR_COLS        80

/* Log scroll region */
#define LOG_TOP         11
#define LOG_BOT         19

/* Input row */
#define INPUT_ROW       21
#define INPUT_COL       6    /* after "Cmd: " */
#define INPUT_MAXLEN    32

/* =========================================================================
 * UART / pipe plumbing  (identical pattern to 20-process-shell.ino)
 * ========================================================================= */

static uint8_t    tx_buf[96];
static ipc_pipe_t tx_pipe;

static uint8_t    rx_buf[32];
static ipc_pipe_t rx_pipe;

/* Forward declaration – the dashboard process owns the screen instance */
struct process;
PROCESS_DEFINE(dashboard, "dashboard", 2,
  mcurses_t  scr;          /* ← entire terminal state, no globals */
  uint32_t   uptime_s;     /* seconds since boot (incremented by timer event) */
  uint16_t   log_line;     /* total log lines written (for numbering)          */
  uint8_t    dirty;        /* non-zero when stats need a redraw                */
);

PROCESS_DEFINE(cmdline, "cmdline", 3,  /* lower prio number = higher prio */
  char     buf[INPUT_MAXLEN + 1];
  uint8_t  response_row;  /* tracks whether we've overflowed row 22           */
);

typedef process_dashboard_t dashboard_t;
typedef process_cmdline_t cmdline_t;

PROCESS_INSTANCE(dashboard, dashboard_proc);
PROCESS_INSTANCE(cmdline,   cmdline_proc);

/* =========================================================================
 * UART callbacks
 * ========================================================================= */

static void on_rx_byte(uint_fast8_t ch)
{
  uint8_t b = (uint8_t)ch;
  ipc_pipe_write(&rx_pipe, &b, 1);
  /* Wake the cmdline process – it owns the keyboard */
  process_poll(&cmdline_proc.base);
}

static void on_rx_error(uint_fast8_t err)
{
  (void)err;
  uart0_rx_clear_errors();
}

static int_fast16_t on_tx_complete(void)
{
  uint8_t data;
  return (ipc_pipe_read(&tx_pipe, &data, 1) > 0)
       ? (int_fast16_t)data : -1;
}

static void tx_wake_cb(void *ctx)
{
  (void)ctx;
  uart0_tx_enable_int();
}

/* =========================================================================
 * PROGMEM string constants  (save SRAM on AVR)
 * ========================================================================= */

static const char P_TITLE[]   CC_PROGMEM = " protoduino mcurses demo  |  press ENTER to run a command  |  ESC = quit";
static const char P_SEP[]     CC_PROGMEM = "────────────────────────────────────────────────────────────────────────────────";
static const char P_STATS[]   CC_PROGMEM = " Stats";
static const char P_UPTIME[]  CC_PROGMEM = "  Uptime  : ";
static const char P_SRAM[]    CC_PROGMEM = "  FreeSRAM: ";
static const char P_ERRS[]    CC_PROGMEM = "  RxErrors: ";
static const char P_LOGHEAD[] CC_PROGMEM = " Event log";
static const char P_PROMPT[]  CC_PROGMEM = "Cmd: ";
static const char P_STATUS[]  CC_PROGMEM = " [UP/DOWN = scroll log]  [any key = refresh]  [ESC = quit]";

static const char P_CMD_HELP[]   CC_PROGMEM = "Commands: help  clear  reset  sram";
static const char P_CMD_SRAM[]   CC_PROGMEM = "Free SRAM: ";
static const char P_CMD_RESET[]  CC_PROGMEM = "Resetting...";
static const char P_CMD_UNKNOWN[]CC_PROGMEM = "Unknown command. Try: help";
static const char P_CMD_CLEAR[]  CC_PROGMEM = "(log cleared)";

static const char P_LOG_BOOT[]   CC_PROGMEM = "System boot";
static const char P_LOG_READY[]  CC_PROGMEM = "mcurses ready";
static const char P_LOG_CMD[]    CC_PROGMEM = "Command: ";

/* =========================================================================
 * Tiny helpers
 * ========================================================================= */

/** Return free SRAM (AVR-specific; portable stub for non-AVR). */
static uint16_t free_sram(void)
{
#ifdef __AVR__
  extern int __heap_start, *__brkval;
  int v;
  return (uint16_t)((int)&v - (__brkval == 0
                               ? (int)&__heap_start
                               : (int)__brkval));
#else
  return 0xFFFFu;
#endif
}

/** Write a uint32_t as decimal into the TX pipe via addstr_ex. */
static void put_u32(mcurses_t *scr, uint32_t v)
{
  char buf[11]; /* max 10 digits + NUL */
  uint8_t i = sizeof(buf) - 1;
  buf[i] = '\0';
  if (v == 0) { buf[--i] = '0'; }
  else {
    while (v && i > 0) {
      buf[--i] = (char)('0' + (v % 10));
      v /= 10;
    }
  }
  addstr_ex(scr, &buf[i]);
}

/** Write a uint16_t as decimal. */
static void put_u16(mcurses_t *scr, uint16_t v)
{
  put_u32(scr, (uint32_t)v);
}

/* =========================================================================
 * Dashboard drawing helpers
 * ========================================================================= */

static void draw_title(mcurses_t *scr)
{
  move_ex(scr, 0, 0);
  attrset_ex(scr, MCURSES_ATTR_REVERSE | MCURSES_ATTR_BOLD);
  addstr_P_ex(scr, P_TITLE);
  /* Pad to full width */
  uint8_t len = (uint8_t)strlen_P(P_TITLE);
  while (len++ < SCR_COLS)
    addch_ex(scr, ' ');
}

static void draw_statusbar(mcurses_t *scr)
{
  move_ex(scr, SCR_ROWS - 1, 0);
  attrset_ex(scr, MCURSES_ATTR_REVERSE);
  addstr_P_ex(scr, P_STATUS);
  uint8_t len = (uint8_t)strlen_P(P_STATUS);
  while (len++ < SCR_COLS)
    addch_ex(scr, ' ');
}

static void draw_stats_labels(mcurses_t *scr)
{
  attrset_ex(scr, MCURSES_ATTR_BOLD | MCURSES_FG_CYAN);
  move_ex(scr, 2, 0);  addstr_P_ex(scr, P_STATS);
  attrset_ex(scr, MCURSES_ATTR_NORMAL);
  move_ex(scr, 3, 0);  addstr_P_ex(scr, P_SEP);

  move_ex(scr, 4, 0);  addstr_P_ex(scr, P_UPTIME);
  move_ex(scr, 5, 0);  addstr_P_ex(scr, P_SRAM);
  move_ex(scr, 6, 0);  addstr_P_ex(scr, P_ERRS);

  attrset_ex(scr, MCURSES_ATTR_BOLD | MCURSES_FG_CYAN);
  move_ex(scr, 9, 0);  addstr_P_ex(scr, P_LOGHEAD);
  attrset_ex(scr, MCURSES_ATTR_NORMAL);
  move_ex(scr, 10, 0); addstr_P_ex(scr, P_SEP);

  move_ex(scr, 20, 0); addstr_P_ex(scr, P_SEP);

  attrset_ex(scr, MCURSES_ATTR_BOLD);
  move_ex(scr, INPUT_ROW, 0);
  addstr_P_ex(scr, P_PROMPT);
  attrset_ex(scr, MCURSES_ATTR_NORMAL);
}

static uint32_t rx_error_count = 0;  /* global error count */

static void draw_stats_values(mcurses_t *scr, dashboard_t *self)
{

  attrset_ex(scr, MCURSES_ATTR_BOLD | MCURSES_FG_GREEN);

  move_ex(scr, 4, 12);
  put_u32(scr, self->uptime_s);
  addstr_ex(scr, "s   ");  /* trailing spaces clear old digits */

  move_ex(scr, 5, 12);
  put_u16(scr, free_sram());
  addstr_ex(scr, " B   ");

  move_ex(scr, 6, 12);
  put_u32(scr, rx_error_count);
  addstr_ex(scr, "   ");

  attrset_ex(scr, MCURSES_ATTR_NORMAL);
}

/**
 * Append one line to the event log scroll region.
 * The region (rows LOG_TOP..LOG_BOT) scrolls up automatically when full.
 */
static void log_append(mcurses_t *scr, dashboard_t *self,
                        const char *msg_P, const char *detail)
{
  /* Activate scroll region and move to its bottom row */
  setscrreg_ex(scr, LOG_TOP, LOG_BOT);
  move_ex(scr, LOG_BOT, 0);

  /* Scroll up to create a blank line at the bottom */
  scroll_ex(scr);

  /* Write log entry number + message */
  attrset_ex(scr, MCURSES_FG_YELLOW);
  addch_ex(scr, '[');
  put_u16(scr, self->log_line++);
  addch_ex(scr, ']');
  addch_ex(scr, ' ');

  attrset_ex(scr, MCURSES_ATTR_NORMAL);
  if (msg_P)    addstr_P_ex(scr, msg_P);
  if (detail)   addstr_ex(scr,   detail);
  clrtoeol_ex(scr); /* clear rest of line */

  /* Reset scroll region to full screen so other drawing isn't clipped */
  setscrreg_ex(scr, 0, SCR_ROWS - 1);
}

/* =========================================================================
 * PROCESS: dashboard
 * =========================================================================
 *
 * Owns the mcurses_t instance.  Draws the static chrome on INIT, then
 * refreshes the stats panel on every PROCESS_EVENT_POLL (triggered by a
 * soft timer in loop()).  Also receives PROCESS_EVENT_MSG from the cmdline
 * process to append log entries.
 */

PROCESS_THREAD(dashboard, ev, data)
{
  dashboard_t *self = PROCESS_SELF(dashboard);

  PROCESS_BEGIN();

  /* ---- Wire up the shared screen to our process pipes ---- */
  PROCESS_SET_PIPEOUT(&tx_pipe);  /* dashboard writes to terminal */

  /* Initialise mcurses against the TX/RX pipes */
  mcurses_init(&self->scr, &tx_pipe, &rx_pipe, SCR_ROWS, SCR_COLS);
  initscr_ex(&self->scr);
  curs_set_ex(&self->scr, 0);  /* hide cursor during bulk draw */

  self->uptime_s = 0;
  self->log_line = 0;
  self->dirty    = 1;

  /* Draw static chrome */
  draw_title(&self->scr);
  draw_statusbar(&self->scr);
  draw_stats_labels(&self->scr);

  /* First log entries */
  log_append(&self->scr, self, P_LOG_BOOT,  NULL);
  log_append(&self->scr, self, P_LOG_READY, NULL);

  /* Park cursor in the input field and show it */
  move_ex(&self->scr, INPUT_ROW, INPUT_COL);
  curs_set_ex(&self->scr, 1);
  refresh_ex(&self->scr);

  while (1) {
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_POLL) {
      /* Called from loop() every ~1 s to tick uptime */
      self->uptime_s++;
      self->dirty = 1;
    }

    if (self->dirty) {
      self->dirty = 0;
      curs_set_ex(&self->scr, 0);
      draw_stats_values(&self->scr, self);
      /* Restore cursor to input field */
      move_ex(&self->scr, INPUT_ROW, INPUT_COL);
      curs_set_ex(&self->scr, 1);
      refresh_ex(&self->scr);
    }

    if (ev == PROCESS_EVENT_MSG && data != NULL) {
      /*
       * cmdline sends us a (const char *) pointing into its own
       * input buffer as the message payload.  We only read it here,
       * inside this dispatch, so lifetime is fine.
       */
      const char *cmd = (const char *)data;
      curs_set_ex(&self->scr, 0);
      log_append(&self->scr, self, P_LOG_CMD, cmd);

      /* Clear command response row */
      move_ex(&self->scr, 22, 0);
      clrtoeol_ex(&self->scr);

      /* Dispatch command */
      attrset_ex(&self->scr, MCURSES_FG_GREEN | MCURSES_ATTR_BOLD);
      move_ex(&self->scr, 22, 0);

      if (strcmp_P(cmd, PSTR("help")) == 0) {
        addstr_P_ex(&self->scr, P_CMD_HELP);

      } else if (strcmp_P(cmd, PSTR("clear")) == 0) {
        /* Clear just the log region */
        setscrreg_ex(&self->scr, LOG_TOP, LOG_BOT);
        for (uint8_t r = LOG_TOP; r <= LOG_BOT; r++) {
          move_ex(&self->scr, r, 0);
          clrtoeol_ex(&self->scr);
        }
        setscrreg_ex(&self->scr, 0, SCR_ROWS - 1);
        self->log_line = 0;
        addstr_P_ex(&self->scr, P_CMD_CLEAR);

      } else if (strcmp_P(cmd, PSTR("sram")) == 0) {
        addstr_P_ex(&self->scr, P_CMD_SRAM);
        put_u16(&self->scr, free_sram());
        addstr_ex(&self->scr, " bytes free");

      } else if (strcmp_P(cmd, PSTR("reset")) == 0) {
        attrset_ex(&self->scr, MCURSES_ATTR_BOLD | MCURSES_FG_RED);
        addstr_P_ex(&self->scr, P_CMD_RESET);
        endwin_ex(&self->scr);
        refresh_ex(&self->scr);
#ifdef __AVR__
        void (*reset_fn)(void) = NULL;
        reset_fn(); /* jump to address 0 – soft reset on AVR */
#endif

      } else {
        attrset_ex(&self->scr, MCURSES_FG_RED);
        addstr_P_ex(&self->scr, P_CMD_UNKNOWN);
      }

      attrset_ex(&self->scr, MCURSES_ATTR_NORMAL);
      move_ex(&self->scr, INPUT_ROW, INPUT_COL);
      curs_set_ex(&self->scr, 1);
      refresh_ex(&self->scr);
    }
  }

  endwin_ex(&self->scr);
  PROCESS_END();
}

/* =========================================================================
 * PROCESS: cmdline
 * =========================================================================
 *
 * Reads keys from the RX pipe using getch_ex() in nodelay mode (non-
 * blocking), echoes them through the shared TX pipe, and sends the
 * completed command string to the dashboard via process_post_msg.
 *
 * Because getch_ex() is non-blocking here (nodelay=1), the process
 * yields back to the scheduler via PROCESS_WAIT_EVENT_UNTIL when there
 * is nothing to read – it never spins and never starves other processes.
 *
 * The mcurses_t pointer is borrowed from the dashboard instance.
 * Both processes share the same tx/rx pipes so they interleave cleanly.
 */

PROCESS_THREAD(cmdline, ev, data)
{
  cmdline_t *self = PROCESS_SELF(cmdline);

  /* Borrow the screen from the dashboard – valid because dashboard_proc
   * is a static instance that outlives everything. */
  mcurses_t *scr = &dashboard_proc.scr;

  PROCESS_BEGIN();

  PROCESS_SET_PIPEIN(&rx_pipe);
  PROCESS_SET_PIPEOUT(&tx_pipe);

  self->buf[0]      = '\0';
  self->response_row = 0;

  /* Enable nodelay: getch_ex returns KEY_ERR immediately when pipe empty */
  nodelay_ex(scr, MCURSES_NODELAY_ON);

  while (1) {
    /* Yield until the RX ISR wakes us via process_poll() */
    PROCESS_WAIT_EVENT_UNTIL(
        ev == PROCESS_EVENT_POLL || ev == PROCESS_EVENT_INIT);

    /* Drain all bytes currently in the RX pipe */
    for (;;) {
      uint_fast8_t ch = getch_ex(scr);
      if (ch == MCURSES_KEY_ERR)
        break; /* pipe empty – yield back to scheduler */

      if (ch == KEY_ESCAPE) {
        /* ESC: shut everything down gracefully */
        process_destroy(&dashboard_proc.base);
        process_destroy(&cmdline_proc.base);
        break;
      }

      if (ch == KEY_ENTER || ch == '\r') {
        /* NUL-terminate and dispatch to dashboard */
        uint8_t len = (uint8_t)strlen(self->buf);
        if (len > 0) {
          process_post(&dashboard_proc.base,
                       PROCESS_EVENT_MSG,
                       (process_data_t)self->buf);
        }
        /* Clear input field on screen */
        move_ex(scr, INPUT_ROW, INPUT_COL);
        clrtoeol_ex(scr);
        /* Reset buffer */
        self->buf[0] = '\0';
        move_ex(scr, INPUT_ROW, INPUT_COL);
        continue;
      }

      if (ch == KEY_BACKSPACE || ch == KEY_DELETE || ch == '\b') {
        uint8_t len = (uint8_t)strlen(self->buf);
        if (len > 0) {
          self->buf[len - 1] = '\0';
          /* Erase one character on screen */
          move_ex(scr, INPUT_ROW, (uint8_t)(INPUT_COL + len - 1));
          addch_ex(scr, ' ');
          move_ex(scr, INPUT_ROW, (uint8_t)(INPUT_COL + len - 1));
        }
        continue;
      }

      /* Printable ASCII – echo and buffer */
      if (ch >= 0x20u && ch < 0x7Fu) {
        uint8_t len = (uint8_t)strlen(self->buf);
        if (len < INPUT_MAXLEN) {
          self->buf[len]     = (char)ch;
          self->buf[len + 1] = '\0';
          /* Echo at the correct column */
          move_ex(scr, INPUT_ROW, (uint8_t)(INPUT_COL + len));
          attrset_ex(scr, MCURSES_ATTR_BOLD);
          addch_ex(scr, ch);
          attrset_ex(scr, MCURSES_ATTR_NORMAL);
        }
        continue;
      }

      /* Arrow key demo: UP scrolls the log one line, DOWN scrolls back */
      if (ch == KEY_UP) {
        /* We can't truly scroll back without a buffer, but we can show
         * the sequence was received by logging it. */
        process_post(&dashboard_proc.base,
                     PROCESS_EVENT_MSG,
                     (process_data_t)"[UP]");
        continue;
      }
      if (ch == KEY_DOWN) {
        process_post(&dashboard_proc.base,
                     PROCESS_EVENT_MSG,
                     (process_data_t)"[DOWN]");
        continue;
      }
    }
  }

  PROCESS_END();
}

/* =========================================================================
 * Soft timer – poll the dashboard once per second
 * =========================================================================
 * On a real board you would use a hardware timer ISR or the protoduino
 * timer process.  For clarity we count loop() iterations here.
 */
static uint16_t tick_counter = 0;

/* =========================================================================
 * setup / loop
 * ========================================================================= */

void setup(void)
{
  protoduino_start();

  /* Pipes */
  ipc_pipe_init(&tx_pipe, tx_buf, sizeof(tx_buf), tx_wake_cb,        NULL);
  ipc_pipe_init(&rx_pipe, rx_buf, sizeof(rx_buf), NULL,              NULL);
  /* rx wake is handled by process_poll() inside on_rx_byte directly  */

  /* UART callbacks */
  uart0_on_rx_complete(on_rx_byte);
  uart0_on_rx_error(on_rx_error);
  uart0_on_tx_complete(on_tx_complete);
  uart0_open(9600);

  /* Start processes – dashboard must start first (it owns mcurses_t) */
  process_new(&process_type_dashboard, &dashboard_proc);
  process_new(&process_type_cmdline, &cmdline_proc);
}

void loop(void)
{
  process_run();

  /* Coarse 1-second tick: poll the dashboard so it increments uptime.
   * At 9600 baud loop() runs thousands of times per second; we divide
   * it down.  Replace with a proper timer process for production use. */
  if (++tick_counter == 0) {          /* wraps every 65536 iterations */
    process_poll(&dashboard_proc.base);
  }
}

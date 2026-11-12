// file: examples/4-modules/33-your-own-module/33-your-own-module.ino

/*
 * 33-your-own-module.ino  –  writing a module
 * ================================================================
 *
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   A module written in the sketch, between two from the library:
 *
 *     ticker | dots | serial:0
 *
 *   dots turns every digit it receives into that many dots and a line end:
 *
 *     .
 *     ..
 *     ...
 *
 * THE FIVE RULES OF A MODULE
 * --------------------------
 *   1. One process type, and all of its state in the process struct. No
 *      static variables: two instances must not share anything, and a
 *      module that is loaded into flash later has no RAM of its own.
 *   2. It owns its input pipe and publishes it when it starts, unless
 *      somebody connected one before. It reads through PROCESS_PIPEIN().
 *   3. It never blocks. It waits: PROCESS_WAIT_EVENT_UNTIL() for a
 *      condition, PROCESS_PIPE_WRITE() for room in the output.
 *   4. Nothing is kept in a local variable across a wait, because the
 *      function returns at every wait. The loop counter below is in the
 *      struct for that reason. (In a sketch, which is C++, a local with an
 *      initial value may not even stand before a wait in the same block.)
 *   5. It ends with a PT_FINALLY block. That is what runs when the process
 *      is ended, and without it the thread has no end to go to.
 *
 *   Move the part between the two lines of dashes to a header of its own
 *   and it is a module like mod/upper.h.
 *
 * TRY IT   (115200 baud)
 */

#include <protoduino.h>
#include <sys/process.h>
#include <sys/process/serial.h>
#include <sys/tick.h>

#include <mod/ticker.h>

/* ---------------------------------------------------------------- */

PROCESS_DEFINE(dots, "dots", 3,
  ipc_pipe_t in;       /* rule 2: its own input            */
  uint8_t    buf[4];
  uint8_t    byte;     /* the byte being worked on         */
  uint8_t    left;     /* rule 4: dots still to be written */
);

PROCESS_THREAD(dots, ev, data)
{
  process_dots_t *self = PROCESS_SELF(dots);
  (void)ev;
  (void)data;

  PROCESS_BEGIN();

  if (!pt_process->has_pipein)
  {
    ipc_pipe_init(&self->in, self->buf, sizeof(self->buf), process_ipc_wake, (void *)pt_process);
    PROCESS_SET_PIPEIN(&self->in);
  }

  while (1)
  {
    /* rule 3: wait for a byte */
    PROCESS_WAIT_EVENT_UNTIL(ipc_pipe_available(PROCESS_PIPEIN()) > 0);
    ipc_pipe_read(PROCESS_PIPEIN(), &self->byte, 1);

    if (self->byte < '0' || self->byte > '9' || !pt_process->has_pipeout)
      continue;

    for (self->left = self->byte - '0'; self->left > 0; self->left--)
    {
      self->byte = '.';
      PROCESS_PIPE_WRITE(&self->byte, 1); /* rule 3: wait for room */
    }
    self->byte = '\r';
    PROCESS_PIPE_WRITE(&self->byte, 1);
    self->byte = '\n';
    PROCESS_PIPE_WRITE(&self->byte, 1);
  }

  /* rule 5: nothing to give back, but this is where the thread ends */
  PT_FINALLY(&pt_process->pt)

  PROCESS_END();
}

/* ---------------------------------------------------------------- */

PROCESS_INSTANCE(ticker, ticker_proc);
PROCESS_INSTANCE(dots, dots_proc);

static bool connected;

void setup(void)
{
  process_init(NULL);

  process_new(&process_type_serial_process0, &serial_process0_proc);
  serial_process0_proc.baud = 115200;
  process_new(&process_type_ticker, &ticker_proc);
  process_new(&process_type_dots, &dots_proc);
}

void loop(void)
{
  process_run();
  tick_run();

  /* ticker | dots | serial:0 */
  if (!connected)
    connected = process_connect(&ticker_proc.base, &dots_proc.base)
             && process_connect(&dots_proc.base, &serial_process0_proc.base);
}

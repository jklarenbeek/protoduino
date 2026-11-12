// file: examples/5-systems/43-monitor/43-monitor.ino

/*
 * 43-monitor.ino  –  a finished system, and a shell that only looks
 * ================================================================
 *
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   A product does not take commands from whoever finds its serial port.
 *   Here the modules are chosen, started and connected by the sketch, and
 *   the shell is a monitor: it shows the system and cannot change it.
 *
 *     ticker | blink        the LED blinks while all is well
 *     vcc | keeper          keeper reads the supply voltage; below
 *                           KEEPER_CONF_TENTHS it stops the LED and
 *                           writes a warning into the log
 *
 *     > ps                  what runs, and in which state
 *     serial:0 W 1
 *     shell    R 2
 *     keeper   W 2
 *     ticker   W 3
 *     blink    P 3          P: paused by keeper, the supply is low
 *     vcc      W 3
 *     > mods                what the build contains; upper was not started
 *     0 ticker +
 *     1 blink +
 *     2 vcc +
 *     3 keeper +
 *     4 upper -
 *     > ipc                 who writes to whom
 *     serial:0 > shell
 *     shell > serial:0
 *     ticker > blink
 *     vcc > keeper
 *     > log                 name, severity, event, code
 *     keeper 5 b1 21        a warning (5) about the power (b1): 0x21 = 3.3 V
 *     > kill blink
 *     ?                     a monitor changes nothing
 *
 *   keeper is a process of this sketch: the part of a system that knows
 *   what the modules are for. It is written like a module (example 33).
 *
 * TRY IT   (115200 baud)
 *   On a board at 5 V nothing is wrong: blink shows W and the log is empty.
 *   Raise KEEPER_CONF_TENTHS to 55 to see the other case.
 */

#include <protoduino.h>
#include <sys/process.h>
#include <sys/process/serial.h>
#include <sys/tick.h>

#include <mod/ticker.h>
#include <mod/blink.h>
#include <mod/vcc.h>
#include <mod/upper.h>

#define SHELL_CONF_READONLY 1
#include <mod/shell.h>

PROCESS_INSTANCE(shell, shell_proc);
PROCESS_INSTANCE(ticker, ticker_proc);
PROCESS_INSTANCE(blink, blink_proc);
PROCESS_INSTANCE(vcc, vcc_proc);
PROCESS_INSTANCE(upper, upper_proc);

/* ---------------------------------------------------------------- */
/* keeper: watches the supply voltage                                */

/* the lowest voltage that is still well, in tenths of a volt */
#ifndef KEEPER_CONF_TENTHS
#define KEEPER_CONF_TENTHS 45
#endif

PROCESS_DEFINE(keeper, "keeper", 2,
  ipc_pipe_t in;
  uint8_t    buf[8];
  uint8_t    byte;
  uint8_t    tenths;   /* the voltage as far as it has been read */
  uint8_t    low;      /* the supply was low the last time       */
);

PROCESS_INSTANCE(keeper, keeper_proc);

PROCESS_THREAD(keeper, ev, data)
{
  process_keeper_t *self = PROCESS_SELF(keeper);
  (void)ev;
  (void)data;

  PROCESS_BEGIN();

  ipc_pipe_init(&self->in, self->buf, sizeof(self->buf), process_ipc_wake, (void *)pt_process);
  PROCESS_SET_PIPEIN(&self->in);

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ipc_pipe_available(PROCESS_PIPEIN()) > 0);
    ipc_pipe_read(PROCESS_PIPEIN(), &self->byte, 1);

    /* "4.9V": the digits are the tenths, the V ends the number */
    if (self->byte >= '0' && self->byte <= '9')
    {
      self->tenths = (uint8_t)(self->tenths * 10 + (self->byte - '0'));
      continue;
    }
    if (self->byte != 'V')
      continue;

    if (self->tenths < KEEPER_CONF_TENTHS && !self->low)
    {
      /* the supply is low: stop the LED and say so in the log */
      self->low = 1;
      process_pause(&blink_proc.base);
      process_warn(pt_process, EVENT_POWER_OK, self->tenths);
    }
    else if (self->tenths >= KEEPER_CONF_TENTHS && self->low)
    {
      self->low = 0;
      process_resume(&blink_proc.base);
    }
    self->tenths = 0;
  }

  PT_FINALLY(&pt_process->pt)

  PROCESS_END();
}

/* ---------------------------------------------------------------- */

SHELL_MODULES_BEGIN
  SHELL_MODULE(ticker, ticker_proc)
  SHELL_MODULE(blink, blink_proc)
  SHELL_MODULE(vcc, vcc_proc)
  SHELL_MODULE(keeper, keeper_proc)
  SHELL_MODULE(upper, upper_proc)
SHELL_MODULES_END

static bool connected;

void setup(void)
{
  /* the shell keeps the log */
  process_init(&shell_proc.base);

  process_new(&process_type_serial_process0, &serial_process0_proc);
  serial_process0_proc.baud = 115200;

  process_new(&process_type_shell, &shell_proc);
  process_new(&process_type_keeper, &keeper_proc);
  process_new(&process_type_ticker, &ticker_proc);
  process_new(&process_type_blink, &blink_proc);
  process_new(&process_type_vcc, &vcc_proc);
}

void loop(void)
{
  process_run();
  tick_run();

  /* the system as it was designed: connected once the readers are there */
  if (!connected)
    connected = process_connect(&ticker_proc.base, &blink_proc.base)
             && process_connect(&vcc_proc.base, &keeper_proc.base);
}

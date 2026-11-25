// file: examples/8-multicore/60-two-cores/60-two-cores.ino
/*
 * 60-two-cores.ino  –  one scheduler on each core of an RP2040
 * ================================================================
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   The shell of 40-shell on core 0, beside the serial line, and the
 *   modules on core 1: the ticker and the LED, and upper. Each core runs a
 *   scheduler of its own, with the same process_loop(); what one core
 *   posts to a process of the other goes through a mailbox, and a pipe
 *   carries bytes from one core to the other as it does on one.
 *
 *     > ps                      the processes of both cores
 *     > join upper serial:0     upper (core 1) writes to the line (core 0)
 *     > write upper hello       the shell (core 0) writes to upper (core 1)
 *     HELLO
 *     > stop blink              the LED of core 1 stops
 *
 * BUILD
 * -----
 *   PROCESS_CONF_CORES 2 has to reach every file of the library: in
 *   src/protoduino-config.h, or as a build flag. With the Pico SDK alone:
 *     tools/build_rp2.sh rp2040 examples/8-multicore/60-two-cores/60-two-cores.ino -DPROCESS_CONF_CORES=2
 *   PlatformIO: build_flags = -DPROCESS_CONF_CORES=2
 *   setup1() and loop1() run on core 1, as in the arduino-pico core.
 */

#include <protoduino.h>
#include <sys/process.h>
#include <sys/process/serial.h>
#include <sys/tick.h>

#if PROCESS_CONF_CORES < 2
#error "60-two-cores: build with -DPROCESS_CONF_CORES=2 (see the comment at the top)"
#endif

#define PROCESS_CONF_SLEEP PROCESS_SLEEP_WAIT
#include <sys/idle.h>

#include <mod/ticker.h>
#include <mod/blink.h>
#include <mod/upper.h>
#include <mod/shell.h>

PROCESS_INSTANCE(shell, shell_proc);
PROCESS_INSTANCE(ticker, ticker_proc);
PROCESS_INSTANCE(blink, blink_proc);
PROCESS_INSTANCE(upper, upper_proc);

SHELL_MODULES_BEGIN
  SHELL_MODULE(ticker, ticker_proc)
  SHELL_MODULE(blink, blink_proc)
  SHELL_MODULE(upper, upper_proc)
SHELL_MODULES_END

static volatile bool core1_ready;
static bool connected;

/* core 0: the line and the shell */
void setup(void)
{
  process_init(&shell_proc.base);
  process_new(&process_type_serial_process0, &serial_process0_proc);
  serial_process0_proc.baud = 115200;
  process_new(&process_type_shell, &shell_proc);
}

void loop(void)
{
  process_loop();
  /* the LED once both of its processes run, on core 1 */
  if (!connected && core1_ready)
    connected = process_connect(&ticker_proc.base, &blink_proc.base);
}

/* core 1: the modules */
void setup1(void)
{
  process_init(NULL);
  process_new(&process_type_ticker, &ticker_proc);
  process_new(&process_type_blink, &blink_proc);
  process_new(&process_type_upper, &upper_proc);
  idle_start();
  core1_ready = true;
}

void loop1(void)
{
  process_loop();
}

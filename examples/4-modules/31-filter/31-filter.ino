// file: examples/4-modules/31-filter/31-filter.ino

/*
 * 31-filter.ino  –  a filter between the serial line and itself
 * ================================================================
 *
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   The serial port is a process too, a device, and it is connected like
 *   any other:
 *
 *     serial:0 | upper | serial:0
 *
 *   What you type comes back in capitals. Compare with 22-process-serial,
 *   which does the same with a process written into the sketch: here the
 *   process is the module mod/upper.h, and the sketch only connects.
 *
 * WHAT TO LOOK AT
 * ---------------
 *   1. A device publishes what it receives and what it transmits.
 *      process_connect(device, p) makes the received bytes the input of p;
 *      process_connect(p, device) makes the output of p what is sent.
 *   2. upper keeps a byte until there is room for it: nothing is lost when
 *      the line is slower than the typist.
 *
 * TRY IT   (115200 baud)
 * ------
 *   hello  ->  HELLO
 */

#include <protoduino.h>
#include <sys/process.h>
#include <sys/process/serial.h>
#include <sys/tick.h>

#include <mod/upper.h>

PROCESS_INSTANCE(upper, upper_proc);

static bool connected;

void setup(void)
{
  process_init(NULL);

  process_new(&process_type_serial_process0, &serial_process0_proc);
  serial_process0_proc.baud = 115200; /* after process_new(), which clears the instance */
  process_new(&process_type_upper, &upper_proc);
}

void loop(void)
{
  process_run();
  tick_run();

  /* serial:0 | upper | serial:0, once both have started */
  if (!connected)
    connected = process_connect(&upper_proc.base, &serial_process0_proc.base)
             && process_connect(&serial_process0_proc.base, &upper_proc.base);
}

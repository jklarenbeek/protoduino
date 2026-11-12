// file: examples/4-modules/32-sensor/32-sensor.ino

/*
 * 32-sensor.ino  –  a module that measures something
 * ================================================================
 *
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   A battery monitor in a dozen lines of sketch:
 *
 *     vcc | serial:0        the supply voltage of the chip, every 2 s
 *
 *   mod/vcc.h needs no part and no pin: the ADC measures its own 1.1 V
 *   reference against the supply, and the supply follows from the reading.
 *
 * WHAT TO LOOK AT   (in mod/vcc.h)
 * ---------------
 *   1. The conversion takes time, and the module waits for it the way it
 *      waits for anything else. Nothing stands still meanwhile.
 *   2. Its last block, after PT_FINALLY, switches the ADC off: that is what
 *      runs when the process is ended, also in the middle of a conversion.
 *
 * TRY IT   (115200 baud)
 * ------
 *   5.0V
 *   5.0V
 */

#include <protoduino.h>
#include <sys/process.h>
#include <sys/process/serial.h>
#include <sys/tick.h>

#include <mod/vcc.h>

PROCESS_INSTANCE(vcc, vcc_proc);

static bool connected;

void setup(void)
{
  process_init(NULL);

  process_new(&process_type_serial_process0, &serial_process0_proc);
  serial_process0_proc.baud = 115200;
  process_new(&process_type_vcc, &vcc_proc);
}

void loop(void)
{
  process_run();
  tick_run();

  if (!connected)
    connected = process_connect(&vcc_proc.base, &serial_process0_proc.base);
}

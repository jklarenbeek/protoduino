// file: examples/22-process-serial/22-process-serial.ino

/*
 * 22-process-serial.ino  –  serial process driver demo for protoduino
 * =====================================================================
 *
 * Target board:  Arduino Mega 2560  (ATmega2560, 4× hardware UART)
 * Target UART:   UART0 @ 9600 baud  (USB-serial on the Mega)
 *
 * What this sketch demonstrates
 * ------------------------------
 *  1. How to start a serial process device (serial_process0) via
 *     process_new() — no manual UART ISR wiring needed.
 *  2. How a user process connects to the device via
 *     SERIAL_PROCESS0_CONNECT() and uses ipc_pipe_* for I/O.
 *  3. A practical "echo-upper" application: each byte received over
 *     UART0 is converted to uppercase and echoed back.
 *
 * How it works
 * ------------
 *   serial_process0  (DEVICE, prio 1)
 *     INIT: open UART0 at 9600 baud, init rx/tx pipes, wire RX ISR.
 *     POLL: drain tx_pipe → serial0 TX ring buffer → hardware.
 *
 *   echo_upper  (PROCESS, prio 2)
 *     INIT: SERIAL_PROCESS0_CONNECT wires pipein / pipeout, and also
 *           sets rx_pipe.wake_cb so the RX ISR will poll echo_upper
 *           directly whenever a byte arrives.
 *     POLL: drain pipein, uppercase each byte, write pipeout, wake
 *           serial_process0 to flush.
 *
 * Verification with protosim
 * ---------------------------
 *   1. Compile (from protoduino root):
 *        arduino-cli compile --fqbn arduino:avr:mega --library . \
 *            --output-dir build/22-process-serial \
 *            examples/22-process-serial/22-process-serial.ino
 *   2. Simulate:
 *        bin\protosim.exe build\22-process-serial\22-process-serial.ino.elf \
 *            -m atmega2560 -f 16000000 --max-steps 500000000
 *   3. Connect to TCP 4000, type "hello\r\n", expect "HELLO\r\n" back.
 */

#include <protoduino.h>
#include <autostart.h>
#include <sys/process.h>
#include <sys/process/serial.h>

/* =========================================================================
 * echo_upper process
 * ========================================================================= */

PROCESS_DEFINE(echo_upper, "echo-upper", 2,
  uint8_t rx_byte;
);
typedef process_echo_upper_t echo_upper_t;
PROCESS_INSTANCE(echo_upper, echo_upper_proc);

PROCESS_THREAD(echo_upper, ev, data)
{
  echo_upper_t *self = PROCESS_SELF(echo_upper);
  (void)data;
  (void)ev;

  PROCESS_BEGIN();

  /*
   * Wire our pipein/pipeout to the serial_process0 device pipes.
   * SERIAL_PROCESS0_CONNECT also sets rx_pipe.wake_cb = process_ipc_wake
   * so the UART RX ISR polls this process directly whenever a byte lands.
   */
  SERIAL_PROCESS0_CONNECT(pt_process);

  /* Main echo loop – PROCESS_WAIT_EVENT unconditionally yields each time. */
  while (1) {
    PROCESS_WAIT_EVENT();

    /* Drain all bytes from rx_pipe (deposited by the UART RX ISR). */
    while (ipc_pipe_available(PROCESS_PIPEIN()) > 0) {
      if (ipc_pipe_read(PROCESS_PIPEIN(), &self->rx_byte, 1) != 1)
        break;

      uint8_t out = self->rx_byte;
      if (out >= 'a' && out <= 'z')
        out = (uint8_t)(out - 'a' + 'A');

      if (ipc_pipe_space(PROCESS_PIPEOUT()) > 0) {
        ipc_pipe_write(PROCESS_PIPEOUT(), &out, 1);
        /* Wake the serial device so it drains tx_pipe to the UART TX. */
        process_poll(&serial_process0_proc.base);
      }
    }
  }

  PROCESS_END();
}

/* =========================================================================
 * setup / loop
 * ========================================================================= */

void setup(void)
{
  protoduino_start();

  /* Start the UART0 device process first so its pipes are initialised
   * before echo_upper's INIT event fires and calls SERIAL_PROCESS0_CONNECT. */
  process_new(&process_type_serial_process0, &serial_process0_proc);
  process_new(&process_type_echo_upper, &echo_upper_proc);
}

void loop(void)
{
  process_run();
}

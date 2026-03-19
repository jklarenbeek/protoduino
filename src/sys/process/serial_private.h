// file: ./src/sys/process/serial_private.h
//
// Implementation template for serial device processes.
// Include once per .c file after defining:
//
//   CC_TMPL_PREFIX          = serial_processN  (e.g. serial_process0)
//   CC_TMPL2_PREFIX         = serialN           (e.g. serial0)
//   SERIAL_PROCESS_NAME_STR = "serial:N"        (e.g. "serial:0")
//
// Emits per UART:
//  1. PROGMEM name string
//  2. Singleton instance  (serial_processN_proc)
//  3. ISR-safe RX callback
//  4. Thread function body (non-static – matches extern in serial.h)
//  5. PROGMEM type descriptor (process_type_serial_processN)
//
// Do NOT include from any header included repeatedly.

#include "serial.h"

// ---- PROGMEM name string -------------------------------------------------
static const char CC_CONCAT2(CC_TMPL_PREFIX, _name)[]
    CC_PROGMEM = SERIAL_PROCESS_NAME_STR;

// ---- Singleton instance --------------------------------------------------
serial_process_t CC_CONCAT2(CC_TMPL_PREFIX, _proc);

// ---- ISR-safe RX callback -----------------------------------------------
static void CC_CONCAT2(CC_TMPL_PREFIX, _on_rx)(uint_fast8_t ch)
{
  uint8_t b = (uint8_t)ch;
  ipc_pipe_write(&CC_CONCAT2(CC_TMPL_PREFIX, _proc).rx_pipe, &b, 1);
  process_poll(&CC_CONCAT2(CC_TMPL_PREFIX, _proc).base);
}

// ---- Thread function body ------------------------------------------------
ptstate_t CC_CONCAT2(process_thread_, CC_TMPL_PREFIX)(
    struct process *pt_process,
    process_event_t ev,
    process_data_t  data)
{
  serial_process_t *self = (serial_process_t *)pt_process;
  (void)data;

  PROCESS_BEGIN();

  /* Default baud rate if caller did not set it before process_new(). */
  if (self->baud == 0)
    self->baud = PROCESS_SERIAL_DEFAULT_BAUD;

  /* RX pipe – no wake callback; ISR wakes us via process_poll directly. */
  ipc_pipe_init(&self->rx_pipe,
                self->rx_buf, sizeof(self->rx_buf),
                NULL, NULL);

  /* TX pipe – wake callback polls this device whenever user writes data. */
  ipc_pipe_init(&self->tx_pipe,
                self->tx_buf, sizeof(self->tx_buf),
                process_ipc_wake, (void *)pt_process);

  /* Expose pipes via the standard process pipeline fields. */
  PROCESS_SET_PIPEIN(&self->rx_pipe);
  PROCESS_SET_PIPEOUT(&self->tx_pipe);

  /* Install the RX callback and open the UART. */
  CC_TMPL2_FN(on_recieved)(CC_CONCAT2(CC_TMPL_PREFIX, _on_rx));
  CC_TMPL2_FN(open)(self->baud);

  /* ---- main service loop ---- */
  while (1) {
    PROCESS_WAIT_EVENT();

    /* Drain tx_pipe → serial TX ring buffer. */
    {
      uint8_t space = CC_TMPL2_FN(write_available)();
      while (space > 0 && ipc_pipe_available(&self->tx_pipe) > 0) {
        uint8_t b;
        if (ipc_pipe_read(&self->tx_pipe, &b, 1) == 1) {
          if (CC_TMPL2_FN(write8)(b))
            --space;
        }
      }
      CC_TMPL2_FN(flush)();
    }
  }

  PROCESS_END(); /* never reached for DEVICE type, but required by PT */
}

// ---- PROGMEM type descriptor --------------------------------------------
const process_type_t CC_CONCAT2(process_type_, CC_TMPL_PREFIX) CC_PROGMEM = {
  CC_CONCAT2(CC_TMPL_PREFIX, _name),          /* name string (PROGMEM) */
  CC_CONCAT2(process_thread_, CC_TMPL_PREFIX), /* thread entry point    */
  sizeof(serial_process_t),                   /* instance size          */
  1,                                          /* priority               */
  PROCESS_TYPE_DEVICE                         /* singleton              */
};

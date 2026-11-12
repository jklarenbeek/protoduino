// file: docs/elfloader/prototype/probes/process_probe.c
/*
 * Probes behind the scheduler and serial-process findings of the document.
 * A sketch on the unmodified library, built without the Arduino core
 * (kernel/core.c). PROBE selects the experiment; the result is read from
 * the `result` array in RAM and from the serial line.
 *
 *   3  80 bytes are written to serial_process0 in one go, then nothing polls it
 *   4  the baud rate is set before process_new(), as serial.h documents
 *   5  a process ends; nothing is posted to it afterwards
 *   6  one process polls itself on every turn; a second one is started
 *   7  two processes are polled in the same sweep; the first one ends on its poll
 */
#include <protoduino.h>
#include <sys/process.h>
#include <sys/process/serial.h>

volatile uint8_t result[8];

#if PROBE == 3
PROCESS(writer, "writer", 2);
PROCESS_THREAD(writer, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_SET_PIPEOUT(&serial_process0_proc.tx_pipe);
  for (result[0] = 0; result[0] < 80; result[0]++) {
    uint8_t ch = (uint8_t)('a' + result[0] % 26);
    ipc_pipe_write(PROCESS_PIPEOUT(), &ch, 1);
  }
  /* the wake callback of tx_pipe has polled the device once */
  while (1) {
    PROCESS_WAIT_EVENT();
  }
  PROCESS_END();
}
#endif

#if PROBE == 5
PROCESS(shortlived, "short", 2);
PROCESS_THREAD(shortlived, ev, data)
{
  PROCESS_BEGIN();
  result[0]++;            /* ran */
  PT_FINALLY(&pt_process->pt)
  {
    result[1]++;          /* cleaned up */
  }
  PROCESS_END();
}
#endif

#if PROBE == 6
PROCESS(busy, "busy", 2);
PROCESS_THREAD(busy, ev, data)
{
  PROCESS_BEGIN();
  while (1) {
    result[0]++;
    process_poll(pt_process);
    PROCESS_WAIT_EVENT();
  }
  PROCESS_END();
}
PROCESS(second, "second", 2);
PROCESS_THREAD(second, ev, data)
{
  PROCESS_BEGIN();
  result[1] = 1;          /* got its INIT event */
  while (1) {
    PROCESS_WAIT_EVENT();
  }
  PROCESS_END();
}
#endif

#if PROBE == 7
PROCESS(first, "first", 2);
PROCESS_THREAD(first, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
  result[0]++;            /* first got its poll, and ends here */
  PROCESS_END();
}
PROCESS(later, "later", 3);
PROCESS_THREAD(later, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
  result[1]++;            /* later got its poll */
  while (1) {
    PROCESS_WAIT_EVENT();
  }
  PROCESS_END();
}
#endif

void setup(void)
{
  process_init(NULL);
#if PROBE == 3
  process_new(&process_type_serial_process0, &serial_process0_proc);
  serial_process0_proc.baud = UART_BAUD_115200;
  process_new(&process_type_writer, &writer);
#elif PROBE == 4
  serial_process0_proc.baud = UART_BAUD_115200;
  process_new(&process_type_serial_process0, &serial_process0_proc);
#elif PROBE == 5
  process_new(&process_type_shortlived, &shortlived);
#elif PROBE == 6
  process_new(&process_type_busy, &busy);
  process_new(&process_type_second, &second);
#elif PROBE == 7
  process_new(&process_type_first, &first);
  process_new(&process_type_later, &later);
#endif
}

void loop(void)
{
  process_run();
#if PROBE == 5
  result[2] = shortlived.base.state;   /* 0 = NONE (slot free), 4 = EXITING */
#endif
#if PROBE == 7
  /* once both have handled INIT and wait: poll both, one time */
  if (result[7] < 20 && ++result[7] == 20) {
    process_poll(&first.base);
    process_poll(&later.base);
  }
  result[2] = later.base.needs_poll;   /* 1: its poll is still pending */
#endif
}

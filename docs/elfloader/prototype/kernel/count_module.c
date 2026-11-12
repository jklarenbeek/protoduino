// file: docs/elfloader/prototype/kernel/count_module.c
/*
 * A loadable module: one process type, written like any process of a
 * sketch. It keeps its state in its process struct, writes to the serial
 * device process of the kernel and uses the scheduler of the kernel.
 */
#include <protoduino.h>
#include <sys/process.h>
#include <sys/process/serial.h>
#include <sys/module.h>

PROCESS_DEFINE(count, "count", 3,
  uint8_t n;
  char    text[4];
);

PROCESS_THREAD(count, ev, data)
{
  process_count_t *self = PROCESS_SELF(count);
  (void)ev;
  (void)data;

  PROCESS_BEGIN();

  PROCESS_SET_PIPEOUT(&serial_process0_proc.tx_pipe);

  for (self->n = 0; self->n < 3; self->n++) {
    self->text[0] = ' ';
    self->text[1] = 'm';
    self->text[2] = (char)('0' + self->n);
    PROCESS_PIPE_WRITE(self->text, 3);
    process_poll(&serial_process0_proc.base);

    /* take the next turn after everybody else */
    process_post(pt_process, EVENT_INTERVAL, NULL);
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}

MODULE(count);

// file: ./src/mod/upper.h

#ifndef __MOD_UPPER_H__
#define __MOD_UPPER_H__

/* upper: a filter. Passes on what it receives, small letters as capitals.
 *
 *   ticker | upper | serial:0
 *
 * - Owns its input pipe.
 * - A filter waits for its reader: when the output pipe is full it keeps
 *   the byte until there is room. With no output connected it drops it.
 */

#include <sys/process.h>

PROCESS_DEFINE(upper, "upper", 3,
  ipc_pipe_t in;
  uint8_t    buf[8];
  uint8_t    byte;
);

PROCESS_THREAD(upper, ev, data)
{
  process_upper_t *self = PROCESS_SELF(upper);
  (void)ev;
  (void)data;

  PROCESS_BEGIN();

  ipc_pipe_init(&self->in, self->buf, sizeof(self->buf), process_ipc_wake, (void *)pt_process);
  PROCESS_SET_PIPEIN(&self->in);

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ipc_pipe_available(&self->in) > 0);
    ipc_pipe_read(&self->in, &self->byte, 1);

    if (self->byte >= 'a' && self->byte <= 'z')
      self->byte -= 'a' - 'A';

    if (pt_process->has_pipeout)
      PROCESS_PIPE_WRITE(&self->byte, 1);
  }

  PT_FINALLY(&pt_process->pt)

  PROCESS_END();
}

#endif /* __MOD_UPPER_H__ */

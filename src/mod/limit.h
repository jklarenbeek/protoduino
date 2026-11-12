// file: ./src/mod/limit.h

#ifndef __MOD_LIMIT_H__
#define __MOD_LIMIT_H__

/* limit: a filter that turns samples into yes and no. Writes a 1 when the
 * signal has reached `on` and a 0 when it has fallen back to `off`; in
 * between it says nothing, so a signal that hovers does not chatter.
 *
 *   sense | limit | keeper       the supply: 1 while it is well, 0 when low
 *
 * - `limit.on` and `limit.off` are set in the instance. With `on` below
 *   `off` the sense is the other way round: 1 at or below `on`, 0 at or
 *   above `off`, for a signal that falls when the quantity rises.
 * - It writes when its answer changes, and the first time it has one.
 * - The deciding itself is limit_step(), a plain function on a limit_t.
 * - No static variables.
 */

#include <sys/process.h>

typedef struct limit {
  uint8_t on;     /* 1 from here ...                  */
  uint8_t off;    /* ... 0 from here                  */
  uint8_t is;     /* the answer: 1 or 0               */
  uint8_t known;  /* there has been an answer         */
} limit_t;

// true when the sample changes the answer, which is then in s->is
static uint8_t limit_step(limit_t *s, uint8_t x)
{
  uint8_t now = s->known ? s->is : 2;

  if (s->on >= s->off)
  {
    if (x >= s->on)
      now = 1;
    else if (x <= s->off)
      now = 0;
  }
  else
  {
    if (x <= s->on)
      now = 1;
    else if (x >= s->off)
      now = 0;
  }
  if (now == 2 || (s->known && now == s->is))
    return 0;
  s->is    = now;
  s->known = 1;
  return 1;
}

PROCESS_DEFINE(limit, "limit", 3,
  ipc_pipe_t in;
  uint8_t    buf[2];
  limit_t    limit;
  uint8_t    byte;
);

PROCESS_THREAD(limit, ev, data)
{
  process_limit_t *self = PROCESS_SELF(limit);
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
    PROCESS_WAIT_EVENT_UNTIL(ipc_pipe_available(PROCESS_PIPEIN()) > 0);
    ipc_pipe_read(PROCESS_PIPEIN(), &self->byte, 1);

    if (limit_step(&self->limit, self->byte) && pt_process->has_pipeout)
      PROCESS_PIPE_WRITE(&self->limit.is, 1);
  }

  PT_FINALLY(&pt_process->pt)

  PROCESS_END();
}

#endif /* __MOD_LIMIT_H__ */

// file: ./src/mod/smooth.h

#ifndef __MOD_SMOOTH_H__
#define __MOD_SMOOTH_H__

/* smooth: a filter for samples. What leaves is the running average of what
 * arrives: each sample moves the average a part of the way towards itself.
 *
 *   sense | smooth | curve | pwm
 *
 * - `smooth.shift`, set in the instance, says which part: 1 a half, 2 a quarter,
 *   3 an eighth ... up to 8. A larger shift is calmer and slower. With 0
 *   the module takes SMOOTH_CONF_SHIFT, 3 by default.
 * - The first sample is the average to start from.
 * - The filter itself is smooth_step(), a plain function on a smooth_t.
 * - No static variables.
 */

#include <sys/process.h>

#ifndef SMOOTH_CONF_SHIFT
#define SMOOTH_CONF_SHIFT 3
#endif

typedef struct smooth {
  uint16_t average;  /* in 256ths of a sample             */
  uint8_t  shift;    /* how far a sample moves it         */
  uint8_t  primed;   /* the average has a starting value  */
} smooth_t;

static uint8_t smooth_step(smooth_t *s, uint8_t x)
{
  uint8_t shift = s->shift ? s->shift : SMOOTH_CONF_SHIFT;

  if (!s->primed)
  {
    s->primed  = 1;
    s->average = (uint16_t)x << 8;
  }
  else
  {
    s->average = (uint16_t)(s->average - (s->average >> shift) + (((uint16_t)x << 8) >> shift));
  }
  return (uint8_t)(s->average >> 8);
}

PROCESS_DEFINE(smooth, "smooth", 3,
  ipc_pipe_t in;
  uint8_t    buf[2];
  smooth_t   smooth;
  uint8_t    byte;
);

PROCESS_THREAD(smooth, ev, data)
{
  process_smooth_t *self = PROCESS_SELF(smooth);
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

    self->byte = smooth_step(&self->smooth, self->byte);

    if (pt_process->has_pipeout)
      PROCESS_PIPE_WRITE(&self->byte, 1);
  }

  PT_FINALLY(&pt_process->pt)

  PROCESS_END();
}

#endif /* __MOD_SMOOTH_H__ */

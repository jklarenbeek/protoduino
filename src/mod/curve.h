// file: ./src/mod/curve.h

#ifndef __MOD_CURVE_H__
#define __MOD_CURVE_H__

/* curve: a filter that maps samples onto a range, along a straight line
 * with a foot and a ceiling.
 *
 *   sense | smooth | curve | pwm
 *
 *   out
 *   max |            .--------
 *       |          .'
 *   min |        .'
 *     0 |_______|
 *              lo   hi      in
 *
 * - Below `lo` the answer is 0; from `hi` on it is `max`; in between it
 *   rises from `min` to `max`. A fan stands still until it is warm enough,
 *   starts at a speed at which it turns at all, and reaches full speed at
 *   the upper temperature. All four are set in the instance, in `curve`.
 * - The mapping itself is curve_map(), a plain function on a curve_t.
 * - No static variables.
 */

#include <sys/process.h>

typedef struct curve {
  uint8_t lo;    /* input at which the line starts ...  */
  uint8_t hi;    /* ... and at which it ends            */
  uint8_t min;   /* output at its start ...             */
  uint8_t max;   /* ... and at its end                  */
} curve_t;

static uint8_t curve_map(const curve_t *s, uint8_t x)
{
  if (x < s->lo)
    return 0;
  if (x >= s->hi || s->max <= s->min)
    return s->max;
  return (uint8_t)(s->min + (uint16_t)(x - s->lo) * (uint8_t)(s->max - s->min) / (uint8_t)(s->hi - s->lo));
}

PROCESS_DEFINE(curve, "curve", 3,
  ipc_pipe_t in;
  uint8_t    buf[2];
  curve_t    curve;
  uint8_t    byte;
);

PROCESS_THREAD(curve, ev, data)
{
  process_curve_t *self = PROCESS_SELF(curve);
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

    self->byte = curve_map(&self->curve, self->byte);

    if (pt_process->has_pipeout)
      PROCESS_PIPE_WRITE(&self->byte, 1);
  }

  PT_FINALLY(&pt_process->pt)

  PROCESS_END();
}

#endif /* __MOD_CURVE_H__ */

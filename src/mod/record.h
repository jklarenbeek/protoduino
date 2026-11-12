// file: ./src/mod/record.h

#ifndef __MOD_RECORD_H__
#define __MOD_RECORD_H__

/* record: a sink that remembers. Keeps the last samples in EEPROM, and the
 * lowest and the highest it ever saw, so they are there after years
 * without power.
 *
 *   sense | record
 *
 * - `record.at` and `record.size`, set in the instance, are the place in EEPROM: two
 *   bytes for the lowest and the highest, then `size` bytes that are used
 *   in a circle. "ring <at> <size> <unit>" in tools/port.mjs shows them.
 * - The newest sample is followed by a mark, 0xFF, and that is how the
 *   place to go on is found after a reset: nothing else is written per
 *   sample. Every cell is therefore written twice per round, and a round of
 *   24 samples at one per ten minutes wears a cell out in about 400 years.
 * - A sample of 255 is kept as 254: 255 is the mark, and what erased
 *   EEPROM holds.
 * - record_open() and record_put() are plain functions on a record_t.
 * - No static variables.
 */

#include <sys/process.h>
#include <avr/eeprom.h>

#define RECORD_MARK 0xFF

typedef struct record {
  uint16_t at;    /* where in EEPROM             */
  uint8_t  size;  /* samples in the circle       */
  uint8_t  pos;   /* where the next one goes     */
} record_t;

// find the mark: the place to go on
static void record_open(record_t *s)
{
  for (s->pos = 0; s->pos < s->size; s->pos++)
  {
    if (eeprom_read_byte((const uint8_t *)(s->at + 2 + s->pos)) == RECORD_MARK)
      return;
  }
  s->pos = 0;
}

static void record_put(record_t *s, uint8_t x)
{
  uint8_t *cell = (uint8_t *)s->at;
  uint8_t was;

  if (x == RECORD_MARK)
    x = RECORD_MARK - 1;

  eeprom_update_byte(cell + 2 + s->pos, x);
  if (++s->pos >= s->size)
    s->pos = 0;
  eeprom_update_byte(cell + 2 + s->pos, RECORD_MARK);

  // erased EEPROM is higher than any sample, and is not a highest one
  if (x < eeprom_read_byte(cell))
    eeprom_update_byte(cell, x);
  was = eeprom_read_byte(cell + 1);
  if (was == RECORD_MARK || x > was)
    eeprom_update_byte(cell + 1, x);
}

PROCESS_DEFINE(record, "record", 3,
  ipc_pipe_t in;
  uint8_t    buf[2];
  record_t   record;
  uint8_t    byte;
);

PROCESS_THREAD(record, ev, data)
{
  process_record_t *self = PROCESS_SELF(record);
  (void)ev;
  (void)data;

  PROCESS_BEGIN();

  if (!pt_process->has_pipein)
  {
    ipc_pipe_init(&self->in, self->buf, sizeof(self->buf), process_ipc_wake, (void *)pt_process);
    PROCESS_SET_PIPEIN(&self->in);
  }
  record_open(&self->record);

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ipc_pipe_available(PROCESS_PIPEIN()) > 0);
    ipc_pipe_read(PROCESS_PIPEIN(), &self->byte, 1);
    record_put(&self->record, self->byte);
  }

  PT_FINALLY(&pt_process->pt)

  PROCESS_END();
}

#endif /* __MOD_RECORD_H__ */

// file: ./src/sys/tick.h

#ifndef __TICK_H__
#define __TICK_H__

/* The tick: one event for every process, at a steady rate.
 *
 * A process only runs when an event or a poll reaches it. A process that
 * waits for a timer, or for room in a pipe, therefore needs somebody to
 * give it a turn now and then. The tick is that somebody: tick_run() posts
 * EVENT_TICK to all processes once per period, and each of them looks at
 * whatever it is waiting for.
 *
 *   void loop(void)
 *   {
 *     process_run();
 *     tick_run();
 *   }
 *
 *   PROCESS_WAIT_EVENT_UNTIL(timer_expired(&self->period));
 *
 * - One event in the queue, whatever the number of processes.
 * - The tick carries no data. A process that posts an event to itself to
 *   get its next turn has to tell that event from the tick, by its number
 *   or by its data: one that answers every tick with another event of its
 *   own fills the queue.
 * - A paused process does not get it.
 * - The period bounds how late a timer is noticed and how fast a writer
 *   gets through a full pipe: TICK_CONF_MILLIS, 16 ms by default.
 */

#include <protoduino.h>
#include <sys/process.h>
#include <sys/timer.h>

#ifndef TICK_CONF_MILLIS
#define TICK_CONF_MILLIS 16
#endif

#define EVENT_TICK EVENT_INTERVAL

static CC_ALWAYS_INLINE void tick_run(void)
{
  static clock_time_t next;

  if ((int32_t)(clock_time() - next) >= 0)
  {
    next = clock_time() + clock_from_millis(TICK_CONF_MILLIS);
    process_post(NULL, EVENT_TICK, NULL);
  }
}

#endif /* __TICK_H__ */

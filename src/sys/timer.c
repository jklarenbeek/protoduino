#include "timer.h"

CC_INLINE void timer_set(struct timer *t, const clock_time_t interval)
{
  t->interval = interval;
  t->start = clock_time();
}

CC_INLINE clock_time_t timer_diff(const struct timer *t) {
  register clock_time_t d = clock_time();
  register clock_time_t s = t->start;
  if (d > s) { 
    return d - s; 
  }
  else {
    return (0xFFFFFFFFL - s) + d; 
  }
}

CC_INLINE uint32_t timer_expired(const struct timer *t) {
  register uint32_t d = (uint32_t)timer_diff(t);
  if (t->interval == 0) {
#if defined ARDUSIM
    return std::min(1,(int)d);
#else
    return min(1, d);
#endif
  }
  if (d > t->interval)
    return d;
  else
    return 0L;
}

CC_INLINE void timer_reset(struct timer *t)
{
  if(timer_expired(t)) {
    t->start += t->interval;
  }
}

CC_INLINE void timer_restart(struct timer *t)
{
  t->start = clock_time();
}

CC_INLINE clock_time_t timer_remaining(struct timer *t)
{
  return (t->start + t->interval) - clock_time();
}

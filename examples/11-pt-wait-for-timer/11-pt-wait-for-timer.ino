
#include <protoduino.h>
#include <sys/pt-timer.h>
#include <dbg/print.h>

struct timer_pt
{
  lc_t lc;
  timer timer1;
};

static ptstate_t protothread1(struct timer_pt *pt)
{
  /* A protothread function must begin with PT_BEGIN() which takes a
     pointer to a struct pt. */
  PT_BEGIN(pt);

  print_line_P(PSTR("Protothread 1: Setting timer1 for 3 seconds and awaits."));

  timer_set(&pt->timer1, clock_from_seconds(3));

  PT_WAIT_UNTIL(pt, timer_expired(&pt->timer1));

  print_line_P(PSTR("Protothread 1: Timer1 expired"));

  /* All protothread functions must end with PT_END() which takes a
     pointer to a struct pt. */
  PT_END(pt);

}

static ptstate_t protothread2(struct pt *pt)
{
  /* A protothread function must begin with PT_BEGIN() which takes a
     pointer to a struct pt. */
  PT_BEGIN(pt);

  print_line_P(PSTR("Protothread 2: Setting timer2 for 5000 milliseconds and awaits."));

  /* Be weary of the static variable being used by PT_WAIT_DELAY().
     This protothread should not be used in a multiple pt instances. */
  PT_WAIT_DELAY(pt, 5000);

  print_line_P(PSTR("Protothread 2: Timer2 expired"));

  /* All protothread functions must end with PT_END() which takes a
     pointer to a struct pt. */
  PT_END(pt);

}

static struct timer_pt pt1;
static struct pt pt2;

void setup()
{
  print_setup();
}

void loop()
{
  print_line_P(PSTR("void loop(): "));

  /* Initialize the protothread state variables each loop. */
  PT_INIT(&pt1);
  PT_INIT(&pt2);

  print_line_P(PSTR("void loop(): using timer_expired()"));

  while(PT_ISRUNNING(protothread1(&pt1)))
    print_count++;

  print_line_P(PSTR("void loop(): using PT_WAIT_DELAY()"));

  while(PT_ISRUNNING(protothread2(&pt2)))
    print_count++;

  delay(1000);
}

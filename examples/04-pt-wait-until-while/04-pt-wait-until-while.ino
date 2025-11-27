/**
 * This example demonstrates the use of the PT_WAIT_UNTIL() and
 * PT_WAIT_WHILE() macros.
 * Since each protothread contains a forever loop, this example
 * runs forever.
 */

#include <protoduino.h>
#include <dbg/examples.h>

/**
 * The first protothread function. A protothread function must always
 * return an integer, but must never explicitly return - returning is
 * performed inside the protothread statements.
 *
 * The protothread function is driven by the main loop further down in
 * the code.
 */
static ptstate_t protothread1(struct pt *pt)
{
  /* A protothread function must begin with PT_BEGIN() which takes a
     pointer to a struct pt. */
  PT_BEGIN(pt);

  /* We loop forever here. */
  forever: while(1) {

    print_count++;
    print_line("Protothread 1 running; wait one");

    PT_WAIT_ONE(pt);
  }

  /* All protothread functions must end with PT_END() which takes a
     pointer to a struct pt. */
  PT_END(pt);
}

/**
 * The second protothread function. This is almost the same as the
 * first one.
 */
static ptstate_t protothread2(struct pt *pt)
{
  PT_BEGIN(pt);

  forever: while(1)
  {
    print_count++;
    print_line("Protothread 2 running; wait until (count % 2) == 0");

    PT_WAIT_UNTIL(pt, ((print_count % 2) == 0));
  }

  PT_END(pt);
}

/**
 * The thirth protothread function. This is almost the same as the
 * first one and introduced to make the example a little more clear.
 */
static ptstate_t protothread3(struct pt *pt)
{
  PT_BEGIN(pt);

  forever: while(1)
  {
    print_count++;
    print_line("Protothread 3 running; wait while (count % 3) != 0");

    PT_WAIT_WHILE(pt, ((print_count % 3) != 0));
  }

  PT_END(pt);
}


/**
 * Finally, we have the main loop. Here is where the protothreads are
 * initialized and scheduled. First, however, we define the
 * protothread state variables pt1 and pt2, which hold the state of
 * the two protothreads.
 */
static struct pt pt1, pt2, pt3;

void setup()
{
  print_setup();

  /* Initialize the protothread state variables with PT_INIT(). */
  PT_INIT(&pt1);
  PT_INIT(&pt2);
  PT_INIT(&pt3);
}

void loop()
{
  print_line("void loop()");

  print_state(F("protothread1"), protothread1(&pt1));
  print_state(F("protothread2"), protothread2(&pt2));
  print_state(F("protothread3"), protothread3(&pt3));

  delay(2000);
}
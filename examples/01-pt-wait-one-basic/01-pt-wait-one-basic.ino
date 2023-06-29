/**
 * This example demonstrates a simple use of protothreads
 * Each protothread will run until a protothread is blocked
 * by the PT_WAIT_ONE() macro. Then the next protothread
 * will run until that thread hits a PT_WAIT_ONE(), etc, etc
 * When all three protothread have run one cylce, the
 * arduino runs the void loop() fu®nction again and runs 
 * each protothread until it hits PT_WAIT_ONE(). Since each
 * protothread contains a while(1)/forever loop, this will
 * go on forever.
 */
#include <protoduino.h>
#include <sys/pt.h>
#include <dbg/examples.h>

/**
 * The first protothread function. A protothread function must always
 * return an integer, but must never explicitly return - returning is
 * performed inside the protothread statements.
 *
 * The protothread function is driven by the main loop further down in
 * the code.
 *
 * Since a loop forever is used within each protothread, it behaves the
 * same as protothread 1.8 by Adam Dunkels.
 *
 */
static ptstate_t protothread1(struct pt *pt)
{
  /* A protothread function must begin with PT_BEGIN() which takes a
     pointer to a struct pt. */
  PT_BEGIN(pt);

  /* We loop forever here. */
  forever: while(1)
  {
    print_count++;
    print_line("Protothread 1 running");

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

  forever: while(1) // and here too
  {
    print_count++;
    print_line("Protothread 2 running");

    PT_WAIT_ONE(pt);

    print_count++;
    print_line("Protothread 2 continue 1");

    PT_WAIT_ONE(pt);
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
    print_line("Protothread 3 running");

    PT_WAIT_ONE(pt);

    print_count++;
    print_line("Protothread 3 continue 1");

    PT_WAIT_ONE(pt);

    print_count++;
    print_line("Protothread 3 continue 2");

    PT_WAIT_ONE(pt);

    /* And we loop. */
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

  print_state(protothread1(&pt1), "void loop():protothread1");
  print_state(protothread2(&pt2), "void loop():protothread2");
  print_state(protothread3(&pt3), "void loop():protothread3");
  
  delay(2000);
}
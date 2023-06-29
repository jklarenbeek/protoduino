/**
 * This example demonstrates the use of the PT_FOREACH() and 
 * PT_ENDEACH() macro. The PT_FOREACH() macro works like the
 * PT_SPAWN() macro and accepts the same arguments. The
 * PT_FOREACH() differs when it recieves a PT_YIELDED return
 * state from its child protothread. It will then flow into
 * the next code block like the well known for(;;) and
 * while() structures. The example also demonstrates that it 
 * is save to nest PT_FOREACH() structures within a single
 * protothread.
 *
 * Note that like the PT_SPAWN() macro, the PT_FOREACH() macro
 * also reinitializes the child protothread.
 * 
 */

#include <protoduino.h>
#include <sys/pt.h>
#include <dbg/examples.h>

/**
 * Although an iterator doesn't need to return a value, this example
 * does, by adding a field to the protothread state structure.
 * we can then read that value in the calling thread.
 * For clarity we added an extra node field that serves as the
 * thread id.
 */
struct ptyield {
  lc_t lc;
  uint8_t node;
  uint8_t value;
};

static void print_iterator(struct ptyield *it)
{
    Serial.print(print_count);
    Serial.print(" - iterator ");
    Serial.print(it->node);
    Serial.print(" (");
    Serial.print(it->lc);
    Serial.print(") ");
    Serial.print(" yielded value: ");
    Serial.println((it->value));
}

/**
 * The first protothread iterator yields a range from 0 to max.
 *
 * The protothread iterator is driven by the main_driver
 * protothread further down in the code.
 */
static ptstate_t iterator1(struct ptyield *self, uint8_t max)
{  
  /* A protothread function must begin with PT_BEGIN() which takes a
     pointer to a struct pt. */
  PT_BEGIN(self);

  /* We loop for 'max' times. */
  for(self->value = 0; self->value < max; ++self->value) {

    /**
     * Notify the caller thread that a value has
     * been yielded.
     */
    PT_YIELD(self);

    /**
     * The PT_FOREACH caller macro is non blocking
     * on the yielded protothread. Blocking is the 
     * responsibility of the called or calling protothread.
     * In this case the responsibility of the iterator1
     * protothread. Try commenting the next line out to 
     * see the counter increment less frequently.
     */ 
    PT_WAIT_ONE(self);
  }

  /* All protothread functions must end with PT_END() which takes a
     pointer to a struct pt. */
  PT_END(self);
}

/**
 * The second protothread iterator yields a random value
 * between 0 and 255. If the random value is greater
 * then the max parameter it exits out of the protothread.
 *
 * The protothread iterator is driven by the main_driver
 * protothread further down in the code.
 */
static ptstate_t iterator2(struct ptyield *self, uint8_t max)
{  
  /* A protothread function must begin with PT_BEGIN() which takes a
     pointer to a struct pt. */
  PT_BEGIN(self);

  /* We loop forever here. */
  forever: while(1) {

    /**
     * As previously mentioned is the calling PT_FOREACH macro non-
     * blocking on a yield. We therefor purposely PT_WAIT_ONE(pt);
     * in order for the scheduler in the void loop() function
     * to increment the counter.
     */
    PT_WAIT_ONE(self);

    /**
     * set the value that we are going to yield to an arbitrary number
     */
    self->value = random(0, 255);

    /**
     * Notify the caller thread that a value has
     * been yielded.
     */
    PT_YIELD(self);

    /**
     * Exit out the protothread iterator when a threshold has
     * been reached
     */
    if (self->value >= max)
      PT_EXIT(self);
    
    /* And we loop. */
  }

  /* All protothread functions must end with PT_END() which takes a
     pointer to a struct pt. */
  PT_END(self);
}

/**
 * This is the main protothread that drives the iterators
 * We define two state variables; one for each iterator.
 * When a iterator yields, the print_iterator will be
 * called. When a iterator waits, the control is given back
 * to the caller of the main_driver protothread; in this
 * case the void loop() function
 */
static ptstate_t main_driver(struct pt *pt)
{
  static struct ptyield it1, it2;

  PT_BEGIN(pt);

  print_line("PT_THREAD(main-driver)");

  it1.node = 1;
  it2.node = 2;
  
  PT_FOREACH(pt, &it1, iterator1(&it1, 16))
  {
    print_iterator(&it1);

    PT_FOREACH(pt, &it2, iterator2(&it2, 92))
      print_iterator(&it2);
    PT_ENDEACH(pt);
  }
  PT_ENDEACH(pt);

  PT_END(pt);
}

/**
 * Finally, we have the main loop. Here is where main protothread is
 * initialized and scheduled. First, however, we define the
 * protothread state variable pt1, which hold the state of
 * the main driver protothread.
 */
static struct pt pt1;

void setup()
{
  print_setup();
}

void loop()
{
  print_line("void loop()");

  /* Initialize the protothread state variables. */
  PT_INIT(&pt1);

  /**
   * Call the main driver protothread until it exits,
   * ends or throws an error
   */
  while(PT_ISRUNNING(main_driver(&pt1)))
  {
    print_count++;
    delay(2000);
  }

  print_count++;
  delay(2000);

}
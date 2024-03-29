/**
 * This example demonstrates two instances of a protothread which
 * randomly yields a number. If a threshold is reached (the random
 * number is greater then 191) the instance will exit. The proto
 * pump driving the protothread instances will remain active until
 * both instances have exited or ended.
 * 
 * This shows a clear departure from the protothread 1.x library in
 * that the protothread instance will not reset its state on exit.
 * Instead it will remain in that state until we manually reset
 * each protothread instance at the beginning of the void loop()
 * function.
 */

#include <protoduino.h>
#include <dbg/examples.h>

/**
 * the protothread state structure is extended to hold
 * - the protothread state variable
 * - the node/instance number
 * - a counter for the protothread for loop
 * - a value that is to be yielded when set
 */
struct yield_pt {
  lc_t lc;
  uint8_t node;
  uint8_t idx;
  uint8_t value;
};

static void print_waitone(const struct yield_pt *p)
{
  SerialLine.print(print_count);
  SerialLine.print(" - instance ");
  SerialLine.print(p->node);
  SerialLine.print(" wait one @");
  SerialLine.println(p->idx);
}

static void print_yield(const struct yield_pt *p)
{
    SerialLine.print(print_count);
    SerialLine.print(" - instance ");
    SerialLine.print(p->node);
    SerialLine.print(" yield random(0, 255) = ");
    SerialLine.println(p->value);

}

static void print_thread(const ptstate_t s, const struct yield_pt *p)
{
  print_state_ex(s, p->lc);
  SerialLine.print(p->node);
  SerialLine.print(" value: ");
  SerialLine.println(p->value);
}

/**
 * This protothread is ment to work with multiple instances.
 * Each instance hold its own variable structure. This in contrast
 * to an singleton protothread which uses static variables inside
 * the protothread to hold its variable structure.
 */
static ptstate_t protothread(struct yield_pt *self)
{  
  PT_BEGIN(self);

  // we loop 5 times, in order to reach a PT_ENDED state.
  // if we reach the threshold we set the state to PT_EXITED.
  for(self->idx = 0; self->idx < 5; self->idx++) 
  {
    // increase to global counter so we can see some interesting behaviour.
    print_count++;

    print_waitone(self);

    // block the protothread one cycle.
    PT_WAIT_ONE(self);

    // set the return value on yield.
    self->value = random(0, 255);

    print_yield(self);

    // yield the random value
    PT_YIELD(self);

    // we reached the treshold so exit.
    if (self->value > 191)
      PT_EXIT(self);
    
  }

  PT_END(self);
}


/**
 * Finally, we have the main loop. Here is where the protothreads are
 * initialized and scheduled. First, however, we define the
 * protothread state variables pt1 and pt2, which hold the state of
 * the two protothread instances.
 */
static struct yield_pt pt1, pt2;
static ptstate_t st1, st2;

void setup()
{
  print_setup();
}

/**
 * Here we initialize the state and variable structure of each
 * protothread instance.
 */
void init_pt(struct yield_pt * p, uint8_t node)
{
  PT_INIT(p);
  p->node = node;
  p->idx = 0;
  p->value = 0;
}

/**
 * This expression acts like a simple protothread pump. It
 * will call the protothread for one cycle and checks if
 * the protothread is still running; ie PT_WAITING or
 * PT_YIELDED.
 * When a protothread has exited or ended, the protothread
 * stays block in its state; as is the designed behaviour
 * of the protothread 2.x library. Therefor we can savely
 * cycle through the protothread instances without changing 
 * a exited or ended state.
 */
bool run_once() 
{
  print_line(" - protopump is_running()");

  bool running = false;
  if (PT_ISRUNNING(st1 = protothread(&pt1)))
    running = true;
  if (PT_ISRUNNING(st2 = protothread(&pt2)))
    running = true;

  if (running == false)
  {
    print_line(" - end of protopump");
  }
  
  return running;
}

void loop()
{
  print_line("void loop()");

  /* Initialize the protothread state variables. */
  init_pt(&pt1,1);
  init_pt(&pt2,2);

  // start the protopump and run until both protothread
  // instances have either exited, ended, or threw an error.
  while(run_once()) {

    print_thread(st1, &pt1);
    print_thread(st2, &pt2);

    delay(2000);

  }

  print_thread(st1, &pt1);
  print_thread(st2, &pt2);

  delay(2000);

}
/***
 * 
*/

#include <protoduino.h>
#include <sys/examples.h>

static ptstate_t protothread1(struct pt *self)
{
  PT_BEGIN(self);

  print_line("PT_BEGIN() protothread1");

  forever: while(1)
  {
    print_line("PT_WAIT_ONE() protothread1");

    PT_WAIT_ONE(self);

    uint8_t v = random(0, 6);
    if (v == 2)
      PT_EXIT(self);
    else if (v == 3)
      PT_RAISE(self, PT_ERROR + v);
    else if (v == 4)
      break;
  }

  print_line("PT_ENDED protothread1");

  PT_CATCHANY(self)
  {
    print_error("PT_CATCHANY() protothread1", PT_ERROR_STATE);
    PT_RETHROW(self);
  }

  PT_FINALLY(self)
  
  print_line("PT_FINALLY() protothread1");

  PT_END(self);
}

void setup()
{
  print_setup();
}

void test1_run()
{
  print_line("void test1_run() START");

  static struct pt pt1;

  static ptstate_t state;

  // set the protothread control structure to the beginning of the protothread
  PT_INIT(&pt1);
  while(PT_ISRUNNING(state = protothread1(&pt1)))
  {
    print_state(state, "void test1_run()");
    delay(1000);
  }
  print_state(state, "void test1_run()");
  delay(1000);

  // set the protothread control structure to the finally control block
  PT_FINAL(&pt1);
  while(PT_ISRUNNING(state = protothread1(&pt1)))
  {
    print_state(state, "void test1_run()");
    delay(1000);  
  }
  print_state(state, "void test1_run()");
  delay(1000);

}

void loop()
{
  test1_run();
  print_count++;

  delay(2000);
}
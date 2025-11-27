/**
 * This examples aims to demonstrate how exception handling works within the
 * protothread2 library.
 *
 * by: https://github.com/jklarenbeek
 */
#include <protoduino.h>
#include <dbg/examples.h>

/**
 * This example illustrates the control flow within a protothread,
 * that contains a PT_CATCHANY clause.
 * Since no error is raised the PT_CATCHANY clause is never called.
 *
 */

static ptstate_t protothread1(struct pt * self)
{
  PT_BEGIN(self);

  print_line("PT_BEGIN() protothread1");

  PT_WAIT_ONE(self);

  print_line("PT_ENDED protothread1");

  PT_CATCHANY(self)
  {
    print_line("PT_CATCHANY() !!UNREACHABLE!! protothread1");
  }

  print_line("PT_END() !!UNREACHABLE!! protothread1");

  PT_END(self);
}

static ptstate_t protothread2(struct pt * self)
{
  PT_BEGIN(self);

  print_line("PT_BEGIN() protothread2");

  PT_WAIT_ONE(self);

  PT_ERROR_STATE = (ptstate_t)(PT_ERROR + random(0, 250));
  print_error("PT_RAISE() protothread2", PT_ERROR_STATE);

  PT_RAISE(self, PT_ERROR_STATE);

  print_line("PT_ENDED !!UNREACHABLE!! protothread2");

  PT_CATCHANY(self)
  {
    print_error("PT_CATCHANY() protothread2", PT_ERROR_STATE);
  }

  // overflows from PT_CATCHANY
  print_line("PT_END() protothread2");

  PT_END(self);
}

static ptstate_t protothread3(struct pt * self)
{
  static uint8_t e;
  PT_BEGIN(self);
  {
    print_line("PT_BEGIN() protothread3");

    e = random(0, 5);
    if (e > 0)
    {
      e = PT_ERROR + e;
      print_error("PT_RAISE() protothread3", e);
      PT_RAISE(self, e);
    }

    print_error("PT_ENDED protothread3", e);
  }
  PT_CATCH(self, PT_ERROR + 1)
  {
    print_error("PT_CATCH(PT_ERROR + 1) protothread3", PT_ERROR_STATE);
  }
  PT_CATCH(self, PT_ERROR + 2)
  {
    print_error("PT_CATCH(PT_ERROR + 2) AND THROW protothread3", PT_ERROR_STATE);
    PT_THROW(self, PT_ERROR + 2);
  }
  PT_CATCH(self, PT_ERROR + 3)
  {
    print_error("PT_CATCH(PT_ERROR + 3) AND THROW protothread3", PT_ERROR_STATE);
    PT_THROW(self, PT_ERROR_STATE);
  }
  PT_CATCHANY(self)
  {
    print_error("PT_CATCHANY() AND RETHROW protothread3", PT_ERROR_STATE);
    PT_RETHROW(self);
  }

  print_line("PT_END() !!UNREACHABLE CODE!! protothread3");

  PT_END(self);
}

/**
 * This iterator is used by the 4th, 5th and 6th example.
 * It just randomly waits, yields, or throws a direct
 * error without raising it to keep it simple.
 */
static ptstate_t iterator1(struct pt * self)
{
  static uint8_t v;
  PT_BEGIN(self);

  print_line("PT_BEGIN() iterator1");

  forever: while(1)
  {
    v = random(0,8);
    if (v < 2)
    {
      print_line("PT_WAIT_ONE() iterator1");
      PT_WAIT_ONE(self);
    }
    else if (v < 4)
    {
      print_line("PT_YIELD() iterator1");
      PT_YIELD(self);
    }
    else if (v < 6)
    {
      print_line("PT_EXIT() iterator1");
      PT_EXIT(self);
    }
    else
    {
      v = random(0,4);
      print_error("PT_THROW() iterator1", PT_ERROR + v);
      PT_THROW(self, PT_ERROR + v);
    }
  }

  print_line("PT_END() iterator1");

  PT_END(self);
}

/***
 * An example how to spawn a protothread and raise an exception if one is thrown by it.
 *
*/
static ptstate_t protothread4(struct pt * self)
{
  static struct pt it1;

  PT_BEGIN(self);

  print_line("PT_BEGIN() protothread4");

  PT_INIT(&it1);
  PT_WAIT_THREAD(self, iterator1(&it1));
  PT_ONERROR(PT_ERROR_STATE)
  	PT_RAISE(self, PT_ERROR_STATE);

  print_line("PT_ENDED protothread4");

  PT_CATCHANY(self)
  {
    print_error("PT_CATCHANY() protothread4", PT_ERROR_STATE);
  }

  // overflows from PT_CATCHANY
  print_line("PT_END() protothread4");

  PT_END(self);
}

/***
 * A shorter example of how to spawn a protothread and raise an exception if one is thrown by it.
 *
*/
static ptstate_t protothread5(struct pt * self)
{
  PT_BEGIN(self);

  print_line("PT_BEGIN() protothread5");

  do {
    static struct pt it1;
    PT_SPAWN(self, &it1, iterator1(&it1));
  } while(0);

  print_line("PT_ENDED protothread5");

  PT_CATCHANY(self)
  {
    print_error("PT_CATCHANY() protothread5", PT_ERROR_STATE);
  }

  // overflows from PT_CATCHANY
  print_line("PT_END() protothread5");

  PT_END(self);
}

static ptstate_t protothread6(struct pt * self)
{
  static struct pt it1;

  PT_BEGIN(self);

  print_line("PT_BEGIN() protothread6");

  PT_FOREACH(self, &it1, iterator1(&it1))
  {
    print_line("PT_FOREACH() PT_YIELDED protothread6");
  }
  PT_ENDEACH(self);

  print_line("PT_ENDED protothread6");

  PT_CATCHANY(self)
  {
    print_error("PT_CATCHANY() protothread6", PT_ERROR_STATE);
  }

  // overflows from PT_CATCHANY
  print_line("PT_END() protothread6");

  PT_END(self);
}

void setup()
{
  print_setup();
}

#define mydelay 1000
void test_run(struct pt * p, ptstate_t (*protothread)(struct pt * pt))
{
  static ptstate_t state;
  print_line("void test_run():START");
  delay(mydelay);

  PT_INIT(p);
  while(PT_ISRUNNING(state = protothread(p)))
  {
    print_state(state, "void test_run():LOOP");
    print_count++;
    delay(mydelay);
  }
  print_state(state, "void test_run():DONE");
  print_count++;
  delay(mydelay);
}

void loop()
{
  // since test_run() executes as a normal function and drives the different tests
  // we only need to allocate one protothread variable for each test.
  static struct pt pt1;

  print_line("void loop()");

  for(int i = 0; i < 8; i++)
  {
    // comment any of these lines out to omit running the test
    // all tests should run!
    test_run(&pt1, protothread1);
    test_run(&pt1, protothread2);
    test_run(&pt1, protothread3);
    //test_run(&pt1, iterator1);
    test_run(&pt1, protothread4);
    test_run(&pt1, protothread5);
    test_run(&pt1, protothread6);
  }

  delay(1000);

}

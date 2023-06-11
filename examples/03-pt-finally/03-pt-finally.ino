#include <protoduino.h>
#include <sys/debug-print.h>

static ptstate_t iterator(struct pt * self)
{
  static uint8_t v;

  PT_BEGIN(self);
  forever: while(1)
  {
    v = random(0,8);
    if (v == 0)
      PT_WAIT_ONE(self); // returns PT_WAITING
    else if (v == 1)
      PT_YIELD(self); // returns PT_YIELDED
    else if (v == 2)
      PT_RESTART(self); // returns PT_WAITING
    else if (v == 3)
      PT_EXIT(self); // returns PT_EXITED
    else if (v == 4)
      PT_RAISE(self, PT_ERROR + v); // returns PT_WAITING
    else if (v == 5)
      break;
    else
      PT_THROW(self, PT_ERROR + v);
  }
  PT_END(self);
}

static struct pt it1;

void setup()
{
  print_setup();
  PT_INIT(&it1);
}

void loop()
{
  ptstate_t state = iterator(&it1);
  print_state(state, it1.lc, "void loop()");
  print_count++;
  delay(1000);
}

#include <protoduino.h>
#include <sys/debug-print.h>

static ptstate_t protothread(struct pt *self)
{
  //static uint8_t v;
  PT_BEGIN(self);

  PT_WAIT_ONE(self); // returns PT_WAITING

  PT_RAISE(self, (PT_ERROR + random(0, 63)));

  print_line("UNREACHABLE protothread");

  PT_CATCHANY(self)
  {
    print_error("PT_CATCHANY() protothread", PT_ERROR_STATE);
  }
  PT_END(self); // returns PT_FINALIZED
}

static struct pt pt1;

void setup()
{
  print_setup();
  PT_INIT(&pt1);
}

void loop()
{
  print_count++;
  delay(1000);

  ptstate_t state = protothread(&pt1);
  print_state(state, pt1.lc, "< protothread");
  if (state == PT_FINALIZED || PT_ISRUNNING(state))
    return;
  
  PT_FINAL(&pt1);
}
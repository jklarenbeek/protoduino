/***
 *
*/

#include <protoduino.h>
#include <sys/errors.h>
#include <dbg/examples.h>

typedef ptstate_t (*protothread_t)(struct pt *self);


static ptstate_t protothread1(struct pt *self)
{
  PT_BEGIN(self);

  print_line(F("protothread1: main workflow without errors"));

  PT_WAIT_ONE(self);

  print_line(F("protothread1: PT_ENDED"));

  PT_CATCHANY(self)
  {
    print_error(F("protothread1: PT_CATCHANY()"), PT_ERROR_STATE);
  }

  PT_FINALLY(self)

  print_line(F("protothread1: PT_FINALLY()"));

  PT_END(self);
}

static ptstate_t protothread2(struct pt *self)
{
  PT_BEGIN(self);

  print_line(F("protothread2: PT_RAISE in main workflow"));

  PT_WAIT_ONE(self);

  print_line(F("protothread2: PT_RAISE(ERR_SYS_UNKNOWN)"));

  PT_RAISE(self, ERR_SYS_UNKNOWN);

  print_line(F("protothread2: PT_ENDED"));

  PT_CATCHANY(self)
  {
    print_error(F("protothread2: PT_CATCHANY()"), PT_ERROR_STATE);
  }

  PT_FINALLY(self)

  print_line(F("protothread2: PT_FINALLY()"));

  PT_END(self);
}

static ptstate_t protothread3(struct pt *self)
{
  PT_BEGIN(self);

  print_line(F("protothread3: PT_THROW in main workflow"));

  PT_WAIT_ONE(self);

  print_line(F("protothread3: PT_THROW(ERR_SYS_INVAL)"));

  PT_THROW(self, ERR_SYS_INVAL);

  print_line(F("protothread3: PT_ENDED"));

  PT_CATCHANY(self)
  {
    print_error(F("protothread3: PT_CATCHANY()"), PT_ERROR_STATE);
  }

  PT_FINALLY(self)

  print_line(F("protothread3: PT_FINALLY()"));

  PT_END(self);
}

static ptstate_t protothread4(struct pt *self)
{
  PT_BEGIN(self);

  print_line(F("protothread4: PT_RETHROW in PT_CATCHANY"));

  PT_WAIT_ONE(self);

  print_line(F("protothread4: PT_THROW(ERR_SYS_INVAL)"));

  PT_THROW(self, ERR_SYS_INVAL);

  print_line(F("protothread4: PT_ENDED"));

  PT_CATCHANY(self)
  {
    print_error(F("protothread4: PT_CATCHANY()"), PT_ERROR_STATE);
    PT_RETHROW(self);
  }

  PT_FINALLY(self)

  print_line(F("protothread4: PT_FINALLY()"));

  PT_END(self);
}

void setup()
{
  print_setup();
}

void test_run(protothread_t protothread, uint8_t idx)
{
  print_line(F("void test_run() START"), idx);

  static struct pt self;
  static ptstate_t state;

  // set the protothread control structure to the beginning of the protothread
  PT_INIT(&self);
  while(PT_ISRUNNING(state = protothread(&self)))
  {
    print_state(F("while PT_ISRUNNING(...) == TRUE"), state);
    delay(1000);
  }
  print_state(F("PT_ISRUNNING() == FALSE"), state);
  delay(1000);

  // set the protothread control structure to the finally control block
  PT_FINAL(&self);
  while(PT_ISRUNNING(state = protothread(&self)))
  {
    print_state(F("while finally PT_ISRUNNING(...) == TRUE"), state);
    delay(1000);
  }

  print_state(F("PT_RUNNING is finally done!"), state);
  delay(1000);

}

void loop()
{
  test_run(&protothread1, 1);
  print_count++;
  test_run(&protothread2, 2);
  print_count++;
  test_run(&protothread3, 3);
  print_count++;
  test_run(&protothread4, 4);
  print_count++;

  delay(1000);
}
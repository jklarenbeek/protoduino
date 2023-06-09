/***
 * 
*/

#include <protoduino.h>
#include <sys/pt.h>

static ptstate_t protothread1(struct pt *self)
{
  PT_BEGIN(self);

  Serial.println("PT_BEGIN() protothread1");

  forever: while(1)
  {
    Serial.println("PT_WAIT_ONE() protothread1");

    PT_WAIT_ONE(self);

    uint8_t v = random(0, 6);
    if (v == 2)
      PT_EXIT(self);
    else if (v == 3)
      PT_RAISE(self, PT_ERROR + v);
    else if (v == 4)
      break;
  }

  Serial.println("PT_ENDED protothread1");

  PT_CATCHANY(self)
  {
    Serial.print("PT_CATCHANY() protothread1:");
    Serial.println(PT_ERROR_STATE);
    PT_RETHROW(self);
  }

  PT_FINALLY(self)
  
  Serial.println("PT_FINALLY() protothread1");

  PT_END(self);
}

void setup()
{
  Serial.begin(9600);
  
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));

  Serial.println(("Done setup, waiting 2 sec."));
  
  delay(2000);
}

void print_state(const ptstate_t state, const char *msg)
{
    if (state == PT_WAITING)
        Serial.print("PT_WAITING");
    else if (state == PT_YIELDED)
        Serial.print("PT_YIELDED");
    else if (state == PT_ENDED)
        Serial.print("PT_ENDED");
    else if (state == PT_EXITED)
        Serial.print("PT_EXITED");
    else if (state == PT_FINALIZED)
        Serial.print("PT_FINALIZED");
    else 
    {
        Serial.print("PT_ERROR:");
        Serial.print(state);
    }
    Serial.print(" ");
    Serial.print(msg);
    Serial.println();
}

void test1_run()
{
  Serial.println("void test1_run() START");

  static struct pt pt1;

  static ptstate_t state;

  /* Initialize the protothread state variables. */
  PT_INIT(&pt1);
  while(PT_ISRUNNING(state = protothread1(&pt1)))
  {
    print_state(state, "void test1_run()");
    delay(1000);
  }
  print_state(state, "void test1_run()");
  delay(1000);

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

    delay(2000);
}
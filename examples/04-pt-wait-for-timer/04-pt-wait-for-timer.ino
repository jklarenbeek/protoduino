
#include <protoduino.h>
#include <sys/pt-timer.h>

static uint32_t count = 0;

static ptstate_t protothread1(struct pt *pt)
{
  struct timer timer1;

  /* A protothread function must begin with PT_BEGIN() which takes a
     pointer to a struct pt. */
  PT_BEGIN(pt);

  Serial.print(count);
  Serial.println(" - protothread1: Setting timer1 for 3 seconds and awaits.");
  
  timer_set(&timer1, get_clock_seconds(3));

  PT_WAIT_UNTIL(pt, timer_expired(&timer1));

  Serial.print(count);
  Serial.println(" - protothread1: Timer1 expired");

  /* All protothread functions must end with PT_END() which takes a
     pointer to a struct pt. */
  PT_END(pt);

}

static ptstate_t protothread2(struct pt *pt)
{
  /* A protothread function must begin with PT_BEGIN() which takes a
     pointer to a struct pt. */
  PT_BEGIN(pt);

  Serial.print(count);
  Serial.println(" - protothread2: Setting timer2 for 5000 milliseconds and awaits.");

  /* Be weary of the static variable being used by PT_WAIT_DELAY().
     This protothread should not be used in a multiple pt instances. */
  PT_WAIT_DELAY(pt, 5000);

  Serial.print(count);
  Serial.println(" - protothread2: Timer2 expired");

  /* All protothread functions must end with PT_END() which takes a
     pointer to a struct pt. */
  PT_END(pt);

}

static struct pt pt1, pt2;

void setup()
{
  Serial.begin(9600);
  
  Serial.println("Done setup, delay 1 sec.");
  
  delay(1000);
}

void loop() 
{
  Serial.print("void loop(): ");
  Serial.println(count);

  /* Initialize the protothread state variables each loop. */
  PT_INIT(&pt1);
  PT_INIT(&pt2);

  Serial.print(count);
  Serial.println(" - Protothread1: using timer_expired()");
  
  while(PT_ISRUNNING(protothread1(&pt1)))
    count++;

  Serial.print(count);
  Serial.println(" - Protothread2: using PT_WAIT_DELAY()");
  
  while(PT_ISRUNNING(protothread2(&pt2)))
    count++;
  
  delay(1000);
}

/**
 * This example demonstrates the use of the PT_WAIT_UNTIL() and 
 * PT_WAIT_WHILE() macros.
 * Since each protothread contains a forever loop, this example
 * runs forever.
 */

#include <protoduino.h>
#include <sys/pt.h>

static int count = 0;

/**
 * The first protothread function. A protothread function must always
 * return an integer, but must never explicitly return - returning is
 * performed inside the protothread statements.
 *
 * The protothread function is driven by the main loop further down in
 * the code.
 */
static ptstate_t protothread1(struct pt *pt)
{  
  /* A protothread function must begin with PT_BEGIN() which takes a
     pointer to a struct pt. */
  PT_BEGIN(pt);

  /* We loop forever here. */
  while(1) {

    Serial.print(++count);
    Serial.println(" - Protothread 1 running; wait one");

    PT_WAIT_ONE(pt);

    /* And we loop. */
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

  while(1) {

    Serial.print(++count);
    Serial.println(" - Protothread 2 running; wait until (count % 2) == 0");

    PT_WAIT_UNTIL(pt, ((count % 2) == 0));

    /* And we loop. */
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

  while(1) {

    Serial.print(++count);
    Serial.println(" - Protothread 3 running; wait while (count % 3) != 0");

    PT_WAIT_WHILE(pt, ((count % 3) != 0));
    
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
  Serial.begin(9600); 

  /* Initialize the protothread state variables with PT_INIT(). */
  PT_INIT(&pt1);
  PT_INIT(&pt2);
  PT_INIT(&pt3);

  Serial.println("Done setup, waiting 5 sec.");
  
  delay(5000);
}

void loop()
{
  Serial.print("void loop(): ");
  Serial.println(count);

  protothread1(&pt1);
  protothread2(&pt2);
  protothread3(&pt3);
  
  delay(2000);
}
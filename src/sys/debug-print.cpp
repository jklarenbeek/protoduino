#include "debug-print.h"

int print_count = 0;

void print_setup()
{
  Serial.begin(9600);
  
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));

  Serial.println("Done setup, waiting 1 sec.");
  
  delay(1000);

}

void print_info(const char * msg)
{
    Serial.println(msg);
}

void print_state(const ptstate_t s, const char * msg)
{
  Serial.print(print_count);

  if (s == PT_WAITING)
    Serial.print(" - PT_WAITING ");
  if (s == PT_YIELDED)
    Serial.print(" - PT_YIELDED ");
  if (s == PT_EXITED)
    Serial.print(" - PT_EXITED ");
  if (s == PT_ENDED)
    Serial.print(" - PT_ENDED ");
  if (s == PT_FINALIZED)
    Serial.print(" - PT_FINALIZED ");
  else if (s >= PT_ERROR)
  {
    Serial.print(" - PT_ERROR (");
    Serial.print(s);
    Serial.print(") ");
  }
  Serial.println(msg);
}

void print_error(const char * str, uint8_t err)
{
  Serial.print(print_count);
  Serial.print(" - ");
  Serial.print(str);
  Serial.print(" (");
  Serial.print(err);
  Serial.println(")");

}

void print_line(const char * str)
{
  Serial.print(print_count);
  Serial.print(" - ");
  Serial.println(str);
}

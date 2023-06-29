#include <Arduino.h>
#include "examples.h"

int print_count = 0;

void print_setup()
{
  Serial.begin(9600);
  
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));

  Serial.println();
  Serial.println("Done setup, waiting 1 sec.");
  
  delay(1000);

}

void print_info(const char * msg)
{
    Serial.println(msg);
}


static void print_state_ex(const ptstate_t s, const lc_t lc)
{
  Serial.print(print_count);
  if (lc != 0)
  {
    Serial.print(":");
    Serial.print(lc);
  }

  if (s == PT_WAITING)
    Serial.print(" - PT_WAITING");
  if (s == PT_YIELDED)
    Serial.print(" - PT_YIELDED");
  if (s == PT_EXITED)
    Serial.print(" - PT_EXITED");
  if (s == PT_ENDED)
    Serial.print(" - PT_ENDED");
  if (s == PT_FINALIZED)
    Serial.print(" - PT_FINALIZED");
  else if (s >= PT_ERROR)
  {
    Serial.print(" - PT_ERROR (");
    Serial.print(s);
    Serial.print(")");
  }
}

void print_state(const ptstate_t s, const char * msg)
{
  print_state_ex(s, 0);
  Serial.print(" ");
  Serial.println(msg);
}

void print_state(const ptstate_t s, const char * msg, const uint8_t value)
{
  print_state(s, 0, msg, value);
}

void print_state(const ptstate_t s, const lc_t lc, const char * msg)
{
  print_state_ex(s, lc);
  Serial.print(" ");
  Serial.print(msg);
  Serial.println();
}

void print_state(const ptstate_t s, const lc_t lc, const char * msg, const uint8_t value)
{
  print_state_ex(s, lc);
  Serial.print(" ");
  Serial.print(msg);
  Serial.print(":");
  Serial.println(value);
}

static void print_error_ex(const char * str, uint8_t err)
{
  Serial.print(print_count);
  Serial.print(" - ");
  Serial.print(str);
  Serial.print(" (");
  Serial.print(err);
  Serial.print(")");
}

void print_error(const char * str, uint8_t err)
{
  print_error_ex(str, err);
  Serial.println("");
}

static void print_line_ex(const lc_t lc, const char *str)
{
  Serial.print(print_count);
  if (lc != 0)
  {
    Serial.print(":");
    Serial.print(lc);
  }
  Serial.print(" - ");
  Serial.print(str);
}

void print_line(const lc_t lc, const char * str)
{
  print_line_ex(lc, str);
  Serial.println();
}

void print_line(const char * str)
{
  print_line(0, str);
}

void print_line(const lc_t lc, const char *str, uint8_t value)
{
  print_line_ex(lc, str);
  Serial.print(":");
  Serial.println(value);
}

void print_line(const char *str, uint8_t value)
{
  print_line(0, str, value);
}


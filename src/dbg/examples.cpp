#include "examples.h"

int print_count = 0;

void print_setup()
{
  SerialLine.begin(9600);
  Stream *st = &SerialLine;

#ifdef USE_ARDUINO_HARDWARESERIAL
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));
#endif

  SerialLine.println();
  SerialLine.println("Done setup, waiting 1 sec.");
  
#ifdef USE_ARDUINO_HARDWARESERIAL
  delay(1000);
#endif

}

void print_info(const char * msg)
{
    SerialLine.println(msg);
}


void print_state_ex(const ptstate_t s, const lc_t lc)
{
  SerialLine.print(print_count);
  if (lc != 0)
  {
    SerialLine.print(":");
    SerialLine.print(lc);
  }

  if (s == PT_WAITING)
    SerialLine.print(" - PT_WAITING");
  if (s == PT_YIELDED)
    SerialLine.print(" - PT_YIELDED");
  if (s == PT_EXITED)
    SerialLine.print(" - PT_EXITED");
  if (s == PT_ENDED)
    SerialLine.print(" - PT_ENDED");
  if (s == PT_FINALIZED)
    SerialLine.print(" - PT_FINALIZED");
  else if (s >= PT_ERROR)
  {
    SerialLine.print(" - PT_ERROR (");
    SerialLine.print(s);
    SerialLine.print(")");
  }
}

void print_state(const ptstate_t s, const char * msg)
{
  print_state_ex(s, 0);
  SerialLine.print(" ");
  SerialLine.println(msg);
}

void print_state(const ptstate_t s, const char * msg, const uint8_t value)
{
  print_state(s, 0, msg, value);
}

void print_state(const ptstate_t s, const lc_t lc, const char * msg)
{
  print_state_ex(s, lc);
  SerialLine.print(" ");
  SerialLine.print(msg);
  SerialLine.println();
}

void print_state(const ptstate_t s, const lc_t lc, const char * msg, const uint8_t value)
{
  print_state_ex(s, lc);
  SerialLine.print(" ");
  SerialLine.print(msg);
  SerialLine.print(":");
  SerialLine.println(value);
}

static void print_error_ex(const char * str, uint8_t err)
{
  SerialLine.print(print_count);
  SerialLine.print(" - ");
  SerialLine.print(str);
  SerialLine.print(" (");
  SerialLine.print(err);
  SerialLine.print(")");
}

void print_error(const char * str, uint8_t err)
{
  print_error_ex(str, err);
  SerialLine.println("");
}

static void print_line_ex(const lc_t lc, const char *str)
{
  SerialLine.print(print_count);
  if (lc != 0)
  {
    SerialLine.print(":");
    SerialLine.print(lc);
  }
  SerialLine.print(" - ");
  SerialLine.print(str);
}

void print_line(const lc_t lc, const char * str)
{
  print_line_ex(lc, str);
  SerialLine.println();
}

void print_line(const char * str)
{
  print_line(0, str);
}

void print_line(const lc_t lc, const char *str, uint8_t value)
{
  print_line_ex(lc, str);
  SerialLine.print(":");
  SerialLine.println(value);
}

void print_line(const char *str, uint8_t value)
{
  print_line(0, str, value);
}

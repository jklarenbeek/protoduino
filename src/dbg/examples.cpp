#include "examples.h"

#ifdef USE_ARDUINO_HARDWARESERIAL
#include <Arduino.h>
#define SerialOut Serial
#else
#include "../sys/tmpl/SerialClass.hpp"
#define SerialOut CSerial0
#endif


int print_count = 0;

void print_setup()
{
  SerialOut.begin(9600);
  Stream *st = &SerialOut;

#ifdef USE_ARDUINO_HARDWARESERIAL
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));
#endif

  SerialOut.println();
  SerialOut.println("Done setup, waiting 1 sec.");
  
#ifdef USE_ARDUINO_HARDWARESERIAL
  delay(1000);
#endif

}

void print_info(const char * msg)
{
    SerialOut.println(msg);
}


static void print_state_ex(const ptstate_t s, const lc_t lc)
{
  SerialOut.print(print_count);
  if (lc != 0)
  {
    SerialOut.print(":");
    SerialOut.print(lc);
  }

  if (s == PT_WAITING)
    SerialOut.print(" - PT_WAITING");
  if (s == PT_YIELDED)
    SerialOut.print(" - PT_YIELDED");
  if (s == PT_EXITED)
    SerialOut.print(" - PT_EXITED");
  if (s == PT_ENDED)
    SerialOut.print(" - PT_ENDED");
  if (s == PT_FINALIZED)
    SerialOut.print(" - PT_FINALIZED");
  else if (s >= PT_ERROR)
  {
    SerialOut.print(" - PT_ERROR (");
    SerialOut.print(s);
    SerialOut.print(")");
  }
}

void print_state(const ptstate_t s, const char * msg)
{
  print_state_ex(s, 0);
  SerialOut.print(" ");
  SerialOut.println(msg);
}

void print_state(const ptstate_t s, const char * msg, const uint8_t value)
{
  print_state(s, 0, msg, value);
}

void print_state(const ptstate_t s, const lc_t lc, const char * msg)
{
  print_state_ex(s, lc);
  SerialOut.print(" ");
  SerialOut.print(msg);
  SerialOut.println();
}

void print_state(const ptstate_t s, const lc_t lc, const char * msg, const uint8_t value)
{
  print_state_ex(s, lc);
  SerialOut.print(" ");
  SerialOut.print(msg);
  SerialOut.print(":");
  SerialOut.println(value);
}

static void print_error_ex(const char * str, uint8_t err)
{
  SerialOut.print(print_count);
  SerialOut.print(" - ");
  SerialOut.print(str);
  SerialOut.print(" (");
  SerialOut.print(err);
  SerialOut.print(")");
}

void print_error(const char * str, uint8_t err)
{
  print_error_ex(str, err);
  SerialOut.println("");
}

static void print_line_ex(const lc_t lc, const char *str)
{
  SerialOut.print(print_count);
  if (lc != 0)
  {
    SerialOut.print(":");
    SerialOut.print(lc);
  }
  SerialOut.print(" - ");
  SerialOut.print(str);
}

void print_line(const lc_t lc, const char * str)
{
  print_line_ex(lc, str);
  SerialOut.println();
}

void print_line(const char * str)
{
  print_line(0, str);
}

void print_line(const lc_t lc, const char *str, uint8_t value)
{
  print_line_ex(lc, str);
  SerialOut.print(":");
  SerialOut.println(value);
}

void print_line(const char *str, uint8_t value)
{
  print_line(0, str, value);
}

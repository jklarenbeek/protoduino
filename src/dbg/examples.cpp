// file: ./src/dbg/examples.cpp

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
  SerialLine.println(F("Done setup, waiting 1 sec."));

#ifdef USE_ARDUINO_HARDWARESERIAL
  delay(1000);
#endif

}

void print_info(const char *msg)
{
    SerialLine.println(msg);
}

void print_state_ex(const ptstate_t s, const lc_t lc)
{
  SerialLine.print(print_count);
  if (lc != 0)
  {
    SerialLine.print(F(":"));
    SerialLine.print(lc);
  }

  if (s == PT_WAITING)
    SerialLine.print(F(" - PT_WAITING"));
  if (s == PT_YIELDED)
    SerialLine.print(F(" - PT_YIELDED"));
  if (s == PT_EXITED)
    SerialLine.print(F(" - PT_EXITED"));
  if (s == PT_ENDED)
    SerialLine.print(F(" - PT_ENDED"));
  if (s == PT_FINALIZED)
    SerialLine.print(F(" - PT_FINALIZED"));
  else if (s >= PT_ERROR)
  {
    SerialLine.print(F(" - PT_ERROR ("));
    SerialLine.print(s);
    SerialLine.print(F(")"));
  }
}

void print_state(const char *msg, const ptstate_t s)
{
  print_state_ex(s, 0);
  SerialLine.print(F(" "));
  SerialLine.println(msg);
}

void print_state(const char *msg, const ptstate_t s, const lc_t lc)
{
  print_state_ex(s, lc);
  SerialLine.print(F(" "));
  SerialLine.print(msg);
  SerialLine.println();
}

void print_state(const char *msg, const ptstate_t s, const uint8_t value)
{
  print_state(msg, s, 0, value);
}

void print_state(const char *msg, const ptstate_t s, const lc_t lc, const uint8_t value)
{
  print_state_ex(s, lc);
  SerialLine.print(F(" "));
  SerialLine.print(msg);
  SerialLine.print(F(":"));
  SerialLine.println(value);
}

void print_state(const __FlashStringHelper *msg, const ptstate_t s)
{
  print_state((const char *)msg, s);
}

void print_state(const __FlashStringHelper *msg, const ptstate_t s, const lc_t lc)
{
  print_state((const char *)msg, s, lc);
}

void print_state(const __FlashStringHelper *msg, const ptstate_t s, const uint8_t value)
{
  print_state((const char *)msg, s, 0, value);
}

void print_state(const __FlashStringHelper *msg, const ptstate_t s, const lc_t lc, const uint8_t value)
{
  print_state((const char *)msg, s, lc, value);
}

static void print_error_ex(const char *msg, uint8_t err)
{
  SerialLine.print(print_count);
  SerialLine.print(F(" - "));
  SerialLine.print(msg);
  SerialLine.print(F(" ("));
  SerialLine.print(err);
  SerialLine.print(F(")"));
}

void print_error(const char *msg, uint8_t err)
{
  print_error_ex(msg, err);
  SerialLine.println(F(""));
}

void print_line(const __FlashStringHelper *msg, uint8_t err)
{
  print_line((const char *)msg, 0, value);
}

static void print_line_ex(const char *msg, const lc_t lc)
{
  SerialLine.print(print_count);
  if (lc != 0)
  {
    SerialLine.print(F(":"));
    SerialLine.print(lc);
  }
  SerialLine.print(F(" - "));
  SerialLine.print(msg);
}

void print_line(const char *msg)
{
  print_line(msg, 0);
}

void print_line(const char *msg, const lc_t lc)
{
  print_line_ex(msg, lc);
  SerialLine.println();
}

void print_line(const char *msg, uint8_t value)
{
  print_line(msg, 0, value);
}

void print_line(const char *msg, const lc_t lc, uint8_t value)
{
  print_line_ex(msg, lc);
  SerialLine.print(F(":"));
  SerialLine.println(value);
}

void print_line(const __FlashStringHelper *msg)
{
  print_line((const char *)msg, 0);
}

void print_line(const __FlashStringHelper *msg, const lc_t lc)
{
  print_line((const char *)msg, lc);
}

void print_line(const __FlashStringHelper *msg, uint8_t value)
{
  print_line((const char *)msg, 0, value);
}

void print_line(const __FlashStringHelper *msg, const lc_t lc, uint8_t value)
{
  print_line((const char *)msg, lc, value);
}

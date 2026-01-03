// file: ./src/dbg/print.c

#include "print.h"
#include "../sys/serial.h"

int print_count = 0;

void print_setup()
{
  serial0_open(9600);

  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  // randomSeed(analogRead(0));

  println();
  print_P(PSTR("Done setup, waiting 1 sec."));
  println();

  delay(1000);

}

void print_info_P(const char *msg)
{
    println_P(msg);
}

void print_state_ex_P(const ptstate_t s, const lc_t lc)
{
  print_dec(print_count);
  if (lc != 0)
  {
    printchar(':');
    print_dec(lc);
  }

  if (s == PT_WAITING)
    print_P(PSTR(" - PT_WAITING"));
  if (s == PT_YIELDED)
    print_P(PSTR(" - PT_YIELDED"));
  if (s == PT_EXITED)
    print_P(PSTR(" - PT_EXITED"));
  if (s == PT_ENDED)
    print_P(PSTR(" - PT_ENDED"));
  if (s == PT_FINALIZED)
    print_P(PSTR(" - PT_FINALIZED"));
  else if (s >= PT_ERROR)
  {
    print_P(PSTR(" - PT_ERROR ("));
    print_dec(s);
    print_P(PSTR(")"));
  }
}

void print_state_P(const char *msg, const ptstate_t s)
{
  print_state_ex_P(s, 0);
  printchar(' ');
  println_P(msg);
}

void print_state_lc_P(const char *msg, const ptstate_t s, const lc_t lc)
{
  print_state_ex_P(s, lc);
  printchar(' ');
  print_P(msg);
  println();
}

void print_state_val_P(const char *msg, const ptstate_t s, const uint8_t value)
{
  print_state_full_P(msg, s, 0, value);
}

void print_state_full_P(const char *msg, const ptstate_t s, const lc_t lc, const uint8_t value)
{
  print_state_ex_P(s, lc);
  printchar(' ');
  print_P(msg);
  printchar(':');
  print_dec(value);
  println();
}

static void print_error_ex_P(const char *msg, uint8_t err)
{
  print_dec(print_count);
  print_P(PSTR(" - "));
  print_P(msg);
  print_P(PSTR(" ("));
  print_dec(err);
  print_P(PSTR(")"));
}

void print_error_P(const char *msg, uint8_t err)
{
  print_error_ex_P(msg, err);
  println();
}

static void print_line_ex_P(const char *msg, const lc_t lc)
{
  print_dec(print_count);
  if (lc != 0)
  {
    printchar(':');
    print_dec(lc);
  }
  print_P(PSTR(" - "));
  print_P(msg);
}

void print_line_P(const char *msg)
{
  print_line_val_P(msg, 0u);
}

void print_line_lc_P(const char *msg, const lc_t lc)
{
  print_line_ex_P(msg, lc);
  println();
}

void print_line_val_P(const char *msg, uint8_t value)
{
  print_line_full_P(msg, 0, value);
}

void print_line_full_P(const char *msg, const lc_t lc, uint8_t value)
{
  print_line_ex_P(msg, lc);
  printchar(':');
  print_dec(value);
  println();
}

#include <protoduino.h>
#include <dbg/print.h>
#include <lib/ringb8.h>

RINGB8(rbuf, 8);

char nextchar()
{
  return 'A' + RINGB8_COUNT(rbuf);
}

void print_size(void)
{
  print_P(PSTR("count:"));
  print_dec(RINGB8_COUNT(rbuf));
  print_P(PSTR(" ,available:"));
  print_dec(RINGB8_AVAILABLE(rbuf));
  print_P(PSTR(" ,head:"));
  print_dec(VAR_RINGB8(rbuf).head);
  print_P(PSTR(" ,tail:"));
  print_dec(VAR_RINGB8(rbuf).tail);
  println();
}

void print_buf(void)
{
    while(RINGB8_COUNT(rbuf) > 0)
        printchar((char)RINGB8_GET(rbuf));

    println_P(PSTR(" -> done"));
}

void setup()
{
  print_setup();
  RINGB8_PUT(rbuf, nextchar());
  RINGB8_PUT(rbuf, nextchar());
  RINGB8_PUT(rbuf, nextchar());
  print_P(PSTR("starting:"));
  print_size();
  delay(1000);
}

void loop()
{
  RINGB8_PUT(rbuf, nextchar());
  print_size();

  if (RINGB8_AVAILABLE(rbuf) == 0)
  {
    print_P(PSTR("printing buffer:"));
    print_buf();
    print_size();
  }
  delay(1000);
}


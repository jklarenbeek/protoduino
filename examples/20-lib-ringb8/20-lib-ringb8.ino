#include <protoduino.h>
#include <dbg/examples.h>
#include <lib/ringb8.h>

RINGB8(rbuf, 8);

char nextchar()
{
  return 'A' + RINGB8_COUNT(rbuf);
}

void print_size(void)
{
  SerialLine.print("count:");
  SerialLine.print(RINGB8_COUNT(rbuf));
  SerialLine.print(" ,available:");
  SerialLine.print(RINGB8_AVAILABLE(rbuf));
  SerialLine.print(" ,head:");
  SerialLine.print(VAR_RINGB8(rbuf).head);
  SerialLine.print(" ,tail:");
  SerialLine.println(VAR_RINGB8(rbuf).tail);
}

void print_buf(void)
{
    while(RINGB8_COUNT(rbuf) > 0)
        SerialLine.print((char)RINGB8_GET(rbuf));
        
    SerialLine.println(" -> done");
}

void setup()
{
  SerialLine.begin(9600);
  RINGB8_PUT(rbuf, nextchar());
  RINGB8_PUT(rbuf, nextchar());
  RINGB8_PUT(rbuf, nextchar());
  SerialLine.print("starting:");
  print_size();
  delay(1000);
}

void loop()
{    
  RINGB8_PUT(rbuf, nextchar());
  print_size();

  if (RINGB8_AVAILABLE(rbuf) == 0)
  {
    SerialLine.print("printing buffer:");
    print_buf();
    print_size();
  } 
  delay(1000);
}


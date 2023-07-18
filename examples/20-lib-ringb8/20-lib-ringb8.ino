#include <protoduino.h>
#include <dbg/examples.h>
#include <sys/ringb8.h>

RINGB8(rbuf, 8);

char nextchar()
{
  return 'A' + RINGB8_COUNT(rbuf);
}

void print_size(void)
{
  SerialOut.print("count:");
  SerialOut.print(RINGB8_COUNT(rbuf));
  SerialOut.print(" ,available:");
  SerialOut.print(RINGB8_AVAILABLE(rbuf));
  SerialOut.print(" ,head:");
  SerialOut.print(VAR_RINGB8(rbuf).head);
  SerialOut.print(" ,tail:");
  SerialOut.println(VAR_RINGB8(rbuf).tail);
}

void print_buf(void)
{
    while(RINGB8_COUNT(rbuf) > 0)
        SerialOut.print((char)RINGB8_GET(rbuf));
        
    SerialOut.println(" -> done");
}

void setup()
{
  SerialOut.begin(9600);
  RINGB8_PUT(rbuf, nextchar());
  RINGB8_PUT(rbuf, nextchar());
  RINGB8_PUT(rbuf, nextchar());
  SerialOut.print("starting:");
  print_size();
  delay(1000);
}

void loop()
{    
  RINGB8_PUT(rbuf, nextchar());
  print_size();

  if (RINGB8_AVAILABLE(rbuf) == 0)
  {
    SerialOut.print("printing buffer:");
    print_buf();
    print_size();
  } 
  delay(1000);
}


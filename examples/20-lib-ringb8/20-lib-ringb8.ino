#include <protoduino.h>
#include <dbg/examples.h>
#include <sys/ringb8.h>

RINGB8(rbuf, 8);

char nextchar()
{
  return 'A' + ringb8_count(&VAR_RINGB8(rbuf));
}

void print_size(void)
{
  SerialOut.print("count:");
  SerialOut.print(ringb8_count(&VAR_RINGB8(rbuf)));
  SerialOut.print(" ,available:");
  SerialOut.print(ringb8_available(&VAR_RINGB8(rbuf)));
  SerialOut.print(" ,head:");
  SerialOut.print(VAR_RINGB8(rbuf).head);
  SerialOut.print(" ,tail:");
  SerialOut.println(VAR_RINGB8(rbuf).tail);
}

void print_buf(void)
{
    while(ringb8_count(&VAR_RINGB8(rbuf)) > 0)
        SerialOut.print((char)ringb8_get(&VAR_RINGB8(rbuf)));
        
    SerialOut.println(" -> done");
}

void setup()
{
  SerialOut.begin(9600);
  ringb8_put(&VAR_RINGB8(rbuf), nextchar());
  ringb8_put(&VAR_RINGB8(rbuf), nextchar());
  ringb8_put(&VAR_RINGB8(rbuf), nextchar());
  SerialOut.print("starting:");
  print_size();
  delay(1000);
}

void loop()
{    
  ringb8_put(&VAR_RINGB8(rbuf), nextchar());
  print_size();

  if (ringb8_available(&VAR_RINGB8(rbuf)) == 0)
  {
    SerialOut.print("printing buffer:");
    print_buf();
    print_size();
  } 
  delay(1000);
}


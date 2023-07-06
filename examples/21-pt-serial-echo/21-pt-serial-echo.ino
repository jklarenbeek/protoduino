/**
 *   @author http://github.com/jklarenbeek
 *
 *	This demo will not work correctly on SimulIde, since it appears not to support unicode in its serial monitor.
 *	SimulIde also has a problem with flushing/sending its buffer in the same way as a real arduino does.
 */ 

#include <protoduino.h>
#include <sys/ringb8.h>
#include <sys/serial0.h>


void setup()
{
  serial0_open(9600);
}

void loop()
{
  serial0_write8('0' + (serial0_write_available()));
  serial0_write8('A');
  //serial0_write8('0' + (serial0_write_available()));
  serial0_write8('B');
  serial0_write8('C');
  //serial0_write8('0' + (serial0_write_available()));
  serial0_write8('D');
  serial0_write8('E');
  //serial0_write8('0' + (serial0_write_available()));
  serial0_write8('F');
  serial0_write8('G');
  serial0_write8('H');
  //serial0_write8('I');
  //serial0_write8('J');
  serial0_write8(13);
delay(1000);
  //while (serial0_flush() > 0);
}


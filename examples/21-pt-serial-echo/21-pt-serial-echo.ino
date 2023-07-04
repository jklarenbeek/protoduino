/**
 *   @author http://github.com/jklarenbeek
 *
 *	This demo will not work correctly on SimulIde, since it appears not to support unicode in its serial monitor.
 *	SimulIde also has a problem with flushing/sending its buffer in the same way as a real arduino does.
 */ 
#define USE_PROTODUINO_SERIAL

#include <protoduino.h>
#include <sys/serial0.h>

void setup()
{
  serial0_open(SERIAL_BAUD_9600);
}

void loop()
{
  if (serial0_write8(65) == 0)
  {
	//delay(1000);
  serial0_write8(66);
  }
//delay(1000);
  //serial0_write8(66);
//delay(1000);
  //serial0_write8(67);
//delay(1000);
  //serial0_write8(13);
//delay(1000);
}


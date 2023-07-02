/**
 *   @author http://github.com/jklarenbeek
 *
 *	This demo will not work correctly on SimulIde, since it appears not to support unicode in its serial monitor.
 *	SimulIde also has a problem with flushing/sending its buffer in the same way as a real arduino does.
 */ 
#include <protoduino.h>
#include <sys/serial0.h>

void setup()
{
  serial0_open(SERIAL_BAUD_9600);
}

void loop()
{
  
}


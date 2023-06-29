/**
 *   @author http://github.com/jklarenbeek
 *
 *	This demo will not work correctly on SimulIde, since it appears not to support unicode in its serial monitor.
 *	SimulIde also has a problem with flushing/sending its buffer in the same way as a real arduino does.
 */ 
#include <protoduino.h>
#include <utf/vt100.h>
#include <utf/utf8-stream.h>


void setup()
{
  Serial.begin(9600);
  
}

void loop()
{
  const rune16_t rune = 64;
  Serial.print("= void loop(): ");
  Serial.println(utf8_len("jaja"));
  
}


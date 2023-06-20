
#include <protoduino.h>
#include <sys/pt.h>
#include <sys/pt-errors.h>
#include <utf/vt100.h>
#include <utf/utf8-stream.h>

void setup()
{
  //serial0_begin(9600, serial0_handler);
  
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  //randomSeed(analogRead(0));

}

void loop()
{
  uint8_t len = utf8_rune16len((rune16_t)0);
  //size_t s = serial0_write((uint8_t)65);
  //serial0_write(66);
  //serial0_write(66);
}


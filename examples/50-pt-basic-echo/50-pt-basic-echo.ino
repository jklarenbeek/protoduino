/**
 *   @author http://github.com/jklarenbeek
 *
 *	This demo will not work correctly on SimulIde, since it appears not to support unicode in its serial monitor.
 *	SimulIde also has a problem with flushing/sending its buffer in the same way as a real arduino does.
 */ 
#include <protoduino.h>
#include <dbg/examples.h>
#include <utf/vt100.h>
#include <utf/utf8-stream.h>

static int count = 0;

struct echo_pt { // : rune16_pt, buf8_pt
  lc_t lc;                      // protothread state
  Stream * stream;              // stream for getc and putc
  rune16_t value;               // yielded rune
  uint8_t idx;               // current index of buffer
  uint8_t buf[VT_ESCAPE_BUFLEN];  // current escape buffer
};


#define PT_GETR(ptecho) \
  PT_WAIT_UNTIL(ptecho, (utf8_getr(ptecho->stream, &ptecho->value) > 0))

static ptstate_t getch(struct echo_pt *self)
{
  PT_BEGIN(self);

  forever: while(1)
  {
    PT_GETR(self);
    if (self->value != KEY_ESCAPE)
    {
      PT_YIELD(self);
    }
    else
    { 
      uint8_t ret;
      rune16_t rune;
      self->idx = 0;

      // from here we buffer all input
      // until we reach an escape terminator code.
      do
      {
        PT_GETR(self);
        ret = vt_escape_add(self->buf, &self->idx, self->value);
      }
      while(ret > 0);

      // the escape sequence was buffered correctly
      if (ret == 0)
      {
        // find the rune keycode for the escape sequence
        rune = vt_escape_match(self->buf, self->idx);
        if (rune == UTF8_DECODE_ERROR)
        {
          PT_THROW(self, ERR_FILE_NOT_FOUND); // escape sequence not found.
        }

        self->value = rune; // we found the keycode.
        PT_YIELD(self);
      }
      else // if (ret < 0)
      {
        if (ret == VT_ERR_INVALID_INPUT)
        {
          PT_THROW(self, ERR_BAD_ARGUMENTS);
        }
        
        PT_THROW(self, ERR_BUFFER_OVERFLOW);
      }
    }
  }

  PT_END(self);
}

#define PT_PUTR(pt, ptecho) \
  PT_WAIT_UNTIL(pt, (utf8_putr(ptecho.stream, ptecho.value) > 0))

static struct echo_pt pt1;

static ptstate_t main_driver(struct pt *self, Stream *stream)
{  
  PT_BEGIN(self);

  pt1.stream = stream;

  PT_FOREACH(self, &pt1, getch(&pt1))
  {
    pt1.value = vt_escape_symbol(pt1.value);

    stream->print("echo '");
    stream->flush();
    
    PT_PUTR(self, pt1);

    stream->flush();
    stream->print("' (");
    stream->print(pt1.value);
    stream->println(")");
  }
  PT_ENDEACH(self);

  PT_END(self);
}

void setup()
{
  SerialOut.begin(9600);
  
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));

  SerialOut.println("Done setup, waiting 3 sec.");
  delay(1000);

  SerialOut.println("waiting 2 sec.");
  delay(1000);

  SerialOut.println("waiting 1 sec.");
  delay(1000);
}


static struct pt main1;

void loop()
{
  SerialOut.print("= Starting loop: ");
  SerialOut.println(count);


  /* Initialize the protothread state variables. */
  PT_INIT(&main1);

  /**
   * Call the main driver protothread until it exits,
   * ends or throws an error
   */
  while(PT_ISRUNNING(main_driver(&main1, &SerialOut)))
  {
    ++count;
  }
  
}


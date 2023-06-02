#include <protoduino.h>
#include <sys/pt.h>
#include <utf/vt100.h>
#include <utf/utf8-stream.h>

static int count = 0;

struct ptterm {
  lc_t lc;                      // protothread state
  Stream * stream;              // stream for getc and putc
  rune16_t value;               // yielded rune
  uint8_t vt_idx;               // current index of buffer
  char vt_buf[VT_KEY_BUF_LEN];  // current escape buffer
};


#define PT_GETR(term_pt, refvalue) \
  PT_WAIT_UNTIL(term_pt, (utf8_getr(term_pt->stream, refvalue) > 0))

static ptstate_t getch(struct ptterm *self)
{
  PT_BEGIN(self);

  forever: while(1)
  {
    PT_GETR(self, &self->value);
    if (self->value != KEY_ESCAPE)
    {
      PT_YIELD(self);
    }
    else
    { 
      uint8_t ret;
      rune16_t rune;
      self->vt_idx = 0;

      // from here we buffer all input
      // until we reach an escape terminator code.
      do
      {
        PT_GETR(self, &self->value);
        ret = vt_buffer_add(self->vt_buf, &self->vt_idx, self->value);
      }
      while(ret > 0);

      // the escape sequence was buffered correctly
      if (ret == 0)
      {
        // find the rune keycode for the escape sequence
        rune = vt_buffer_match(self->vt_buf, self->vt_idx);
        if (rune == UTF8_DECODE_ERROR)
        {
          PT_THROW(self, PT_ERROR_FILE_NOT_FOUND); // escape sequence not found.
        }

        self->value = rune; // we found the keycode.
        PT_YIELD(self);
      }
      else
      {
        if (ret == VT_ERR_INVALID_INPUT)
        {
          PT_THROW(self, PT_ERROR_BAD_ARGUMENTS);
        }
        
        PT_THROW(self, PT_ERROR_BUFFER_OVERFLOW);
      }
    }
  }

  PT_END(self);
}

static ptstate_t main_driver(struct pt *self, Stream *stream)
{
  static struct ptterm pt1;
  
  PT_BEGIN(self);

  pt1.stream = stream;

  PT_FOREACH(self, &pt1, getch(&pt1))
  {
    stream->print("echo '");

    utf8_putr(stream, vt_get_key_symbol(pt1.value));

    stream->print("' (");
    stream->print(pt1.value);
    stream->println(")");
  }
  PT_ENDEACH(self);

  PT_END(self);
}

void setup()
{
  Serial.begin(9600);
  
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));

  Serial.println("Done setup, waiting 3 sec.");
  delay(1000);

  Serial.println("waiting 2 sec.");
  delay(1000);

  Serial.println("waiting 1 sec.");
  delay(1000);
}

void loop()
{
  Serial.print("= Starting loop: ");
  Serial.println(count);

  static struct pt main1;

  /* Initialize the protothread state variables. */
  PT_INIT(&main1);

  /**
   * Call the main driver protothread until it exits,
   * ends or throws an error
   */
  while(PT_ISRUNNING(main_driver(&main1, &Serial)))
  {
    ++count;
  }
  
}


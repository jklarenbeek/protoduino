/**
 * examples/13-pt-basic-echo
 *
 * Echo every key the terminal sends, as a protothread.
 *
 * Raw bytes from the Stream are fed to the byte-fed ANSI parser (ansi.h),
 * which decodes UTF-8 printables, C0 controls, and CSI/SS3 escape sequences
 * into KEY_* codes (vtkeys.h) — no blocking waits, no fixed escape buffer.
 * Special keys are echoed as their Unicode key-cap symbol via
 * vtkey_symbol16().
 *
 * A lone ESC keypress is recognised with a small inter-byte timeout: if the
 * parser is mid-sequence and no byte follows within ESC_GAP, the pending
 * ESC is flushed out as the ESC key.
 *
 * This demo will not work correctly on SimulIde, since it appears not to
 * support unicode in its serial monitor. SimulIde also has a problem with
 * flushing/sending its buffer in the same way as a real arduino does.
 */
#include <protoduino.h>
#include <sys/clock.h>
#include <sys/serial/SerialClass.hpp>
#include <lib/text/utf8_stream.h>
#include <lib/text/ansi.h>

static int count = 0;

/* Inter-byte gap after which a pending lone ESC is delivered. */
#define ESC_GAP  clock_from_millis(20)

struct echo_pt {
  lc_t lc;                       // protothread state
  Stream *stream;                // stream for getc and putc
  rune16_t value;                // yielded rune / KEY_* code
  uint8_t pending;               // a decoded key is waiting
  clock_time_t last_rx;          // time of the last byte fed to the parser
  ansi_parser_t parser;          // byte-fed VT500 escape decoder
};

#define PT_PUTR(pt, ptecho)                                                    \
  PT_WAIT_UNTIL(pt, (utf8_putr(ptecho.stream, ptecho.value) > 0))

/* ---- ansi.h handlers: capture one decoded key per parser event ---- */

static void echo_set(struct echo_pt *self, uint16_t code)
{
  if (self->pending)
    return;                      /* keep the first event */
  self->value   = (rune16_t)code;
  self->pending = 1;
}

static void echo_on_print(void *ctx, uint32_t cp)
{
  /* rune16_t is BMP-only: fold wider code-points to U+FFFD. */
  echo_set((struct echo_pt *)ctx,
           (cp > 0xFFFFu) ? 0xFFFDu : (uint16_t)cp);
}

static void echo_on_execute(void *ctx, uint8_t ctrl)
{
  echo_set((struct echo_pt *)ctx, ctrl);
}

static void echo_on_csi(void *ctx, const ansi_csi_t *csi)
{
  uint8_t mods = 0;
  uint16_t key = ansi_key_from_csi(csi, &mods);
  if (key)
    echo_set((struct echo_pt *)ctx, key);
}

static void echo_on_esc(void *ctx, uint8_t intermediate, uint8_t final)
{
  if (intermediate == 'O') {     /* SS3: app-cursor arrows, F1-F4 */
    uint8_t mods = 0;
    uint16_t key = ansi_key_from_ss3(final, &mods);
    if (key)
      echo_set((struct echo_pt *)ctx, key);
  }
}

static const ansi_handlers_t ECHO_HANDLERS = {
  echo_on_print, echo_on_execute, echo_on_csi, echo_on_esc,
  /* osc */ 0, /* dcs_hook */ 0, /* dcs_put */ 0, /* dcs_unhook */ 0,
};

/* ---- getch protothread: yields one decoded key per PT_YIELD ---- */

static ptstate_t getch(struct echo_pt *self) {
  PT_BEGIN(self);

  while (1) {
    /* Feed every available byte until a key pops out. */
    while (!self->pending && self->stream->available() > 0) {
      ansi_parse(&self->parser, (uint8_t)self->stream->read());
      self->last_rx = clock_time();
    }

    if (!self->pending) {
      if (ansi_parser_idle(&self->parser)) {
        /* Nothing pending, nothing partial: wait for the next byte. */
        PT_WAIT_UNTIL(self, self->stream->available() > 0);
        continue;
      }
      /* Mid-sequence: wait for the rest of the burst or the ESC gap. */
      PT_WAIT_UNTIL(self, self->stream->available() > 0
                       || (clock_time() - self->last_rx) > ESC_GAP);
      if (self->stream->available() > 0)
        continue;
      /* Gap expired: deliver a bare ESC; drop any other stale partial. */
      ansi_parser_flush(&self->parser);
      if (!ansi_parser_idle(&self->parser))
        ansi_parser_reset(&self->parser);
      if (!self->pending)
        continue;
    }

    self->pending = 0;
    PT_YIELD(self);
  }

  PT_END(self);
}

static struct echo_pt pt1;

static ptstate_t main_driver(struct pt *self, Stream *stream) {
  PT_BEGIN(self);

  pt1.stream  = stream;
  pt1.pending = 0;
  ansi_parser_init(&pt1.parser, &ECHO_HANDLERS, &pt1, NULL, 0);

  PT_FOREACH(self, &pt1, getch(&pt1)) {
    pt1.value = (rune16_t)vtkey_symbol16((uint16_t)pt1.value);

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

void setup() {
  SerialLine.begin(9600);

  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));

  SerialLine.println("Done setup, waiting 3 sec.");
  delay(1000);

  SerialLine.println("waiting 2 sec.");
  delay(1000);

  SerialLine.println("waiting 1 sec.");
  delay(1000);
}

static struct pt main1;

void loop() {
  SerialLine.print("= Starting loop: ");
  SerialLine.println(count);

  /* Initialize the protothread state variables. */
  PT_INIT(&main1);

  /**
   * Call the main driver protothread until it exits,
   * ends or throws an error
   */
  while (PT_ISRUNNING(main_driver(&main1, &SerialLine))) {
    ++count;
  }
}

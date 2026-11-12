/*
 * probe.c - exercises protoduino's src/cpu/avr/uart0.c under protosim.
 * Bare-metal (no Arduino core). Select a test with -DTEST=n.
 * Output always goes through the driver's own polled TX path, the way the
 * protoduino examples do it: wait for uart0_tx_is_available(), then write.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>

#include <protoduino.h>
#include <sys/uart.h>

#ifndef PROBE_BAUD
#define PROBE_BAUD 9600
#endif

static void put(char c)
{
  while (!uart0_tx_is_available()) { }
  uart0_tx_write8((uint8_t)c);
}

static void puts_(const char *s)
{
  while (*s) put(*s++);
}

static void putu(uint32_t v)
{
  char buf[11];
  uint8_t i = 0;
  do { buf[i++] = (char)('0' + (v % 10)); v /= 10; } while (v);
  while (i) put(buf[--i]);
}

static void kv(const char *k, uint32_t v)
{
  puts_(k); put('='); putu(v); put(' ');
}

static void done(void)
{
  puts_("\nDONE\n");
  for (;;) { }
}

/* ------------------------------------------------------------------ */
#if TEST == 1
/* What does uart0_baudrate() report versus what the registers produce? */
static const uint32_t bauds[] = { 9600UL, 19200UL, 57600UL, 115200UL, 300UL };
#define NB (sizeof(bauds) / sizeof(bauds[0]))

int main(void)
{
  uint16_t ubrr[NB];
  uint8_t  u2x[NB];
  uint32_t rep[NB];

  for (uint8_t i = 0; i < NB; i++) {
    uart0_open(bauds[i]);
    ubrr[i] = (uint16_t)UBRR0L | ((uint16_t)UBRR0H << 8);
    u2x[i]  = (UCSR0A & _BV(U2X0)) ? 1 : 0;
    rep[i]  = uart0_baudrate();
  }

  uart0_open(9600);
  for (uint8_t i = 0; i < NB; i++) {
    uint32_t real = F_CPU / ((u2x[i] ? 8UL : 16UL) * ((uint32_t)ubrr[i] + 1));
    kv("req", bauds[i]);
    kv("ubrr", ubrr[i]);
    kv("u2x", u2x[i]);
    kv("real", real);
    kv("reported", rep[i]);
    put('\n');
  }
  done();
}

/* ------------------------------------------------------------------ */
#elif TEST == 2
/* Interrupts ON (the way the examples run). Who sees the received bytes:
 * the on_rx_complete callback, or main-loop polling of rx_is_ready()? */
static volatile uint8_t  isr_bytes;
static volatile uint8_t  isr_errs;
static volatile char     got[16];

static void on_rx(uint_fast8_t d) { if (isr_bytes < 15) got[isr_bytes] = (char)d; isr_bytes++; }
static void on_err(uint_fast8_t e) { (void)e; isr_errs++; }

int main(void)
{
  uint32_t polled_hits = 0;

  uart0_on_rx_complete(on_rx);
  uart0_on_rx_error(on_err);
  uart0_open(9600);
  sei();

  /* ~12 ms of simulated time; protosim injects one byte per ms */
  for (uint32_t i = 0; i < 30000UL; i++) {
    if (uart0_rx_is_ready())
      polled_hits++;
  }

  cli();
  kv("isr_bytes", isr_bytes);
  kv("isr_errs", isr_errs);
  kv("polled_rx_is_ready_hits", polled_hits);
  puts_("data=");
  for (uint8_t i = 0; i < isr_bytes && i < 15; i++) put(got[i]);
  done();
}

/* ------------------------------------------------------------------ */
#elif TEST == 3
/* Interrupts never enabled (the bootloader situation). Does the polled
 * API - rx_is_ready() / rx_read8() - work after uart0_open()? */
int main(void)
{
  uint8_t n = 0;

  uart0_open(9600);            /* note: no sei() anywhere */

  puts_("echo=");
  for (uint32_t i = 0; i < 60000UL && n < 5; i++) {
    if (uart0_rx_is_ready()) {
      uint8_t c = uart0_rx_read8();
      put((char)c);
      n++;
    }
  }
  put(' ');
  kv("polled_bytes", n);
  kv("sreg_i", (SREG & 0x80) ? 1 : 0);
  done();
}

/* ------------------------------------------------------------------ */
#elif TEST == 4
/* uart0_close() called with interrupts disabled: is the I flag restored? */
int main(void)
{
  uint8_t before, after;

  uart0_open(9600);
  cli();
  before = (SREG & 0x80) ? 1 : 0;
  uart0_close();
  after = (SREG & 0x80) ? 1 : 0;
  cli();

  uart0_open(9600);
  kv("sreg_i_before_close", before);
  kv("sreg_i_after_close", after);
  done();
}

/* ------------------------------------------------------------------ */
#elif TEST == 5
/* Overrun the receiver with interrupts off, then look at rx_error() and
 * what rx_clear_errors() does to it - on this simulator. */
int main(void)
{
  uint8_t ucsra_a, ucsra_b, e_a, e_b, n = 0;

  uart0_open(9600);            /* no sei(): nobody drains the receiver */

  /* let protosim inject far more bytes than the receive FIFO holds */
  for (uint8_t i = 0; i < 120; i++) _delay_ms(1);

  ucsra_a = UCSR0A;
  e_a     = (uint8_t)uart0_rx_error();
  uart0_rx_clear_errors();
  ucsra_b = UCSR0A;
  e_b     = (uint8_t)uart0_rx_error();

  while (uart0_rx_is_ready()) { (void)uart0_rx_read8(); n++; }

  kv("ucsra_before", ucsra_a);
  kv("rx_error_before", e_a);
  kv("ucsra_after_clear", ucsra_b);
  kv("rx_error_after_clear", e_b);
  kv("ERR_SERIAL_OVERRUN", ERR_SERIAL_OVERRUN);
  kv("bytes_drained", n);
  done();
}

/* ------------------------------------------------------------------ */
#elif TEST == 6
/* The examples' transmit model: fill a buffer, call tx_enable_int(), and
 * let the UDRE interrupt pull bytes through on_tx_complete(). */
static const char msg[] = "pulled-by-udre-isr";
static volatile uint8_t pos;
static volatile uint8_t udre_calls;

static int_fast16_t on_tx(void)
{
  udre_calls++;
  if (msg[pos] == 0) return -1;
  return (uint8_t)msg[pos++];
}

int main(void)
{
  uart0_on_tx_complete(on_tx);
  uart0_open(9600);
  sei();
  uart0_tx_enable_int();
  while (uart0_tx_is_int_enabled()) { }   /* ISR turns UDRIE off when drained */
  _delay_ms(5);
  cli();
  put(' ');
  kv("udre_isr_calls", udre_calls);
  kv("sent", pos);
  done();
}

/* ------------------------------------------------------------------ */
#elif TEST == 7
/* Follow-up to test 2: when main sees RXC set with interrupts on, how long
 * does it stay set, and has the ISR already delivered that byte? */
static volatile uint8_t isr_bytes;
static void on_rx(uint_fast8_t d) { (void)d; isr_bytes++; }

int main(void)
{
  uint16_t lens[8];
  uint8_t  first[8], last[8];
  uint8_t  episodes = 0;
  uint16_t run = 0;

  uart0_on_rx_complete(on_rx);
  uart0_open(PROBE_BAUD);
  sei();

  for (uint32_t i = 0; i < 30000UL; i++) {
    if (uart0_rx_is_ready()) {
      if (run == 0 && episodes < 8) first[episodes] = isr_bytes;
      run++;
    } else if (run) {
      if (episodes < 8) { lens[episodes] = run; last[episodes] = isr_bytes; }
      episodes++;
      run = 0;
    }
  }

  cli();
  uart0_open(9600);
  kv("isr_bytes", isr_bytes);
  kv("episodes", episodes);
  put('\n');
  for (uint8_t i = 0; i < episodes && i < 8; i++) {
    kv("episode", i);
    kv("consecutive_polls_seeing_RXC", lens[i]);
    kv("isr_bytes_at_start", first[i]);
    kv("isr_bytes_at_end", last[i]);
    put('\n');
  }
  done();
}

/* ------------------------------------------------------------------ */
#elif TEST == 8
/* Same situation, but main also READS UDR when it sees RXC (a polled reader
 * competing with the ISR). Who ends up with the bytes? */
static volatile uint8_t isr_bytes;
static volatile char isr_got[8];
static void on_rx(uint_fast8_t d) { if (isr_bytes < 8) isr_got[isr_bytes] = (char)d; isr_bytes++; }

int main(void)
{
  char    polled[8];
  uint8_t pn = 0;

  uart0_on_rx_complete(on_rx);
  uart0_open(9600);
  sei();

  for (uint32_t i = 0; i < 30000UL; i++) {
    if (uart0_rx_is_ready()) {
      uint8_t c = uart0_rx_read8();
      if (pn < 8) polled[pn] = (char)c;
      pn++;
    }
  }

  cli();
  kv("isr_bytes", isr_bytes);
  puts_("isr_data=");
  for (uint8_t i = 0; i < isr_bytes && i < 8; i++) put(isr_got[i]);
  put(' ');
  kv("polled_reads", pn);
  puts_("polled_data=");
  for (uint8_t i = 0; i < pn && i < 8; i++) put(polled[i] ? polled[i] : '.');
  done();
}

#else
#error "pick a TEST"
#endif

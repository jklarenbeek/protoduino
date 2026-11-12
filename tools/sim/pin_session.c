/* pin_session.c - talk to a protoduino shell on a part without a USART.
 *
 * Runs the firmware in simavr and is the other end of a serial line made of two
 * port B pins: it drives the RX pin of the part bit by bit and decodes what the
 * part puts on its TX pin. The line is half duplex, so a character is sent only
 * after the echo of the previous one has arrived, and a command only after the
 * prompt. A command "wait:<ms>" lets simulated time pass.
 *
 * usage: pin_session <mcu> <f_cpu> <firmware.elf> <baud> <command> [<command> ...]
 *
 *   S=<protosim>/libraries/simavr/simavr
 *   gcc -O2 -o pin_session pin_session.c -I$S/sim $S/obj-x86_64-linux-gnu/libsimavr.a -lelf -lm -lpthread
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim_avr.h"
#include "sim_elf.h"
#include "avr_ioport.h"

#define RX_PIN 0 /* PB0: into the part */
#define TX_PIN 1 /* PB1: out of the part */

static avr_t *avr;
static avr_irq_t *rx_irq;
static uint64_t bit;                  /* cycles per bit */

/* ---- what the part sends ---- */
static int tx_level = 1, tx_busy, tx_bits;
static uint8_t tx_shift;
static char seen[4096]; static int seen_len;

static avr_cycle_count_t tx_sample(avr_t *a, avr_cycle_count_t when, void *param)
{
  (void)a; (void)param;
  if (tx_bits < 8) {
    tx_shift = (uint8_t)((tx_shift >> 1) | (tx_level ? 0x80 : 0));
    tx_bits++;
    return when + bit;
  }
  if (seen_len < (int)sizeof seen - 1) seen[seen_len++] = (char)tx_shift;   /* the stop bit */
  putchar(tx_shift == '\r' ? ' ' : tx_shift); fflush(stdout);
  tx_busy = 0;
  return 0;
}

static void tx_change(struct avr_irq_t *irq, uint32_t value, void *param)
{
  (void)irq; (void)param;
  tx_level = value != 0;
  if (!tx_level && !tx_busy) {        /* a start bit */
    tx_busy = 1; tx_bits = 0;
    avr_cycle_timer_register(avr, bit + bit / 2, tx_sample, NULL);
  }
}

/* ---- what the part receives ---- */
static int rx_busy, rx_bits;
static uint16_t rx_frame;

static avr_cycle_count_t rx_drive(avr_t *a, avr_cycle_count_t when, void *param)
{
  (void)a; (void)param;
  if (rx_bits == 10) { rx_busy = 0; return 0; }
  avr_raise_irq(rx_irq, rx_frame & 1);
  rx_frame >>= 1; rx_bits++;
  return when + bit;
}

static void send_byte(uint8_t b)
{
  rx_frame = (uint16_t)(((uint16_t)b << 1) | 0x200);   /* start, 8 data, stop */
  rx_bits = 0; rx_busy = 1;
  avr_cycle_timer_register(avr, 1, rx_drive, NULL);
}

/* run until `what` ends what was seen, or for `ms` of simulated time */
static int run_until(const char *what, double ms)
{
  uint64_t end = avr->cycle + (uint64_t)(ms * avr->frequency / 1000.0);
  size_t n = what ? strlen(what) : 0;
  while (avr->cycle < end) {
    int st = avr_run(avr);
    if (getenv("PIN_SESSION_WATCH")) {       /* debugging aid: report every change of a 16-bit RAM word */
      static long ad = -1; static unsigned last;
      if (ad < 0) ad = strtol(getenv("PIN_SESSION_WATCH"), 0, 0);
      unsigned v = avr->data[ad] | (avr->data[ad + 1] << 8);
      if (v != last) { printf("\n[0x%lx: %04x -> %04x at pc=0x%04x]", ad, last, v, avr->pc); last = v; }
    }
    if (st == cpu_Done || st == cpu_Crashed) { printf("\n[cpu stopped]\n"); exit(1); }
    if (n && !tx_busy && seen_len >= (int)n && !memcmp(seen + seen_len - n, what, n)) return 1;
  }
  return 0;
}

int main(int argc, char **argv)
{
  if (argc < 5) { fprintf(stderr, "usage: pin_session <mcu> <f_cpu> <firmware.elf> <baud> <command>...\n"); return 2; }
  elf_firmware_t f = {{0}};
  if (elf_read_firmware(argv[3], &f)) { fprintf(stderr, "cannot read %s\n", argv[3]); return 2; }
  avr = avr_make_mcu_by_name(argv[1]);
  if (!avr) { fprintf(stderr, "unknown mcu %s\n", argv[1]); return 2; }
  avr_init(avr);
  avr->frequency = (uint32_t)strtoul(argv[2], 0, 0);
  avr->log = LOG_ERROR;
  avr_load_firmware(avr, &f);
  bit = avr->frequency / strtoul(argv[4], 0, 0);

  rx_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), RX_PIN);
  avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), TX_PIN), tx_change, NULL);
  avr_raise_irq(rx_irq, 1);                              /* the line idles high */

  run_until(NULL, 50);                                   /* the system starts */
  send_byte('\r');
  if (!run_until("\n> ", 2000)) { printf("\n[no prompt]\n"); return 1; }

  for (int i = 5; i < argc; i++) {
    if (!strncmp(argv[i], "wait:", 5)) {
      run_until(NULL, atof(argv[i] + 5));
      send_byte('\r');
      run_until("\n> ", 2000);
      continue;
    }
    run_until(NULL, 2);                                  /* the prompt has only just gone out */
    for (const char *c = argv[i]; *c; c++) {
      int before = seen_len;
      send_byte((uint8_t)*c);
      uint64_t end = avr->cycle + avr->frequency / 5;    /* the echo, within 200 ms */
      while (avr->cycle < end && (seen_len == before || tx_busy || rx_busy)) avr_run(avr);
      if (seen_len == before) { printf("\n[no echo of '%c']\n", *c); return 1; }
      run_until(NULL, 2);                                /* a person does not answer within a bit time */
    }
    send_byte('\r');
    if (!run_until("\n> ", 4000)) {
      printf("\n[no prompt after \"%s\"]\n", argv[i]);
      if (getenv("PIN_SESSION_DEBUG")) {
        printf("pc=0x%04x I=%d sp=0x%04x tx_busy=%d rx_busy=%d\n", avr->pc, avr->sreg[7], avr->data[0x5d] | (avr->data[0x5e] << 8), tx_busy, rx_busy);
        for (const char *d = getenv("PIN_SESSION_DUMP"); d && *d; ) {
          char *e; long ad = strtol(d, &e, 0), n = strtol(e + 1, &e, 0);
          printf(" ram 0x%lx:", ad); for (long j = 0; j < n; j++) printf(" %02x", avr->data[ad + j]); printf("\n");
          d = *e ? e + 1 : e;
        }
        for (int k = 0; k < 4; k++) { for (int j = 0; j < 40000; j++) avr_run(avr); printf(" pc=0x%04x I=%d ovf=%u TIMSK=%02x TCCR0B=%02x TCNT0=%02x TIFR=%02x GIMSK=%02x cycle=%llu\n", avr->pc, avr->sreg[7], avr->data[0x7f] | (avr->data[0x80] << 8), avr->data[0x59], avr->data[0x53], avr->data[0x52], avr->data[0x58], avr->data[0x5b], (unsigned long long)avr->cycle); }
      }
      return 1;
    }
  }
  printf("\n[%.1f s of simulated time]\n", avr->cycle / (double)avr->frequency);
  return 0;
}

/* sim_harness.c - drive the prototype loader inside simavr.
 * Sends a file with XMODEM-CRC exactly as a terminal program would, then
 * compares the simulated flash with what simavr's own ELF reader produces
 * for the same file (the oracle).
 *
 * usage: sim_harness <mcu> <boot.elf> <app.elf> [--corrupt N] [--drop-ack N]
 *                    [--send <file>] [--max-mcycles M] [--expect-refusal]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "sim_avr.h"
#include "sim_elf.h"
#include "avr_uart.h"
#include "sim_io.h"
#include "avr_ioport.h"
#include "avr_eeprom.h"

enum { SOH = 1, EOT = 4, ACK = 6, NAK = 0x15, CAN = 0x18 };

static avr_t *avr;
static avr_irq_t *rx_irq;
static uint8_t *file; static size_t file_len, nblocks;
static uint8_t txq[140]; static int txq_len, txq_pos;
static unsigned cur_blk = 0;            /* 0 = not started, 1..n = block being sent, n+1 = EOT */
static int corrupt_blk = -1, drop_ack_blk = -1, drop_done, corrupt_done;
static unsigned naks, resends, cans, c_probes, acks, dup_sent;
static int finished, refused = -1, eot_acked, after_can;
static uint64_t t_first_block, t_eot_ack;
static char devtext[256]; static int devtext_len;

static uint16_t crc16(const uint8_t *p, int n)
{
  uint16_t c = 0;
  while (n--) { c ^= (uint16_t)*p++ << 8; for (int i = 0; i < 8; i++) c = c & 0x8000 ? (c << 1) ^ 0x1021 : c << 1; }
  return c;
}

static void queue_block(unsigned b, int corrupt)
{
  uint8_t *q = txq;
  size_t off = (size_t)(b - 1) * 128;
  q[0] = SOH; q[1] = (uint8_t)b; q[2] = (uint8_t)~b;
  for (int i = 0; i < 128; i++) q[3 + i] = off + i < file_len ? file[off + i] : 0x1A;
  uint16_t c = crc16(q + 3, 128);
  q[131] = c >> 8; q[132] = (uint8_t)c;
  if (corrupt) q[3 + 77] ^= 0x10;
  txq_len = 133; txq_pos = 0;
}

static void queue_byte(uint8_t v) { txq[0] = v; txq_len = 1; txq_pos = 0; }

static avr_cycle_count_t pump(avr_t *a, avr_cycle_count_t when, void *param)
{
  (void)param;
  if (txq_pos < txq_len) avr_raise_irq(rx_irq, txq[txq_pos++]);
  return when + (avr_cycle_count_t)(a->frequency / 11520 * 1.15);   /* one 115200 8N1 byte time + margin */
}

static void from_device(struct avr_irq_t *irq, uint32_t value, void *param)
{
  (void)irq; (void)param;
  uint8_t v = (uint8_t)value;
  if (after_can) {                       /* CAN CAN <reason> */
    if (v == CAN) return;
    refused = v; finished = 1; return;
  }
  if (cur_blk == 0) {
    if (v == 'C') { c_probes++; cur_blk = 1; t_first_block = avr->cycle; queue_block(1, corrupt_blk == 1 && !corrupt_done++); }
    else if (devtext_len < 255) devtext[devtext_len++] = (char)v;
    return;
  }
  if (v == 'C') { c_probes++; return; }
  if (v == CAN) { cans++; after_can = 1; return; }
  if (v == NAK) { naks++; resends++;
    if (cur_blk > nblocks) queue_byte(EOT); else queue_block(cur_blk, 0);
    return; }
  if (v == ACK) {
    acks++;
    if (cur_blk > nblocks) { eot_acked = 1; t_eot_ack = avr->cycle; finished = 1; return; }
    if ((int)cur_blk == drop_ack_blk && !drop_done) {     /* pretend the ACK was lost: send the block again */
      drop_done = 1; dup_sent++; resends++;
      queue_block(cur_blk, 0);
      return;
    }
    cur_blk++;
    if (cur_blk > nblocks) queue_byte(EOT);
    else queue_block(cur_blk, (int)cur_blk == corrupt_blk && !corrupt_done++);
  }
}

int main(int argc, char **argv)
{
  if (argc < 4) { fprintf(stderr, "usage\n"); return 2; }
  const char *mcu = argv[1], *boot = argv[2], *app = argv[3], *send = argv[3];
  uint64_t max_cycles = 4000ull * 1000000ull; int expect_refusal = 0; const char *then_send = NULL; long check_page = -1; long fall_addr = -1, fall_r31 = 0;
  for (int i = 4; i < argc; i++) {
    if (!strcmp(argv[i], "--corrupt")) corrupt_blk = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--drop-ack")) drop_ack_blk = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--send")) send = argv[++i];
    else if (!strcmp(argv[i], "--max-mcycles")) max_cycles = strtoull(argv[++i], 0, 0) * 1000000ull;
    else if (!strcmp(argv[i], "--expect-refusal")) expect_refusal = 1;
    else if (!strcmp(argv[i], "--then-send")) then_send = argv[++i];
    else if (!strcmp(argv[i], "--check-page")) check_page = strtoul(argv[++i], 0, 0);
    else if (!strcmp(argv[i], "--fallthrough")) { fall_addr = strtol(argv[++i], 0, 0); fall_r31 = strtol(argv[++i], 0, 0); }
  }

  FILE *f = fopen(send, "rb"); if (!f) { perror(send); return 2; }
  file = malloc(1 << 22); file_len = fread(file, 1, 1 << 22, f); fclose(f);
  nblocks = (file_len + 127) / 128;

  elf_firmware_t bl = {{0}}, ap = {{0}};
  if (elf_read_firmware(boot, &bl) || elf_read_firmware(app, &ap)) { fprintf(stderr, "cannot read ELF\n"); return 2; }

  /* simavr's own ELF reader gives the reference images */
  avr_t *ob = avr_make_mcu_by_name(mcu), *oa = avr_make_mcu_by_name(mcu);
  if (!ob || !oa) { fprintf(stderr, "unknown mcu %s\n", mcu); return 2; }
  avr_init(ob); avr_init(oa);
  ob->log = oa->log = LOG_ERROR;
  memset(ob->flash, 0xFF, ob->flashend + 1); memset(oa->flash, 0xFF, oa->flashend + 1);
  avr_load_firmware(ob, &bl); avr_load_firmware(oa, &ap);
  uint32_t boot_start = 0, page = 0;
  while (boot_start <= ob->flashend && ob->flash[boot_start] == 0xFF) boot_start++;   /* first loader byte */
  if (boot_start == 0 || boot_start > ob->flashend) { fprintf(stderr, "loader has no base address\n"); return 2; }
  uint32_t boot_size = 0;
  for (uint32_t i = boot_start; i <= ob->flashend; i++) if (ob->flash[i] != 0xFF) boot_size = i + 1 - boot_start;

  avr = avr_make_mcu_by_name(mcu);
  avr_init(avr);
  avr->frequency = 16000000;
  avr->log = LOG_ERROR;
  memset(avr->flash, 0xFF, avr->flashend + 1);
  memcpy(avr->flash + boot_start, ob->flash + boot_start, avr->flashend + 1 - boot_start);
  avr->pc = avr->reset_pc = boot_start;            /* BOOTRST: reset enters the loader */
  avr->codeend = avr->flashend;
  page = avr->flashend > 0xFFFF ? 256 : (avr->flashend > 0x1FFF ? 128 : 64);
  uint32_t rec = boot_start - page;

  uint32_t fl = 0;
  avr_ioctl(avr, AVR_IOCTL_UART_GET_FLAGS('0'), &fl);
  fl &= ~AVR_UART_FLAG_STDIO; fl &= ~AVR_UART_FLAG_POLL_SLEEP;
  avr_ioctl(avr, AVR_IOCTL_UART_SET_FLAGS('0'), &fl);
  rx_irq = avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_INPUT);
  avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUTPUT), from_device, NULL);
  avr_cycle_timer_register(avr, 2000, pump, NULL);

  /* Erased-flash test: start executing in erased flash with a chosen r31 and
   * see whether execution arrives in the loader (its first 'C' probe proves it). */
  if (fall_addr >= 0) {
    avr->pc = (avr_flashaddr_t)fall_addr;
    avr->data[31] = (uint8_t)fall_r31;
    cur_blk = 0xFFFF;                               /* never answer the probe */
    int st = cpu_Running;
    while (avr->cycle < 40000000ull && !c_probes) {
      st = avr_run(avr);
      if (st == cpu_Done || st == cpu_Crashed) break;
    }
    printf("enter erased flash at 0x%lx with r31=0x%02lx: %s\n", fall_addr, fall_r31,
           c_probes ? "loader reached (it sent its probe)" : (st == cpu_Crashed ? "CPU crashed" : "no probe"));
    return c_probes ? 0 : 1;
  }

  int state = cpu_Running; uint64_t app_pc_seen = 0;
  while (avr->cycle < max_cycles) {
    state = avr_run(avr);
    if (state == cpu_Done || state == cpu_Crashed) break;
    if (finished && refused >= 0) break;
    if (eot_acked && avr->pc < boot_start) { app_pc_seen = avr->cycle; break; }
  }

  /* optional second session: reset, offer another file, see what survives */
  if (then_send && app_pc_seen) {
    printf("first install ok; resetting and sending %s\n", then_send);
    f = fopen(then_send, "rb"); if (!f) { perror(then_send); return 2; }
    file_len = fread(file, 1, 1 << 22, f); fclose(f);
    nblocks = (file_len + 127) / 128;
    cur_blk = 0; finished = 0; refused = -1; eot_acked = 0; after_can = 0; txq_len = txq_pos = 0;
    avr_reset(avr); avr->pc = boot_start;
    avr_cycle_timer_register(avr, 2000, pump, NULL);      /* reset drops pending timers */
    uint64_t t0 = avr->cycle; app_pc_seen = 0; int saw_refusal = -1;
    while (avr->cycle < t0 + max_cycles) {
      state = avr_run(avr);
      if (state == cpu_Done || state == cpu_Crashed) break;
      if (refused >= 0 && saw_refusal < 0) { saw_refusal = refused; txq_len = txq_pos = 0; cur_blk = 0xFFFF; }
      if (avr->pc < boot_start) { app_pc_seen = avr->cycle; break; }
    }
    printf("second session: refusal code=%s0x%02x, previous app started again=%s (%.1f s after reset)\n",
           saw_refusal < 0 ? "none " : "", saw_refusal < 0 ? 0 : saw_refusal, app_pc_seen ? "yes" : "no", (avr->cycle - t0) / 16e6);
    refused = -1;
  }

  if (check_page >= 0 && app_pc_seen) {
    uint64_t t0 = avr->cycle;
    while (avr->cycle < t0 + 3000000) avr_run(avr);          /* let the application run */
    printf("application ran; flash page @0x%lx:", check_page);
    for (int i = 0; i < 8; i++) printf(" %02x", avr->flash[check_page + i]);
    printf(" ... DDRB=%02x\n", avr->data[0x24]);
  }

  size_t mism = 0, first = 0, image = 0;
  for (size_t i = 0; i < rec; i++) {
    if (oa->flash[i] != 0xFF) image = i + 1;
    if (avr->flash[i] != oa->flash[i]) { if (!mism) first = i; mism++; }
  }

  printf("mcu=%s loader=%u bytes at 0x%x  file=%zu bytes (%zu blocks)\n", mcu, boot_size, boot_start, file_len, nblocks);
  printf("C-probes=%u acks=%u naks=%u resends=%u duplicate-blocks-sent=%u cancels=%u\n", c_probes, acks, naks, resends, dup_sent, cans);
  if (refused >= 0) printf("device refused the file: code 0x%02x\n", refused);
  printf("transfer: %.1f ms of simulated time (%llu cycles); app reached=%s\n",
         t_eot_ack ? (t_eot_ack - t_first_block) / 16000.0 : -1.0, (unsigned long long)(t_eot_ack - t_first_block), app_pc_seen ? "yes" : "no");
  printf("flash vs simavr's ELF reader: image %zu bytes, region 0..0x%x compared, %zu mismatches%s\n", image, rec, mism, mism ? " (FAIL)" : "");
  if (mism) printf("first mismatch at 0x%zx: got %02x want %02x\n", first, avr->flash[first], oa->flash[first]);
  printf("record @0x%x:", rec); for (int i = 0; i < 11; i++) printf(" %02x", avr->flash[rec + i]); printf("\n");
  if (devtext_len) printf("device text: %.*s\n", devtext_len, devtext);
  { uint8_t ee[5] = {0}; avr_eeprom_desc_t d = { .ee = ee, .offset = 0, .size = 5 };
    avr_ioctl(avr, AVR_IOCTL_EEPROM_GET, &d);
    printf("eeprom[0..4]: %02x %02x %02x %02x %02x\n", ee[0], ee[1], ee[2], ee[3], ee[4]); }
  if (expect_refusal) return refused >= 0 && !app_pc_seen ? 0 : 1;
  return (mism == 0 && app_pc_seen) ? 0 : 1;
}

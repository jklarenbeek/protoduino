/* boot.c - prototype ELF bootloader built around the streaming core.
 *
 *   transport : XMODEM-CRC over a polled UART (any terminal program can send)
 *   payload   : the unmodified .elf file produced by avr-gcc
 *   safety    : image record written last; no record -> stay in the loader
 *
 * One translation unit ("unity build") so the sink is inlined into the core.
 * Build-time switches:
 *   BOOT_START                 byte address the loader is linked at
 *   ELFBOOT_FEAT_IMAGE_CRC     store a CRC-16 of the image in the record
 *   ELFBOOT_FEAT_VERIFY_BOOT   re-check that CRC on every start
 *   ELFBOOT_FEAT_INFO          answer '?' with a capability line
 *   ELFBOOT_FEAT_SERVICE       export a page-write service to the application
 *   ELFBOOT_VIRTUAL_RESET      part has no boot section: patch the reset vector
 *   ELFBOOT_SOFT_UART          part has no USART: bit-banged serial
 *   ELFBOOT_FEAT_LANDING_PAD   one-word no-op at the loader's first address (default on)
 */
#include <avr/io.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/crc16.h>
#include <util/delay_basic.h>

#ifndef BOOT_START
#error "define BOOT_START (byte address the loader is linked at)"
#endif
#ifndef BAUD
#define BAUD 115200UL
#endif
#ifndef ELFBOOT_FEAT_IMAGE_CRC
#define ELFBOOT_FEAT_IMAGE_CRC 1
#endif
#ifndef ELFBOOT_FEAT_VERIFY_BOOT
#define ELFBOOT_FEAT_VERIFY_BOOT 0
#endif
#ifndef ELFBOOT_FEAT_INFO
#define ELFBOOT_FEAT_INFO 0
#endif
#ifndef ELFBOOT_FEAT_SERVICE
#define ELFBOOT_FEAT_SERVICE 0
#endif
#ifndef ELFBOOT_VIRTUAL_RESET
#define ELFBOOT_VIRTUAL_RESET 0
#endif
#ifndef ELFBOOT_SOFT_UART
#define ELFBOOT_SOFT_UART 0
#endif
#ifndef ELFBOOT_FEAT_LANDING_PAD
#define ELFBOOT_FEAT_LANDING_PAD 1    /* 0 only for the control experiment */
#endif
#ifndef ELFBOOT_CONF_TIMEOUT_POLLS
#define ELFBOOT_CONF_TIMEOUT_POLLS (F_CPU / 10)   /* about one second of polling */
#endif
#ifndef ELFBOOT_CONF_WINDOW
#define ELFBOOT_CONF_WINDOW 3      /* 'C' probes before starting a valid image */
#endif

#define ELFLD_SINK(space, addr, b) sink_put((space), (addr), (b))
#include "elfld.h"
static uint8_t sink_put(uint8_t space, elfld_addr_t addr, uint8_t b);
#include "elfld.c"

#define REC_ADDR   ((elfld_addr_t)(BOOT_START - SPM_PAGESIZE))   /* image record page */
#define REC_MAGIC0 'p'
#define REC_MAGIC1 'D'

#if FLASHEND > 0xFFFFUL
#define FLASH_RD(a) pgm_read_byte_far(a)
#else
#define FLASH_RD(a) pgm_read_byte(a)
#endif

#if SPM_PAGESIZE > 255
typedef uint16_t pgidx_t;
#else
typedef uint8_t pgidx_t;
#endif

/* ---------------------------------------------------------------- state */
static uint8_t      pg[SPM_PAGESIZE];   /* the one page being assembled      */
static elfld_addr_t pg_base;
static uint8_t      pg_live;
static uint8_t      dirty;              /* flash has been modified this session */
static elfld_addr_t img_end;            /* highest flash address written + 1 */
static uint16_t     app_entry;          /* word address to start the image   */
static uint8_t      timed_out;
static elfld_t      ld;
static uint8_t      blk[128];

/* ---------------------------------------------------------------- serial */
#if ELFBOOT_SOFT_UART
/* half-duplex bit-banged 8N1; RX = PB0, TX = PB1 */
#define SU_BIT ((uint8_t)((F_CPU / BAUD - 8) / 3))
static void uart_init(void) { PORTB |= _BV(PB1) | _BV(PB0); DDRB |= _BV(PB1); }
static void putch(uint8_t c)
{
  uint16_t f = ((uint16_t)c << 1) | 0x200;          /* start, 8 data, stop */
  for (uint8_t i = 10; i; i--) {
    if (f & 1) PORTB |= _BV(PB1); else PORTB &= ~_BV(PB1);
    f >>= 1;
    _delay_loop_1(SU_BIT);
  }
}
static uint8_t getch(void)
{
  __uint24 n = ELFBOOT_CONF_TIMEOUT_POLLS;
  uint8_t c = 0;
  while (PINB & _BV(PB0)) { if (--n == 0) { timed_out = 1; return 0; } }
  _delay_loop_1(SU_BIT / 2);
  for (uint8_t i = 8; i; i--) {
    _delay_loop_1(SU_BIT);
    c >>= 1;
    if (PINB & _BV(PB0)) c |= 0x80;
  }
  _delay_loop_1(SU_BIT);
  return c;
}
#else
static void uart_init(void)
{
  UCSR0A = _BV(U2X0);
  UBRR0H = 0;
  UBRR0L = (uint8_t)((F_CPU / 4 / BAUD - 1) / 2);
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);     /* polled: no interrupt enables */
}
static void putch(uint8_t c)
{
  while (!(UCSR0A & _BV(UDRE0))) { }
  UDR0 = c;
}
static uint8_t getch(void)
{
  __uint24 n = ELFBOOT_CONF_TIMEOUT_POLLS;
  while (!(UCSR0A & _BV(RXC0))) { if (--n == 0) { timed_out = 1; return 0; } }
  return UDR0;
}
#endif

/* ---------------------------------------------------------------- flash */
static void page_erase(elfld_addr_t a)
{
  boot_page_erase(a);
  boot_spm_busy_wait();
}

/* Erase and program one page from a buffer. Touches no loader variables, so
 * it is safe to reach from the application through the service entry. */
static void page_program(elfld_addr_t base, const uint8_t *data) __attribute__((noinline));
static void page_program(elfld_addr_t base, const uint8_t *data)
{
  pgidx_t i = 0;

  page_erase(base);
  do {
    boot_page_fill(base + i, (uint16_t)data[i] | ((uint16_t)data[i + 1] << 8));
    i += 2;
  } while (i != (pgidx_t)SPM_PAGESIZE);
  boot_page_write(base);
  boot_spm_busy_wait();
#ifdef RWWSRE
  boot_rww_enable();
#endif
}

static void page_commit(void)
{
  if (!pg_live) return;
  pg_live = 0;

#if ELFBOOT_VIRTUAL_RESET
  /* No hardware boot section: every reset must still reach the loader.
   * Keep the image's own start address and aim its reset vector at us. */
  if (pg_base == 0) {
#  if FLASHEND > 0x1FFFU
    app_entry = (uint16_t)pg[2] | ((uint16_t)pg[3] << 8);         /* jmp k  */
    pg[2] = (uint8_t)((BOOT_START / 2) & 0xFF);
    pg[3] = (uint8_t)((BOOT_START / 2) >> 8);
#  else
    uint16_t w = (uint16_t)pg[0] | ((uint16_t)pg[1] << 8);        /* rjmp k */
    app_entry = (w + 1) & 0x0FFF;
    w = 0xC000 | ((BOOT_START / 2 - 1) & 0x0FFF);
    pg[0] = (uint8_t)w;
    pg[1] = (uint8_t)(w >> 8);
#  endif
  }
#endif

  if (!dirty) {
    /* First change to flash. The headers have been accepted by now, so the
     * installed image stops being valid before a single byte of it changes. */
    dirty = 1;
#if ELFBOOT_VIRTUAL_RESET
    /* Top-down, page 0 last: at every instant a reset reaches the loader,
     * either through the old vector or by running through erased flash. */
    {
      elfld_addr_t a = REC_ADDR;
      for (;;) { page_erase(a); if (a == 0) break; a -= SPM_PAGESIZE; }
    }
#else
    page_erase(REC_ADDR);
#endif
  }
  if (pg_base >= img_end) img_end = pg_base + SPM_PAGESIZE;   /* page granular */
  page_program(pg_base, pg);
}

static void page_open(elfld_addr_t base)
{
  pgidx_t i = 0;
  page_commit();
  do { pg[i] = 0xFF; } while (++i != (pgidx_t)SPM_PAGESIZE);
  pg_base = base;
  pg_live = 1;
}

static uint8_t sink_put(uint8_t space, elfld_addr_t addr, uint8_t b)
{
#if ELFLD_FEAT_EEPROM
  if (space == ELFLD_SPACE_EEPROM) {
    eeprom_update_byte((uint8_t *)(uint16_t)addr, b);
    return 0;
  }
#endif
#if ELFLD_FEAT_SIGNATURE
  if (space == ELFLD_SPACE_SIG) {       /* file bytes are stored sig2, sig1, sig0 */
    return boot_signature_byte_get((uint8_t)(4 - 2 * (uint8_t)addr)) != b;
  }
#endif
  (void)space;
  {
    elfld_addr_t base = addr & ~(elfld_addr_t)(SPM_PAGESIZE - 1);
    if (!pg_live || base != pg_base) page_open(base);
    pg[(pgidx_t)addr & (SPM_PAGESIZE - 1)] = b;
  }
  return 0;
}

/* ---------------------------------------------------------------- record */
/* Image record: [0..1] magic  [2..3] entry (word address)
 *               [4..6] image end  [7..8] image CRC-16  [9..10] record CRC-16 */
#define REC_LEN 11

static uint16_t crc_flash(elfld_addr_t from, elfld_addr_t to) __attribute__((noinline));
static uint16_t crc_flash(elfld_addr_t from, elfld_addr_t to)
{
  uint16_t crc = 0;
  while (from != to) crc = _crc_xmodem_update(crc, FLASH_RD(from++));
  return crc;
}

static uint8_t record_ok(void)
{
  if (FLASH_RD(REC_ADDR) != REC_MAGIC0 || FLASH_RD(REC_ADDR + 1) != REC_MAGIC1) return 0;
  if (crc_flash(REC_ADDR, REC_ADDR + REC_LEN) != 0) return 0;   /* CRC over data+CRC is 0 */
#if ELFBOOT_FEAT_VERIFY_BOOT
  {
    elfld_addr_t end = (elfld_addr_t)FLASH_RD(REC_ADDR + 4) |
                       ((elfld_addr_t)FLASH_RD(REC_ADDR + 5) << 8)
#  if ELFLD_CONF_ADDR_BITS > 16
                       | ((elfld_addr_t)FLASH_RD(REC_ADDR + 6) << 16)
#  endif
                       ;
    uint16_t want = (uint16_t)FLASH_RD(REC_ADDR + 8) << 8 | FLASH_RD(REC_ADDR + 7);
    if (crc_flash(0, end) != want) return 0;
  }
#endif
  return 1;
}

static void record_write(void)
{
  uint8_t  r[REC_LEN];
  uint16_t crc = 0;
  uint8_t  i;

  r[0] = REC_MAGIC0;  r[1] = REC_MAGIC1;
  r[2] = (uint8_t)app_entry;  r[3] = (uint8_t)(app_entry >> 8);
  r[4] = (uint8_t)img_end;    r[5] = (uint8_t)(img_end >> 8);
#if ELFLD_CONF_ADDR_BITS > 16
  r[6] = (uint8_t)(img_end >> 16);
#else
  r[6] = 0;
#endif
#if ELFBOOT_FEAT_IMAGE_CRC
  crc = crc_flash(0, img_end);
#endif
  r[7] = (uint8_t)crc;  r[8] = (uint8_t)(crc >> 8);
  crc = 0;
  for (i = 0; i < REC_LEN - 2; i++) crc = _crc_xmodem_update(crc, r[i]);
  r[9] = (uint8_t)(crc >> 8);  r[10] = (uint8_t)crc;

  page_open(REC_ADDR);
  for (i = 0; i < REC_LEN; i++) pg[i] = r[i];
  page_commit();
}

/* ---------------------------------------------------------------- service */
#if ELFBOOT_FEAT_SERVICE
/* Lets the application program one page although SPM only works from here.
 * Reached through the fixed stub at the very end of flash. Uses the caller's
 * buffer and stack only: the loader's own variables are not valid while the
 * application is running. */
void bootsvc_write_page(uint32_t addr, const uint8_t *data) __attribute__((used, noinline));
void bootsvc_write_page(uint32_t addr, const uint8_t *data)
{
  uint8_t sreg = SREG;
  __asm__ __volatile__("cli");            /* the caller's interrupt handlers live in the section being programmed */
  page_program((elfld_addr_t)addr & ~(elfld_addr_t)(SPM_PAGESIZE - 1), data);
  SREG = sreg;
}
#endif

/* ---------------------------------------------------------------- start image */
static void start_image(void) __attribute__((noreturn));
static void start_image(void)
{
#if !ELFBOOT_SOFT_UART
  UCSR0B = 0;
#endif
#ifdef RAMPZ
  RAMPZ = 0;
#endif
  {
    uint16_t e = (uint16_t)FLASH_RD(REC_ADDR + 2) | ((uint16_t)FLASH_RD(REC_ADDR + 3) << 8);
#ifdef EIND
    EIND = 0;
#endif
    ((void (*)(void))e)();
  }
  for (;;) { }
}

/* ---------------------------------------------------------------- XMODEM-CRC */
#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

static void boot_main(void) __attribute__((noreturn, used));
static void boot_main(void)
{
  uint8_t expect, result, started, tries;

  uart_init();

restart:                                  /* a fresh session; nothing has been touched yet */
  expect  = 1;
  result  = ELFLD_MORE;
  started = 0;
  tries   = ELFBOOT_CONF_WINDOW;
  pg_live = 0;
  dirty   = 0;
  elfld_begin(&ld, 0, REC_ADDR);

  for (;;) {
    uint8_t c, n, nn, i;
    uint16_t crc;

    timed_out = 0;
    c = getch();
    if (timed_out) {
      if (!started) {
        if (tries-- == 0 && record_ok()) start_image();
        putch('C');                         /* ask the sender for CRC mode */
      } else {
        putch(NAK);
      }
      continue;
    }
#if ELFBOOT_FEAT_INFO
    if (c == '?' && !started) {
      static const char info[] PROGMEM = "pD1 " ELFBOOT_INFO_STRING "\r\n";
      const char *p = info;
      while ((c = pgm_read_byte(p++)) != 0) putch(c);
      continue;
    }
#endif
    if (c == EOT) {
      putch(ACK);
      break;
    }
    if (c != SOH) continue;

    n  = getch();
    nn = getch();
    crc = 0;
    for (i = 0; i < 128; i++) {
      c = getch();
      blk[i] = c;
      crc = _crc_xmodem_update(crc, c);
    }
    crc = _crc_xmodem_update(crc, getch());
    crc = _crc_xmodem_update(crc, getch());     /* data + CRC must sum to 0 */
    if (timed_out || crc != 0 || (uint8_t)(n + nn) != 0xFF) {
      putch(NAK);
      continue;
    }
    if (n == expect) {
      started = 1;
      if (result == ELFLD_MORE) result = elfld_write(&ld, blk, 128);
      expect++;
    } else if (n != (uint8_t)(expect - 1)) {     /* a repeat is acknowledged, not applied */
      result = ELFLD_E_ORDER;
    }
    if (result & 0x80) {                         /* refused: say why, then start over */
      putch(CAN);
      putch(CAN);
      putch(result);
      if (dirty) for (;;) { }                    /* flash already changed: stay in the loader */
      goto restart;
    }
    putch(ACK);
  }

  if (result == ELFLD_DONE) {
    page_commit();
#if !ELFBOOT_VIRTUAL_RESET
    app_entry = (uint16_t)(ld.entry >> 1);
#endif
    record_write();
    start_image();
  }
  if (!dirty) goto restart;
  for (;;) { }      /* incomplete: no record, so the next reset lands here again */
}

/* ---------------------------------------------------------------- entry */
/* No C runtime (-nostartfiles): the first word of the loader is a landing pad,
 * then the usual .init chain sets up r1, SREG and the stack, libgcc clears
 * .bss in .init4, and .init9 enters boot_main. */
void boot_vec(void) __attribute__((naked, used, section(".vectors")));
void boot_vec(void)
{
  __asm__ __volatile__(
#if ELFBOOT_FEAT_LANDING_PAD
    "nop\n"                /* erased flash (0xFFFF) may skip one word on the way in */
#endif
    "rjmp boot_init\n");
}
void boot_init(void) __attribute__((naked, used, section(".init0")));
void boot_init(void)
{
  __asm__ __volatile__(
    "clr r1\n"
    "out 0x3f, r1\n");     /* SREG = 0: interrupts stay off for the whole loader */
  SP = RAMEND;
}
void boot_go(void) __attribute__((naked, used, section(".init9")));
void boot_go(void)
{
  __asm__ __volatile__("rjmp boot_main");
}

#if ELFBOOT_FEAT_SERVICE
/* Fixed address: last 4 bytes of flash. The application calls FLASHEND-3. */
void bootsvc_stub(void) __attribute__((naked, used, section(".bootsvc")));
void bootsvc_stub(void)
{
  __asm__ __volatile__("rjmp bootsvc_write_page\n" ".byte 'p', 'D'\n");
}
#endif

/* boot.c - prototype ELF bootloader built around the streaming core.
 *
 *   transport : XMODEM-CRC over a polled UART (any terminal program can send)
 *   payload   : the unmodified .elf file produced by avr-gcc
 *   safety    : image record written last; no record -> stay in the loader
 *   services  : the running system can call back in, to program a page or to
 *               have a module loaded into a flash slot (table at the end of flash)
 *
 * One translation unit ("unity build") so the sink is inlined into the core.
 * Build-time switches:
 *   BOOT_START                 byte address the loader is linked at
 *   ELFBOOT_FEAT_IMAGE_CRC     store a CRC-16 of the image in the record
 *   ELFBOOT_FEAT_VERIFY_BOOT   re-check that CRC on every start
 *   ELFBOOT_FEAT_INFO          answer '?' with a capability line
 *   ELFBOOT_FEAT_SERVICE       export a page-write entry to the application
 *   ELFBOOT_FEAT_MODULES       export the whole load session, so the running
 *                              system can have a module loaded (implies SERVICE)
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
#ifndef ELFBOOT_FEAT_MODULES
#define ELFBOOT_FEAT_MODULES 0
#endif
#ifndef ELFBOOT_FEAT_SERVICE
#define ELFBOOT_FEAT_SERVICE ELFBOOT_FEAT_MODULES
#endif
#if ELFBOOT_FEAT_MODULES && !ELFBOOT_FEAT_SERVICE
#error "ELFBOOT_FEAT_MODULES needs ELFBOOT_FEAT_SERVICE"
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
#if ELFBOOT_FEAT_MODULES && !defined(ELFLD_FEAT_WINDOW_LO)
#define ELFLD_FEAT_WINDOW_LO 1     /* a module slot has a lower bound too */
#endif

#define SESS_FEAT_IMAGE 1
#if !ELFBOOT_FEAT_MODULES
#define SESS_STATIC_WS ws               /* only the loader itself ever runs a session */
#endif
#define SESS_OTHER_SPACE(space, addr, b) other_space((space), (uint16_t)(addr), (b))
static uint8_t other_space(uint8_t space, uint16_t addr, uint8_t b);
#include "elfsess.h"

#define REC_ADDR   ((elfld_addr_t)(BOOT_START - SPM_PAGESIZE))   /* image record page */
#define REC_MAGIC0 'p'
#define REC_MAGIC1 'D'

#if FLASHEND > 0xFFFFUL
#define FLASH_RD(a) pgm_read_byte_far(a)
#else
#define FLASH_RD(a) pgm_read_byte(a)
#endif

#if ELFBOOT_FEAT_MODULES
static elfsess_t ws;                    /* the loader's own workspace; callers of the service bring theirs */
#endif

/* ---------------------------------------------------------------- serial */
#if ELFBOOT_SOFT_UART
/* half-duplex bit-banged 8N1; RX = PB0, TX = PB1 */
#define SU_BIT ((uint8_t)((F_CPU / BAUD - 8) / 3))
static void uart_init(void) { PORTB |= _BV(PB1) | _BV(PB0); DDRB |= _BV(PB1); }
static void sess_putch(uint8_t c)
{
  uint16_t f = ((uint16_t)c << 1) | 0x200;          /* start, 8 data, stop */
  for (uint8_t i = 10; i; i--) {
    if (f & 1) PORTB |= _BV(PB1); else PORTB &= ~_BV(PB1);
    f >>= 1;
    _delay_loop_1(SU_BIT);
  }
}
static uint8_t sess_getch(elfsess_t *w)
{
  __uint24 n = ELFBOOT_CONF_TIMEOUT_POLLS;
  uint8_t c = 0;
  while (PINB & _BV(PB0)) { if (--n == 0) { w->timed_out = 1; return 0; } }
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
static void sess_putch(uint8_t c)
{
  while (!(UCSR0A & _BV(UDRE0))) { }
  UDR0 = c;
}
static uint8_t sess_getch(elfsess_t *w)
{
  __uint24 n = ELFBOOT_CONF_TIMEOUT_POLLS;
  while (!(UCSR0A & _BV(RXC0))) { if (--n == 0) { w->timed_out = 1; return 0; } }
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
static void sess_page_program(elfld_addr_t base, const uint8_t *data) __attribute__((noinline));
static void sess_page_program(elfld_addr_t base, const uint8_t *data)
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

/* The installed image stops being valid before a single byte of it changes. */
static void sess_first_write(elfsess_t *w)
{
  if (w->mode != SESS_IMAGE) return;        /* a module load leaves the image record alone */
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

static uint8_t other_space(uint8_t space, uint16_t addr, uint8_t b)
{
#if ELFLD_FEAT_EEPROM
  if (space == ELFLD_SPACE_EEPROM) {
    eeprom_update_byte((uint8_t *)addr, b);
    return 0;
  }
#endif
#if ELFLD_FEAT_SIGNATURE
  if (space == ELFLD_SPACE_SIG) {       /* file bytes are stored sig2, sig1, sig0 */
    return boot_signature_byte_get((uint8_t)(4 - 2 * (uint8_t)addr)) != b;
  }
#endif
  (void)space; (void)addr; (void)b;
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

static void record_write(elfsess_t *w)
{
  uint8_t *r = w->pg;                    /* the page buffer is free again by now */
  uint16_t crc = 0;
  pgidx_t  k = 0;
  uint8_t  i;

  do { r[k] = 0xFF; } while (++k != (pgidx_t)SPM_PAGESIZE);
  r[0] = REC_MAGIC0;  r[1] = REC_MAGIC1;
  r[2] = (uint8_t)w->app_entry;  r[3] = (uint8_t)(w->app_entry >> 8);
  r[4] = (uint8_t)w->img_end;    r[5] = (uint8_t)(w->img_end >> 8);
#if ELFLD_CONF_ADDR_BITS > 16
  r[6] = (uint8_t)(w->img_end >> 16);
#else
  r[6] = 0;
#endif
#if ELFBOOT_FEAT_IMAGE_CRC
  crc = crc_flash(0, w->img_end);
#endif
  r[7] = (uint8_t)crc;  r[8] = (uint8_t)(crc >> 8);
  crc = 0;
  for (i = 0; i < REC_LEN - 2; i++) crc = _crc_xmodem_update(crc, r[i]);
  r[9] = (uint8_t)(crc >> 8);  r[10] = (uint8_t)crc;
  sess_page_program(REC_ADDR, r);
}

/* ---------------------------------------------------------------- services */
/* The running system calls these through the fixed table at the end of flash.
 * They use the caller's memory and stack only: while the application runs,
 * the loader's own variables are the application's RAM. */
#if ELFBOOT_FEAT_SERVICE
void bootsvc_write_page(uint32_t addr, const uint8_t *data) __attribute__((used, noinline));
void bootsvc_write_page(uint32_t addr, const uint8_t *data)
{
  uint8_t sreg = SREG;
  __asm__ __volatile__("cli");            /* the caller's interrupt handlers live in the section being programmed */
  sess_page_program((elfld_addr_t)addr & ~(elfld_addr_t)(SPM_PAGESIZE - 1), data);
  SREG = sreg;
}
#endif

#if ELFBOOT_FEAT_MODULES
/* Receive a file over the serial line and load it into [lo, hi), using the
 * caller's workspace. The same session code that loads firmware at reset. */
uint8_t bootsvc_session(void *workspace, uint32_t lo, uint32_t hi, uint32_t *entry, uint8_t tries)
  __attribute__((used, noinline));
uint8_t bootsvc_session(void *workspace, uint32_t lo, uint32_t hi, uint32_t *entry, uint8_t tries)
{
  elfsess_t *w = (elfsess_t *)workspace;
  uint8_t sreg = SREG, r;
  __asm__ __volatile__("cli");
  r = elfsess_run(w, (elfld_addr_t)lo, (elfld_addr_t)hi, SESS_MODULE, tries);
  *entry = w->ld.entry;
  SREG = sreg;
  return r;
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

/* ---------------------------------------------------------------- main */
static void boot_main(void) __attribute__((noreturn, used));
static void boot_main(void)
{
  uart_init();
  for (;;) {
    uint8_t r = elfsess_run(&ws, 0, REC_ADDR, SESS_IMAGE, ELFBOOT_CONF_WINDOW);
    if (r == ELFLD_DONE) {
#if !ELFBOOT_VIRTUAL_RESET
      ws.app_entry = (uint16_t)(ws.ld.entry >> 1);
#endif
      record_write(&ws);
      start_image();
    }
    if (r == ELFLD_MORE) {                 /* nobody is sending */
      if (record_ok()) start_image();
      continue;
    }
    if (ws.dirty) for (;;) { }             /* refused after flash changed: no record, so a reset lands here again */
  }
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

/* ---------------------------------------------------------------- service table */
/* Last twelve bytes of flash. Addresses, not jumps, so that neither the reach
 * of a relative jump nor linker relaxation can change the layout:
 *   +0  byte address of bootsvc_session   (0 when modules are not built in)
 *   +4  byte address of bootsvc_write_page
 *   +8  table version
 *   +9  workspace size the session needs, in units of 4 bytes
 *   +10 'p'  +11 'D'                                                       */
#if ELFBOOT_FEAT_SERVICE
__asm__(".section .bootsvc,\"a\",@progbits\n"
        "  .global bootsvc_table\n"
        "bootsvc_table:\n"
#if ELFBOOT_FEAT_MODULES
        "  .long bootsvc_session\n"
#else
        "  .long 0\n"
#endif
        "  .long bootsvc_write_page\n"
        "  .text\n");
const uint8_t bootsvc_info[4] __attribute__((used, section(".bootsvc_info"))) = {
  1, (uint8_t)((sizeof(elfsess_t) + 3) / 4), 'p', 'D'
};
#endif

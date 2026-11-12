/* kernel_demo.c - a stand-in for the protoduino kernel that shows the module
 * modes and loader choices. It is not the real scheduler: a module "thread"
 * is called three times with a block of state the kernel owns.
 *
 *   PD_MODULES   0  no module support
 *                1  start modules that are already in flash; cannot load
 *                2  also load modules while running
 *   PD_LOADER    1  ROM: the protoduino bootloader's session service does the work
 *                2  LINKED: the kernel carries the session code itself
 *   PD_FLASH     1  page writes through the protoduino bootloader's service table
 *                2  page writes through Optiboot's do_spm
 */
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <util/crc16.h>
#include "pd_module.h"

#ifndef PD_MODULES
#define PD_MODULES 2
#endif
#ifndef PD_LOADER
#define PD_LOADER 1
#endif
#ifndef PD_FLASH
#define PD_FLASH 1
#endif
#ifndef SVC_TABLE
#define SVC_TABLE (FLASHEND - 11)     /* the bootloader's service table: see boot.c */
#endif

#define SLOT_LO  0x6000u            /* module slots: 4 KB */
#define SLOT_HI  0x7000u
#define MODREC   0x7000u            /* one page of module records */

/* ---- serial ---- */
static void put(uint8_t c) { while (!(UCSR0A & _BV(UDRE0))) { } UDR0 = c; }
static void puts_(const char *s) { while (*s) put(*s++); }
static void hex8(uint8_t v) { static const char d[] = "0123456789abcdef"; put(d[v >> 4]); put(d[v & 15]); }

/* ---- what modules may call ---- */
uint8_t kernel_add(uint8_t a, uint8_t b) __attribute__((noinline, used));
uint8_t kernel_add(uint8_t a, uint8_t b) { return a + b; }
void    kernel_note(uint8_t v) __attribute__((noinline, used));
void    kernel_note(uint8_t v) { put(' '); hex8(v); }

#if PD_MODULES >= 1
/* ---- identity of this kernel build: CRC-16 of its own flash image ---- */
extern char __data_load_end;
static uint16_t kernel_id(void)
{
  uint16_t crc = 0;
  for (uint16_t a = 0; a != (uint16_t)&__data_load_end; a++) crc = _crc_xmodem_update(crc, pgm_read_byte(a));
  return crc;
}

static uint8_t modram[16];          /* state handed to a module */

static uint8_t run_module(uint16_t desc, uint16_t kid)
{
  pd_module_t m;
  memcpy_P(&m, (const void *)desc, sizeof m);
  if (m.magic != PD_MODULE_MAGIC) return 1;
  if (m.kernel_id != kid) return 2;                 /* linked against another kernel build */
  if (m.size > sizeof modram) return 3;
  for (uint8_t i = 0; i < sizeof modram; i++) modram[i] = 0;
  for (uint8_t i = 0; i < 3; i++) m.thread(modram);
  return 0;
}
#endif

#if PD_MODULES >= 2
/* ---- flash back-end ---- */
#if PD_FLASH == 1
typedef void (*svc_write_page_fn)(uint32_t addr, const uint8_t *data);
static uint8_t flash_service_present(void)
{
  return pgm_read_byte(SVC_TABLE + 10) == 'p' && pgm_read_byte(SVC_TABLE + 11) == 'D';
}
static void flash_page(uint16_t base, const uint8_t *data)
{
  ((svc_write_page_fn)(uint16_t)(pgm_read_dword(SVC_TABLE + 4) >> 1))(base, data);
}
#else
/* Optiboot 8 and later: the second word of the bootloader is "rjmp do_spm". */
typedef void (*do_spm_t)(uint16_t address, uint8_t command, uint16_t data);
#define OPTIBOOT_START (FLASHEND - 511)
static uint8_t flash_service_present(void)
{
  return pgm_read_byte(FLASHEND) >= 8 &&                           /* major version */
         (pgm_read_byte(OPTIBOOT_START + 3) & 0xF0) == 0xC0;       /* an rjmp, not the "ret" of a build without do_spm */
}
static void flash_page(uint16_t base, const uint8_t *data)
{
  do_spm_t do_spm = (do_spm_t)((OPTIBOOT_START + 2) >> 1);
  uint8_t sreg = SREG;
  cli();
  do_spm(base, __BOOT_PAGE_ERASE, 0);
  for (uint8_t i = 0; i < SPM_PAGESIZE; i += 2)
    do_spm(base + i, __BOOT_PAGE_FILL, (uint16_t)data[i] | ((uint16_t)data[i + 1] << 8));
  do_spm(base, __BOOT_PAGE_WRITE, 0);
  SREG = sreg;
}
#endif

/* ---- loader ---- */
#if PD_LOADER == 2
#define ELFLD_FEAT_WINDOW_LO 1
#include "../elfsess.h"
static void    sess_putch(uint8_t c) { put(c); }
static uint8_t sess_getch(elfsess_t *w)
{
  __uint24 n = F_CPU / 10;
  while (!(UCSR0A & _BV(RXC0))) { if (--n == 0) { w->timed_out = 1; return 0; } }
  return UDR0;
}
static void sess_page_program(elfld_addr_t base, const uint8_t *data) { flash_page(base, data); }
static elfsess_t ws;
static uint8_t load_module(uint16_t *entry)
{
  uint8_t r = elfsess_run(&ws, SLOT_LO, SLOT_HI, SESS_MODULE, 200);
  *entry = ws.ld.entry;
  return r;
}
#else
typedef uint8_t (*svc_session_fn)(void *ws, uint32_t lo, uint32_t hi, uint32_t *entry, uint8_t tries);
static uint8_t load_module(uint16_t *entry)
{
  uint32_t fn = pgm_read_dword(SVC_TABLE);                    /* 0: this bootloader cannot load modules */
  if (!fn) return 0xFE;
  uint8_t  ws[(uint16_t)pgm_read_byte(SVC_TABLE + 9) * 4];     /* size the boot ROM asks for */
  uint32_t e = 0;
  uint8_t  r = ((svc_session_fn)(uint16_t)(fn >> 1))(ws, SLOT_LO, SLOT_HI, &e, 200);
  *entry = (uint16_t)e;
  return r;
}
#endif

static void record_module(uint16_t desc, uint16_t kid)
{
  static uint8_t page[SPM_PAGESIZE];
  for (uint8_t i = 0; i < SPM_PAGESIZE; i++) page[i] = 0xFF;
  page[0] = 'p'; page[1] = 'M';
  page[2] = (uint8_t)desc; page[3] = (uint8_t)(desc >> 8);
  page[4] = (uint8_t)kid;  page[5] = (uint8_t)(kid >> 8);
  flash_page(MODREC, page);
}
#endif

int main(void)
{
  UCSR0A = _BV(U2X0); UBRR0L = 16; UCSR0B = _BV(RXEN0) | _BV(TXEN0);
  puts_("\nkernel up");

#if PD_MODULES >= 1
  uint16_t kid = kernel_id();
  puts_(" id="); hex8(kid >> 8); hex8((uint8_t)kid);

  if (pgm_read_byte(MODREC) == 'p' && pgm_read_byte(MODREC + 1) == 'M') {
    uint16_t desc = pgm_read_word(MODREC + 2);
    puts_(" | module in flash:");
    uint8_t r = run_module(desc, kid);
    if (r) { puts_(" not started, reason "); hex8(r); }
  } else {
    puts_(" | no module in flash");
  }
#endif

#if PD_MODULES >= 2
  if (!flash_service_present()) {
    puts_(" | no flash service in the bootloader\n");
  } else {
    uint16_t entry = 0;
    puts_(" | waiting for a module\n");
    uint8_t r = load_module(&entry);
    puts_("\nload result "); hex8(r);
    if (r == 1) {
      puts_(" | module runs:");
      r = run_module(entry, kid);
      if (r) { puts_(" refused, reason "); hex8(r); }
      else   { record_module(entry, kid); puts_(" | recorded"); }
    }
  }
#endif
  puts_(" | end\n");
  for (;;) { }
}

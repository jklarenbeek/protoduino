/* elfsess.h - one load session: XMODEM-CRC in, ELF core, page cache out.
 *
 * Everything a session needs lives in one caller-owned workspace, so the same
 * code serves three users:
 *   - the bootloader, loading a firmware image;
 *   - the bootloader called by the running system through its service entry,
 *     loading a module into a slot ("boot ROM" use);
 *   - a kernel that links this file itself and writes flash through whatever
 *     the installed bootloader offers (ours, or Optiboot's do_spm).
 *
 * The includer supplies the port functions declared below and, before
 * including this file, may set:
 *   SESS_FEAT_IMAGE      1: firmware-image handling (first-write hook,
 *                           reset-vector patch on parts without a boot section)
 *   ELFBOOT_FEAT_INFO    answer '?' with a description line
 */
#ifndef ELFSESS_H
#define ELFSESS_H

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/crc16.h>

struct elfsess;
#define ELFLD_SINK(space, addr, b) sess_sink((struct elfsess *)c, (space), (addr), (b))
#include "elfld.h"
static uint8_t sess_sink(struct elfsess *w, uint8_t space, elfld_addr_t addr, uint8_t b);
#include "elfld.c"

#ifndef SESS_FEAT_IMAGE
#define SESS_FEAT_IMAGE 0
#endif
#ifndef ELFBOOT_VIRTUAL_RESET
#define ELFBOOT_VIRTUAL_RESET 0
#endif
#ifndef ELFBOOT_FEAT_INFO
#define ELFBOOT_FEAT_INFO 0
#endif

#define ELFLD_E_INCOMPLETE 0x8B     /* the sender finished before the file was complete */

#define SESS_IMAGE  0               /* a firmware image: record handling applies */
#define SESS_MODULE 1               /* a module: only the pages in the window are touched */

#if SPM_PAGESIZE > 255
typedef uint16_t pgidx_t;
#else
typedef uint8_t pgidx_t;
#endif

typedef struct elfsess {
  elfld_t      ld;                  /* must be first: the core's sink finds the session through it */
  elfld_addr_t pg_base;             /* page held in pg[]                          */
  elfld_addr_t img_end;             /* highest flash address written + 1, by page */
  uint16_t     app_entry;           /* word address to start a firmware image at  */
  uint8_t      pg_live;
  uint8_t      dirty;               /* flash has been changed in this session     */
  uint8_t      timed_out;
  uint8_t      mode;
  uint8_t      pg[SPM_PAGESIZE];    /* the one page being assembled               */
  uint8_t      blk[128];            /* one XMODEM block, held until its CRC passed */
} elfsess_t;

/* When nobody outside may bring a workspace (no module service), the session
 * can be bound to one static instance. The compiler then addresses the fields
 * directly, which is smaller than going through a pointer. */
#ifdef SESS_STATIC_WS
static elfsess_t SESS_STATIC_WS;
#define SESS_BIND(w) ((w) = &SESS_STATIC_WS)
#else
#define SESS_BIND(w) ((void)0)
#endif

/* ---- supplied by the includer -------------------------------------------- */
static uint8_t sess_getch(elfsess_t *w);          /* 0 and w->timed_out = 1 on timeout */
static void    sess_putch(uint8_t c);
static void    sess_page_program(elfld_addr_t base, const uint8_t *data);   /* erase + write one page */
#if SESS_FEAT_IMAGE
static void    sess_first_write(elfsess_t *w);    /* called once, before the first page changes */
#endif

/* ---- page cache ----------------------------------------------------------- */
static void sess_page_commit(elfsess_t *w)
{
  SESS_BIND(w);
  if (!w->pg_live) return;
  w->pg_live = 0;

#if SESS_FEAT_IMAGE && ELFBOOT_VIRTUAL_RESET
  /* No hardware boot section: every reset must still reach the loader.
   * Keep the image's own start address and aim its reset vector at us. */
  if (w->mode == SESS_IMAGE && w->pg_base == 0) {
#  if FLASHEND > 0x1FFFU
    w->app_entry = (uint16_t)w->pg[2] | ((uint16_t)w->pg[3] << 8);        /* jmp k  */
    w->pg[2] = (uint8_t)((BOOT_START / 2) & 0xFF);
    w->pg[3] = (uint8_t)((BOOT_START / 2) >> 8);
#  else
    uint16_t v = (uint16_t)w->pg[0] | ((uint16_t)w->pg[1] << 8);          /* rjmp k */
    w->app_entry = (v + 1) & 0x0FFF;
    v = 0xC000 | ((BOOT_START / 2 - 1) & 0x0FFF);
    w->pg[0] = (uint8_t)v;
    w->pg[1] = (uint8_t)(v >> 8);
#  endif
  }
#endif

  if (!w->dirty) {
    /* First change to flash. The headers have been accepted by now. */
    w->dirty = 1;
#if SESS_FEAT_IMAGE
    sess_first_write(w);
#endif
  }
  if (w->pg_base >= w->img_end) w->img_end = w->pg_base + SPM_PAGESIZE;
  sess_page_program(w->pg_base, w->pg);
}

static void sess_page_open(elfsess_t *w, elfld_addr_t base)
{
  pgidx_t i = 0;
  SESS_BIND(w);
  sess_page_commit(w);
  do { w->pg[i] = 0xFF; } while (++i != (pgidx_t)SPM_PAGESIZE);
  w->pg_base = base;
  w->pg_live = 1;
}

/* Further sinks (EEPROM, signature) are the includer's business. */
#ifndef SESS_OTHER_SPACE
#define SESS_OTHER_SPACE(space, addr, b) 0
#endif

static uint8_t sess_sink(elfsess_t *w, uint8_t space, elfld_addr_t addr, uint8_t b)
{
  SESS_BIND(w);
  if (space != ELFLD_SPACE_FLASH) return SESS_OTHER_SPACE(space, addr, b);
  {
    elfld_addr_t base = addr & ~(elfld_addr_t)(SPM_PAGESIZE - 1);
    if (!w->pg_live || base != w->pg_base) sess_page_open(w, base);
    w->pg[(pgidx_t)addr & (SPM_PAGESIZE - 1)] = b;
  }
  return 0;
}

/* ---- the session ---------------------------------------------------------- */
#define XM_SOH 0x01
#define XM_EOT 0x04
#define XM_ACK 0x06
#define XM_NAK 0x15
#define XM_CAN 0x18

/* Receive one file. Flash bytes are accepted in [lo, hi) only.
 * Returns ELFLD_DONE   the file was loaded and its last page committed;
 *         ELFLD_MORE   no transfer began within `tries` probes;
 *         ELFLD_E_*    the file was refused (the reason was sent to the sender). */
static uint8_t elfsess_run(elfsess_t *w, elfld_addr_t lo, elfld_addr_t hi, uint8_t mode, uint8_t tries)
{
  uint8_t expect = 1, result = ELFLD_MORE, started = 0;

  SESS_BIND(w);
  w->pg_live = 0;
  w->dirty   = 0;
  w->img_end = 0;
  w->mode    = mode;
  elfld_begin(&w->ld, lo, hi);

  for (;;) {
    uint8_t c, n, nn, i;
    uint16_t crc;

    w->timed_out = 0;
    c = sess_getch(w);
    if (w->timed_out) {
      if (!started) {
        if (tries-- == 0) return ELFLD_MORE;
        sess_putch('C');                        /* ask the sender for CRC mode */
      } else {
        sess_putch(XM_NAK);
      }
      continue;
    }
#if ELFBOOT_FEAT_INFO
    if (c == '?' && !started) {
      static const char info[] PROGMEM = "pD1 " ELFBOOT_INFO_STRING "\r\n";
      const char *p = info;
      while ((c = pgm_read_byte(p++)) != 0) sess_putch(c);
      continue;
    }
#endif
    if (c == XM_EOT) {
      sess_putch(XM_ACK);
      break;
    }
    if (c != XM_SOH) continue;

    n  = sess_getch(w);
    nn = sess_getch(w);
    crc = 0;
    for (i = 0; i < 128; i++) {
      c = sess_getch(w);
      w->blk[i] = c;
      crc = _crc_xmodem_update(crc, c);
    }
    crc = _crc_xmodem_update(crc, sess_getch(w));
    crc = _crc_xmodem_update(crc, sess_getch(w));   /* data + CRC must come to 0 */
    if (w->timed_out || crc != 0 || (uint8_t)(n + nn) != 0xFF) {
      sess_putch(XM_NAK);
      continue;
    }
    if (n == expect) {
      started = 1;
      if (result == ELFLD_MORE) result = elfld_write(&w->ld, w->blk, 128);
      expect++;
    } else if (n != (uint8_t)(expect - 1)) {        /* a repeat is acknowledged, not applied */
      result = ELFLD_E_ORDER;
    }
    if (result & 0x80) {                            /* refused: say why */
      sess_putch(XM_CAN);
      sess_putch(XM_CAN);
      sess_putch(result);
      return result;
    }
    sess_putch(XM_ACK);
  }

  if (result != ELFLD_DONE) return ELFLD_E_INCOMPLETE;
  sess_page_commit(w);
  return ELFLD_DONE;
}

#endif

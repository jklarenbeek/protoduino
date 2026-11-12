/* elfld.c - streaming ELF32 loader core (see elfld.h). */
#include "elfld.h"

#ifdef __AVR__
#include <avr/pgmspace.h>
#define ELFLD_ROM        PROGMEM
/* Above 64 KB (a boot section on a large part) LPM cannot reach the table:
 * the read has to go through ELPM with the full address. */
#if FLASHEND > 0xFFFFUL
#define ELFLD_RULE(i)    pgm_read_byte_far(pgm_get_far_address(ehdr_rules) + (i))
#else
#define ELFLD_RULE(i)    pgm_read_byte(&ehdr_rules[i])
#endif
#define ELFLD_NOINLINE   __attribute__((noinline))
#else
#define ELFLD_ROM
#define ELFLD_RULE(i)    (ehdr_rules[i])
#define ELFLD_NOINLINE
#endif

enum { S_EHDR = 0, S_PHDR, S_BODY, S_END };

#define RD16(p) ((uint16_t)((p)[0] | ((uint16_t)(p)[1] << 8)))

#if ELFLD_CONF_ADDR_BITS == 16
#  define RDA(p)   ((elfld_addr_t)RD16(p))
#  define HIGH(p)  ((p)[2] | (p)[3])          /* bytes that must be zero to fit */
#elif ELFLD_CONF_ADDR_BITS == 24
#  define RDA(p)   ((elfld_addr_t)RD16(p) | ((elfld_addr_t)(p)[2] << 16))
#  define HIGH(p)  ((p)[3])
#else
#  define RDA(p)   ((elfld_addr_t)RD16(p) | ((elfld_addr_t)(p)[2] << 16) | ((elfld_addr_t)(p)[3] << 24))
#  define HIGH(p)  0
#endif

/* ELF header bytes that must have one exact value: {offset, value}. */
static const uint8_t ehdr_rules[] ELFLD_ROM = {
   0, 0x7F,  1, 'E',  2, 'L',  3, 'F',
   4, 1,              /* ELFCLASS32                          */
   5, 1,              /* little endian                       */
  16, 2,   17, 0,     /* e_type      = ET_EXEC               */
  18, 83,  19, 0,     /* e_machine   = EM_AVR                */
  28, 52,  29, 0,  30, 0,  31, 0,   /* e_phoff = 52: headers lead the file */
  42, 32,  43, 0,     /* e_phentsize = 32                    */
  45, 0               /* e_phnum     < 256                   */
};

static ELFLD_NOINLINE uint8_t check_ehdr(elfld_t *c)
{
  const uint8_t *h = c->scratch;
  uint8_t i = 0;

  do {
    if (h[ELFLD_RULE(i)] != ELFLD_RULE(i + 1))
      return ELFLD_E_FORMAT;
    i += 2;
  } while (i != sizeof(ehdr_rules));

#if ELFLD_FEAT_ARCH_CHECK
#  ifdef __AVR__
  if ((h[36] & 0x7F) != ELFLD_CONF_ARCH) return ELFLD_E_ARCH;
#  else
  if (c->arch && (h[36] & 0x7F) != c->arch) return ELFLD_E_ARCH;
#  endif
#endif
  if (HIGH(h + 24) || h[44] == 0) return ELFLD_E_LAYOUT;
  c->entry  = RDA(h + 24);
  c->phleft = h[44];
  /* segment data can only start after the program header table */
  c->floor  = (elfld_addr_t)52 + (elfld_addr_t)((uint16_t)32 * h[44]);
  c->state  = S_PHDR;
  c->need   = 32;
  return ELFLD_MORE;
}

static ELFLD_NOINLINE uint8_t add_phdr(elfld_t *c)
{
  const uint8_t *h = c->scratch;
  elfld_addr_t off, addr, len;
  elfld_seg_t *s;

  if (h[0] != 1 || (h[1] | h[2] | h[3])) return ELFLD_MORE;   /* not PT_LOAD */

#if ELFLD_FEAT_RAM_EXTENT
  if (h[10] == ELFLD_SPACE_RAM) {                             /* p_vaddr in RAM */
    uint16_t end = RD16(h + 8) + RD16(h + 20);
    if (end > c->ram_end) c->ram_end = end;
  }
#endif

  if (!(h[16] | h[17] | h[18] | h[19])) return ELFLD_MORE;    /* p_filesz == 0 */
  if (HIGH(h + 4) | HIGH(h + 16)) return ELFLD_E_RANGE;

  off = RDA(h + 4);
  len = RDA(h + 16);

  if (h[14] & 0x80) {                    /* RAM, EEPROM, fuses, lock, signature */
#if ELFLD_FEAT_SPACES
    if (1
#  if ELFLD_FEAT_EEPROM
        && h[14] != ELFLD_SPACE_EEPROM
#  endif
#  if ELFLD_FEAT_SIGNATURE
        && h[14] != ELFLD_SPACE_SIG
#  endif
       ) return ELFLD_MORE;
    addr = (elfld_addr_t)RD16(h + 12);
#else
    return ELFLD_MORE;                   /* not ours to write */
#endif
  } else {
    addr = RDA(h + 12);
    if (HIGH(h + 12) || addr > c->hi || len > (elfld_addr_t)(c->hi - addr)
#if ELFLD_FEAT_WINDOW_LO
        || addr < c->lo
#endif
       ) return ELFLD_E_RANGE;
  }
  if (off < c->floor) return ELFLD_E_ORDER;

  s = c->sp;                             /* the segment remembered last */
#if ELFLD_FEAT_COALESCE
  if (c->nseg && off == c->floor && addr == (elfld_addr_t)(s->addr + s->len)
#  if ELFLD_FEAT_SPACES
      && (h[14] & 0x80 ? h[14] : 0) == s->space
#  endif
     ) {
    s->len += len;
  } else
#endif
  {
    if (c->nseg == ELFLD_CONF_MAX_SEGMENTS) return ELFLD_E_SEGMENTS;
    c->nseg++;
    c->sp = ++s;
    s->gap  = off - c->floor;
    s->addr = addr;
    s->len  = len;
#if ELFLD_FEAT_SPACES
    s->space = h[14] & 0x80 ? h[14] : 0;
#endif
  }
  c->floor = off + len;
  return ELFLD_MORE;
}

void elfld_begin(elfld_t *c, elfld_addr_t lo, elfld_addr_t hi)
{
  uint8_t *p = &c->state;
  uint8_t  n = (uint8_t)sizeof(elfld_t) - 52;
  do { *p++ = 0; } while (--n);
  c->need = 52;
  c->sp   = c->seg - 1;                  /* "nothing remembered yet" */
#if ELFLD_FEAT_WINDOW_LO
  c->lo = lo;
#else
  (void)lo;
#endif
  c->hi = hi;
}

uint8_t elfld_write(elfld_t *c, const uint8_t *p, uint8_t n)
{
  uint8_t r;

  for (; n; n--) {
    uint8_t b = *p++;

    if (c->state == S_BODY) {
      elfld_seg_t *s = c->sp;
      if (s->gap) { s->gap--; continue; }
#if ELFLD_FEAT_SPACES
      if (ELFLD_SINK(s->space, s->addr, b)) { r = ELFLD_E_SINK; goto stop; }
#else
      if (ELFLD_SINK(ELFLD_SPACE_FLASH, s->addr, b)) { r = ELFLD_E_SINK; goto stop; }
#endif
      s->addr++;
      if (--s->len) continue;
      c->sp = s + 1;
      if (--c->nseg) continue;
      r = ELFLD_DONE;
      goto stop;
    }
    if (c->state > S_BODY) return c->result;

    c->scratch[c->fill] = b;
    if (++c->fill != c->need) continue;
    c->fill = 0;
    if (c->state == S_EHDR) {
      r = check_ehdr(c);
    } else {
      r = add_phdr(c);
      if (--c->phleft == 0) {
        c->state = S_BODY;
        c->sp = c->seg;
        if (r == ELFLD_MORE && c->nseg == 0) r = ELFLD_E_EMPTY;
      }
    }
    if (r != ELFLD_MORE) goto stop;
  }
  return ELFLD_MORE;

stop:
  c->state  = S_END;
  c->result = r;
  return r;
}

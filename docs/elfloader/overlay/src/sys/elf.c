// file: ./src/sys/elf.c

#include "elf.h"

#ifdef __AVR__
#include <avr/pgmspace.h>
#define ELF_PROGMEM      PROGMEM
// above 64 KB (a boot section on a large part) LPM cannot reach the
// table: the read has to go through ELPM with the full address.
#if (FLASHEND > 0xFFFFUL)
#define ELF_RULE(i)      pgm_read_byte_far(pgm_get_far_address(elf_header_rules) + (i))
#else
#define ELF_RULE(i)      pgm_read_byte(&elf_header_rules[i])
#endif
#define ELF_NOINLINE     __attribute__((noinline))
#define ELF_ARCH(s)      (__AVR_ARCH__)
#else
#define ELF_PROGMEM
#define ELF_RULE(i)      (elf_header_rules[i])
#define ELF_NOINLINE
#define ELF_ARCH(s)      ((s)->arch)
#endif

// states of the stream
#define ELF_STATE_EHDR 0  // reading the ELF header
#define ELF_STATE_PHDR 1  // reading the program headers
#define ELF_STATE_BODY 2  // skipping gaps and delivering segment bytes
#define ELF_STATE_DONE 3  // ended: result holds the outcome

#define ELF_EHDR_SIZE 52
#define ELF_PHDR_SIZE 32

#define ELF_RD16(p) ((uint16_t)((p)[0] | ((uint16_t)(p)[1] << 8)))

// read an address field of the file; ELF_HIGH() gives the bytes that
// must be zero for the value to fit into an elf_addr_t.
#if ELF_CONF_ADDR_BITS == 16
#define ELF_RDA(p)  ((elf_addr_t)ELF_RD16(p))
#define ELF_HIGH(p) ((p)[2] | (p)[3])
#elif ELF_CONF_ADDR_BITS == 24
#define ELF_RDA(p)  ((elf_addr_t)ELF_RD16(p) | ((elf_addr_t)(p)[2] << 16))
#define ELF_HIGH(p) ((p)[3])
#else
#define ELF_RDA(p)  ((elf_addr_t)ELF_RD16(p) | ((elf_addr_t)(p)[2] << 16) | ((elf_addr_t)(p)[3] << 24))
#define ELF_HIGH(p) 0
#endif

#define ELF_SPACES (ELF_CONF_EEPROM || ELF_CONF_SIGNATURE)

// ELF header bytes that must have one exact value: {offset, value}.
static const uint8_t elf_header_rules[] ELF_PROGMEM = {
   0, 0x7F,  1, 'E',  2, 'L',  3, 'F',
   4, 1,                            // ELFCLASS32
   5, 1,                            // little endian
  16, 2,   17, 0,                   // e_type      = ET_EXEC
  18, 83,  19, 0,                   // e_machine   = EM_AVR
  28, 52,  29, 0,  30, 0,  31, 0,   // e_phoff     = 52: headers lead the file
  42, 32,  43, 0,                   // e_phentsize = 32
  45, 0                             // e_phnum     < 256
};

static ELF_NOINLINE uint8_t elf_stream_header(elf_stream_t *s)
{
  const uint8_t *h = s->scratch;
  uint8_t i = 0;

  do
  {
    if (h[ELF_RULE(i)] != ELF_RULE(i + 1))
      return ERR_BIN_MAGIC;
    i += 2;
  }
  while (i != sizeof(elf_header_rules));

#if ELF_CONF_ARCH_CHECK
  // the low seven bits of e_flags name the core family the file was built for
  if (ELF_ARCH(s) && (h[36] & 0x7F) != ELF_ARCH(s))
    return ERR_BIN_VERSION;
#endif

  if (ELF_HIGH(h + 24) || h[44] == 0)
    return ERR_BIN_HEADER;

  s->entry  = ELF_RDA(h + 24);
  s->phleft = h[44];
  // segment data can only start after the program header table
  s->floor  = (elf_addr_t)ELF_EHDR_SIZE + (elf_addr_t)((uint16_t)ELF_PHDR_SIZE * h[44]);
  s->state  = ELF_STATE_PHDR;
  s->need   = ELF_PHDR_SIZE;
  return PT_WAITING;
}

static ELF_NOINLINE uint8_t elf_stream_segment(elf_stream_t *s)
{
  const uint8_t *h = s->scratch;
  elf_addr_t off, addr, len;
  elf_segment_t *seg;

  // not PT_LOAD, or nothing in the file (.bss)
  if (h[0] != 1 || (h[1] | h[2] | h[3]))
    return PT_WAITING;
  if (!(h[16] | h[17] | h[18] | h[19]))
    return PT_WAITING;
  if (ELF_HIGH(h + 4) | ELF_HIGH(h + 16))
    return ERR_BOUNDS_UPPER;

  off = ELF_RDA(h + 4);
  len = ELF_RDA(h + 16);

  // the top byte of p_paddr names the memory: flash is everything below 0x80
  if (h[14] & 0x80)
  {
#if ELF_SPACES
    if (1
#if ELF_CONF_EEPROM
        && h[14] != ELF_SPACE_EEPROM
#endif
#if ELF_CONF_SIGNATURE
        && h[14] != ELF_SPACE_SIGNATURE
#endif
       )
      return PT_WAITING;
    addr = (elf_addr_t)ELF_RD16(h + 12);
#else
    return PT_WAITING; // RAM, EEPROM, fuses, lock, signature: not ours to write
#endif
  }
  else
  {
    addr = ELF_RDA(h + 12);
    if (ELF_HIGH(h + 12) || addr > s->hi || len > (elf_addr_t)(s->hi - addr))
      return ERR_BOUNDS_UPPER;
#if ELF_CONF_WINDOW_LO
    if (addr < s->lo)
      return ERR_BOUNDS_LOWER;
#endif
  }

  if (off < s->floor)
    return ERR_ORDER_SEQUENCE;

  seg = s->sp; // the segment remembered last
#if ELF_CONF_COALESCE
  if (s->nseg && off == s->floor && addr == (elf_addr_t)(seg->addr + seg->len)
#if ELF_SPACES
      && (h[14] & 0x80 ? h[14] : 0) == seg->space
#endif
     )
  {
    seg->len += len;
  }
  else
#endif
  {
    if (s->nseg == ELF_CONF_MAX_SEGMENTS)
      return ERR_BUF_FULL;
    s->nseg++;
    s->sp = ++seg;
    seg->gap  = off - s->floor;
    seg->addr = addr;
    seg->len  = len;
#if ELF_SPACES
    seg->space = h[14] & 0x80 ? h[14] : 0;
#endif
  }
  s->floor = off + len;
  return PT_WAITING;
}

void elf_stream_init(elf_stream_t *s, elf_addr_t lo, elf_addr_t hi)
{
  uint8_t *p = &s->state;
  uint8_t  n = (uint8_t)sizeof(elf_stream_t) - sizeof(s->scratch);

  // everything behind the scratch area starts at zero
  do
  {
    *p++ = 0;
  }
  while (--n);

  s->need = ELF_EHDR_SIZE;
  s->sp   = s->seg - 1; // "nothing remembered yet"
#if ELF_CONF_WINDOW_LO
  s->lo = lo;
#else
  (void)lo;
#endif
  s->hi = hi;
}

ptstate_t elf_stream_write(elf_stream_t *s, const uint8_t *src, uint8_t len)
{
  uint8_t r;

  for (; len; len--)
  {
    uint8_t b = *src++;

    if (s->state == ELF_STATE_BODY)
    {
      elf_segment_t *seg = s->sp;

      if (seg->gap)
      {
        seg->gap--;
        continue;
      }
#if ELF_SPACES
      r = ELF_STREAM_SINK(s, seg->space, seg->addr, b);
#else
      r = ELF_STREAM_SINK(s, ELF_SPACE_FLASH, seg->addr, b);
#endif
      if (r != ERR_SUCCESS)
        goto done;
      seg->addr++;
      if (--seg->len)
        continue;
      s->sp = seg + 1;
      if (--s->nseg)
        continue;
      r = PT_ENDED;
      goto done;
    }
    if (s->state > ELF_STATE_BODY)
      return (ptstate_t)s->result;

    s->scratch[s->fill] = b;
    if (++s->fill != s->need)
      continue;
    s->fill = 0;

    if (s->state == ELF_STATE_EHDR)
    {
      r = elf_stream_header(s);
    }
    else
    {
      r = elf_stream_segment(s);
      if (--s->phleft == 0)
      {
        s->state = ELF_STATE_BODY;
        s->sp = s->seg;
        if (r == PT_WAITING && s->nseg == 0)
          r = ERR_VALID_REQUIRED;
      }
    }
    if (r != PT_WAITING)
      goto done;
  }
  return PT_WAITING;

done:
  s->state  = ELF_STATE_DONE;
  s->result = r;
  return (ptstate_t)r;
}

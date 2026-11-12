/* elfld.h - streaming ELF32 loader core.
 *
 * The file is pushed in, in order, a few bytes at a time. The core never
 * seeks, never allocates and never touches hardware: every byte that belongs
 * in a memory is handed to elfld_sink(). State lives in one caller-owned
 * struct, so the same code can serve a bootloader and an application. */
#ifndef ELFLD_H
#define ELFLD_H

#include <stdint.h>
#include "elfld_conf.h"

#if ELFLD_CONF_ADDR_BITS == 16
typedef uint16_t elfld_addr_t;
#elif ELFLD_CONF_ADDR_BITS == 24 && defined(__AVR__)
typedef __uint24 elfld_addr_t;
#else
typedef uint32_t elfld_addr_t;
#endif

/* results */
#define ELFLD_MORE       0x00   /* keep feeding */
#define ELFLD_DONE       0x01   /* every loadable byte has been delivered */
#define ELFLD_E_FORMAT   0x80   /* not a little-endian ELF32 executable for AVR */
#define ELFLD_E_ARCH     0x81   /* built for another AVR core family */
#define ELFLD_E_LAYOUT   0x82   /* headers not where a streaming reader needs them */
#define ELFLD_E_RANGE    0x83   /* a segment falls outside the permitted window */
#define ELFLD_E_ORDER    0x84   /* segment data is not in ascending file order */
#define ELFLD_E_SEGMENTS 0x85   /* more segments than ELFLD_CONF_MAX_SEGMENTS */
#define ELFLD_E_EMPTY    0x86   /* nothing to load */
#define ELFLD_E_SINK     0x87   /* the sink refused a byte */

/* memory spaces: bits 23..16 of p_paddr in files produced by avr-gcc */
#define ELFLD_SPACE_FLASH  0x00
#define ELFLD_SPACE_RAM    0x80
#define ELFLD_SPACE_EEPROM 0x81
#define ELFLD_SPACE_SIG    0x84

typedef struct {
  elfld_addr_t gap;      /* file bytes to skip before this segment  */
  elfld_addr_t addr;     /* next address to write inside its space  */
  elfld_addr_t len;      /* bytes still to deliver                  */
#if ELFLD_FEAT_SPACES
  uint8_t      space;
#endif
} elfld_seg_t;

typedef struct elfld {
  /* The scratch area comes first so that header bytes and the hot control
   * fields are all reachable with a short displacement from one pointer. */
  uint8_t      scratch[52];
  uint8_t      state;
  uint8_t      fill;     /* bytes held in scratch                    */
  uint8_t      need;     /* size of the header record being read     */
  uint8_t      phleft;   /* program headers still to read            */
  uint8_t      result;   /* outcome once the load has ended          */
  uint8_t      nseg;     /* segments remembered / still to deliver   */
  elfld_seg_t *sp;       /* last remembered, later current, segment  */
  elfld_addr_t floor;    /* lowest file offset a new segment may use */
  elfld_addr_t hi;       /* flash window [lo, hi) the file may touch */
#if ELFLD_FEAT_WINDOW_LO
  elfld_addr_t lo;
#endif
  elfld_addr_t entry;    /* e_entry                                  */
#if ELFLD_FEAT_RAM_EXTENT
  uint16_t     ram_end;  /* highest RAM address + 1 the file claims  */
#endif
#if !defined(__AVR__)
  uint8_t      arch;     /* host build: expected e_flags core family */
#endif
  elfld_seg_t  seg[ELFLD_CONF_MAX_SEGMENTS];
} elfld_t;

/* Start a load. Flash bytes outside [lo, hi) are refused before anything
 * is written. */
void    elfld_begin(elfld_t *c, elfld_addr_t lo, elfld_addr_t hi);

/* Push the next n bytes of the file. Returns ELFLD_MORE, ELFLD_DONE or an
 * ELFLD_E_* code; after DONE or an error the rest of the file is ignored. */
uint8_t elfld_write(elfld_t *c, const uint8_t *p, uint8_t n);

/* Supplied by the port: store one byte. Return 0 to continue. */
#ifndef ELFLD_SINK
uint8_t elfld_sink(uint8_t space, elfld_addr_t addr, uint8_t byte);
#define ELFLD_SINK(space, addr, byte) elfld_sink((space), (addr), (byte))
#endif

#endif

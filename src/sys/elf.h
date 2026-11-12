// file: ./src/sys/elf.h

#ifndef __ELF_H__
#define __ELF_H__

/* Streaming ELF32 loader for AVR.
 *
 * - The file is pushed in, in order, a few bytes at a time. The stream
 *   never seeks, never allocates and never touches hardware: every byte
 *   that belongs in a memory is handed to a sink.
 * - Independent from process.c/process.h
 * - Uses the state of ./src/sys/pt.h and error codes from ./src/sys/errors.h
 *
 * Design:
 *  - elf_stream_t: the whole state, owned by the caller, so the same code
 *    can serve a bootloader and a running kernel
 *  - elf_stream_write(): returns PT_WAITING while it wants more, PT_ENDED
 *    when every loadable byte has been delivered, or an error
 *  - the sink is bound at compile time (ELF_STREAM_SINK); by default it is
 *    the function elf_stream_sink() that you supply
 */

#include <protoduino.h>
#include "elf.conf.h"

#ifdef __cplusplus
extern "C" {
#endif

#if ELF_CONF_ADDR_BITS == 16
typedef uint16_t elf_addr_t;
#elif ELF_CONF_ADDR_BITS == 24 && defined(__AVR__)
typedef __uint24 elf_addr_t;
#else
typedef uint32_t elf_addr_t;
#endif

/* memory spaces: bits 23..16 of p_paddr in files produced by avr-gcc */
#define ELF_SPACE_FLASH     0x00
#define ELF_SPACE_RAM       0x80
#define ELF_SPACE_EEPROM    0x81
#define ELF_SPACE_SIGNATURE 0x84

/* why a file is refused (see errors.h)
 *  ERR_BIN_MAGIC        not a little-endian ELF32 executable for AVR
 *  ERR_BIN_VERSION      built for another AVR core family
 *  ERR_BIN_HEADER       headers not where a streaming reader needs them
 *  ERR_BOUNDS_LOWER     a segment starts below the flash window
 *  ERR_BOUNDS_UPPER     a segment ends above the flash window
 *  ERR_ORDER_SEQUENCE   segment data is not in ascending file order
 *  ERR_BUF_FULL         more segments than ELF_CONF_MAX_SEGMENTS
 *  ERR_VALID_REQUIRED   nothing to load
 *  anything else        the sink refused a byte, with this code
 */

typedef struct elf_segment {
  elf_addr_t gap;       /* file bytes to skip before this segment  */
  elf_addr_t addr;      /* next address to write inside its space  */
  elf_addr_t len;       /* bytes still to deliver                  */
#if ELF_CONF_EEPROM || ELF_CONF_SIGNATURE
  uint8_t    space;
#endif
} elf_segment_t;

typedef struct elf_stream {
  /* the scratch area comes first so that header bytes and the hot
   * control fields are reachable with a short displacement */
  uint8_t        scratch[52];
  uint8_t        state;
  uint8_t        fill;      /* bytes held in scratch                    */
  uint8_t        need;      /* size of the header record being read     */
  uint8_t        phleft;    /* program headers still to read            */
  uint8_t        result;    /* outcome once the stream has ended        */
  uint8_t        nseg;      /* segments remembered / still to deliver   */
  elf_segment_t *sp;        /* last remembered, later current, segment  */
  elf_addr_t     floor;     /* lowest file offset a new segment may use */
  elf_addr_t     hi;        /* flash window [lo, hi) the file may touch */
#if ELF_CONF_WINDOW_LO
  elf_addr_t     lo;
#endif
  elf_addr_t     entry;     /* e_entry                                  */
#if !defined(__AVR__)
  uint8_t        arch;      /* host build: expected core family, 0 = any */
#endif
  elf_segment_t  seg[ELF_CONF_MAX_SEGMENTS];
} elf_stream_t;

/* Start a stream. Flash bytes outside [lo, hi) are refused before
 * anything reaches the sink. */
void elf_stream_init(elf_stream_t *s, elf_addr_t lo, elf_addr_t hi);

/* Push the next len bytes of the file.
 * Returns PT_WAITING to be fed more, PT_ENDED when every loadable byte has
 * been delivered, or an error code. After PT_ENDED or an error the rest of
 * the file is ignored and the same state is returned again. */
ptstate_t elf_stream_write(elf_stream_t *s, const uint8_t *src, uint8_t len);

/* The sink: store one byte. Returns ERR_SUCCESS to continue, or the error
 * the stream then ends with. */
#ifndef ELF_STREAM_SINK
uint8_t elf_stream_sink(elf_stream_t *s, uint8_t space, elf_addr_t addr, uint8_t byte);
#define ELF_STREAM_SINK(s, space, addr, byte) elf_stream_sink((s), (space), (addr), (byte))
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ELF_H__ */

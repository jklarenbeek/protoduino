// file: ./src/sys/bootrom.h

#ifndef __BOOTROM_H__
#define __BOOTROM_H__

/* What a running system can ask of the protoduino bootloader.
 *
 * The bootloader stays in flash after it has started the system, so the
 * system can use parts of it as a ROM. The last twelve bytes of flash hold
 * a table. It contains addresses, not jumps, so that neither the reach of a
 * relative jump nor linker relaxation can change its layout:
 *
 *   +0   byte address of elf_load_thread()   (0: not built into this bootloader)
 *   +4   byte address of flash_page_write()
 *   +8   version of this table
 *   +9   sizeof(struct elf_load) of the bootloader's build, in units of 4 bytes
 *   +10  'p'
 *   +11  'D'
 *
 * - Independent from process.c/process.h
 * - elf_load_thread() is the protothread of ./src/sys/elf/load.h: the system
 *   schedules the very same code that loaded its own image, with a workspace
 *   that the system owns.
 */

#include <protoduino.h>

#ifdef __AVR__
#include <avr/io.h>
#include <avr/pgmspace.h>
#endif

#define BOOTROM_VERSION 2

// byte address of the table. Tests move it: the ELF reader of simavr
// cannot load the last byte of flash.
#ifndef BOOTROM_CONF_TABLE
#define BOOTROM_CONF_TABLE (FLASHEND - 11)
#endif

#if (FLASHEND > 0xFFFFUL)
typedef uint32_t bootrom_addr_t;
#define BOOTROM_RD8(off)  pgm_read_byte_far((uint32_t)(BOOTROM_CONF_TABLE) + (off))
#define BOOTROM_RD32(off) pgm_read_dword_far((uint32_t)(BOOTROM_CONF_TABLE) + (off))
#else
typedef uint16_t bootrom_addr_t;
#define BOOTROM_RD8(off)  pgm_read_byte((uint16_t)(BOOTROM_CONF_TABLE) + (off))
#define BOOTROM_RD32(off) pgm_read_dword((uint16_t)(BOOTROM_CONF_TABLE) + (off))
#endif

struct elf_load;

typedef ptstate_t (*bootrom_load_thread_fn)(struct elf_load *self);
typedef void (*bootrom_page_write_fn)(bootrom_addr_t base, const uint8_t *data);

// true when a protoduino bootloader with a service table is installed.
static CC_ALWAYS_INLINE bool bootrom_is_present(void)
{
  return BOOTROM_RD8(10) == 'p' && BOOTROM_RD8(11) == 'D';
}

static CC_ALWAYS_INLINE uint8_t bootrom_version(void)
{
  return BOOTROM_RD8(8);
}

// bytes of workspace the load thread of the bootloader works on.
static CC_ALWAYS_INLINE uint16_t bootrom_load_size(void)
{
  return (uint16_t)BOOTROM_RD8(9) * 4;
}

// the load thread of the bootloader, or 0 when it was built without.
static CC_ALWAYS_INLINE bootrom_load_thread_fn bootrom_load_thread(void)
{
  return (bootrom_load_thread_fn)(uint16_t)(BOOTROM_RD32(0) >> 1);
}

// erase and program one page of flash from a buffer.
static CC_ALWAYS_INLINE bootrom_page_write_fn bootrom_page_write(void)
{
  return (bootrom_page_write_fn)(uint16_t)(BOOTROM_RD32(4) >> 1);
}

#endif /* __BOOTROM_H__ */

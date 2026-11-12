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
 * - The services are called, not fetched: bootrom_load_thread(self) and
 *   bootrom_page_write(base, data) reach the boot section wherever it lies.
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

// true when a protoduino bootloader with a service table is installed.
static CC_ALWAYS_INLINE bool bootrom_is_present(void)
{
  return BOOTROM_RD8(10) == 'p' && BOOTROM_RD8(11) == 'D';
}

static CC_ALWAYS_INLINE uint8_t bootrom_version(void)
{
  return BOOTROM_RD8(8);
}

// true when the bootloader was built with its load thread.
static CC_ALWAYS_INLINE bool bootrom_has_load_thread(void)
{
  return BOOTROM_RD32(0) != 0;
}

// bytes of workspace the load thread of the bootloader works on.
static CC_ALWAYS_INLINE uint16_t bootrom_load_size(void)
{
  return (uint16_t)BOOTROM_RD8(9) * 4;
}

#if defined(__AVR_3_BYTE_PC__)

/* Parts with more than 128 KB of flash. A function pointer has 16 bits and
 * reaches the first 128 KB; the boot section lies above. The two routines
 * below put the word address of the service on the stack and return to it,
 * so the service returns to whoever called the routine.
 *
 * - EIND is not touched: an interrupt handler that calls through a pointer
 *   while the service runs still arrives where it should.
 * - The address is the last argument, so the arguments of the service are
 *   already in the registers the service expects them in.
 * - They are written at file level and only declared to the compiler, which
 *   therefore assumes nothing about them. Every file that includes this
 *   header carries a copy; the linker keeps one.
 */

// self: r24-r25, word_address: r20-r23
ptstate_t bootrom_far_load_thread(struct elf_load *self, uint32_t word_address);

// base: r22-r25, data: r20-r21, word_address: r16-r19
void bootrom_far_page_write(bootrom_addr_t base, const uint8_t *data, uint32_t word_address);

__asm__ (
  ".pushsection .text.bootrom_far_load_thread,\"axG\",@progbits,bootrom_far_load_thread,comdat\n"
  ".weak bootrom_far_load_thread\n"
  ".type bootrom_far_load_thread, @function\n"
  "bootrom_far_load_thread:\n"
  "  push r20\n"
  "  push r21\n"
  "  push r22\n"
  "  ret\n"
  ".size bootrom_far_load_thread, .-bootrom_far_load_thread\n"
  ".popsection\n"
  ".pushsection .text.bootrom_far_page_write,\"axG\",@progbits,bootrom_far_page_write,comdat\n"
  ".weak bootrom_far_page_write\n"
  ".type bootrom_far_page_write, @function\n"
  "bootrom_far_page_write:\n"
  "  push r16\n"
  "  push r17\n"
  "  push r18\n"
  "  ret\n"
  ".size bootrom_far_page_write, .-bootrom_far_page_write\n"
  ".popsection\n"
);

// give the load thread of the bootloader one turn.
static CC_ALWAYS_INLINE ptstate_t bootrom_load_thread(struct elf_load *self)
{
  return bootrom_far_load_thread(self, BOOTROM_RD32(0) >> 1);
}

// erase and program one page of flash from a buffer.
static CC_ALWAYS_INLINE void bootrom_page_write(bootrom_addr_t base, const uint8_t *data)
{
  bootrom_far_page_write(base, data, BOOTROM_RD32(4) >> 1);
}

#else

typedef ptstate_t (*bootrom_load_thread_fn)(struct elf_load *self);
typedef void (*bootrom_page_write_fn)(bootrom_addr_t base, const uint8_t *data);

// give the load thread of the bootloader one turn.
static CC_ALWAYS_INLINE ptstate_t bootrom_load_thread(struct elf_load *self)
{
  return ((bootrom_load_thread_fn)(uint16_t)(BOOTROM_RD32(0) >> 1))(self);
}

// erase and program one page of flash from a buffer.
static CC_ALWAYS_INLINE void bootrom_page_write(bootrom_addr_t base, const uint8_t *data)
{
  ((bootrom_page_write_fn)(uint16_t)(BOOTROM_RD32(4) >> 1))(base, data);
}

#endif

#endif /* __BOOTROM_H__ */

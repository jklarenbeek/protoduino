// file: ./src/sys/elf/load.h

#ifndef __ELF_LOAD_H__
#define __ELF_LOAD_H__

/* Load thread: receives an ELF file with XMODEM-CRC and programs it into flash.
 *
 * - A Protothreads v2 thread. It never blocks: between two bytes of the file
 *   it returns PT_WAITING, so whoever schedules it decides what else runs.
 *   Schedule it whenever its mailbox has news, or simply on every pass.
 * - Independent from process.c/process.h
 * - Does no I/O and reads no clock. Bytes go in and out through a mailbox,
 *   and elf_load_timeout() tells the thread that the line stayed silent.
 *   The bootloader fills the mailbox from a polled UART; a process fills
 *   it from its pipes (see ./src/sys/process/loader.h).
 * - All state lives in struct elf_load, owned by the caller. The same code can
 *   therefore run in the boot section for the bootloader and, called through
 *   ./src/sys/bootrom.h, for the running system.
 *
 * Results of elf_load_thread(), also kept in self->result:
 *   PT_WAITING   feed the mailbox, take the replies, report silence
 *   PT_ENDED     the file is in flash; self->entry holds its entry point
 *   PT_EXITED    nobody started to send within self->tries probes
 *   error        the file was refused; the sender was told why (see elf.h)
 *     ERR_PROTO_SEQUENCE  a block was skipped
 *     ERR_BUF_UNDERFLOW   the sender finished before the file was complete
 *     ERR_VALID_CONSTRAINT the file was built for another device
 *     ERR_INIT_DEPENDENCY the module was linked against another kernel build
 *
 * Usage:
 *
 *   static struct elf_load load;
 *
 *   elf_load_init(&load, lo, hi, ELF_LOAD_MODULE, 30);
 *   do
 *   {
 *     if (received(&c) && elf_load_write_available(&load))
 *       elf_load_write8(&load, c);
 *     else if (silent_for_a_second())
 *       elf_load_timeout(&load);
 *
 *     state = elf_load_thread(&load);
 *
 *     // replies have to be taken before the thread is called again
 *     while (elf_load_read_available(&load))
 *       send(elf_load_read8(&load));
 *   }
 *   while (PT_ISRUNNING(state));
 */

#include <stdbool.h>
#include "../elf.h"

#ifdef __AVR__
#include <avr/io.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// what the file is, passed to elf_load_init()
#define ELF_LOAD_IMAGE   0x00  // the firmware image itself (bootloader only)
#define ELF_LOAD_MODULE  0x80  // a module: only pages inside the window change,
                               // and the file has to carry self->key

// where a module carries its key: the two bytes at this offset from the
// start of the window (kernel_id in ./src/sys/module.h)
#define ELF_LOAD_KEY_OFFSET 4

// self->flags
#define ELF_LOAD_RX      0x01  // mailbox: rx holds a byte the thread has not taken yet
#define ELF_LOAD_SILENT  0x02  // mailbox: the line stayed silent
#define ELF_LOAD_STARTED 0x04  // the first block has been accepted
#define ELF_LOAD_PAGE    0x08  // page[] holds a page that is not in flash yet
#define ELF_LOAD_DIRTY   0x10  // flash has been changed by this load

// one XMODEM block: SOH, number, complement, 128 data bytes, CRC-16
#define ELF_LOAD_BLOCK_SIZE 133

#if defined(SPM_PAGESIZE)
#define ELF_LOAD_PAGE_SIZE SPM_PAGESIZE
#else
#define ELF_LOAD_PAGE_SIZE 128
#endif

struct elf_load {
  struct pt    pt;        /* MUST be first - protothread state                */

  /* The head: the same layout in every build for one device, because a
   * system may hand its workspace to the thread inside the bootloader. */
  uint8_t      flags;     /* mailbox state, progress and ELF_LOAD_MODULE      */
  uint8_t      rx;        /* mailbox: byte for the thread                     */
  uint8_t      txn;       /* mailbox: bytes waiting in tx[]                   */
  uint8_t      tx[3];     /* mailbox: bytes for the sender, last one first    */
  uint8_t      tries;     /* probes left before the thread exits              */
  uint8_t      result;    /* how the load ended: PT_ENDED, PT_EXITED or error */
  uint8_t      seq;       /* number of the block expected next                */
  elf_addr_t   lo;        /* flash window [lo, hi) the file may touch         */
  elf_addr_t   hi;
  elf_addr_t   entry;     /* e_entry of the loaded file                       */
  uint16_t     key;       /* ELF_LOAD_MODULE: what the file must carry        */

  /* Private to the build that runs the thread; sized by bootrom_load_size(). */
  elf_addr_t   base;      /* address of the page held in page[]               */
  elf_addr_t   end;       /* highest flash address written + 1, by page       */
  uint16_t     start;     /* word address a firmware image is started at      */
  uint8_t      fill;      /* bytes of the block received so far               */
  uint8_t      block[ELF_LOAD_BLOCK_SIZE];
  elf_stream_t stream;
  uint8_t      page[ELF_LOAD_PAGE_SIZE];
};

/**
 * Prepare a load.
 *
 * \param self  The workspace.
 * \param lo    First flash address the file may touch.
 * \param hi    First flash address behind the window.
 * \param mode  ELF_LOAD_IMAGE or ELF_LOAD_MODULE. For a module, set
 *              self->key afterwards.
 * \param tries Probes to send before giving up on a silent line; one probe
 *              follows each elf_load_timeout().
 */
static CC_ALWAYS_INLINE void elf_load_init(struct elf_load *self, elf_addr_t lo, elf_addr_t hi,
                                           uint8_t mode, uint8_t tries)
{
  PT_INIT(&self->pt);
  self->flags = mode;
  self->txn   = 0;
  self->tries = tries;
  self->lo    = lo;
  self->hi    = hi;
}

/**
 * The load thread. Schedule it until it stops running.
 */
PT_THREAD(elf_load_thread(struct elf_load *self));

/* The mailbox, named after ./src/sys/serial.h -------------------------- */

// true when the thread can be given a byte.
static CC_ALWAYS_INLINE bool elf_load_write_available(const struct elf_load *self)
{
  return (self->flags & ELF_LOAD_RX) == 0;
}

// give the thread the next byte from the sender.
static CC_ALWAYS_INLINE void elf_load_write8(struct elf_load *self, const uint8_t data)
{
  self->rx = data;
  self->flags |= ELF_LOAD_RX;
}

// tell the thread that the sender has been silent for about a second.
static CC_ALWAYS_INLINE void elf_load_timeout(struct elf_load *self)
{
  self->flags |= ELF_LOAD_SILENT;
}

// number of bytes the thread has for the sender. Check this after every
// call of the thread, also after the one that ended it, and take them all
// before the thread is called again.
static CC_ALWAYS_INLINE uint8_t elf_load_read_available(const struct elf_load *self)
{
  return self->txn;
}

// take the next byte for the sender.
static CC_ALWAYS_INLINE uint8_t elf_load_read8(struct elf_load *self)
{
  return self->tx[--self->txn];
}

#if ELF_LOAD_CONF_IMAGE
/* Supplied by the bootloader: called before a page is programmed. This is
 * where the installed image stops being valid. */
void elf_load_image_page(struct elf_load *self);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ELF_LOAD_H__ */

// file: ./src/sys/elf/load.c

#include "load.h"
#include "../flash.h"

#include <stddef.h>
#include <avr/pgmspace.h>
#include <util/crc16.h>
#if ELF_CONF_EEPROM
#include <avr/eeprom.h>
#endif
#if ELF_CONF_SIGNATURE
#include <avr/boot.h>
#endif

// XMODEM
#define ELF_LOAD_SOH 0x01  // start of a 128 byte block
#define ELF_LOAD_EOT 0x04  // end of transmission
#define ELF_LOAD_ACK 0x06  // block accepted
#define ELF_LOAD_NAK 0x15  // send the block again
#define ELF_LOAD_CAN 0x18  // transfer cancelled
#define ELF_LOAD_CRC 'C'   // probe: start sending, with CRC-16

#if (ELF_LOAD_PAGE_SIZE > 255)
typedef uint16_t elf_load_index_t;
#else
typedef uint8_t elf_load_index_t;
#endif

// the workspace a stream belongs to
#define ELF_LOAD_OF(s) ((struct elf_load *)((uint8_t *)(s) - offsetof(struct elf_load, stream)))

/**
 * Block and wait for the next byte from the sender, or for the line to
 * stay silent.
 */
#define ELF_LOAD_WAIT_RX(pt, self) \
  PT_WAIT_UNTIL((pt), (self)->flags & (ELF_LOAD_RX | ELF_LOAD_SILENT))

/**
 * Raise an exception and run its handler within this call. PT_RAISE() takes
 * effect the next time the thread is scheduled, which for this thread is
 * the next byte or a second of silence; the sender should not wait for a
 * refusal that long.
 */
#define ELF_LOAD_RAISE(pt, self, err) \
  do { \
    LC_RAISE((pt)->lc, (err)); \
    return elf_load_thread(self); \
  } while (0)

// leave one byte for the sender in the mailbox
#define ELF_LOAD_REPLY(self, data) \
  do { \
    (self)->tx[0] = (data); \
    (self)->txn = 1; \
  } while (0)

/* ------------------------------------------------------------------ */
/* Page cache: the one page of flash that is being put together        */
/* ------------------------------------------------------------------ */

static void elf_load_page_commit(struct elf_load *self)
{
  if (!(self->flags & ELF_LOAD_PAGE))
    return;
  self->flags &= ~ELF_LOAD_PAGE;

#if ELF_LOAD_CONF_IMAGE
  elf_load_image_page(self);
#endif
  // the first change to flash: the headers have been accepted by now
  self->flags |= ELF_LOAD_DIRTY;

  if (self->base >= self->end)
    self->end = self->base + ELF_LOAD_PAGE_SIZE;
  flash_page_write(self->base, self->page);
}

static void elf_load_page_open(struct elf_load *self, elf_addr_t base)
{
  elf_load_index_t i = 0;

  elf_load_page_commit(self);
  do
  {
    self->page[i] = 0xFF;
  }
  while (++i != (elf_load_index_t)ELF_LOAD_PAGE_SIZE);
  self->base = base;
  self->flags |= ELF_LOAD_PAGE;
}

/* The sink of the stream (see ./src/sys/elf.h) */
uint8_t elf_stream_sink(elf_stream_t *s, uint8_t space, elf_addr_t addr, uint8_t byte)
{
  struct elf_load *self = ELF_LOAD_OF(s);

#if ELF_CONF_EEPROM
  if (space == ELF_SPACE_EEPROM)
  {
    eeprom_update_byte((uint8_t *)(uint16_t)addr, byte);
    return ERR_SUCCESS;
  }
#endif
#if ELF_CONF_SIGNATURE
  if (space == ELF_SPACE_SIGNATURE)
  {
    // the file stores the signature as sig2, sig1, sig0
    return boot_signature_byte_get((uint8_t)(4 - 2 * (uint8_t)addr)) == byte
      ? ERR_SUCCESS
      : ERR_VALID_CONSTRAINT;
  }
#endif
  (void)space;

#if ELF_LOAD_CONF_KEY
  // A module names the kernel build it was linked against in its first
  // page. A module for another build is refused here, before that page
  // or any other is programmed.
  if (self->flags & ELF_LOAD_MODULE)
  {
    elf_addr_t offset = addr - self->lo;

    if ((offset == ELF_LOAD_KEY_OFFSET     && byte != (uint8_t)self->key)
     || (offset == ELF_LOAD_KEY_OFFSET + 1 && byte != (uint8_t)(self->key >> 8)))
      return ERR_INIT_DEPENDENCY;
  }
#endif

  elf_addr_t base = addr & ~(elf_addr_t)(ELF_LOAD_PAGE_SIZE - 1);
  if (!(self->flags & ELF_LOAD_PAGE) || base != self->base)
    elf_load_page_open(self, base);
  self->page[(elf_load_index_t)addr & (ELF_LOAD_PAGE_SIZE - 1)] = byte;
  return ERR_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* One received frame                                                  */
/* ------------------------------------------------------------------ */

/**
 * Judge the frame in self->block and leave the answer for the sender in
 * the mailbox. A frame is a whole block, the single byte that is not the
 * start of one, or nothing at all when the line stayed silent.
 *
 * Returns PT_WAITING to go on, PT_ENDED when the sender has finished a
 * complete file, PT_EXITED when nobody started to send, or the error the
 * file is refused with.
 */
static uint8_t elf_load_frame(struct elf_load *self)
{
  const uint8_t *b = self->block;
  uint8_t reply = ELF_LOAD_NAK;
  uint8_t r = PT_WAITING;

  if (self->fill == 0)
  {
    // Silence. The sender never saw our last answer, or has not started.
    if (!(self->flags & ELF_LOAD_STARTED))
    {
      if (self->tries == 0)
        return PT_EXITED;
      self->tries--;
      reply = ELF_LOAD_CRC;
    }
  }
  else if (b[0] != ELF_LOAD_SOH)
  {
    if (b[0] != ELF_LOAD_EOT)
      return PT_WAITING; // noise: nothing to say

    // the sender is done: the file must be complete by now
    if (self->stream.result != PT_ENDED)
      return ERR_BUF_UNDERFLOW;
    elf_load_page_commit(self);
    self->entry = self->stream.entry;
    reply = ELF_LOAD_ACK;
    r = PT_ENDED;
  }
  else if (self->fill == ELF_LOAD_BLOCK_SIZE && (uint8_t)(b[1] + b[2]) == 0xFF)
  {
    uint16_t crc = 0;
    uint8_t i;

    // data and CRC together must come to zero
    for (i = 3; i != ELF_LOAD_BLOCK_SIZE; i++)
      crc = _crc_xmodem_update(crc, b[i]);

    if (crc == 0)
    {
      if (b[1] == self->seq)
      {
        self->seq++;
        self->flags |= ELF_LOAD_STARTED;
        // pages are in flash before the sender is told to go on
        r = (uint8_t)elf_stream_write(&self->stream, b + 3, 128);
        if (r >= PT_ERROR)
          return r;
        r = PT_WAITING;
      }
      else if (b[1] != (uint8_t)(self->seq - 1))
      {
        // not the block we wait for, and not a repeat of the last one,
        // which is acknowledged again without being applied twice
        return ERR_PROTO_SEQUENCE;
      }
      reply = ELF_LOAD_ACK;
    }
  }

  ELF_LOAD_REPLY(self, reply);
  return r;
}

/* ------------------------------------------------------------------ */
/* The thread                                                          */
/* ------------------------------------------------------------------ */

PT_THREAD(elf_load_thread(struct elf_load *self))
{
  struct pt *pt = &self->pt;

  PT_BEGIN(pt);

  self->seq = 1;
  self->end = 0;
  elf_stream_init(&self->stream, self->lo, self->hi);

  do
  {
    // Collect one frame: a block that starts with SOH, or the single byte
    // that is not the start of one, or nothing when the line stays silent.
    for (self->fill = 0; self->fill != ELF_LOAD_BLOCK_SIZE; )
    {
      uint8_t event;

      ELF_LOAD_WAIT_RX(pt, self);
      // take what the mailbox holds: a byte, or the news that there is none
      event = self->flags & (ELF_LOAD_RX | ELF_LOAD_SILENT);
      self->flags ^= event;
      if (!(event & ELF_LOAD_RX))
        break;
      self->block[self->fill++] = self->rx;
      if (self->block[0] != ELF_LOAD_SOH)
        break;
    }

    // Judge it. The answer for the sender is in the mailbox afterwards.
    self->result = elf_load_frame(self);
    if (self->result >= PT_ERROR)
      ELF_LOAD_RAISE(pt, self, self->result);
    if (self->result == PT_EXITED)
      PT_EXIT(pt); // nobody is sending
  }
  while (self->result != PT_ENDED);

  PT_CATCHANY(pt)
  {
    // Refused: cancel the transfer, tell the sender why, and end with the
    // error that was raised.
    self->tx[2] = ELF_LOAD_CAN;
    self->tx[1] = ELF_LOAD_CAN;
    self->tx[0] = self->result;
    self->txn = 3;
    PT_THROW(pt, self->result);
  }

  PT_END(pt);
}

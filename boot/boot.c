// file: ./boot/boot.c

/* The protoduino bootloader.
 *
 *   transport : XMODEM-CRC over uart0, polled (any terminal program can send)
 *   payload   : the unmodified .elf file produced by avr-gcc
 *   safety    : the image record is written last; no record, no start
 *   services  : the running system can call back in, to have a page
 *               programmed or to schedule the load thread itself
 *               (see ./src/sys/bootrom.h)
 *
 * The bootloader is one protothread, elf_load_thread(), and the loop below
 * that schedules it. At reset nothing else exists, so the loop is the whole
 * system. Once the image runs, the same thread can be scheduled by a
 * process (see ./src/mod/loader.h) while every other process keeps
 * its turn.
 *
 * One translation unit: the sources are included below, so that the
 * compiler sees the stream, its sink and the flash driver together.
 */

#include "boot.conf.h"

#include <protoduino.h>
#include <sys/elf/load.h>
#include <sys/flash.h>
#include <sys/bootrom.h>

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <util/crc16.h>
#include <util/delay_basic.h>

// the load thread keeps working on the workspace it is handed, also when
// the only caller in this file hands it boot_load
PT_THREAD(elf_load_thread(struct elf_load *self)) __attribute__((noinline, noclone, used, externally_visible));
void flash_page_write(flash_addr_t base, const uint8_t *data) __attribute__((noinline, noclone, used, externally_visible));

#include <sys/elf.c>
#include <cpu/avr/flash.c>

#if BOOT_CONF_SOFT_UART
#include "softuart.h"
#else
#include <sys/uart.h>
#include <cpu/avr/uart0.c>
#endif

/* ------------------------------------------------------------------ */
/* Image record                                                        */
/* ------------------------------------------------------------------ */

/* The page below the bootloader. It is erased before the first page of a
 * new image is programmed and written after the last one, so a record that
 * checks out means a complete image.
 *
 *   [0..1]  'p' 'D'
 *   [2..3]  entry of the image (word address)
 *   [4..6]  end of the image
 *   [7..8]  CRC-16 of the image
 *   [9..10] CRC-16 of this record
 */
#define BOOT_RECORD      ((elf_addr_t)(BOOT_CONF_START - SPM_PAGESIZE))
#define BOOT_RECORD_SIZE 11

#if (FLASHEND > 0xFFFFUL)
#define BOOT_RD8(a) pgm_read_byte_far(a)
#else
#define BOOT_RD8(a) pgm_read_byte(a)
#endif

/* Called by the load thread before a page is programmed (see load.h). */
void elf_load_image_page(struct elf_load *self)
{
#if BOOT_CONF_LOAD_SERVICE
  // a module leaves the image and its record alone
  if (self->flags & ELF_LOAD_MODULE)
    return;
#endif

#if BOOT_CONF_VIRTUAL_RESET
  // No boot section: every reset must still reach the bootloader. Keep the
  // start address of the image and aim its reset vector at us.
  if (self->base == 0)
  {
#if (FLASHEND > 0x1FFFU)
    self->start = (uint16_t)self->page[2] | ((uint16_t)self->page[3] << 8); // jmp k
    self->page[2] = (uint8_t)((BOOT_CONF_START / 2) & 0xFF);
    self->page[3] = (uint8_t)((BOOT_CONF_START / 2) >> 8);
#else
    uint16_t v = (uint16_t)self->page[0] | ((uint16_t)self->page[1] << 8);   // rjmp k
    self->start = (v + 1) & 0x0FFF;
    v = 0xC000 | ((BOOT_CONF_START / 2 - 1) & 0x0FFF);
    self->page[0] = (uint8_t)v;
    self->page[1] = (uint8_t)(v >> 8);
#endif
  }
#endif

  if (self->flags & ELF_LOAD_DIRTY)
    return;

  // The installed image stops being valid before a single byte of it changes.
#if BOOT_CONF_VIRTUAL_RESET
  // Top down, page 0 last: at every instant a reset reaches the bootloader,
  // through the old vector or by running through erased flash.
  {
    elf_addr_t a = BOOT_RECORD;

    for (;;)
    {
      flash_page_erase(a);
      if (a == 0)
        break;
      a -= SPM_PAGESIZE;
    }
  }
#else
  flash_page_erase(BOOT_RECORD);
#endif
}

// the load thread itself, after the hook it calls
#include <sys/elf/load.c>

// The workspace of the load thread while the bootloader itself runs it. A
// system that schedules the thread through the service table brings its own.
// Not cleared at reset: elf_load_init() and the thread set every field
// before they read it, and the bootloader has no other variables.
static struct elf_load boot_load __attribute__((section(".noinit")));

static CC_NO_INLINE uint16_t boot_crc(elf_addr_t from, elf_addr_t to)
{
  uint16_t crc = 0;

  while (from != to)
    crc = _crc_xmodem_update(crc, BOOT_RD8(from++));
  return crc;
}

static bool boot_record_ok(void)
{
  if (BOOT_RD8(BOOT_RECORD) != 'p' || BOOT_RD8(BOOT_RECORD + 1) != 'D')
    return false;
  // data and CRC together must come to zero
  if (boot_crc(BOOT_RECORD, BOOT_RECORD + BOOT_RECORD_SIZE) != 0)
    return false;

#if BOOT_CONF_VERIFY
  {
    elf_addr_t end = (elf_addr_t)BOOT_RD8(BOOT_RECORD + 4)
                   | ((elf_addr_t)BOOT_RD8(BOOT_RECORD + 5) << 8)
#if ELF_CONF_ADDR_BITS > 16
                   | ((elf_addr_t)BOOT_RD8(BOOT_RECORD + 6) << 16)
#endif
                   ;
    uint16_t crc = (uint16_t)BOOT_RD8(BOOT_RECORD + 8) << 8 | BOOT_RD8(BOOT_RECORD + 7);

    if (boot_crc(0, end) != crc)
      return false;
  }
#endif
  return true;
}

static CC_NO_INLINE void boot_record_write(struct elf_load *self)
{
  uint8_t *r = self->page; // the page buffer is free again by now
  uint8_t *p = r;
  uint16_t crc = 0;
  elf_load_index_t k = 0;

  do
  {
    r[k] = 0xFF;
  }
  while (++k != (elf_load_index_t)SPM_PAGESIZE);

  *p++ = 'p';
  *p++ = 'D';
  *p++ = (uint8_t)self->start;
  *p++ = (uint8_t)(self->start >> 8);
  *p++ = (uint8_t)self->end;
  *p++ = (uint8_t)(self->end >> 8);
#if ELF_CONF_ADDR_BITS > 16
  *p++ = (uint8_t)(self->end >> 16);
#else
  *p++ = 0;
#endif
#if BOOT_CONF_IMAGE_CRC
  crc = boot_crc(0, self->end);
#endif
  *p++ = (uint8_t)crc;
  *p++ = (uint8_t)(crc >> 8);

  crc = 0;
  for (k = 0; k != BOOT_RECORD_SIZE - 2; k++)
    crc = _crc_xmodem_update(crc, r[k]);
  *p++ = (uint8_t)(crc >> 8);
  *p   = (uint8_t)crc;

  flash_page_write(BOOT_RECORD, r);
}

/* ------------------------------------------------------------------ */
/* Start of the image                                                  */
/* ------------------------------------------------------------------ */

static void boot_image_start(void) __attribute__((noreturn));
static void boot_image_start(void)
{
#if !BOOT_CONF_SOFT_UART
  // not uart0_close(): it ends with sei(), and the image has not set up
  // its interrupt handlers yet. The image finds the UART as a reset leaves
  // it: off, single speed, divisor zero.
  UCSR0B = 0;
#if BOOT_CONF_UART_RESET
  UCSR0A = 0;
  UBRR0H = 0;
  UBRR0L = 0;
#endif
#endif
#ifdef RAMPZ
  RAMPZ = 0;
#endif
  // The entry is a word address in the image record. IJMP leaves nothing on
  // the stack and, on parts with a 22-bit program counter, clears its upper
  // bits, so the image starts as it would after a reset.
#if (FLASHEND > 0xFFFFUL)
  uint16_t entry = pgm_read_word_far(BOOT_RECORD + 2);
#else
  uint16_t entry = pgm_read_word(BOOT_RECORD + 2);
#endif
  __asm__ __volatile__ ("ijmp" : : "z" (entry));
  __builtin_unreachable();
}

/* ------------------------------------------------------------------ */
/* The scheduler of a system with one thread                           */
/* ------------------------------------------------------------------ */

/* uart0_open() works out its divisor at run time, with 32-bit divisions
 * that cost 286 bytes here, and enables the receive interrupt. A bootloader
 * knows its baud rate when it is built and polls: same registers, set
 * directly. Everything after this goes through ./src/cpu/avr/uart.h. */
static CC_ALWAYS_INLINE void boot_uart_open(void)
{
#if BOOT_CONF_SOFT_UART
  uart0_open(BOOT_CONF_BAUD);
#else
  UCSR0A = _BV(U2X0);
  UBRR0H = (uint8_t)(((F_CPU / 4 / BOOT_CONF_BAUD - 1) / 2) >> 8);
  UBRR0L = (uint8_t)((F_CPU / 4 / BOOT_CONF_BAUD - 1) / 2);
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);
#endif
}

static void boot_tx_write8(const uint8_t data)
{
  while (!uart0_tx_is_ready()) { }
  uart0_tx_write8(data);
}

#if BOOT_CONF_INFO
static const char boot_info[] PROGMEM = "pD2 " BOOT_CONF_INFO_TEXT "\r\n";

static void boot_info_write(void)
{
  uint8_t i = 0;
  uint8_t data;

#if (FLASHEND > 0xFFFFUL)
  while ((data = pgm_read_byte_far(pgm_get_far_address(boot_info) + i++)) != 0)
#else
  while ((data = pgm_read_byte(&boot_info[i++])) != 0)
#endif
    boot_tx_write8(data);
}
#endif

/* Schedule the load thread until it stops running. uart0 fills its mailbox
 * and takes its replies, the idle passes of this loop are its clock, and
 * the thread runs whenever one of the two has news for it. */
static CC_NO_INLINE ptstate_t boot_schedule(struct elf_load *self)
{
  uint16_t idle = 0;
#if BOOT_CONF_INFO
  // the first call lets the thread set itself up
  ptstate_t state = elf_load_thread(self);
#else
  ptstate_t state = PT_WAITING;
#endif

  do
  {
    if (uart0_rx_is_ready())
    {
      uint8_t data = uart0_rx_read8();

#if BOOT_CONF_INFO
      // '?' between two frames, before a transfer has started: say who we are
      if (data == '?' && !(self->flags & ELF_LOAD_STARTED) && self->fill == 0)
      {
        boot_info_write();
        continue;
      }
#endif
      elf_load_write8(self, data);
      idle = 0;
    }
    else
    {
#if BOOT_CONF_WATCHDOG
      // for a part whose fuses keep the watchdog running
      __asm__ __volatile__ ("wdr");
#endif
#if BOOT_CONF_IDLE_DELAY > 0
      _delay_loop_1(BOOT_CONF_IDLE_DELAY);
#endif
      // 65,536 idle passes in a row: nobody is sending
      if (++idle != 0)
        continue;
      elf_load_timeout(self);
    }

    state = elf_load_thread(self);

    while (elf_load_read_available(self))
      boot_tx_write8(elf_load_read8(self));
  }
  while (PT_ISRUNNING(state));

  return state;
}

#define BOOT_RESET_ANY (_BV(PORF) | _BV(EXTRF) | _BV(BORF) | _BV(WDRF))
#if (BOOT_RESET_ANY != 15)
#error "boot: the reset flags of this part are not where BOOT_CONF_WINDOW_AFTER expects them"
#endif
#define BOOT_WINDOW_ALWAYS \
  (!BOOT_CONF_WATCHDOG || ((BOOT_CONF_WINDOW_AFTER) & BOOT_RESET_ANY) == BOOT_RESET_ANY)

// true when this start should wait for a sender before the image starts.
// A cause of 0 is no reset at all: the image jumped here because it wants
// to be replaced.
static CC_ALWAYS_INLINE bool boot_window_wanted(void)
{
#if BOOT_WINDOW_ALWAYS
  return true;
#else
  uint8_t cause = BOOT_RESET_CAUSE;

  return cause == 0 || (cause & (BOOT_CONF_WINDOW_AFTER)) != 0;
#endif
}

void boot_main(void) __attribute__((noreturn, used, section(".init9")));
void boot_main(void)
{
  ptstate_t state;
  bool wait = boot_window_wanted();

  boot_uart_open();

  for (;;)
  {
    // PT_EXITED is what a silent line ends in. A reset that nobody uses to
    // send a file, such as the watchdog stopping a hung image, goes there
    // without waiting.
    state = PT_EXITED;
    if (wait)
    {
      // a firmware image may use everything below the image record
      elf_load_init(&boot_load, 0, BOOT_RECORD, ELF_LOAD_IMAGE, BOOT_CONF_WINDOW);
      state = boot_schedule(&boot_load);
    }
    wait = true;

    if (state == PT_ENDED)
    {
#if !BOOT_CONF_VIRTUAL_RESET
      boot_load.start = (uint16_t)(boot_load.entry >> 1);
#endif
      boot_record_write(&boot_load);
      boot_image_start();
    }

    // nobody is sending: start the image, if there is a complete one.
    // After a refusal the window opens again; a file that was refused
    // after flash had changed has left no record behind.
    if (state == PT_EXITED && boot_record_ok())
      boot_image_start();
  }
}

/* ------------------------------------------------------------------ */
/* Entry                                                               */
/* ------------------------------------------------------------------ */

/* No C runtime (-nostartfiles). The first word of the bootloader is a
 * landing pad, then .init0 sets up r1, SREG and the stack and execution
 * runs on into boot_main, which is placed in .init9. Nothing clears .bss:
 * there is none (see boot_load). */
void boot_vector(void) __attribute__((naked, used, section(".vectors")));
void boot_vector(void)
{
  __asm__ __volatile__(
#if BOOT_CONF_LANDING_PAD
    "nop\n"
#endif
    "rjmp boot_init\n");
}

void boot_init(void) __attribute__((naked, used, section(".init0")));
void boot_init(void)
{
  __asm__ __volatile__(
    "clr r1\n"
    "out 0x3f, r1\n"); // SREG = 0: interrupts stay off in the bootloader
  SP = RAMEND;

#if BOOT_CONF_WATCHDOG
  // After a watchdog reset the watchdog is still running, at its shortest
  // period, and stays on for as long as WDRF is set: a bootloader that
  // waits for a sender would be reset again before the first probe. So the
  // reset cause moves to where the image can read it, the flags are
  // cleared, and the watchdog is switched off. Where a fuse keeps it on,
  // that last write still gives it its longest period, and the scheduling
  // loop feeds it.
  BOOT_RESET_CAUSE = MCUSR;
  MCUSR = 0;
  BOOT_WDT = _BV(WDCE) | _BV(WDE);
  BOOT_WDT = _BV(WDP3) | _BV(WDP0);
#endif
}

/* ------------------------------------------------------------------ */
/* Service table (see ./src/sys/bootrom.h)                             */
/* ------------------------------------------------------------------ */

#if BOOT_CONF_PAGE_SERVICE
__asm__(".section .bootrom,\"a\",@progbits\n"
        "  .global bootrom_table\n"
        "bootrom_table:\n"
#if BOOT_CONF_LOAD_SERVICE
        "  .long elf_load_thread\n"
#else
        "  .long 0\n"
#endif
        "  .long flash_page_write\n"
        "  .text\n");

const uint8_t bootrom_info[4] __attribute__((used, section(".bootrom_info"))) = {
  BOOTROM_VERSION, (uint8_t)((sizeof(struct elf_load) + 3) / 4), 'p', 'D'
};
#endif

// file: ./src/cpu/avr/flash.c

#include "flash.h"

#if FLASH_CONF_DRIVER != FLASH_DRIVER_NONE

#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

#if (SPM_PAGESIZE > 255)
typedef uint16_t flash_index_t;
#else
typedef uint8_t flash_index_t;
#endif

#if FLASH_CONF_DRIVER == FLASH_DRIVER_SPM

bool flash_is_writable(void)
{
  return true;
}

#if FLASH_CONF_SPM_ASM

/* The SPM sequences in assembly, in the way Optiboot writes them: the
 * control register is written with OUT, and one small routine issues every
 * SPM and waits for it. The registers are fixed by the instruction set:
 * Z holds the address, r1:r0 the word for the page buffer.
 *
 *   command  r24     what to do, a value for SPMCSR
 *   address  Z       any address in the page
 */
#define FLASH_SPMCSR _SFR_IO_ADDR(__SPM_REG)

#define FLASH_SPM_ERASE (_BV(__SPM_ENABLE) | _BV(PGERS))
#define FLASH_SPM_FILL  (_BV(__SPM_ENABLE))
#define FLASH_SPM_WRITE (_BV(__SPM_ENABLE) | _BV(PGWRT))
#ifdef RWWSRE
#define FLASH_SPM_RWW   (_BV(__SPM_ENABLE) | _BV(RWWSRE))
#endif

// only the bootloader itself erases without programming, and it runs with
// interrupts disabled
void flash_page_erase(flash_addr_t base)
{
  uint8_t command = FLASH_SPM_ERASE;

#ifdef RAMPZ
  RAMPZ = (uint8_t)(base >> 16);
#endif
  __asm__ __volatile__ (
    "out  %[spmcsr], %[command]"  "\n\t"
    "spm"                         "\n\t"
    "1:"                          "\n\t"
    "in   %[command], %[spmcsr]"  "\n\t"
    "sbrc %[command], %[busy]"    "\n\t"
    "rjmp 1b"                     "\n\t"
    : [command] "+d" (command)
    : "z" ((uint16_t)base),
      [spmcsr] "I" (FLASH_SPMCSR),
      [busy] "I" (__SPM_ENABLE)
  );
}

// Touches no variables of its own, so it is safe to reach from the running
// system through the service table (see ./src/sys/bootrom.h).
CC_NO_INLINE void flash_page_write(flash_addr_t base, const uint8_t *data)
{
  uint8_t words = (uint8_t)(SPM_PAGESIZE / 2);
  uint16_t address = (uint16_t)base;
  uint8_t command;
#ifdef RAMPZ
  uint8_t rampz = RAMPZ;
#endif

  CC_ATOMIC_RESTORE()
  {
#ifdef RAMPZ
    RAMPZ = (uint8_t)(base >> 16);
#endif
    __asm__ __volatile__ (
      "ldi  %[command], %[erase]"   "\n\t"  // erase the page
      "rcall 9f"                    "\n\t"
      "1:"                          "\n\t"  // fill the page buffer, a word at a time
      "ld   r0, X+"                 "\n\t"
      "ld   r1, X+"                 "\n\t"
      "ldi  %[command], %[fill]"    "\n\t"
      "rcall 9f"                    "\n\t"
      "adiw r30, 2"                 "\n\t"
      "dec  %[words]"               "\n\t"
      "brne 1b"                     "\n\t"
      "clr  r1"                     "\n\t"
      "subi r30, lo8(%[size])"      "\n\t"  // back to the start of the page
      "sbci r31, hi8(%[size])"      "\n\t"
      "ldi  %[command], %[write]"   "\n\t"  // program it
      "rcall 9f"                    "\n\t"
#ifdef RWWSRE
      "ldi  %[command], %[rww]"     "\n\t"  // let the application section be read again
      "rcall 9f"                    "\n\t"
#endif
      "rjmp 8f"                     "\n\t"
      "9:"                          "\n\t"  // issue one SPM and wait for it
      "out  %[spmcsr], %[command]"  "\n\t"
      "spm"                         "\n\t"
      "7:"                          "\n\t"
      "in   %[command], %[spmcsr]"  "\n\t"
      "sbrc %[command], %[busy]"    "\n\t"
      "rjmp 7b"                     "\n\t"
      "ret"                         "\n\t"
      "8:"                          "\n\t"
      : [command] "=&d" (command),
        [words] "+d" (words),
        "+z" (address),
        "+x" (data)
      : [spmcsr] "I" (FLASH_SPMCSR),
        [busy] "I" (__SPM_ENABLE),
        [erase] "M" (FLASH_SPM_ERASE),
        [fill] "M" (FLASH_SPM_FILL),
        [write] "M" (FLASH_SPM_WRITE),
#ifdef RWWSRE
        [rww] "M" (FLASH_SPM_RWW),
#endif
        [size] "n" (SPM_PAGESIZE)
      : "r0", "memory"
    );
#ifdef RAMPZ
    RAMPZ = rampz;
#endif
  }
}

#else /* FLASH_CONF_SPM_ASM */

// only the bootloader itself erases without programming, and it runs with
// interrupts disabled
void flash_page_erase(flash_addr_t base)
{
  boot_page_erase(base);
  boot_spm_busy_wait();
}

// Touches no variables of its own, so it is safe to reach from the running
// system through the service table (see ./src/sys/bootrom.h).
CC_NO_INLINE void flash_page_write(flash_addr_t base, const uint8_t *data)
{
  flash_index_t i = 0;

  CC_ATOMIC_RESTORE()
  {
    flash_page_erase(base);
    do
    {
      boot_page_fill(base + i, (uint16_t)data[i] | ((uint16_t)data[i + 1] << 8));
      i += 2;
    }
    while (i != (flash_index_t)SPM_PAGESIZE);
    boot_page_write(base);
    boot_spm_busy_wait();
#ifdef RWWSRE
    // let the application section be read again
    boot_rww_enable();
#endif
  }
}

#endif /* FLASH_CONF_SPM_ASM */

#elif FLASH_CONF_DRIVER == FLASH_DRIVER_BOOTROM

#include "../../sys/bootrom.h"

bool flash_is_writable(void)
{
  return bootrom_is_present();
}

void flash_page_write(flash_addr_t base, const uint8_t *data)
{
  bootrom_page_write()(base, data);
}

#elif FLASH_CONF_DRIVER == FLASH_DRIVER_OPTIBOOT

// Optiboot 8 and later: the second word of the bootloader is "rjmp do_spm",
// and the last byte of flash holds its major version.
typedef void (*flash_do_spm_fn)(uint16_t address, uint8_t command, uint16_t data);

#if (FLASHEND > 0xFFFFUL)
#define FLASH_RD8(a) pgm_read_byte_far(a)
#else
#define FLASH_RD8(a) pgm_read_byte(a)
#endif

bool flash_is_writable(void)
{
  return FLASH_RD8(FLASHEND) >= 8
      // an rjmp, not the "ret" of a build without do_spm
      && (FLASH_RD8(FLASH_CONF_OPTIBOOT_START + 3) & 0xF0) == 0xC0;
}

void flash_page_write(flash_addr_t base, const uint8_t *data)
{
  flash_do_spm_fn do_spm = (flash_do_spm_fn)((FLASH_CONF_OPTIBOOT_START + 2) >> 1);
  flash_index_t i = 0;

  CC_ATOMIC_RESTORE()
  {
    do_spm(base, __BOOT_PAGE_ERASE, 0);
    do
    {
      do_spm(base + i, __BOOT_PAGE_FILL, (uint16_t)data[i] | ((uint16_t)data[i + 1] << 8));
      i += 2;
    }
    while (i != (flash_index_t)SPM_PAGESIZE);
    do_spm(base, __BOOT_PAGE_WRITE, 0);
  }
}

#endif

#endif /* FLASH_CONF_DRIVER != FLASH_DRIVER_NONE */

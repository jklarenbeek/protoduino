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

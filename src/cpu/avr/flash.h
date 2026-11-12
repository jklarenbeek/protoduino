// file: ./src/cpu/avr/flash.h

#ifndef __AVR_FLASH_H__
#define __AVR_FLASH_H__

/* Self-programming of the flash memory, one page at a time.
 *
 * An AVR can only execute SPM from its boot section, so a running system
 * needs somebody there to do it. FLASH_CONF_DRIVER names that somebody, or
 * leaves it to be found out at run time; the functions below read the same
 * whoever it is.
 *
 * While a page is erased and programmed the processor cannot execute code
 * from the application section, and interrupt handlers live there. A page
 * write therefore runs with interrupts disabled and takes about 8 ms.
 */

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <cc.h>
#include "flash.conf.h"

#if (FLASHEND > 0xFFFFUL)
typedef uint32_t flash_addr_t;
#else
typedef uint16_t flash_addr_t;
#endif

/**
 * @fn bool flash_is_writable(void)
 * @brief Checks if the selected driver is present and can program a page.
 *
 * @return true if flash_page_write() may be called, false otherwise.
 */
CC_EXTERN bool flash_is_writable(void);

/**
 * @fn void flash_page_write(flash_addr_t base, const uint8_t *data)
 * @brief Erases one page and programs it from a buffer.
 *
 * @param base The address of the first byte of the page.
 * @param data SPM_PAGESIZE bytes.
 */
CC_EXTERN void flash_page_write(flash_addr_t base, const uint8_t *data);

#if FLASH_CONF_DRIVER == FLASH_DRIVER_SPM
/**
 * @fn void flash_page_erase(flash_addr_t base)
 * @brief Erases one page. Only the boot section can do this on its own.
 *
 * @param base The address of the first byte of the page.
 */
CC_EXTERN void flash_page_erase(flash_addr_t base);
#endif

#endif /* __AVR_FLASH_H__ */

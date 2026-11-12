// file: ./src/sys/flash.h

#ifndef __FLASH_H__
#define __FLASH_H__

/*
 * Platform selector for self-programming of the flash memory.
 *
 * Add new architectures here as needed.
 */

#if defined(__AVR__)
#include "../cpu/avr/flash.h"
#else
#error "flash: Unsupported Platform!"
#endif

#endif /* __FLASH_H__ */

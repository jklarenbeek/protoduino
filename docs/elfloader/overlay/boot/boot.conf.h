// file: ./boot/boot.conf.h

#ifndef __BOOT_CONF_H__
#define __BOOT_CONF_H__

/* ------------------------------------------------------------------ */
/* Configuration of the bootloader (override on the command line)      */
/* ------------------------------------------------------------------ */

#include <avr/io.h>

// bytes of the boot section the BOOTSZ fuses reserve. The bootloader fills
// 2 KB on parts with up to 64 KB of flash; above that it takes 4 KB, where
// every option fits.
#ifndef BOOT_CONF_SECTION
#if (FLASHEND > 0xFFFFUL)
#define BOOT_CONF_SECTION 4096
#else
#define BOOT_CONF_SECTION 2048
#endif
#endif

// byte address the bootloader is linked at: the start of that boot section.
// The linker has to be given the same address. A part without a boot
// section has no default.
#ifndef BOOT_CONF_START
#if defined(BOOT_CONF_VIRTUAL_RESET) && BOOT_CONF_VIRTUAL_RESET
#error "boot: define BOOT_CONF_START (byte address the bootloader is linked at)"
#else
#define BOOT_CONF_START (FLASHEND + 1UL - BOOT_CONF_SECTION)
#endif
#endif

#ifndef BOOT_CONF_BAUD
#define BOOT_CONF_BAUD 115200UL
#endif

// probes to send, about one per second, before a valid image is started.
#ifndef BOOT_CONF_WINDOW
#define BOOT_CONF_WINDOW 3
#endif

// The scheduling loop counts its idle passes in 16 bits: 65,536 of them
// are the silence after which the load thread is told that nobody sends.
// An idle pass takes about 20 clock cycles; this many delay iterations of
// 3 cycles stretch it so that the silence lasts about one second.
#ifndef BOOT_CONF_IDLE_DELAY
#if (F_CPU / 65536UL > 23)
#define BOOT_CONF_IDLE_DELAY ((F_CPU / 65536UL - 20) / 3)
#else
#define BOOT_CONF_IDLE_DELAY 0
#endif
#endif

// flag to store a CRC-16 of the image in the image record.
#ifndef BOOT_CONF_IMAGE_CRC
#define BOOT_CONF_IMAGE_CRC 1
#endif

// flag to check that CRC again on every start.
#ifndef BOOT_CONF_VERIFY
#define BOOT_CONF_VERIFY 0
#endif

// flag to answer '?' with a line that describes this bootloader:
// "pD2 " BOOT_CONF_INFO_TEXT.
#ifndef BOOT_CONF_INFO
#define BOOT_CONF_INFO 0
#endif

#ifndef BOOT_CONF_INFO_TEXT
#define BOOT_CONF_INFO_TEXT ""
#endif

// flag to offer the load thread to the running system (see bootrom.h).
#ifndef BOOT_CONF_LOAD_SERVICE
#define BOOT_CONF_LOAD_SERVICE 0
#endif

// flag to offer flash_page_write() to the running system.
#ifndef BOOT_CONF_PAGE_SERVICE
#define BOOT_CONF_PAGE_SERVICE BOOT_CONF_LOAD_SERVICE
#endif

#if BOOT_CONF_LOAD_SERVICE && !BOOT_CONF_PAGE_SERVICE
#error "boot: BOOT_CONF_LOAD_SERVICE needs BOOT_CONF_PAGE_SERVICE"
#endif

// flag for parts without a boot section: the reset vector of the image
// is redirected to the bootloader.
#ifndef BOOT_CONF_VIRTUAL_RESET
#define BOOT_CONF_VIRTUAL_RESET 0
#endif

// flag for parts without a USART: serial on two port pins.
#ifndef BOOT_CONF_SOFT_UART
#define BOOT_CONF_SOFT_UART 0
#endif

// flag for the one-word no-op at the first address of the bootloader.
// Erased flash (0xFFFF) executes as a skip and may jump over one word.
#ifndef BOOT_CONF_LANDING_PAD
#define BOOT_CONF_LANDING_PAD 1
#endif

/* what the bootloader asks of the protothreads, of the load thread and of
 * the flash driver */

// continuation points numbered 1, 2, 3 ... instead of by source line: a
// number below 256 is stored and compared in one byte.
#ifndef LC_INCLUDE
#define LC_INCLUDE <sys/lc-counter.h>
#endif

#define ELF_LOAD_CONF_IMAGE 1
#define FLASH_CONF_DRIVER   1 /* FLASH_DRIVER_SPM */

// a module slot has a lower bound too
#if BOOT_CONF_LOAD_SERVICE && !defined(ELF_CONF_WINDOW_LO)
#define ELF_CONF_WINDOW_LO 1
#endif

#endif /* __BOOT_CONF_H__ */

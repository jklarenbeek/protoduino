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

// flag to take care of the watchdog. An application that is reset by the
// watchdog, or that resets itself through it, leaves it running; without
// this the bootloader is then reset over and over and the image never
// starts again. Off only for a part that has no watchdog reset.
#ifndef BOOT_CONF_WATCHDOG
#define BOOT_CONF_WATCHDOG 1
#endif

// where the bootloader leaves the reset cause (the MCUSR it found) for the
// image: MCUSR itself has to be cleared.
#ifndef BOOT_RESET_CAUSE
#define BOOT_RESET_CAUSE GPIOR0
#endif

// the reset causes after which the bootloader waits for a sender before it
// starts the image, as a mask of MCUSR flags: 1 power-on, 2 reset pin,
// 4 brown-out, 8 watchdog. After any other reset a complete image starts
// at once.
//    2   only after the reset pin was pulled, which is what an upload tool
//        and a reset button do; the rule Optiboot uses. A board starts at
//        once when power comes on.
//    7   also after power-on and brown-out, for a board without a reset
//        line that is updated by cycling its power
//   15   after every reset
// An image that jumps to the bootloader is always waited for: it wants to
// be replaced. Needs BOOT_CONF_WATCHDOG.
#ifndef BOOT_CONF_WINDOW_AFTER
#define BOOT_CONF_WINDOW_AFTER 2
#endif

// the watchdog control register under its two names
#ifdef WDTCSR
#define BOOT_WDT WDTCSR
#else
#define BOOT_WDT WDTCR
#endif

// flag to put the UART registers back to their reset values before the
// image starts. Without it the double-speed bit and the divisor keep the
// values of the bootloader, and an image that relies on the reset values
// runs at the wrong rate.
#ifndef BOOT_CONF_UART_RESET
#define BOOT_CONF_UART_RESET 1
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

/* what the bootloader asks of the stream, of the protothreads, of the load
 * thread and of the flash driver */

// Segments of a file are remembered one by one. Merging adjacent ones costs
// 62 bytes of flash to save six bytes of RAM per segment, and RAM is what a
// bootloader has: four entries hold code, initial data and two more.
#ifndef ELF_CONF_COALESCE
#define ELF_CONF_COALESCE 0
#endif
#ifndef ELF_CONF_MAX_SEGMENTS
#define ELF_CONF_MAX_SEGMENTS 4
#endif

// continuation points numbered 1, 2, 3 ... instead of by source line: a
// number below 256 is stored and compared in one byte.
#ifndef LC_INCLUDE
#define LC_INCLUDE <sys/lc-counter.h>
#endif

#define ELF_LOAD_CONF_IMAGE 1
#define FLASH_CONF_DRIVER   1 /* FLASH_DRIVER_SPM */

// a module slot has a lower bound too; a firmware image starts at 0
#ifndef ELF_CONF_WINDOW_LO
#define ELF_CONF_WINDOW_LO BOOT_CONF_LOAD_SERVICE
#endif

#endif /* __BOOT_CONF_H__ */

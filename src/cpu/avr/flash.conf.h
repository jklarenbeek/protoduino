// file: ./src/cpu/avr/flash.conf.h

#ifndef __AVR_FLASH_CONF_H__
#define __AVR_FLASH_CONF_H__

/* ------------------------------------------------------------------ */
/* Configuration (override in protoduino-config.h)                     */
/* ------------------------------------------------------------------ */

#define FLASH_DRIVER_NONE     0  // this build cannot change its own flash
#define FLASH_DRIVER_SPM      1  // this code executes SPM: it is linked into the boot section
#define FLASH_DRIVER_BOOTROM  2  // the protoduino bootloader does, through its service table
#define FLASH_DRIVER_OPTIBOOT 3  // Optiboot 8 or later does, through its do_spm entry
#define FLASH_DRIVER_AUTO     4  // whichever of the two is installed, found out at run time

// who executes the SPM instruction. The default serves a sketch on either
// bootloader without a setting; naming the one in use saves the code for
// the other.
#ifndef FLASH_CONF_DRIVER
#define FLASH_CONF_DRIVER FLASH_DRIVER_AUTO
#endif

// flag to program a page with a routine written in assembly, when this
// code executes SPM itself. It is half the size of what the compiler makes
// of the C version, which stays as the reference.
#ifndef FLASH_CONF_SPM_ASM
#define FLASH_CONF_SPM_ASM 1
#endif

// byte address Optiboot is linked at: 512 bytes below the end of flash,
// 1024 bytes on parts with more than 64 KB.
#ifndef FLASH_CONF_OPTIBOOT_START
#if (FLASHEND > 0xFFFFUL)
#define FLASH_CONF_OPTIBOOT_START (FLASHEND - 1023)
#else
#define FLASH_CONF_OPTIBOOT_START (FLASHEND - 511)
#endif
#endif

#endif /* __AVR_FLASH_CONF_H__ */

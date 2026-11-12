// file: ./src/sys/elf.conf.h

#ifndef __ELF_CONF_H__
#define __ELF_CONF_H__

/* ------------------------------------------------------------------ */
/* Configuration (override in protoduino-config.h)                     */
/* ------------------------------------------------------------------ */

/* The defaults are those of a kernel that loads modules with the load
 * thread of the library (LOADER_THREAD_OWN in ./src/mod/loader.h). The
 * bootloader sets its own in ./boot/boot.conf.h. A sketch cannot change
 * them: the library is compiled apart from it. */

// width of the file offsets and addresses the stream carries around:
// 16 covers every part with up to 64 KB of flash, 24 covers the rest.
#ifndef ELF_CONF_ADDR_BITS
#if defined(__AVR__)
#include <avr/io.h>
#if (FLASHEND > 0xFFFFUL)
#define ELF_CONF_ADDR_BITS 24
#else
#define ELF_CONF_ADDR_BITS 16
#endif
#else
#define ELF_CONF_ADDR_BITS 32
#endif
#endif

// loadable segments remembered between the program headers and their
// data. Adjacent segments are merged, so plain firmware needs one.
#ifndef ELF_CONF_MAX_SEGMENTS
#define ELF_CONF_MAX_SEGMENTS 3
#endif

// flag to refuse files built for another AVR core family
// (e_flags of the file against __AVR_ARCH__ of this build).
#ifndef ELF_CONF_ARCH_CHECK
#define ELF_CONF_ARCH_CHECK 1
#endif

// flag to merge segments that are contiguous in the file and in memory.
#ifndef ELF_CONF_COALESCE
#define ELF_CONF_COALESCE 1
#endif

// flag to hand the EEPROM image (0x81xxxx) to the sink.
#ifndef ELF_CONF_EEPROM
#define ELF_CONF_EEPROM 0
#endif

// flag to hand the device signature (0x84xxxx) to the sink.
#ifndef ELF_CONF_SIGNATURE
#define ELF_CONF_SIGNATURE 0
#endif

// flag to give the flash window a lower bound as well (module slots).
#ifndef ELF_CONF_WINDOW_LO
#define ELF_CONF_WINDOW_LO 1
#endif

// flag to check the key of a module while it arrives (see elf/load.h).
// Needs the lower bound of the window to find it.
#ifndef ELF_LOAD_CONF_KEY
#define ELF_LOAD_CONF_KEY ELF_CONF_WINDOW_LO
#endif

// flag for firmware-image handling in the load thread: the hook before
// the first page changes, and the reset-vector patch on parts without
// a boot section. Only a bootloader needs it.
#ifndef ELF_LOAD_CONF_IMAGE
#define ELF_LOAD_CONF_IMAGE 0
#endif

#endif /* __ELF_CONF_H__ */

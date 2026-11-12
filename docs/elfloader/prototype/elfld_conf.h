/* elfld_conf.h - compile-time configuration of the streaming ELF32 core.
 * Every knob has a default; override with -D or a project config header. */
#ifndef ELFLD_CONF_H
#define ELFLD_CONF_H

/* Width of file offsets and addresses the core carries around.
 * 16 covers every part with <= 64 KB of flash; 24 covers the rest. */
#ifndef ELFLD_CONF_ADDR_BITS
#  if defined(__AVR__)
#    include <avr/io.h>
#    if FLASHEND > 0xFFFFUL
#      define ELFLD_CONF_ADDR_BITS 24
#    else
#      define ELFLD_CONF_ADDR_BITS 16
#    endif
#  else
#    define ELFLD_CONF_ADDR_BITS 32
#  endif
#endif

/* Loadable segments remembered between the program headers and their data.
 * Adjacent segments are merged, so plain firmware needs 1. */
#ifndef ELFLD_CONF_MAX_SEGMENTS
#define ELFLD_CONF_MAX_SEGMENTS 3
#endif

/* Reject files built for another AVR core family (e_flags vs __AVR_ARCH__). */
#ifndef ELFLD_FEAT_ARCH_CHECK
#define ELFLD_FEAT_ARCH_CHECK 1
#endif
#ifndef ELFLD_CONF_ARCH
#  if defined(__AVR_ARCH__)
#    define ELFLD_CONF_ARCH __AVR_ARCH__
#  else
#    define ELFLD_CONF_ARCH 0          /* host build: set at run time */
#  endif
#endif

/* Merge segments that are contiguous in the file and in memory. */
#ifndef ELFLD_FEAT_COALESCE
#define ELFLD_FEAT_COALESCE 1
#endif

/* Route the EEPROM image (0x81xxxx) to the sink instead of skipping it. */
#ifndef ELFLD_FEAT_EEPROM
#define ELFLD_FEAT_EEPROM 0
#endif

/* Route the .signature bytes (0x84xxxx) to the sink for a device check. */
#ifndef ELFLD_FEAT_SIGNATURE
#define ELFLD_FEAT_SIGNATURE 0
#endif

/* Record the RAM footprint (.data + .bss end) declared by the file. */
#ifndef ELFLD_FEAT_RAM_EXTENT
#define ELFLD_FEAT_RAM_EXTENT 0
#endif

/* Honour a lower bound as well (module slots). A bootloader starts at 0. */
#ifndef ELFLD_FEAT_WINDOW_LO
#define ELFLD_FEAT_WINDOW_LO 0
#endif

#define ELFLD_FEAT_SPACES (ELFLD_FEAT_EEPROM || ELFLD_FEAT_SIGNATURE)

#endif

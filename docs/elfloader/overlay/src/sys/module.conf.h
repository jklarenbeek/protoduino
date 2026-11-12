// file: ./src/sys/module.conf.h

#ifndef __MODULE_CONF_H__
#define __MODULE_CONF_H__

/* ------------------------------------------------------------------ */
/* Configuration (override in protoduino-config.h)                     */
/* ------------------------------------------------------------------ */

// what the kernel can do with modules:
//   0  nothing: no module support is compiled in
//   1  start the modules that are already in flash
//   2  also load modules while running (see process/loader.h)
#ifndef MODULE_CONF_SUPPORT
#define MODULE_CONF_SUPPORT 0
#endif

// number of flash slots, one module each.
#ifndef MODULE_CONF_SLOTS
#define MODULE_CONF_SLOTS 2
#endif

// bytes of flash per slot; a multiple of the page size.
#ifndef MODULE_CONF_SLOT_SIZE
#define MODULE_CONF_SLOT_SIZE 2048
#endif

// address of the first slot. The slots are followed by one page that
// holds the module table. Everything has to lie above the kernel and
// below the image record of the bootloader.
#ifndef MODULE_CONF_BASE
#define MODULE_CONF_BASE 0x6000
#endif

// bytes of RAM a module gets for its process struct.
#ifndef MODULE_CONF_RAM_SIZE
#define MODULE_CONF_RAM_SIZE 48
#endif

#endif /* __MODULE_CONF_H__ */

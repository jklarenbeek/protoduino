// file: ./src/sys/process/loader.conf.h

#ifndef __PROCESS_LOADER_CONF_H__
#define __PROCESS_LOADER_CONF_H__

/* ------------------------------------------------------------------ */
/* Configuration (override in protoduino-config.h)                     */
/* ------------------------------------------------------------------ */

#define LOADER_THREAD_ROM 1  // schedule the load thread inside the protoduino bootloader
#define LOADER_THREAD_OWN 2  // link a copy of the load thread into the kernel

// where the load thread comes from. LOADER_THREAD_ROM costs the kernel
// no code but needs a protoduino bootloader that offers the thread.
// LOADER_THREAD_OWN works with every bootloader that can program a page
// for the running system (see FLASH_CONF_DRIVER), Optiboot included.
#ifndef LOADER_CONF_THREAD
#define LOADER_CONF_THREAD LOADER_THREAD_ROM
#endif

// seconds to wait for a sender before the loader gives up.
#ifndef LOADER_CONF_PATIENCE
#define LOADER_CONF_PATIENCE 30
#endif

// bytes the workspace may need beyond the kernel's own struct elf_load,
// for a bootloader that was built with more features.
#ifndef LOADER_CONF_WORKSPACE_MARGIN
#define LOADER_CONF_WORKSPACE_MARGIN 16
#endif

#endif /* __PROCESS_LOADER_CONF_H__ */

// file: ./src/sys/process/loader.h
//
// Public API for the loader process.
//
// The loader process receives one module over serial_process0, programs it
// into a flash slot, starts it and ends. While it does, every other process
// keeps its turn: the loader only schedules the load thread
// (./src/sys/elf/load.h) with the bytes its pipein delivers, and passes the
// replies of the thread on to its pipeout.
//
// The load thread is the one inside the protoduino bootloader when
// LOADER_CONF_THREAD is LOADER_THREAD_ROM, and a copy linked into the
// kernel when it is LOADER_THREAD_OWN.
//
// USAGE
// -----
// 1. Start the serial device and the module support:
//
//      process_new(&process_type_serial_process0, &serial_process0_proc);
//      module_init();
//
// 2. Choose a slot and who to tell, then start the loader:
//
//      if (loader_is_available())
//        loader_start(0, pt_process);
//
// 3. The module runs:   `notify` gets EVENT_MODULE_INIT, data = its process.
//    The load failed:   the loader ends with the error; the error logger of
//                       process_init() receives it like any process error.
//
//      ERR_INIT_NOSUP       the bootloader cannot program a page for us,
//                           or offers no load thread that fits
//      ERR_STATE_LOCKED     the slot holds a module
//      ERR_CONN_TIMEOUT     nobody sent a file
//      anything else        see ./src/sys/elf/load.h and ./src/sys/module.h

#ifndef __PROCESS_LOADER_H__
#define __PROCESS_LOADER_H__

#include <protoduino.h>
#include <sys/process.h>
#include <sys/ipc.h>
#include <sys/timer.h>
#include <sys/module.h>
#include "loader.conf.h"

#if MODULE_CONF_SUPPORT > 1

struct elf_load;

// ---------------------------------------------------------------------------
// Per-instance struct type.
// process_new() clears the struct, so `slot` and `notify` are set after it:
// loader_start() does both.
// ---------------------------------------------------------------------------
typedef struct {
  struct process  base;      /* MUST be first – §6.7.2.1 */
  uint8_t         slot;      /* Slot to load into */
  struct process *notify;    /* Who gets EVENT_MODULE_INIT – may be NULL */
  struct timer    silence;   /* Time since the sender was last heard */
  ipc_wake_cb_t   wake_cb;   /* Whoever listened to the serial device before */
  void           *wake_ctx;
  uint8_t         state;     /* Last state of the load thread */
  uint8_t         byte;      /* Byte on its way through a pipe */
  uint8_t         ticking;   /* An event of our own is in the queue */
} loader_process_t;

/* Thread function – defined in loader.c */
ptstate_t process_thread_loader_process(struct process *pt_process,
                                         process_event_t ev,
                                         process_data_t  data);

/* PROGMEM type descriptor – defined in loader.c */
extern const process_type_t process_type_loader_process;

/* Singleton instance – defined in loader.c */
extern loader_process_t loader_process_proc;

/**
 * Checks if a module can be loaded at all: somebody can program a page for
 * the running system and a load thread is at hand.
 */
CC_EXTERN bool loader_is_available(void);

/**
 * Start the loader for a slot.
 *
 * \param slot   Slot to load into.
 * \param notify Process that gets EVENT_MODULE_INIT, or NULL.
 */
CC_EXTERN void loader_start(uint8_t slot, struct process *notify);

#endif /* MODULE_CONF_SUPPORT > 1 */

#endif /* __PROCESS_LOADER_H__ */

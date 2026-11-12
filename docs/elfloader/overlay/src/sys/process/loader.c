// file: ./src/sys/process/loader.c
// Loader process implementation.

#include "loader.h"

#if MODULE_CONF_SUPPORT > 1

#include <sys/process/serial.h>
#include <sys/elf/load.h>
#include <sys/flash.h>
#include <sys/bootrom.h>

// ---- PROGMEM name string -------------------------------------------------
static const char loader_process_name[] CC_PROGMEM = "loader";

// ---- Singleton instance --------------------------------------------------
loader_process_t loader_process_proc;

// ---- Workspace of the load thread ----------------------------------------
// Too large for a process struct, and its real size is that of whoever runs
// the thread: with LOADER_THREAD_ROM that is the bootloader.
static union {
  struct elf_load load;
  uint8_t         bytes[sizeof(struct elf_load) + LOADER_CONF_WORKSPACE_MARGIN];
} loader_workspace;

// ---- The load thread to schedule -----------------------------------------
static bool loader_has_thread(void)
{
#if LOADER_CONF_THREAD == LOADER_THREAD_ROM
  return bootrom_is_present()
      && bootrom_version() == BOOTROM_VERSION
      && bootrom_has_load_thread()
      && bootrom_load_size() <= sizeof(loader_workspace);
#else
  return true;
#endif
}

static CC_ALWAYS_INLINE ptstate_t loader_thread(struct elf_load *load)
{
#if LOADER_CONF_THREAD == LOADER_THREAD_ROM
  return bootrom_load_thread(load);
#else
  return elf_load_thread(load);
#endif
}

bool loader_is_available(void)
{
  return flash_is_writable() && loader_has_thread();
}

void loader_start(uint8_t slot, struct process *notify)
{
  if (process_new(&process_type_loader_process, &loader_process_proc) == NULL)
    return;
  // process_new() cleared the struct; the INIT event has not been handled yet
  loader_process_proc.slot   = slot;
  loader_process_proc.notify = notify;
}

/* Raise an error in the loader process. PT_RAISE() takes effect on the next
 * turn of the thread, and a process only gets a turn when somebody asks for
 * one. */
#define LOADER_RAISE(err) \
  do { \
    process_poll(pt_process); \
    PT_RAISE(&pt_process->pt, (err)); \
  } while (0)

// ---- Thread function body ------------------------------------------------
ptstate_t process_thread_loader_process(
    struct process *pt_process,
    process_event_t ev,
    process_data_t  data)
{
  loader_process_t *self = (loader_process_t *)pt_process;
  struct elf_load *load = &loader_workspace.load;
  (void)data;

  /* Above PROCESS_BEGIN: runs on every turn. Our own event has arrived. */
  if (ev == EVENT_INTERVAL)
    self->ticking = 0;

  PROCESS_BEGIN();

  /* Somebody has to program the pages, and the slot has to be free. */
  if (!loader_has_thread() || !flash_is_writable())
    LOADER_RAISE(ERR_INIT_NOSUP);
  if (self->slot >= MODULE_CONF_SLOTS || module_is_installed(self->slot))
    LOADER_RAISE(ERR_STATE_LOCKED);

  /* Listen to the serial device; remember who did, to give it back. */
  self->wake_cb  = serial_process0_proc.rx_pipe.wake_cb;
  self->wake_ctx = serial_process0_proc.rx_pipe.wake_ctx;
  SERIAL_PROCESS0_CONNECT(pt_process);

  /* The file may touch this slot only, and has to name this kernel build. */
  elf_load_init(load,
                MODULE_SLOT_ADDR(self->slot),
                MODULE_SLOT_ADDR(self->slot) + MODULE_CONF_SLOT_SIZE,
                ELF_LOAD_MODULE,
                LOADER_CONF_PATIENCE);
  load->key = module_kernel_id();
  timer_set(&self->silence, clock_from_seconds(1));

  /* ---- schedule the load thread: once per turn, once per byte ---- */
  do {
    if (!elf_load_write_available(load)) {
      /* The thread has not taken the last byte yet. */
    } else if (ipc_pipe_available(PROCESS_PIPEIN()) > 0) {
      /* The next byte from the sender. */
      ipc_pipe_read(PROCESS_PIPEIN(), &self->byte, 1);
      elf_load_write8(load, self->byte);
      timer_restart(&self->silence);
    } else if (timer_expired(&self->silence)) {
      /* Or the news that there is none. */
      elf_load_timeout(load);
      timer_restart(&self->silence);
    }

    self->state = (uint8_t)loader_thread(load);

    /* Its replies go out before it runs again. */
    while (elf_load_read_available(load) > 0) {
      self->byte = elf_load_read8(load);
      PROCESS_PIPE_WRITE(&self->byte, 1);
      process_poll(&serial_process0_proc.base);
    }

    if (PT_ISRUNNING(self->state) && ipc_pipe_available(PROCESS_PIPEIN()) == 0) {
      /* Nothing more for the thread now: everybody else runs. One event of
       * our own brings us back for the next turn; a byte in rx_pipe polls
       * us earlier. Never more than one: the queue belongs to everybody. */
      if (!self->ticking)
        self->ticking = process_post(pt_process, EVENT_INTERVAL, NULL);
      PROCESS_WAIT_EVENT();
    }
  } while (PT_ISRUNNING(self->state));

  if (self->state == PT_EXITED)
    LOADER_RAISE(ERR_CONN_TIMEOUT);
  if (self->state != PT_ENDED)
    LOADER_RAISE(self->state);

  /* The file is in the slot. It has to be a module that can run here. */
  self->state = module_check(self->slot);
  if (self->state != ERR_SUCCESS)
    LOADER_RAISE(self->state);

  /* Enter it into the module table, then start it like any process type.
   * The workspace is free again by now and serves as the page buffer. */
  module_table_write(self->slot, true, loader_workspace.bytes);
  if (self->notify != NULL)
    process_post(self->notify, EVENT_MODULE_INIT, module_start(self->slot));
  else
    module_start(self->slot);

  /* An event of our own brings us back for the PT_FINALLY block. */
  process_post(pt_process, PROCESS_EVENT_EXIT, NULL);

  PT_CATCHANY(&pt_process->pt)
  {
    /* End with the error: the scheduler passes it to the error logger. */
    self->state = (uint8_t)PT_ERROR_STATE;
    /* An event of our own brings us back for the PT_FINALLY block. */
    process_post(pt_process, PROCESS_EVENT_EXIT, NULL);
    PT_THROW(&pt_process->pt, self->state);
  }

  PT_FINALLY(&pt_process->pt)
  {
    /* Give the serial device back to whoever listened before. */
    if (serial_process0_proc.rx_pipe.wake_ctx == (void *)pt_process) {
      serial_process0_proc.rx_pipe.wake_cb  = self->wake_cb;
      serial_process0_proc.rx_pipe.wake_ctx = self->wake_ctx;
    }
  }

  PROCESS_END();
}

// ---- PROGMEM type descriptor --------------------------------------------
const process_type_t process_type_loader_process CC_PROGMEM = {
  loader_process_name,           /* name string (PROGMEM) */
  process_thread_loader_process, /* thread entry point    */
  sizeof(loader_process_t),      /* instance size          */
  2,                             /* priority               */
  PROCESS_TYPE_PROCESS           /* started for each load  */
};

#endif /* MODULE_CONF_SUPPORT > 1 */

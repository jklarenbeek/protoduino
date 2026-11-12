// file: ./src/mod/loader.h

#ifndef __MOD_LOADER_H__
#define __MOD_LOADER_H__

/* loader: receives one module and puts it into a flash slot.
 *
 * The loader reads an ELF file from its input, sent with XMODEM-CRC,
 * programs it into the slot it was started for, starts the module and ends.
 * While it does, every other process keeps its turn: the loader only
 * schedules the load thread (./src/sys/elf/load.h) with the bytes its input
 * delivers, and passes the replies of the thread on to its output.
 *
 *   loader_start(0, &serial_process0_proc.base);
 *
 * - loader_start() connects the loader to a device in both directions, and
 *   the loader gives the input of the device back to whoever read it before
 *   when it ends. A shell can therefore start it on its own line.
 * - The module runs:  the loader is gone and module_is_installed(slot) is
 *                     true. `notify`, when set, gets EVENT_MODULE_INIT with
 *                     the process of the module as data.
 *   The load failed:  the loader ends with the error; the error logger of
 *                     process_init() receives it like any process error.
 *
 *     ERR_INIT_NOSUP       nobody can program a page for the running
 *                          system, or the bootloader offers no load thread
 *     ERR_STATE_LOCKED     the slot holds a module, or its process runs
 *     ERR_PIPE_BROKEN      the loader was started without a line
 *     ERR_CONN_TIMEOUT     nobody sent a file
 *     anything else        see ./src/sys/elf/load.h and ./src/sys/module.h
 *
 * - The load thread is the one inside the protoduino bootloader when
 *   LOADER_CONF_THREAD is LOADER_THREAD_ROM, and the copy of the library
 *   when it is LOADER_THREAD_OWN. The second works with every bootloader
 *   that can program a page for the running system, Optiboot 8 included
 *   (see ./src/cpu/avr/flash.h).
 * - Needs the tick (sys/tick.h): its replies wait for room in the pipe.
 * - Part of the kernel, not a loadable module: it is the one process type
 *   of this directory with a variable of its own, the workspace of the load
 *   thread, and it brings its instance along. There is one loader.
 */

#include <sys/process.h>
#include <sys/timer.h>

#ifndef MODULE_CONF_SUPPORT
#define MODULE_CONF_SUPPORT 2
#endif
#include <sys/module.h>

#if MODULE_CONF_SUPPORT < 2
#error "loader: needs MODULE_CONF_SUPPORT 2"
#endif

#include <sys/elf/load.h>
#include <sys/flash.h>
#include <sys/bootrom.h>

/* ------------------------------------------------------------------ */
/* Configuration (define in the sketch, before this header)            */
/* ------------------------------------------------------------------ */

#define LOADER_THREAD_ROM 1  // schedule the load thread inside the protoduino bootloader
#define LOADER_THREAD_OWN 2  // use the load thread of the library, linked into the kernel

// where the load thread comes from. LOADER_THREAD_ROM costs the kernel
// no code but needs a protoduino bootloader that offers the thread.
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

/* ------------------------------------------------------------------ */
/* The process                                                         */
/* ------------------------------------------------------------------ */

PROCESS_DEFINE(loader, "loader", 2,
  uint8_t         slot;      /* slot to load into                           */
  struct process *notify;    /* who gets EVENT_MODULE_INIT - may be NULL    */
  struct process *line;      /* the device the file arrives on              */
  struct process *previous;  /* who read from that device before            */
  struct timer    silence;   /* time since the sender was last heard        */
  uint8_t         state;     /* last state of the load thread               */
  uint8_t         byte;      /* byte on its way through a pipe              */
  uint8_t         ticking;   /* an event of our own is in the queue         */
);

PROCESS_INSTANCE(loader, loader_proc);

// Workspace of the load thread. Too large for a process struct, and its
// real size is that of whoever runs the thread: with LOADER_THREAD_ROM
// that is the bootloader. A page buffer for the module table when no load
// is under way.
union loader_workspace {
  struct elf_load load;
  uint8_t         bytes[sizeof(struct elf_load) + LOADER_CONF_WORKSPACE_MARGIN];
} loader_workspace;

static inline bool loader_has_thread(void)
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

/**
 * Checks if a module can be loaded at all: somebody can program a page for
 * the running system and a load thread is at hand.
 */
static inline bool loader_is_available(void)
{
  return flash_is_writable() && loader_has_thread();
}

/**
 * Checks if a load is under way.
 */
static CC_ALWAYS_INLINE bool loader_is_running(void)
{
  return loader_proc.base.state != PROCESS_STATE_NONE;
}

/**
 * Start the loader for a slot, on a line.
 *
 * \param slot Slot to load into.
 * \param line The device the file arrives on; the replies go to it.
 *
 * \return the loader process, or NULL when a load is under way already.
 *         Set its `notify` afterwards to be told when the module runs.
 */
static inline struct process *loader_start(uint8_t slot, struct process *line)
{
  struct process *previous = line->has_pipein ? process_reader(line->pipein) : NULL;

  if (loader_is_running() || process_new(&process_type_loader, &loader_proc) == NULL)
    return NULL;
  // process_new() cleared the struct; the INIT event has not been handled yet
  loader_proc.slot     = slot;
  loader_proc.line     = line;
  loader_proc.previous = previous;
  process_connect(line, &loader_proc.base);
  process_connect(&loader_proc.base, line);
  return &loader_proc.base;
}

/* Raise an error in the loader process. PT_RAISE() takes effect on the next
 * turn of the thread, and a process only gets a turn when somebody asks for
 * one. */
#define LOADER_RAISE(err) \
  do { \
    process_poll(pt_process); \
    PT_RAISE(&pt_process->pt, (err)); \
  } while (0)

PROCESS_THREAD(loader, ev, data)
{
  process_loader_t *self = PROCESS_SELF(loader);
  struct elf_load *load = &loader_workspace.load;

  /* Above PROCESS_BEGIN: runs on every turn. Our own event has arrived.
   * It carries us as its data: the tick is the same event for everybody,
   * and taking a tick for our own would put a second one into the queue,
   * then a third, until nobody else can post. */
  if (ev == EVENT_INTERVAL && data == (process_data_t)pt_process)
    self->ticking = 0;

  PROCESS_BEGIN();

  /* Somebody has to program the pages, the slot has to be free and the
   * file has to come from somewhere. */
  if (!loader_has_thread() || !flash_is_writable())
    LOADER_RAISE(ERR_INIT_NOSUP);
  if (self->slot >= MODULE_CONF_SLOTS
      || module_is_installed(self->slot)
      || module_process(self->slot)->state != PROCESS_STATE_NONE)
    LOADER_RAISE(ERR_STATE_LOCKED);
  if (!pt_process->has_pipein || !pt_process->has_pipeout)
    LOADER_RAISE(ERR_PIPE_BROKEN);

  /* The file may touch this slot only, and has to name this kernel build. */
  elf_load_init(load,
                MODULE_SLOT_ADDR(self->slot),
                MODULE_SLOT_ADDR(self->slot) + MODULE_CONF_SLOT_SIZE,
                ELF_LOAD_MODULE,
                LOADER_CONF_PATIENCE);
  load->key = module_kernel_id();
  timer_set(&self->silence, clock_from_seconds(1));

  /* ---- schedule the load thread: once per turn, once per byte ---- */
  do
  {
    if (!elf_load_write_available(load))
    {
      /* The thread has not taken the last byte yet. */
    }
    else if (ipc_pipe_available(PROCESS_PIPEIN()) > 0)
    {
      /* The next byte from the sender. */
      ipc_pipe_read(PROCESS_PIPEIN(), &self->byte, 1);
      elf_load_write8(load, self->byte);
      timer_restart(&self->silence);
    }
    else if (timer_expired(&self->silence))
    {
      /* Or the news that there is none. */
      elf_load_timeout(load);
      timer_restart(&self->silence);
    }

    self->state = (uint8_t)loader_thread(load);

    /* Its replies go out before it runs again. */
    while (elf_load_read_available(load) > 0)
    {
      self->byte = elf_load_read8(load);
      PROCESS_PIPE_WRITE(&self->byte, 1);
    }

    if (PT_ISRUNNING(self->state) && ipc_pipe_available(PROCESS_PIPEIN()) == 0)
    {
      /* Nothing more for the thread now: everybody else runs. One event of
       * our own brings us back for the next turn; a byte on the line polls
       * us earlier. Never more than one: the queue belongs to everybody. */
      if (!self->ticking)
        self->ticking = process_post(pt_process, EVENT_INTERVAL, (process_data_t)pt_process);
      PROCESS_WAIT_EVENT();
    }
  }
  while (PT_ISRUNNING(self->state));

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
    /* Give the line back to whoever read from it before. */
    if (self->line != NULL && self->previous != NULL)
      process_connect(self->line, self->previous);
  }

  PROCESS_END();
}

#endif /* __MOD_LOADER_H__ */

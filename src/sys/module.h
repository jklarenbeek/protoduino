// file: ./src/sys/module.h

#ifndef __MODULE_H__
#define __MODULE_H__

/* Loadable modules.
 *
 * A module is a process type that was not linked into the kernel. It is an
 * ELF file, linked to run in one flash slot against the symbols of one
 * kernel build. Nothing has to be relocated when it arrives: the load
 * thread (./src/sys/elf/load.h) programs it and the kernel starts it with
 * process_new(), like a process type of its own.
 *
 * - A module has no static variables. Its state lives in its process
 *   struct, the fields of PROCESS_DEFINE, in RAM that the kernel hands out.
 * - A module starts with its descriptor; the entry point of the ELF file is
 *   the address of that descriptor.
 * - A module only runs on the kernel build it was linked against.
 *   module_kernel_id() names that build.
 * - A slot holds a module once the module table says so. The table is
 *   written after the module has been checked, so a load that was cut
 *   short leaves nothing that could be started.
 *
 * In the module:
 *
 *   PROCESS_DEFINE(blink, "blink", 3,
 *     uint8_t count;
 *   );
 *
 *   PROCESS_THREAD(blink, ev, data)
 *   {
 *     ...
 *   }
 *
 *   MODULE(blink);
 *
 * In the sketch that is the kernel, before the header is included:
 *
 *   #define MODULE_CONF_SUPPORT 1     // 2: also load at run time
 *   #include <sys/module.h>
 *
 *   module_init();
 *   module_autostart();   // start what the module table lists
 *
 * - The slot functions are part of the sketch: they are compiled with the
 *   settings of the sketch (./src/sys/module.conf.h), and one source file
 *   of a program includes this header with MODULE_CONF_SUPPORT set.
 */

#include <protoduino.h>
#include "process.h"
#include "module.conf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MODULE_MAGIC 0x4D70 /* 'p' 'M' */
#define MODULE_ABI   2

/* First bytes of every module. kernel_id sits at ELF_LOAD_KEY_OFFSET, where
 * the load thread looks for it while the module arrives. */
typedef struct module {
  uint16_t              magic;     /**< MODULE_MAGIC                           */
  uint8_t               abi;       /**< MODULE_ABI                             */
  uint8_t               flags;     /**< reserved, 0                            */
  uint16_t              kernel_id; /**< kernel build the module is linked for  */
  const process_type_t *type;      /**< the process type the module adds       */
} module_t;

/* Supplied when a module is linked: the address of this symbol is the
 * identity of the kernel build (see ld/module.ld). */
extern const uint8_t __module_kernel_id;

/**
 * MODULE(name)
 *
 * Emits the descriptor of a module that adds the process type `name`,
 * declared with PROCESS_DEFINE or DEVICE_DEFINE in the same file.
 */
#define MODULE(name)                                                           \
  const module_t module_descriptor                                             \
      __attribute__((section(".module"), used)) = {                            \
    MODULE_MAGIC,                                                              \
    MODULE_ABI,                                                                \
    0,                                                                         \
    (uint16_t)&__module_kernel_id,                                             \
    &process_type_##name                                                       \
  }

#if MODULE_CONF_SUPPORT > 0

#include <avr/pgmspace.h>
#include <util/crc16.h>

#if MODULE_CONF_SUPPORT > 1
#include "flash.h"
#endif

// first address of a slot, and of the module table behind the last slot
#define MODULE_SLOT_ADDR(slot) \
  ((uint16_t)(MODULE_CONF_BASE) + (uint16_t)(slot) * (uint16_t)(MODULE_CONF_SLOT_SIZE))
#define MODULE_TABLE_ADDR MODULE_SLOT_ADDR(MODULE_CONF_SLOTS)

/* The module table: one page behind the last slot.
 *
 *   [0..1]        'p' 'M'
 *   [2 + 2*slot]  kernel_id of the module in that slot; 0xFFFF: none
 */
#define MODULE_TABLE_ENTRY(slot) (MODULE_TABLE_ADDR + 2 + 2 * (uint16_t)(slot))

/* What a kernel offers to its modules.
 *
 * A module is linked against the functions of one kernel build, and a
 * build only contains the functions somebody in it refers to: what the
 * kernel does not call itself is left out, and what it calls in one way
 * only may be reshaped for that way. This table refers to the functions a
 * module is likely to need, so that every kernel has them as they are
 * declared. Add your own with MODULE_CONF_API, each followed by a comma:
 *
 *   #define MODULE_CONF_API (module_api_fn)analogRead, (module_api_fn)digitalWrite,
 */
typedef void (*module_api_fn)(void);

#ifndef MODULE_CONF_API
#define MODULE_CONF_API
#endif

extern const module_api_fn module_api[]; // a name of its own, also in a sketch
const module_api_fn module_api[] __attribute__((used)) CC_PROGMEM = {
  MODULE_CONF_API
  (module_api_fn)process_post,
  (module_api_fn)process_poll,
  (module_api_fn)process_new,
  (module_api_fn)process_destroy,
  (module_api_fn)process_lookup_n,
#if PROCESS_CONF_PIPELINES > 0
  (module_api_fn)process_connect,
  (module_api_fn)process_disconnect,
  (module_api_fn)process_ipc_wake,
  (module_api_fn)ipc_pipe_init,
  (module_api_fn)ipc_pipe_read,
  (module_api_fn)ipc_pipe_write,
  (module_api_fn)ipc_pipe_available,
  (module_api_fn)ipc_pipe_space,
#endif
  (module_api_fn)micros
};

/* Where the slots are, for whoever builds a module for this kernel:
 * tools/module.mjs reads it from the .elf file of the kernel. */
extern const uint32_t module_layout[5];
const uint32_t module_layout[5] __attribute__((used)) CC_PROGMEM = {
  MODULE_CONF_BASE, MODULE_CONF_SLOT_SIZE, MODULE_CONF_SLOTS, MODULE_CONF_RAM_SIZE, F_CPU
};

// end of the kernel image in flash, from the linker
extern const char __data_load_end[];

uint16_t module_id;

// RAM for the process struct of each module
uint8_t module_ram[MODULE_CONF_SLOTS][MODULE_CONF_RAM_SIZE];

/**
 * Work out the identity of this kernel build: a CRC-16 of its own image in
 * flash. Call once, before any other module function.
 */
static inline void module_init(void)
{
  uint16_t crc = 0;

  // Nobody in the kernel reads the two tables above, and the linker leaves
  // out what nobody refers to. Naming them here keeps them in the build.
  __asm__ __volatile__ ("" : : "r" (&module_api[0]), "r" (&module_layout[0]));

  for (uint16_t a = 0; a != (uint16_t)__data_load_end; a++)
    crc = _crc_xmodem_update(crc, pgm_read_byte(a));
  module_id = crc;
}

/** Identity of this kernel build. */
static CC_ALWAYS_INLINE uint16_t module_kernel_id(void)
{
  return module_id;
}

/**
 * Checks whether the module table lists a module for this kernel in a slot.
 *
 * \return true if the slot holds a module that can be started.
 */
static inline bool module_is_installed(uint8_t slot)
{
  return slot < MODULE_CONF_SLOTS
      && pgm_read_word(MODULE_TABLE_ADDR) == MODULE_MAGIC
      && pgm_read_word(MODULE_TABLE_ENTRY(slot)) == module_id;
}

/**
 * Checks the descriptor at the start of a slot.
 *
 * \return ERR_SUCCESS, or why what is in the slot cannot run here:
 *   ERR_BIN_MAGIC       no module descriptor
 *   ERR_BIN_VERSION     descriptor of another module ABI
 *   ERR_INIT_DEPENDENCY linked against another kernel build
 *   ERR_HEAP_OOM        its process struct does not fit MODULE_CONF_RAM_SIZE
 */
static inline uint8_t module_check(uint8_t slot)
{
  const module_t *m = (const module_t *)MODULE_SLOT_ADDR(slot);
  const process_type_t *type;

  if (slot >= MODULE_CONF_SLOTS || pgm_read_word(&m->magic) != MODULE_MAGIC)
    return ERR_BIN_MAGIC;
  if (pgm_read_byte(&m->abi) != MODULE_ABI)
    return ERR_BIN_VERSION;
  if (pgm_read_word(&m->kernel_id) != module_id)
    return ERR_INIT_DEPENDENCY;

  type = (const process_type_t *)pgm_read_ptr(&m->type);
  if (pgm_read_byte(&type->size) > MODULE_CONF_RAM_SIZE)
    return ERR_HEAP_OOM;

  return ERR_SUCCESS;
}

/**
 * The process type of the module in a slot. Only for a slot that
 * module_is_installed() or module_check() accepts.
 */
static CC_ALWAYS_INLINE const process_type_t *module_type(uint8_t slot)
{
  return (const process_type_t *)pgm_read_ptr(&((const module_t *)MODULE_SLOT_ADDR(slot))->type);
}

/**
 * The process a slot runs in. Its state is PROCESS_STATE_NONE while the
 * module is not running.
 */
static CC_ALWAYS_INLINE struct process *module_process(uint8_t slot)
{
  return (struct process *)module_ram[slot];
}

/**
 * Start the process of the module in a slot.
 *
 * \return the process, or NULL when module_check() fails, the module runs
 *         already or the scheduler refuses the process.
 */
static inline struct process *module_start(uint8_t slot)
{
  if (module_check(slot) != ERR_SUCCESS || module_process(slot)->state != PROCESS_STATE_NONE)
    return NULL;
  // a module is a process type like any other from here on
  return process_new(module_type(slot), module_ram[slot]);
}

/**
 * Start the process of every module the module table lists.
 *
 * \return number of processes started.
 */
static inline uint8_t module_autostart(void)
{
  uint8_t started = 0;

  for (uint8_t slot = 0; slot < MODULE_CONF_SLOTS; slot++)
  {
    if (module_is_installed(slot) && module_start(slot) != NULL)
      started++;
  }
  return started;
}

#if MODULE_CONF_SUPPORT > 1
/**
 * Enter a slot into the module table, or take it out.
 *
 * \param slot      The slot.
 * \param installed true: the slot holds a checked module for this kernel.
 * \param page      SPM_PAGESIZE bytes of scratch memory.
 */
static inline void module_table_write(uint8_t slot, bool installed, uint8_t *page)
{
  uint16_t entry = installed ? module_id : 0xFFFF;
  uint16_t i;

  // the table as it is, or an empty one
  for (i = 0; i < SPM_PAGESIZE; i++)
    page[i] = pgm_read_word(MODULE_TABLE_ADDR) == MODULE_MAGIC ? pgm_read_byte(MODULE_TABLE_ADDR + i) : 0xFF;

  page[0] = (uint8_t)MODULE_MAGIC;
  page[1] = (uint8_t)(MODULE_MAGIC >> 8);
  page[2 + 2 * slot] = (uint8_t)entry;
  page[3 + 2 * slot] = (uint8_t)(entry >> 8);

  flash_page_write(MODULE_TABLE_ADDR, page);
}
#endif

#endif /* MODULE_CONF_SUPPORT > 0 */

#ifdef __cplusplus
}
#endif

#endif /* __MODULE_H__ */

// file: ./src/sys/module.c

#include "module.h"

#if MODULE_CONF_SUPPORT > 0

#include <avr/pgmspace.h>
#include <util/crc16.h>

#if MODULE_CONF_SUPPORT > 1
#include "flash.h"
#endif

/* The module table: one page behind the last slot.
 *
 *   [0..1]        'p' 'M'
 *   [2 + 2*slot]  kernel_id of the module in that slot; 0xFFFF: none
 */
#define MODULE_TABLE_ENTRY(slot) (MODULE_TABLE_ADDR + 2 + 2 * (uint16_t)(slot))

// end of the kernel image in flash, from the linker
extern const char __data_load_end[];

static uint16_t module_id;

// RAM for the process struct of each module
static uint8_t module_ram[MODULE_CONF_SLOTS][MODULE_CONF_RAM_SIZE];

void module_init(void)
{
  uint16_t crc = 0;

  for (uint16_t a = 0; a != (uint16_t)__data_load_end; a++)
    crc = _crc_xmodem_update(crc, pgm_read_byte(a));
  module_id = crc;
}

uint16_t module_kernel_id(void)
{
  return module_id;
}

bool module_is_installed(uint8_t slot)
{
  return slot < MODULE_CONF_SLOTS
      && pgm_read_word(MODULE_TABLE_ADDR) == MODULE_MAGIC
      && pgm_read_word(MODULE_TABLE_ENTRY(slot)) == module_id;
}

uint8_t module_check(uint8_t slot)
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

struct process *module_start(uint8_t slot)
{
  const module_t *m = (const module_t *)MODULE_SLOT_ADDR(slot);

  if (module_check(slot) != ERR_SUCCESS)
    return NULL;
  // a module is a process type like any other from here on
  return process_new((const process_type_t *)pgm_read_ptr(&m->type), module_ram[slot]);
}

uint8_t module_autostart(void)
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

void module_table_write(uint8_t slot, bool installed, uint8_t *page)
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

#endif /* MODULE_CONF_SUPPORT > 1 */

#endif /* MODULE_CONF_SUPPORT > 0 */

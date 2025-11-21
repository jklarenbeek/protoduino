// file: ./src/sys/dsc.h

/*
 * This file is part of the Protoduino operating framework.
 */

/**
 * \file
 * Declaration of the DSC program description structure (improved for shell integration).
 */

#ifndef DSC_H_
#define DSC_H_

#include "process.h"  /* For process integration */

/**
 * \addtogroup loader
 * @{
 */

/**
 * The DSC structure describes a loadable module or program.
 * It includes name, description, and hooks for shell listing/loading.
 */
struct dsc {
  const char *name;         /* Module name */
  const char *description;  /* One-line description */
  void (*init)(void);               /* Optional init hook */
  struct process *process;          /* Associated process (if static) */
  const uint8_t *elf_data;          /* ELF binary (if embedded) */
  uint16_t elf_size;                /* ELF size */
};

// Helper macros to place strings in PROGMEM
#define DSC_PSTR(x) ((const char PROGMEM *)(x))

/**
 * Macro to define a DSC.
 *
 * \param dscname Variable name.
 * \param strname Name string.
 * \param desc Description.
 * \param proc Associated process.
 */
#define DSC(dscname, strname, desc, proc) \
  static const char dscname##_name[] PROGMEM = strname; \
  static const char dscname##_desc[] PROGMEM = desc; \
  const struct dsc dscname PROGMEM = { \
    dscname##_name, \
    dscname##_desc, \
    NULL, \
    &proc, \
    NULL, \
    0 \
  }

#define DSC_HEADER(name) extern struct dsc name

/* External list of all DSCs for shell enumeration */
CC_EXTERN const struct dsc * const dsc_list[] PROGMEM;

#define DSC_LIST(...) extern const struct dsc * const dsc_list[] PROGMEM = {__VA_ARGS__, NULL}

/** @} */

#endif /* DSC_H_ */
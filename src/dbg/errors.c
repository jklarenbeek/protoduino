// file: ./src/dbg/errors.c

/*
 * Flash-resident string table for the 8-bit error ontology.
 *
 * The switch body is AUTO-GENERATED from src/sys/errors.h by
 * tools/ontology-sync.js — never edit errors-strings.inc by hand;
 * fix the header comment and regenerate instead.
 *
 * Storage contract: every literal is wrapped in PSTR(), so the strings
 * live in flash (PROGMEM on AVR) and consume no SRAM. The returned
 * pointer is a flash address — print it with print_P() (or copy it out
 * with strcpy_P), never with plain RAM string functions on AVR.
 *
 * Configuration (see src/sys/errors.conf.h):
 *   ERRORS_CONF_STRINGS 0   no table — stream the raw 8-bit codes and
 *                           decode host-side (ATtiny-class targets).
 *   ERRORS_CONF_STRINGS 1   tiny names, e.g. "HEAP_OOM" (default).
 *   ERRORS_CONF_STRINGS 2   verbose, e.g. "Out of memory".
 */
#include <protoduino.h>
#include "errors.h"

#ifdef __cplusplus
extern "C" {
#endif

#if ERRORS_CONF_STRINGS == 0

const char *error_to_string(uint8_t err) {
  (void)err;
  return NULL; /* callers stream the raw code instead */
}

#else

#if ERRORS_CONF_STRINGS >= 2
#define S(tiny, verbose) PSTR(verbose)
#else
#define S(tiny, verbose) PSTR(tiny)
#endif

const char *error_to_string(uint8_t err) {
  switch (err) {
#include "errors-strings.inc"
  }
  return PSTR("?"); /* unreachable: the table covers all 256 codes */
}

#undef S

#endif /* ERRORS_CONF_STRINGS */

#ifdef __cplusplus
}
#endif

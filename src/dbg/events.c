// file: ./src/dbg/events.c

/*
 * Flash-resident string table for the 8-bit event ontology.
 *
 * The switch body is AUTO-GENERATED from src/sys/events.h by
 * tools/ontology-sync.js — never edit events-strings.inc by hand;
 * fix the header comment and regenerate instead.
 *
 * Storage contract and configuration are identical to errors.c:
 * strings live in flash (PROGMEM on AVR), never SRAM; print the
 * returned pointer with print_P(). ERRORS_CONF_STRINGS selects
 * none (0) / tiny (1) / verbose (2) for both tables.
 */
#include <protoduino.h>
#include "events.h"

#ifdef __cplusplus
extern "C" {
#endif

#if ERRORS_CONF_STRINGS == 0

const char *event_to_string(uint8_t ev) {
  (void)ev;
  return NULL; /* callers stream the raw code instead */
}

#else

#if ERRORS_CONF_STRINGS >= 2
#define S(tiny, verbose) PSTR(verbose)
#else
#define S(tiny, verbose) PSTR(tiny)
#endif

const char *event_to_string(uint8_t ev) {
  switch (ev) {
#include "events-strings.inc"
  }
  return PSTR("?"); /* unreachable: the table covers all 256 codes */
}

#undef S

#endif /* ERRORS_CONF_STRINGS */

#ifdef __cplusplus
}
#endif

// file: ./src/sys/errors.conf.h

#ifndef __ERRORS_CONF_H__
#define __ERRORS_CONF_H__

// --------------------------------------------------------------------------
// Configuration: error/event string tables (override in protoduino-config.h)
// --------------------------------------------------------------------------
//
// All strings are flash-resident (PROGMEM on AVR) — they never occupy
// SRAM. The only cost of enabling them is flash space.
//
// On very small targets (ATtiny) set ERRORS_CONF_STRINGS to 0 and stream
// the raw 8-bit codes instead: the receiving side decodes them with
// docs/ontology-proof.csv and can plot events and errors as a 16x16
// heatmap (an event and its failure mode are point-symmetric through the
// center of the map, since error = ~event). See docs/ontology.md.
//
// 0: no string tables — error_to_string()/event_to_string() return NULL.
// 1: tiny names, e.g. "HEAP_OOM" (default).
// 2: verbose descriptions, e.g. "Out of memory" (32KB+ flash).
#ifndef ERRORS_CONF_STRINGS
// Back-compat: ERRORS_CONF_VERBOSE_MESSAGES=1 used to select verbose.
#if defined(ERRORS_CONF_VERBOSE_MESSAGES) && ERRORS_CONF_VERBOSE_MESSAGES
#define ERRORS_CONF_STRINGS 2
#else
#define ERRORS_CONF_STRINGS 1
#endif
#endif

#ifndef ERRORS_CONF_VERBOSE_MESSAGES
#define ERRORS_CONF_VERBOSE_MESSAGES (ERRORS_CONF_STRINGS >= 2)
#endif

#endif

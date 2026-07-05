// file: ./src/dbg/events.h
#ifndef __DBG_EVENTS_H__
#define __DBG_EVENTS_H__

/*
 * ONTOLOGICAL 8-BIT EVENT TAXONOMY — analysis helpers
 * -----------------------------------------------------------------------------
 * Event-side mirror of ./src/dbg/errors.h.
 *
 * Events and errors are dual views of the same 8-bit space
 * (event = ~error & 0xFF), so every ontology operation is IDENTICAL for
 * both. The EV_* names below are thin aliases of the ERR_* macros and
 * err_op_* functions — one implementation, two vocabularies, zero drift.
 * See docs/ontology.md § Event-Error Duality.
 * -----------------------------------------------------------------------------
 */
#include "errors.h"

/* Reserved kernel codes are shared between both spaces. */
#define EV_IS_RESERVED(ev)   ERR_IS_RESERVED(ev)

/* Nibble helpers */
#define EV_NIBBLE_LEFT(ev)   ERR_NIBBLE_LEFT(ev)
#define EV_NIBBLE_RIGHT(ev)  ERR_NIBBLE_RIGHT(ev)

/* Structural predicates */
#define EV_IS_TWIN(ev)       ERR_IS_TWIN(ev)
#define EV_IS_SHADOW(ev)     ERR_IS_SHADOW(ev)
#define EV_IS_MIRROR(ev)     ERR_IS_MIRROR(ev)

/* Hierarchy predicates */
#define EV_IS_ABSTRACT(ev)   ERR_IS_ABSTRACT(ev)
#define EV_IS_MOVEMENT(ev)   ERR_IS_MOVEMENT(ev)
#define EV_IS_ROOT(ev)       ERR_IS_ROOT(ev)
#define EV_IS_DOMAIN(ev)     ERR_IS_DOMAIN(ev)
#define EV_IS_SECTION(ev)    ERR_IS_SECTION(ev)
#define EV_IS_LEAF(ev)       ERR_IS_LEAF(ev)

/* Symmetry classes */
#define EV_IS_BALANCED(ev)   ERR_IS_BALANCED(ev)
#define EV_IS_UNBALANCED(ev) ERR_IS_UNBALANCED(ev)
#define EV_IS_PARITY(ev)     ERR_IS_PARITY(ev)
#define EV_IS_ANTICODE(ev)   ERR_IS_ANTICODE(ev)
#define EV_IS_MARGIN(ev)     ERR_IS_MARGIN(ev)
#define EV_IS_ABSOLUTE(ev)   ERR_IS_ABSOLUTE(ev)
#define EV_IS_REPEATER(ev)   ERR_IS_REPEATER(ev)
#define EV_IS_IMPULSE(ev)    ERR_IS_IMPULSE(ev)
#define EV_IS_ASYMMETRY(ev)  ERR_IS_ASYMMETRY(ev)

/* Operations */
#define ev_op_inverse(ev)    err_op_inverse(ev)
#define ev_op_reverse(ev)    err_op_reverse(ev)
#define ev_op_opposite(ev)   err_op_opposite(ev)
#define ev_op_center(ev)     err_op_center(ev)
#define ev_op_root(ev)       err_op_root(ev)
#define ev_op_depth(ev)      err_op_depth(ev)
#define ev_op_one_count(ev)  err_op_one_count(ev)
#define ev_op_distance(l, r) err_op_distance((l), (r))
#define ev_op_entropy(ev)    err_op_entropy(ev)
#define ev_op_balance(ev)    err_op_balance(ev)
#define ev_op_relation(l, r) err_op_relation((l), (r))

/*
 * Flash-resident name for an event code, or NULL when the string
 * tables are disabled (ERRORS_CONF_STRINGS == 0). The returned pointer
 * is a PROGMEM address on AVR — print with print_P().
 */
CC_EXTERN const char *event_to_string(uint8_t ev);

#endif // __DBG_EVENTS_H__

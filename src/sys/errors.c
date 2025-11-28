// file:./src/sys/errors.h

#include "protoduino.h"
#include "errors.h"

/* -------------------------------------------------------------------------
 * String table stored in CC_PROGMEM to avoid wasting RAM.
 * ------------------------------------------------------------------------- */

static const char unknown_error[] CC_PROGMEM = "Unknown Error";

/* -------------------------------------------------------------------------
 * Main lookup function
 * ------------------------------------------------------------------------- */
const char *error_to_string(uint8_t err)
{
  switch(err) {
    // needs implementation
    default:
      return unknown_error;
  }
}

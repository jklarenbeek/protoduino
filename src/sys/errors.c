// file:./src/sys/errors.h

#include "protoduino.h"
#include "errors.h"

/* -------------------------------------------------------------------------
 * String table stored in PROGMEM to avoid wasting RAM.
 * ------------------------------------------------------------------------- */

// TODO: PROGMEM macro
// this is not usable for a cross device setup;
// PROGMEM is not always relevant for the platform.
// therefore a macro must be made that decides wether or
// not PROGMEM is included.
static const char unknown_error[] PROGMEM = "Unknown Error";

/* PT errors (4–31) */
static const char str_pt_general[]           PROGMEM = "PT: General Error";
static const char str_pt_timeout[]           PROGMEM = "PT: Timeout";
static const char str_pt_invalid_state[]     PROGMEM = "PT: Invalid State";
static const char str_pt_illegal_op[]        PROGMEM = "PT: Illegal Operation";
static const char str_pt_stack_corrupt[]     PROGMEM = "PT: Stack Corruption";
static const char str_pt_child_failed[]      PROGMEM = "PT: Child Thread Failed";
static const char str_pt_uncaught[]          PROGMEM = "PT: Uncaught Error";
static const char str_pt_assert_failed[]     PROGMEM = "PT: Assertion Failed";
static const char str_pt_restart_loop[]      PROGMEM = "PT: Infinite Restart Loop";

/* System errors (32–63) */
static const char str_sys_general[]          PROGMEM = "System: General Error";
static const char str_sys_init_failed[]      PROGMEM = "System: Init Failed";
static const char str_sys_no_memory[]        PROGMEM = "System: No Memory";
static const char str_sys_invalid_handle[]   PROGMEM = "System: Invalid Handle";
static const char str_sys_bad_argument[]     PROGMEM = "System: Bad Argument";
static const char str_sys_unsupported[]      PROGMEM = "System: Not Supported";
static const char str_sys_overflow[]         PROGMEM = "System: Overflow";
static const char str_sys_timeout[]          PROGMEM = "System: Timeout";
static const char str_sys_denied[]           PROGMEM = "System: Access Denied";
static const char str_sys_not_ready[]        PROGMEM = "System: Not Ready";

/* I/O errors (64–95) */
static const char str_io_general[]           PROGMEM = "I/O: General Error";
static const char str_io_frame_error[]       PROGMEM = "I/O: Frame Error";
static const char str_io_parity_error[]      PROGMEM = "I/O: Parity Error";
static const char str_io_bad_length[]        PROGMEM = "I/O: Bad Length";
static const char str_io_corrupt_data[]      PROGMEM = "I/O: Corrupt Data";
static const char str_io_timeout[]           PROGMEM = "I/O: Timeout";
static const char str_io_not_ready[]         PROGMEM = "I/O: Not Ready";
static const char str_io_disconnected[]      PROGMEM = "I/O: Disconnected";

/* Data errors (96–127) */
static const char str_data_general[]         PROGMEM = "Data: General Error";
static const char str_data_overflow[]        PROGMEM = "Data: Overflow";
static const char str_data_underflow[]       PROGMEM = "Data: Underflow";
static const char str_data_invalid_format[]  PROGMEM = "Data: Invalid Format";
static const char str_data_checksum[]        PROGMEM = "Data: Bad Checksum";
static const char str_data_range[]           PROGMEM = "Data: Out of Range";

/* Resource errors (128–159) */
static const char str_res_general[]          PROGMEM = "Resource: General Error";
static const char str_res_no_slot[]          PROGMEM = "Resource: No Slot Available";
static const char str_res_locked[]           PROGMEM = "Resource: Locked";
static const char str_res_busy[]             PROGMEM = "Resource: Busy";
static const char str_res_not_found[]        PROGMEM = "Resource: Not Found";
static const char str_res_exists[]           PROGMEM = "Resource: Already Exists";
static const char str_res_invalid_state[]    PROGMEM = "Resource: Invalid State";

/* IPC & Protocol errors (160–191) */
static const char str_proto_general[]        PROGMEM = "IPC: General Error";
static const char str_proto_bad_message[]    PROGMEM = "IPC: Bad Message";
static const char str_proto_bad_header[]     PROGMEM = "IPC: Bad Header";
static const char str_proto_unsupported[]    PROGMEM = "IPC: Unsupported";
static const char str_proto_cycle[]          PROGMEM = "IPC: Pipe Cycle Detected";
static const char str_proto_broken[]         PROGMEM = "IPC: Pipe Broken";
static const char str_proto_overflow[]       PROGMEM = "IPC: Pipe Overflow";
static const char str_proto_no_rcpt[]        PROGMEM = "IPC: No Recipient";
static const char str_proto_queue_full[]     PROGMEM = "IPC: Queue Full";
static const char str_proto_corrupt[]        PROGMEM = "IPC: Corrupt Data";

/* Module / Loader errors (192–223) */
static const char str_mod_general[]          PROGMEM = "Module: General Error";
static const char str_mod_load_failed[]      PROGMEM = "Module: Load Failed";
static const char str_mod_unload_failed[]    PROGMEM = "Module: Unload Failed";
static const char str_mod_bad_id[]           PROGMEM = "Module: Invalid ID";
static const char str_mod_restricted[]       PROGMEM = "Module: Restricted";

/* Application errors (224–254) */
static const char str_app_general[]          PROGMEM = "App: General Error";
static const char str_app_invalid[]          PROGMEM = "App: Invalid Input";
static const char str_app_failed[]           PROGMEM = "App: Operation Failed";
static const char str_app_not_impl[]         PROGMEM = "App: Not Implemented";
static const char str_app_out_range[]        PROGMEM = "App: Out of Range";
static const char str_app_assert_failed[]    PROGMEM = "App: Assertion Failed";



/* -------------------------------------------------------------------------
 * Main lookup function
 * ------------------------------------------------------------------------- */
const char *error_to_string(uint8_t err)
{
  switch(err) {

    /* PT errors */
    case ERR_PT_GENERAL:           return str_pt_general;
    case ERR_PT_TIMEOUT:           return str_pt_timeout;
    case ERR_PT_INVALID_STATE:     return str_pt_invalid_state;
    case ERR_PT_ILLEGAL_OP:        return str_pt_illegal_op;
    case ERR_PT_STACK_CORRUPT:     return str_pt_stack_corrupt;
    case ERR_PT_CHILD_FAILED:      return str_pt_child_failed;
    case ERR_PT_UNCAUGHT:          return str_pt_uncaught;
    case ERR_PT_ASSERT_FAILED:     return str_pt_assert_failed;
    case ERR_PT_RESTART_LOOP:      return str_pt_restart_loop;

    /* System */
    case ERR_SYS_GENERAL:          return str_sys_general;
    case ERR_SYS_INIT_FAILED:      return str_sys_init_failed;
    case ERR_SYS_NO_MEMORY:        return str_sys_no_memory;
    case ERR_SYS_INVALID_HANDLE:   return str_sys_invalid_handle;
    case ERR_SYS_BAD_ARGUMENT:     return str_sys_bad_argument;
    case ERR_SYS_UNSUPPORTED:      return str_sys_unsupported;
    case ERR_SYS_OVERFLOW:         return str_sys_overflow;
    case ERR_SYS_TIMEOUT:          return str_sys_timeout;
    case ERR_SYS_DENIED:           return str_sys_denied;
    case ERR_SYS_NOT_READY:        return str_sys_not_ready;

    /* IO */
    case ERR_IO_GENERAL:           return str_io_general;
    case ERR_IO_FRAME_ERROR:       return str_io_frame_error;
    case ERR_IO_PARITY_ERROR:      return str_io_parity_error;
    case ERR_IO_BAD_LENGTH:        return str_io_bad_length;
    case ERR_IO_CORRUPT_DATA:      return str_io_corrupt_data;
    case ERR_IO_TIMEOUT:           return str_io_timeout;
    case ERR_IO_NOT_READY:         return str_io_not_ready;
    case ERR_IO_DISCONNECTED:      return str_io_disconnected;

    /* Data */
    case ERR_DATA_GENERAL:         return str_data_general;
    case ERR_DATA_OVERFLOW:        return str_data_overflow;
    case ERR_DATA_UNDERFLOW:       return str_data_underflow;
    case ERR_DATA_INVALID_FORMAT:  return str_data_invalid_format;
    case ERR_DATA_CHECKSUM:        return str_data_checksum;
    case ERR_DATA_OUT_OF_RANGE:    return str_data_range;

    /* Resources */
    case ERR_RES_GENERAL:          return str_res_general;
    case ERR_RES_NO_SLOT:          return str_res_no_slot;
    case ERR_RES_LOCKED:           return str_res_locked;
    case ERR_RES_BUSY:             return str_res_busy;
    case ERR_RES_NOT_FOUND:        return str_res_not_found;
    case ERR_RES_ALREADY_EXISTS:   return str_res_exists;
    case ERR_RES_INVALID_STATE:    return str_res_invalid_state;

    /* Protocol */
    case ERR_PROTO_GENERAL:        return str_proto_general;
    case ERR_PROTO_BAD_MESSAGE:    return str_proto_bad_message;
    case ERR_PROTO_BAD_HEADER:     return str_proto_bad_header;
    case ERR_PROTO_UNSUPPORTED:    return str_proto_unsupported;
    case ERR_PROTO_PIPE_CYCLE:     return str_proto_cycle;
    case ERR_PROTO_PIPE_BROKEN:    return str_proto_broken;
    case ERR_PROTO_PIPE_OVERFLOW:  return str_proto_overflow;
    case ERR_PROTO_NO_RECIPIENT:   return str_proto_no_rcpt;
    case ERR_PROTO_QUEUE_FULL:     return str_proto_queue_full;
    case ERR_PROTO_DATA_CORRUPT:   return str_proto_corrupt;

    /* Module */
    case ERR_MODULE_GENERAL:       return str_mod_general;
    case ERR_MODULE_LOAD_FAILED:   return str_mod_load_failed;
    case ERR_MODULE_UNLOAD_FAILED: return str_mod_unload_failed;
    case ERR_MODULE_BAD_ID:        return str_mod_bad_id;
    case ERR_MODULE_RESTRICTED:    return str_mod_restricted;

    /* Application */
    case ERR_APP_GENERAL:          return str_app_general;
    case ERR_APP_INVALID_INPUT:    return str_app_invalid;
    case ERR_APP_OPERATION_FAILED: return str_app_failed;
    case ERR_APP_NOT_IMPLEMENTED:  return str_app_not_impl;
    case ERR_APP_OUT_OF_RANGE:     return str_app_out_range;
    case ERR_APP_ASSERT_FAILED:    return str_app_assert_failed;

    default:
      return unknown_error;
  }
}

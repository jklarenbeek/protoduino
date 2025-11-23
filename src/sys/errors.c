// file:./src/sys/errors.h

#include "protoduino.h"
#include "errors.h"

/* -------------------------------------------------------------------------
 * String table stored in CC_PROGMEM to avoid wasting RAM.
 * ------------------------------------------------------------------------- */

static const char unknown_error[] CC_PROGMEM = "Unknown Error";

/* PT errors (4–31) */
static const char str_pt_general[]           CC_PROGMEM = "PT: General Error";
static const char str_pt_timeout[]           CC_PROGMEM = "PT: Timeout";
static const char str_pt_invalid_state[]     CC_PROGMEM = "PT: Invalid State";
static const char str_pt_illegal_op[]        CC_PROGMEM = "PT: Illegal Operation";
static const char str_pt_stack_corrupt[]     CC_PROGMEM = "PT: Stack Corruption";
static const char str_pt_child_failed[]      CC_PROGMEM = "PT: Child Thread Failed";
static const char str_pt_uncaught[]          CC_PROGMEM = "PT: Uncaught Error";
static const char str_pt_assert_failed[]     CC_PROGMEM = "PT: Assertion Failed";
static const char str_pt_restart_loop[]      CC_PROGMEM = "PT: Infinite Restart Loop";

/* System errors (32–63) */
static const char str_sys_general[]          CC_PROGMEM = "System: General Error";
static const char str_sys_init_failed[]      CC_PROGMEM = "System: Init Failed";
static const char str_sys_no_memory[]        CC_PROGMEM = "System: No Memory";
static const char str_sys_invalid_handle[]   CC_PROGMEM = "System: Invalid Handle";
static const char str_sys_bad_argument[]     CC_PROGMEM = "System: Bad Argument";
static const char str_sys_unsupported[]      CC_PROGMEM = "System: Not Supported";
static const char str_sys_overflow[]         CC_PROGMEM = "System: Overflow";
static const char str_sys_timeout[]          CC_PROGMEM = "System: Timeout";
static const char str_sys_denied[]           CC_PROGMEM = "System: Access Denied";
static const char str_sys_not_ready[]        CC_PROGMEM = "System: Not Ready";

/* I/O errors (64–95) */
static const char str_io_general[]           CC_PROGMEM = "I/O: General Error";
static const char str_io_frame_error[]       CC_PROGMEM = "I/O: Frame Error";
static const char str_io_parity_error[]      CC_PROGMEM = "I/O: Parity Error";
static const char str_io_bad_length[]        CC_PROGMEM = "I/O: Bad Length";
static const char str_io_corrupt_data[]      CC_PROGMEM = "I/O: Corrupt Data";
static const char str_io_timeout[]           CC_PROGMEM = "I/O: Timeout";
static const char str_io_not_ready[]         CC_PROGMEM = "I/O: Not Ready";
static const char str_io_disconnected[]      CC_PROGMEM = "I/O: Disconnected";

/* Data errors (96–127) */
static const char str_data_general[]         CC_PROGMEM = "Data: General Error";
static const char str_data_overflow[]        CC_PROGMEM = "Data: Overflow";
static const char str_data_underflow[]       CC_PROGMEM = "Data: Underflow";
static const char str_data_invalid_format[]  CC_PROGMEM = "Data: Invalid Format";
static const char str_data_checksum[]        CC_PROGMEM = "Data: Bad Checksum";
static const char str_data_range[]           CC_PROGMEM = "Data: Out of Range";

/* Resource errors (128–159) */
static const char str_res_general[]          CC_PROGMEM = "Resource: General Error";
static const char str_res_no_slot[]          CC_PROGMEM = "Resource: No Slot Available";
static const char str_res_locked[]           CC_PROGMEM = "Resource: Locked";
static const char str_res_busy[]             CC_PROGMEM = "Resource: Busy";
static const char str_res_not_found[]        CC_PROGMEM = "Resource: Not Found";
static const char str_res_exists[]           CC_PROGMEM = "Resource: Already Exists";
static const char str_res_invalid_state[]    CC_PROGMEM = "Resource: Invalid State";

/* IPC & Protocol errors (160–191) */
static const char str_proto_general[]        CC_PROGMEM = "IPC: General Error";
static const char str_proto_bad_message[]    CC_PROGMEM = "IPC: Bad Message";
static const char str_proto_bad_header[]     CC_PROGMEM = "IPC: Bad Header";
static const char str_proto_unsupported[]    CC_PROGMEM = "IPC: Unsupported";
static const char str_proto_cycle[]          CC_PROGMEM = "IPC: Pipe Cycle Detected";
static const char str_proto_broken[]         CC_PROGMEM = "IPC: Pipe Broken";
static const char str_proto_overflow[]       CC_PROGMEM = "IPC: Pipe Overflow";
static const char str_proto_no_rcpt[]        CC_PROGMEM = "IPC: No Recipient";
static const char str_proto_queue_full[]     CC_PROGMEM = "IPC: Queue Full";
static const char str_proto_corrupt[]        CC_PROGMEM = "IPC: Corrupt Data";

/* Module / Loader errors (192–223) */
static const char str_mod_general[]          CC_PROGMEM = "Module: General Error";
static const char str_mod_load_failed[]      CC_PROGMEM = "Module: Load Failed";
static const char str_mod_unload_failed[]    CC_PROGMEM = "Module: Unload Failed";
static const char str_mod_bad_id[]           CC_PROGMEM = "Module: Invalid ID";
static const char str_mod_restricted[]       CC_PROGMEM = "Module: Restricted";

/* Application errors (224–254) */
static const char str_app_general[]          CC_PROGMEM = "App: General Error";
static const char str_app_invalid[]          CC_PROGMEM = "App: Invalid Input";
static const char str_app_failed[]           CC_PROGMEM = "App: Operation Failed";
static const char str_app_not_impl[]         CC_PROGMEM = "App: Not Implemented";
static const char str_app_out_range[]        CC_PROGMEM = "App: Out of Range";
static const char str_app_assert_failed[]    CC_PROGMEM = "App: Assertion Failed";



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

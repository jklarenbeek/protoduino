// file:./src/sys/errors.h

/*
 * Unified Error Codes For Protothreads v2 and the Process Kernel
 * --------------------------------------------------------------
 *
 * This file merges PTv2 exception error codes and Process/Kernel/
 * IPC/Module/Resource errors into a single shared namespace.
 *
 * RULES:
 *  - 0  = success
 *  - 1–3 are reserved by protothread state machine (WAITING, YIELDED, EXITED, ENDED)
 *  - 4–254 are error states for PT_RAISE() or PT_THROW()
 *  - The scheduler sends these codes via PROCESS_EVENT_ERROR
 *
 * PROTOTHREAD GUARANTEES:
 *  - PT_ERROR == 4
 *  - PT_FINALIZED == 255  (not an error)
 *
 * RANGES:
 *  - 000–003 : PT internal states (reserved)
 *  - 004–031 : Protothread internal errors
 *  - 032–063 : System/Kernel errors
 *  - 064–095 : I/O communication errors (serial, I2C, SPI, etc.)
 *  - 096–127 : Data & buffer errors
 *  - 128–159 : Resource & memory allocation errors
 *  - 160–191 : IPC / protocol / pipe errors
 *  - 192–223 : Dynamic module / loader errors
 *  - 224–254 : Application/user-defined errors
 *  - 255      : PT_FINALIZED (not an error)
 */

#ifndef __ERRORS_H__
#define __ERRORS_H__

/* ---------------------------------------------------------------------------
 * BASE CONSTANTS
 * ---------------------------------------------------------------------------*/
#define ERR_SUCCESS              0
#define ERR_PT_BASE              PT_ERROR
#define ERR_SYSTEM_BASE          32
#define ERR_IO_BASE              64
#define ERR_DATA_BASE            96
#define ERR_RESOURCE_BASE        128
#define ERR_PROTO_BASE           160
#define ERR_MODULE_BASE          192
#define ERR_APP_BASE             224

/* ===========================================================================
 * PROTOTHREAD INTERNAL ERRORS  (4–31)
 *   These errors are used with PT_RAISE(), PT_THROW(), and PT_RETHROW().
 *   They represent exceptional conditions in protothread execution.
 * ===========================================================================*/
#define ERR_PT_GENERAL           (ERR_PT_BASE + 0)   /* unspecified protothread exception */
#define ERR_PT_TIMEOUT           (ERR_PT_BASE + 1)   /* waited too long */
#define ERR_PT_INVALID_STATE     (ERR_PT_BASE + 2)   /* illegal LC or state logic */
#define ERR_PT_ILLEGAL_OP        (ERR_PT_BASE + 3)   /* illegal PT macro use */
#define ERR_PT_STACK_CORRUPT     (ERR_PT_BASE + 4)   /* continuation corrupted */
#define ERR_PT_CHILD_FAILED      (ERR_PT_BASE + 5)   /* spawned child ended with an error */
#define ERR_PT_UNCAUGHT          (ERR_PT_BASE + 6)   /* uncaught/unknown PT error */
#define ERR_PT_ASSERT_FAILED     (ERR_PT_BASE + 7)   /* internal assertion failed */
#define ERR_PT_RESTART_LOOP      (ERR_PT_BASE + 8)   /* infinite restart loop detected */

/* ===========================================================================
 * SYSTEM / KERNEL ERRORS (32–63)
 * ===========================================================================*/
#define ERR_SYS_GENERAL          (ERR_SYSTEM_BASE + 0)
#define ERR_SYS_INIT_FAILED      (ERR_SYSTEM_BASE + 1)
#define ERR_SYS_NO_MEMORY        (ERR_SYSTEM_BASE + 2)
#define ERR_SYS_INVALID_HANDLE   (ERR_SYSTEM_BASE + 3)
#define ERR_SYS_BAD_ARGUMENT     (ERR_SYSTEM_BASE + 4)
#define ERR_SYS_UNSUPPORTED      (ERR_SYSTEM_BASE + 5)
#define ERR_SYS_OVERFLOW         (ERR_SYSTEM_BASE + 6)
#define ERR_SYS_TIMEOUT          (ERR_SYSTEM_BASE + 7)
#define ERR_SYS_DENIED           (ERR_SYSTEM_BASE + 8)  /* permission denied */
#define ERR_SYS_NOT_READY        (ERR_SYSTEM_BASE + 9)

/* ===========================================================================
 * I/O & COMMUNICATION ERRORS (64–95)
 *   UART, I2C, SPI, network, radio, etc.
 * ===========================================================================*/
#define ERR_IO_GENERAL           (ERR_IO_BASE + 0)
#define ERR_IO_FRAME_ERROR       (ERR_IO_BASE + 1)
#define ERR_IO_PARITY_ERROR      (ERR_IO_BASE + 2)
#define ERR_IO_BAD_LENGTH        (ERR_IO_BASE + 3)
#define ERR_IO_CORRUPT_DATA      (ERR_IO_BASE + 4)
#define ERR_IO_TIMEOUT           (ERR_IO_BASE + 5)
#define ERR_IO_NOT_READY         (ERR_IO_BASE + 6)
#define ERR_IO_DISCONNECTED      (ERR_IO_BASE + 7)

/* ===========================================================================
 * DATA / BUFFER ERRORS (96–127)
 * ===========================================================================*/
#define ERR_DATA_GENERAL         (ERR_DATA_BASE + 0)
#define ERR_DATA_OVERFLOW        (ERR_DATA_BASE + 1)
#define ERR_DATA_UNDERFLOW       (ERR_DATA_BASE + 2)
#define ERR_DATA_INVALID_FORMAT  (ERR_DATA_BASE + 3)
#define ERR_DATA_CHECKSUM        (ERR_DATA_BASE + 4)
#define ERR_DATA_OUT_OF_RANGE    (ERR_DATA_BASE + 5)

/* ===========================================================================
 * RESOURCE / MEMORY ERRORS (128–159)
 *   Process slots, memory ownership, locks, buffers…
 * ===========================================================================*/
#define ERR_RES_GENERAL          (ERR_RESOURCE_BASE + 0)
#define ERR_RES_NO_SLOT          (ERR_RESOURCE_BASE + 1)  /* no process slot */
#define ERR_RES_LOCKED           (ERR_RESOURCE_BASE + 2)  /* locked by someone */
#define ERR_RES_BUSY             (ERR_RESOURCE_BASE + 3)  /* busy/occupied */
#define ERR_RES_NOT_FOUND        (ERR_RESOURCE_BASE + 4)
#define ERR_RES_ALREADY_EXISTS   (ERR_RESOURCE_BASE + 5)
#define ERR_RES_INVALID_STATE    (ERR_RESOURCE_BASE + 6)

/* ===========================================================================
 * IPC / PIPE / PROTOCOL ERRORS (160–191)
 *   process_post, messaging, interprocess pipes, etc.
 * ===========================================================================*/
#define ERR_PROTO_GENERAL        (ERR_PROTO_BASE + 0)
#define ERR_PROTO_BAD_MESSAGE    (ERR_PROTO_BASE + 1)
#define ERR_PROTO_BAD_HEADER     (ERR_PROTO_BASE + 2)
#define ERR_PROTO_UNSUPPORTED    (ERR_PROTO_BASE + 3)
#define ERR_PROTO_PIPE_CYCLE     (ERR_PROTO_BASE + 4)  /* cyclic pipe graph */
#define ERR_PROTO_PIPE_BROKEN    (ERR_PROTO_BASE + 5)  /* closed/unlinked */
#define ERR_PROTO_PIPE_OVERFLOW  (ERR_PROTO_BASE + 6)
#define ERR_PROTO_NO_RECIPIENT   (ERR_PROTO_BASE + 7)
#define ERR_PROTO_QUEUE_FULL     (ERR_PROTO_BASE + 8)
#define ERR_PROTO_DATA_CORRUPT   (ERR_PROTO_BASE + 9)

/* ===========================================================================
 * MODULE / LOADER ERRORS (192–223)
 *   Dynamic code loading, virtual processes, etc.
 * ===========================================================================*/
#define ERR_MODULE_GENERAL       (ERR_MODULE_BASE + 0)
#define ERR_MODULE_LOAD_FAILED   (ERR_MODULE_BASE + 1)
#define ERR_MODULE_UNLOAD_FAILED (ERR_MODULE_BASE + 2)
#define ERR_MODULE_BAD_ID        (ERR_MODULE_BASE + 3)
#define ERR_MODULE_RESTRICTED    (ERR_MODULE_BASE + 4)

/* ===========================================================================
 * APPLICATION / USER ERRORS (224–254)
 * ===========================================================================*/
#define ERR_APP_GENERAL          (ERR_APP_BASE + 0)
#define ERR_APP_INVALID_INPUT    (ERR_APP_BASE + 1)
#define ERR_APP_OPERATION_FAILED (ERR_APP_BASE + 2)
#define ERR_APP_NOT_IMPLEMENTED  (ERR_APP_BASE + 3)
#define ERR_APP_OUT_OF_RANGE     (ERR_APP_BASE + 4)
#define ERR_APP_ASSERT_FAILED    (ERR_APP_BASE + 5)

/* ===========================================================================
 * RESERVED (255)
 * ===========================================================================*/
#define ERR_PT_FINALIZED         255  /* not an error, forced by PTv2 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Lookup human-readable error text.
 * Returns a pointer to a CC_PROGMEM string.
 *
 * If the code is unknown:
 *   returns PSTR("Unknown Error")
 */
const char *error_to_string(uint8_t err);

#ifdef __cplusplus
}
#endif

#endif /* __ERRORS_H__ */

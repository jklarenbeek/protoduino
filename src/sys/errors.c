// file: ./src/sys/errors.c
#include "errors.h"
#include "errors.conf.h"
#include <stdint.h>

/*
 * If the S macro is not provided elsewhere, provide the
 * compile-time selector: tiny (short) vs verbose messages.
 */
#ifndef S
  #if ERRORS_CONF_VERBOSE_MESSAGES
    #define S(tiny, verbose) (verbose)
  #else
    #define S(tiny, verbose) (tiny)
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

const char *error_to_string(uint8_t err)
{
    switch (err) {
        /* 0..3: success / protothread flow states */
        case ERR_SUCCESS:
        case ERR_WAITING:         return S("OK", "Success");
        case ERR_YIELDING:        return S("YIELD", "Yielded (protothread)");
        case ERR_EXITING:         return S("EXIT", "Exiting (protothread)");
        case ERR_ENDING:          return S("END", "Ending (protothread)");

        /* Generic failure / kernel base (4..) */
        case ERR_FAILURE:
        case ERR_BASE_PT:
        case ERR_PT_ERROR:        return S("FAIL", "Failure (generic)");
        case ERR_PT_YIELD:        return S("PT_YLD", "Protothread forced yield");
        case ERR_PT_DETACHED:     return S("DETACH", "Thread detached / orphaned");
        case ERR_PT_CORRUPT:      return S("CORR", "Context / stack corruption detected");
        case ERR_PT_DEADLOCK:     return S("DEADLK", "Scheduler detected deadlock");
        case ERR_PT_STALLED:      return S("STALL", "Thread stalled / starved (WDT warned)");
        case ERR_PT_LIMIT:        return S("LIMIT", "Process/thread limit reached");
        case ERR_PT_SPAWN:        return S("SPAWN", "Failed to spawn/fork process/thread");
        case ERR_PT_OVERFLOW:     return S("Q_OVR", "Kernel message queue overflow");
        case ERR_PT_NOT_FOUND:    return S("NOTFND", "Process or thread not found");
        case ERR_PT_ILLEGAL:      return S("ILL", "Illegal operation in current context");
        case ERR_PT_RECURSION:    return S("RECUR", "Illegal recursion / stack risk");
        case ERR_PT_TIMEOUT:      return S("PT_TO", "Protothread timeout");
        case ERR_PT_SEMAPHORE:    return S("SEM_ERR", "Semaphore error");

        /* System & POSIX-compatible (32..) */
        case ERR_BASE_SYS:
        case ERR_SYS_UNKNOWN:     return S("SYS", "System unknown error");
        case ERR_SYS_INVAL:       return S("INVAL", "Invalid argument");
        case ERR_SYS_NOMEM:       return S("NOMEM", "Out of memory");
        case ERR_SYS_PERM:        return S("PERM", "Permission denied");
        case ERR_SYS_BUSY:        return S("BUSY", "Resource busy");
        case ERR_SYS_TIMEDOUT:    return S("TIMEO", "Operation timed out");
        case ERR_SYS_OVERFLOW:    return S("OVF", "Overflow");
        case ERR_SYS_UNDERFLOW:   return S("UNDF", "Underflow");
        case ERR_SYS_NOSYS:       return S("NOSYS", "Not implemented / no system support");
        case ERR_SYS_AGAIN:       return S("AGAIN", "Try again");
        case ERR_SYS_CANCELED:    return S("CANCEL", "Operation canceled");
        case ERR_SYS_STATE:       return S("STATE", "Invalid system state");
        case ERR_SYS_POWER:       return S("PWR", "Power failure / low battery");
        case ERR_SYS_WATCHDOG:    return S("WDT", "Watchdog reset / imminent");
        case ERR_SYS_HW_FAIL:     return S("HWFAIL", "Critical hardware failure");
        case ERR_SYS_CLOCK:       return S("CLK", "Clock / crystal failure");
        case ERR_SYS_FAULT:       return S("FAULT", "Bad address / fault");

        /* Hardware I/O & drivers (64..) */
        case ERR_BASE_IO:
        case ERR_IO_GENERAL:      return S("IO", "I/O general error");
        case ERR_IO_DISCONNECTED: return S("DISC", "Disconnected / PHY down");
        case ERR_IO_ABORT:        return S("ABORT", "Driver aborted transaction");
        case ERR_IO_RETRY:        return S("RETRY", "Retry request (transient)");
        case ERR_IO_SIZE:         return S("SZ", "DMA/Buffer size mismatch");
        case ERR_IO_ALIGN:        return S("ALIGN", "Memory alignment error");
        case ERR_IO_CRC:          return S("CRC", "CRC mismatch");
        case ERR_IO_ARBITRATION:  return S("ARB", "Bus arbitration lost");
        case ERR_IO_NACK:         return S("NACK", "No ACK / no response");
        case ERR_IO_PIN_LOCK:     return S("PINLK", "Pin mux / lock conflict");
        case ERR_IO_ADC_RANGE:    return S("ADC_RNG", "ADC out of range");
        case ERR_IO_DMA:          return S("DMA_ERR", "DMA controller error");
        case ERR_IO_INIT:         return S("INIT", "Peripheral init failed");
        case ERR_IO_FRAME_ERROR:  return S("FRAME", "UART framing error");
        case ERR_IO_PARITY_ERROR: return S("PARITY", "UART parity error");
        case ERR_IO_OVERRUN:      return S("OVERRUN", "Buffer overrun");

        /* Storage & parsing (96..) */
        case ERR_BASE_STORE:
        case ERR_STORE_GENERAL:   return S("STORE", "Storage general error");
        case ERR_STORE_MOUNT:     return S("MOUNT", "Filesystem mount failed");
        case ERR_STORE_NOENT:     return S("NOENT", "File / key not found");
        case ERR_STORE_NOSPC:     return S("NOSPC", "No space left on device");
        case ERR_STORE_LOCKED:    return S("LOCK", "File locked");
        case ERR_STORE_CORRUPT:   return S("CORRUPT", "Filesystem corrupt");
        case ERR_STORE_WEAR:      return S("WEAR", "Flash wear limit reached");
        case ERR_STORE_PROTECTED: return S("PROT", "Write protection active");

        case ERR_DATA_FORMAT:     return S("FMT", "Data format / parse error");
        case ERR_DATA_TRUNCATED:  return S("TRUNC", "Truncated data / buffer too small");
        case ERR_DATA_EMPTY:      return S("EMPTY", "Empty data");
        case ERR_DATA_TYPE:       return S("TYPE", "Unexpected data type");
        case ERR_DATA_CHECKSUM:   return S("CHK", "Checksum / CRC mismatch");
        case ERR_DATA_NULL:       return S("NULL", "Null pointer in data");
        case ERR_DATA_BAD_MESSAGE:return S("BADMSG", "Bad message");
        case ERR_DATA_BAD_HEADER: return S("BADHDR", "Bad header");
        case ERR_DATA_UNSUPPORTED:return S("UNSUP", "Unsupported data");
        case ERR_DATA_OVERFLOW:   return S("D_OVR", "Data overflow");
        case ERR_DATA_SEQUENCE:   return S("SEQ", "Sequence error");

        /* Network & transport (128..) */
        case ERR_BASE_NET:
        case ERR_NET_GENERAL:     return S("NET", "Network general error");
        case ERR_NET_DOWN:        return S("NDOWN", "Network interface down");
        case ERR_NET_UNREACHABLE: return S("NUNR", "Host unreachable");
        case ERR_NET_RESOLVE:     return S("DNS", "Name resolution failed");
        case ERR_NET_CONN_REFUSED:return S("REFUSE", "Connection refused");
        case ERR_NET_CONN_LOST:   return S("CONNLST", "Connection lost / reset");
        case ERR_NET_PROTOCOL:    return S("PROTO", "Protocol violation");
        case ERR_NET_PAYLOAD:     return S("PAYLD", "Invalid payload");
        case ERR_NET_AUTH:        return S("NAUTH", "Network authentication failed");
        case ERR_NET_BIND:        return S("BIND", "Bind / port in use");
        case ERR_NET_PACKET:      return S("PKT", "Malformed packet");
        case ERR_NET_COLLISION:   return S("COL", "Collision detected");
        case ERR_NET_TIMEOUT:     return S("NTO", "Network timeout");

        case ERR_PIPE_CYCLE:      return S("PCYCLE", "Cyclic pipe graph error");
        case ERR_PIPE_BROKEN:     return S("PBROKE", "Pipe closed / unlinked");
        case ERR_PIPE_OVERFLOW:   return S("P_OVR", "Pipe overflow");

        /* Security & crypto (160..) */
        case ERR_BASE_SEC:
        case ERR_SEC_GENERAL:     return S("SEC", "Security general error");
        case ERR_SEC_INIT:        return S("SEC_INIT", "Crypto init failed");
        case ERR_SEC_ENTROPY:     return S("ENT", "Insufficient entropy / RNG weak");
        case ERR_SEC_AUTH_FAIL:   return S("AUTH_FAIL", "Credential validation failed");
        case ERR_SEC_SIGNATURE:   return S("SIG", "Invalid signature");
        case ERR_SEC_KEY_INVALID: return S("KEY_INV", "Invalid key format");
        case ERR_SEC_KEY_MISSING: return S("KEY_MISS", "Key slot missing");
        case ERR_SEC_CERT_EXPIRED:return S("CERT_EXP", "Certificate expired");
        case ERR_SEC_CERT_REVOKED:return S("CERT_REV", "Certificate revoked");
        case ERR_SEC_HASH_MISMATCH:return S("HASH", "Hash mismatch");
        case ERR_SEC_UNTRUSTED:   return S("UNTRUST", "Untrusted root / verify failed");
        case ERR_SEC_LOCKED:      return S("LOCKED", "Secure element locked");
        case ERR_SEC_ACCESS:      return S("NOACCESS", "Access control denied");

        /* Resources & modules (192..) */
        case ERR_BASE_RES:
        case ERR_RES_GENERAL:     return S("RES", "Resource general error");
        case ERR_RES_EXHAUSTED:   return S("EXH", "Resources exhausted");
        case ERR_RES_LEAK:        return S("LEAK", "Resource leak detected");
        case ERR_RES_INVALID_ID:  return S("BADID", "Invalid handle/ID");
        case ERR_RES_OWNERSHIP:   return S("OWN", "Ownership violation");
        case ERR_MOD_HEADER:      return S("MOD_HDR", "Module/ELF header invalid");
        case ERR_MOD_VERSION:     return S("MOD_VER", "Module version mismatch");
        case ERR_MOD_DEPENDENCY:  return S("MOD_DEP", "Missing dependency");
        case ERR_MOD_COMPAT:      return S("MOD_COMP", "Module incompatible with platform");
        case ERR_MOD_SIZE:        return S("MOD_SZ", "Module image too large");
        case ERR_MOD_RESTRICTED:  return S("MOD_RSTR", "Module restricted");

        /* Application space: 224..254 intentionally left to apps.
         * We'll return a generic app label for codes in that range not defined above.
         */
        case ERR_BASE_APP:        return S("APP", "Application general error");

        /* 255: Last protothread flow state, can not continue without PT_INIT(...) */
        case ERR_FINALIZED:       return S("FINAL", "Protothread finalized (not an error)");

        default:
            /* Catch application range or any unlisted code */
            if (err >= ERR_BASE_APP && err < ERR_FINALIZED) {
                return S("APP_ERR", "Application error (user-defined)");
            }
            return S("UNK", "Unknown error code");
    }
}

const char *error_subsystem_to_string(uint8_t err)
{
#if ERRORS_CONF_VERBOSE_MESSAGES

    uint8_t s = ERR_GET_SUBSYSTEM(err);

    switch (s) {
        case 0: return S("PT", "Protothread / Kernel");
        case 1: return S("SYS", "System / POSIX");
        case 2: return S("IO", "Hardware I/O");
        case 3: return S("STORE", "Storage & Data");
        case 4: return S("NET", "Network / IPC");
        case 5: return S("SEC", "Security / Crypto");
        case 6: return S("RES", "Resources / Modules");
        case 7: return S("APP", "Application / User Space");
        default: return S("UNK", "Unknown Subsystem");
    }

#else
    return error_to_string(err);
#endif

}

#ifdef __cplusplus
}
#endif

// file:./src/sys/errors.h

/*
 * Embedded Error Taxonomy v2.0
 * ----------------------------
 * Designed for Safety-Critical & Resource-Constrained Kernels
 *
 * This file merges PTv2 exception error codes and Process/Kernel/
 * IPC/Module/Resource errors into a single shared namespace.
 *
 * These errors are used with PT_RAISE(), PT_THROW(), and PT_RETHROW().
 * They represent exceptional conditions in protothread execution.
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
 * 1. STRUCTURAL ALIGNMENT (The "32-Block" Rule):
 *    Retained 32-value blocks for fast bitwise categorization (e.g., (err >> 5) gives the subsystem ID 0-7).
 *    - 000-031: Core/PT (Meta) – Expanded slightly with ERR_PT_TIMEOUT for consistency with original PT errors.
 *    - 032-063: System/POSIX (Standard errnos) – Added ERR_SYS_EFAULT for bad address (common in embedded pointer errors).
 *    - 064-095: Hardware I/O (Physical Layer) – Added ERR_IO_OVERRUN for buffer issues, common in UART/SPI.
 *    - 096-127: Storage & Data (Persistence & Parsing) – Merged checksum/CRC logically; added ERR_DATA_NULL for null ptr (POSIX-like).
 *    - 128-159: Network & IPC (Transport Layer) – Added ERR_NET_TIMEOUT for protocol timeouts.
 *    - 160-191: Security & Crypto (Auth & Integrity) – Added ERR_SEC_ACCESS for access control violations.
 *    - 192-223: Resource & Lifecycle (Dynamic Alloc/Loading) – Added ERR_RES_OWNERSHIP for ownership checks.
 *    - 224-254: Application (User Space) – Kept fully free.
 *
 * 2. POSIX COMPATIBILITY:
 *    Enhanced mappings (e.g., ERR_SYS_INVAL, ERR_SYS_NOMEM) based on Zephyr/FreeRTOS+POSIX usage for developer familiarity.
 *
 * 3. CRITICAL MISSING DOMAINS:
 *    Retained v2.0 additions (Power, Watchdog, Security, Filesystem); added a few high-frequency from audit (e.g., null ptr, overrun).
 *
 * 4. CONSOLIDATION & CLARITY:
 *    Eliminated any remaining overlaps (e.g., unified overflow/underflow in sys); ensured names are precise and non-ambiguous.
 *
 * 5. CONSTRAINTS:
 *    - Kept 0=Success, 1-3=PT States, 4=PT_ERROR, 255=PT_FINALIZED.
 *    - 31 app slots free.
 *    - Added <20 new codes total (net +8 after merges) to stay practical.
 */

#ifndef __ERRORS_H__
#define __ERRORS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------
 * GLOBAL CONSTANTS
 * ---------------------------------------------------------------------------*/
#define ERR_SUCCESS              0
#define ERR_FAILURE              4    /* Generic catch-all if specific code unavailable */

/* Subsystem Bases (Aligned to 32 for fast bit-shifting) */
#define ERR_BASE_PT              ERR_FAILURE
#define ERR_BASE_SYS             32
#define ERR_BASE_IO              64
#define ERR_BASE_STORE           96
#define ERR_BASE_NET             128
#define ERR_BASE_SEC             160
#define ERR_BASE_RES             192
#define ERR_BASE_APP             224

/* Helper: Get Subsystem ID (0-7) from error code */
#define ERR_GET_SUBSYSTEM(e)     ((e) >> 5)

/* ===========================================================================
 * 1. PROTOTHREAD & KERNEL CORE (04–31)
 *    Kernel Exceptions
 *    Reserved: 0-3 (Success/Waiting, Yielded, Exited, Ended).
 *    Range: 4-31.
 * ===========================================================================*/

/* Kernel Exceptions */
#define ERR_PT_ERROR             (ERR_BASE_PT + 0)   /* Standard PT_ERROR base */
#define ERR_PT_YIELD             (ERR_BASE_PT + 1)   /* Forced yield (not error, but flow control) */
#define ERR_PT_DETACHED          (ERR_BASE_PT + 2)   /* Thread detached/orphaned */
#define ERR_PT_CORRUPT           (ERR_BASE_PT + 3)   /* Context/Stack corruption detected */
#define ERR_PT_DEADLOCK          (ERR_BASE_PT + 4)   /* Schedular detected circular wait */
#define ERR_PT_STALLED           (ERR_BASE_PT + 5)   /* Thread starved/stalled (WDT pre-warning) */
#define ERR_PT_LIMIT             (ERR_BASE_PT + 6)   /* Number of process or thread limit reached */
#define ERR_PT_SPAWN             (ERR_BASE_PT + 7)   /* Failed to fork/spawn */
#define ERR_PT_OVERFLOW          (ERR_BASE_PT + 8)   /* Kernel message queue overflow */
#define ERR_PT_NOT_FOUND         (ERR_BASE_PT + 9)   /* Process or thread not found */
#define ERR_PT_ILLEGAL           (ERR_BASE_PT + 10)  /* Op not allowed in current context (ISR vs Thread) */
#define ERR_PT_RECURSION         (ERR_BASE_PT + 11)  /* Stack overflow risk / Illegal recursion */
#define ERR_PT_TIMEOUT           (ERR_BASE_PT + 12)  /* General PT timeout */
#define ERR_PT_SEMAPHORE         (ERR_BASE_PT + 13)  /* Semaphore error */

/* ===========================================================================
 * 2. SYSTEM & POSIX-COMPAT (32–63)
 *    General system health, parameters, and arithmetic.
 * ===========================================================================*/
#define ERR_SYS_UNKNOWN          (ERR_BASE_SYS + 0)
#define ERR_SYS_INVAL            (ERR_BASE_SYS + 1)  /* Invalid argument (EINVAL) */
#define ERR_SYS_NOMEM            (ERR_BASE_SYS + 2)  /* Out of heap/memory (ENOMEM) */
#define ERR_SYS_PERM             (ERR_BASE_SYS + 3)  /* Permission denied (EPERM) */
#define ERR_SYS_BUSY             (ERR_BASE_SYS + 4)  /* Resource busy (EBUSY) */
#define ERR_SYS_TIMEDOUT         (ERR_BASE_SYS + 5)  /* Operation timed out (ETIMEDOUT) */
#define ERR_SYS_OVERFLOW         (ERR_BASE_SYS + 6)  /* Math/Buffer overflow (EOVERFLOW) */
#define ERR_SYS_UNDERFLOW        (ERR_BASE_SYS + 7)  /* Math/Buffer underflow */
#define ERR_SYS_NOSYS            (ERR_BASE_SYS + 8)  /* Not implemented/supported (ENOSYS) */
#define ERR_SYS_AGAIN            (ERR_BASE_SYS + 9)  /* Try again later (EAGAIN) */
#define ERR_SYS_CANCELED         (ERR_BASE_SYS + 10) /* Operation canceled (ECANCELED) */
#define ERR_SYS_STATE            (ERR_BASE_SYS + 11) /* Invalid system state for op */
#define ERR_SYS_POWER            (ERR_BASE_SYS + 12) /* Power failure / Low Battery */
#define ERR_SYS_WATCHDOG         (ERR_BASE_SYS + 13) /* Watchdog reset occurred/imminent */
#define ERR_SYS_HW_FAIL          (ERR_BASE_SYS + 14) /* Critical Hardware Failure */
#define ERR_SYS_CLOCK            (ERR_BASE_SYS + 15) /* Clock/Crystal failure */
#define ERR_SYS_FAULT            (ERR_BASE_SYS + 16) /* Bad address (EFAULT, added for pointer errors) */

/* ===========================================================================
 * 3. HARDWARE I/O & DRIVERS (64–95)
 *    Physical Layer, Buses (I2C/SPI), GPIO.
 * ===========================================================================*/
#define ERR_IO_GENERAL           (ERR_BASE_IO + 0)
#define ERR_IO_DISCONNECTED      (ERR_BASE_IO + 1)  /* Cable unplugged / Phy down */
#define ERR_IO_ABORT             (ERR_BASE_IO + 2)  /* Driver aborted transaction */
#define ERR_IO_RETRY             (ERR_BASE_IO + 3)  /* Request retry (transient noise) */
#define ERR_IO_SIZE              (ERR_BASE_IO + 4)  /* DMA/Buffer size mismatch */
#define ERR_IO_ALIGN             (ERR_BASE_IO + 5)  /* DMA/Memory alignment error */
#define ERR_IO_CRC               (ERR_BASE_IO + 6)  /* Hardware CRC mismatch */
#define ERR_IO_ARBITRATION       (ERR_BASE_IO + 7)  /* I2C/CAN Arbitration lost */
#define ERR_IO_NACK              (ERR_BASE_IO + 8)  /* I2C NACK / No response */
#define ERR_IO_PIN_LOCK          (ERR_BASE_IO + 9)  /* Pin muxing/lock conflict */
#define ERR_IO_ADC_RANGE         (ERR_BASE_IO + 10) /* Analog value out of range */
#define ERR_IO_DMA               (ERR_BASE_IO + 11) /* DMA controller error */
#define ERR_IO_INIT              (ERR_BASE_IO + 12) /* Peripheral init failed */
#define ERR_IO_FRAME_ERROR       (ERR_BASE_IO + 13) /* UART framing error */
#define ERR_IO_PARITY_ERROR      (ERR_BASE_IO + 14) /* UART parity error */
#define ERR_IO_OVERRUN           (ERR_BASE_IO + 15) /* Buffer overrun (added for common UART/SPI errors) */

/* ===========================================================================
 * 4. STORAGE & DATA PARSING (96–127)
 *    Flash, Filesystem, Buffers, Serialization (JSON/CBOR).
 * ===========================================================================*/
#define ERR_STORE_GENERAL        (ERR_BASE_STORE + 0)
#define ERR_STORE_MOUNT          (ERR_BASE_STORE + 1) /* FS Mount failed */
#define ERR_STORE_NOENT          (ERR_BASE_STORE + 2) /* File/Key not found (ENOENT) */
#define ERR_STORE_NOSPC          (ERR_BASE_STORE + 3) /* Disk/Flash full (ENOSPC) */
#define ERR_STORE_LOCKED         (ERR_BASE_STORE + 4) /* File locked */
#define ERR_STORE_CORRUPT        (ERR_BASE_STORE + 5) /* FS structure corrupt */
#define ERR_STORE_WEAR           (ERR_BASE_STORE + 6) /* Flash wear limit reached */
#define ERR_STORE_PROTECTED      (ERR_BASE_STORE + 7) /* Write protection active */

#define ERR_DATA_FORMAT          (ERR_BASE_STORE + 8) /* Parse error (JSON/XML/CBOR/etc) */
#define ERR_DATA_TRUNCATED       (ERR_BASE_STORE + 9) /* Buffer too small for payload */
#define ERR_DATA_EMPTY           (ERR_BASE_STORE + 10)
#define ERR_DATA_TYPE            (ERR_BASE_STORE + 11) /* Wrong type (Int vs String) */
#define ERR_DATA_CHECKSUM        (ERR_BASE_STORE + 12) /* Logical checksum (LRC/CRC) */
#define ERR_DATA_NULL            (ERR_BASE_STORE + 13) /* Null pointer (added for common data errors) */
#define ERR_DATA_BAD_MESSAGE     (ERR_BASE_STORE + 14)
#define ERR_DATA_BAD_HEADER      (ERR_BASE_STORE + 15)
#define ERR_DATA_UNSUPPORTED     (ERR_BASE_STORE + 16)
#define ERR_DATA_OVERFLOW        (ERR_BASE_STORE + 17)
#define ERR_DATA_SEQUENCE        (ERR_BASE_STORE + 18)

/* ===========================================================================
 * 5. NETWORK & TRANSPORT (128–159)
 *    Stacks (LwIP, Zephyr), Radio, Protocols (MQTT, CoAP).
 * ===========================================================================*/
#define ERR_NET_GENERAL          (ERR_BASE_NET + 0)
#define ERR_NET_DOWN             (ERR_BASE_NET + 1) /* Interface down */
#define ERR_NET_UNREACHABLE      (ERR_BASE_NET + 2) /* No route to host */
#define ERR_NET_RESOLVE          (ERR_BASE_NET + 3) /* DNS/Discovery failed */
#define ERR_NET_CONN_REFUSED     (ERR_BASE_NET + 4) /* TCP/Connection refused */
#define ERR_NET_CONN_LOST        (ERR_BASE_NET + 5) /* Connection reset/dropped */
#define ERR_NET_PROTO            (ERR_BASE_NET + 6) /* Protocol violation */
#define ERR_NET_PAYLOAD          (ERR_BASE_NET + 7) /* Invalid payload for proto */
#define ERR_NET_AUTH             (ERR_BASE_NET + 8) /* Network-level auth failed */
#define ERR_NET_BIND             (ERR_BASE_NET + 9) /* Port/Socket in use */
#define ERR_NET_PACKET           (ERR_BASE_NET + 10) /* Malformed packet */
#define ERR_NET_COLLISION        (ERR_BASE_NET + 11) /* Radio/Ethernet collision */
#define ERR_NET_TIMEOUT          (ERR_BASE_NET + 12) /* Network/protocol timeout (added for completeness) */

#define ERR_PIPE_CYCLE           (ERR_BASE_NET + 13) /* Cyclic pipe graph error */
#define ERR_PIPE_BROKEN          (ERR_BASE_NET + 14) /* Closed/Unlinked */
#define ERR_PIPE_OVERFLOW        (ERR_BASE_NET + 15)

/* ===========================================================================
 * 6. SECURITY & CRYPTOGRAPHY (160–191)
 *    TLS, Keys, RNG, Access Control.
 * ===========================================================================*/
#define ERR_SEC_GENERAL          (ERR_BASE_SEC + 0)
#define ERR_SEC_INIT             (ERR_BASE_SEC + 1) /* Crypto engine init fail */
#define ERR_SEC_ENTROPY          (ERR_BASE_SEC + 2) /* RNG empty / weak entropy */
#define ERR_SEC_AUTH_FAIL        (ERR_BASE_SEC + 3) /* Credential validation failed */
#define ERR_SEC_SIGNATURE        (ERR_BASE_SEC + 4) /* Invalid signature */
#define ERR_SEC_KEY_INVALID      (ERR_BASE_SEC + 5) /* Bad key format */
#define ERR_SEC_KEY_MISSING      (ERR_BASE_SEC + 6) /* Key slot empty */
#define ERR_SEC_CERT_EXPIRED     (ERR_BASE_SEC + 7)
#define ERR_SEC_CERT_REVOKED     (ERR_BASE_SEC + 8)
#define ERR_SEC_HASH_MISMATCH    (ERR_BASE_SEC + 9)
#define ERR_SEC_UNTRUSTED        (ERR_BASE_SEC + 10) /* Root of trust verify failed */
#define ERR_SEC_LOCKED           (ERR_BASE_SEC + 11) /* Secure element locked */
#define ERR_SEC_ACCESS           (ERR_BASE_SEC + 12) /* Access control violation */
/* 173-191: Reserved for Security expansion */

/* ===========================================================================
 * 7. RESOURCES & DYNAMIC MODULES (192–223)
 *    Handles, Allocators, Dynamic Loading, OTA.
 * ===========================================================================*/
#define ERR_RES_GENERAL          (ERR_BASE_RES + 0)
#define ERR_RES_EXHAUSTED        (ERR_BASE_RES + 1) /* Pool/Slots empty */
#define ERR_RES_LEAK             (ERR_BASE_RES + 2) /* Leak detected */
#define ERR_RES_INVALID_ID       (ERR_BASE_RES + 3) /* Bad Handle/ID */
#define ERR_RES_OWNERSHIP        (ERR_BASE_RES + 4) /* Ownership violation */

#define ERR_MOD_HEADER           (ERR_BASE_RES + 5) /* ELF/Image header bad */
#define ERR_MOD_VERSION          (ERR_BASE_RES + 6) /* Version mismatch (OTA) */
#define ERR_MOD_DEPENDENCY       (ERR_BASE_RES + 7) /* Symbol/Lib missing */
#define ERR_MOD_COMPAT           (ERR_BASE_RES + 8) /* Arch/Platform mismatch */
#define ERR_MOD_SIZE             (ERR_BASE_RES + 9) /* Image too large */
#define ERR_MOD_RESTRICTED       (ERR_BASE_RES + 10)

/* 203-223: Reserved for Resource expansion */

/* ===========================================================================
 * 8. APPLICATION SPACE (224–254)
 *    Strictly for User/Business Logic.
 * ===========================================================================*/

/*
 * Applications should define their errors relative to this base:
 * #define ERR_MYAPP_BAD_CONFIG  (ERR_APP_BASE + 0)
 * ...
 * #define ERR_MYAPP_MAX         (ERR_APP_BASE + 30)
 */

/* ===========================================================================
 * RESERVED (255)
 * ===========================================================================*/
#define ERR_FINALIZED             255 /* Protothread Teardown Flag (Non-Error) */


/**
 * @brief Checks if an error code is critical (System or Hardware level)
 */
#define ERR_IS_CRITICAL(e)       (((e) >= ERR_BASE_SYS) && ((e) < ERR_BASE_STORE))

/**
 * @brief Convert error code to human-readable string (stored in Flash/RO).
 */
const char *error_to_string(uint8_t err);

#ifdef __cplusplus
}
#endif

#endif /* __ERRORS_H__ */
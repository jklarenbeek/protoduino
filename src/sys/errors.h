#ifndef PROTODUINO_ERRORS_H
#define PROTODUINO_ERRORS_H

#include "../lib/math/float32.h" // for very_fast_log2
#include <math.h>
#include <stdint.h>

/* =========================================================================
   RESERVED KERNEL CODES (0x00 - 0x03, 0xFF)
   These are defined by the kernel state machine and listed here for reference.
   ========================================================================= */
// #define ERR_SUCCESS      0x00  // Operation completed successfully
// #define ERR_YIELDED      0x01  // Process yielded control with value or waiting for event
// #define ERR_EXITED       0x02  // Process exited externally
// #define ERR_ENDED        0x03  // Process was terminated normally
// #define ERR_FINALIZED    0xFF  // System finalized/Shutdown

/* =========================================================================
   DOMAIN 0x0X: KERNEL LIFECYCLE & FLOW
   Desc: Fundamental execution flow issues, similar to errno EAGAIN/EINTR.
   ========================================================================= */
#define ERR_LIFECYCLE_GENERAL       0x04 // Generic lifecycle error
#define ERR_LIFECYCLE_TIMEOUT       0x05 // Operation timed out
#define ERR_LIFECYCLE_BUSY          0x06 // Resource is busy (EBUSY)
#define ERR_LIFECYCLE_CANCELLED     0x07 // Operation cancelled by user
#define ERR_LIFECYCLE_UNSUPPORTED   0x08 // Operation not supported
#define ERR_LIFECYCLE_NOT_IMPLEMENTED 0x09 // Functionality missing
#define ERR_LIFECYCLE_DEPRECATED    0x0A // Feature removed
#define ERR_LIFECYCLE_TRY_AGAIN     0x0B // Temporary failure, retry (EAGAIN)
#define ERR_LIFECYCLE_INTERRUPTED   0x0C // Signal/IRQ interrupted call
#define ERR_LIFECYCLE_PENDING       0x0D // Async operation pending
#define ERR_LIFECYCLE_STALLED       0x0E // Process stalled/hung
#define ERR_LIFECYCLE_CRITICAL      0x0F // Unrecoverable kernel panic

/* =========================================================================
   DOMAIN 0x1X: MEMORY & RESOURCES
   Desc: Heap, stack, and pointer management.
   ========================================================================= */
#define ERR_MEM_ALLOCATION          0x10 // General allocation failure
#define ERR_MEM_FOUNDATION          0x11 // [FOUNDATION] Integrity Check Fail
#define ERR_MEM_OUT_OF_MEMORY       0x12 // Heap exhausted (ENOMEM)
#define ERR_MEM_STACK_OVERFLOW      0x13 // Stack limit reached
#define ERR_MEM_NULL_POINTER        0x14 // Dereferenced NULL
#define ERR_MEM_ALIGNMENT           0x15 // Invalid alignment access
#define ERR_MEM_FRAGMENTATION       0x16 // Heap too fragmented to alloc
#define ERR_MEM_DOUBLE_FREE         0x17 // Freed memory twice
#define ERR_MEM_USE_AFTER_FREE      0x18 // Accessing freed memory
#define ERR_MEM_BUFFER_OVERFLOW     0x19 // Write past end of buffer
#define ERR_MEM_BUFFER_UNDERFLOW    0x1A // Read before start of buffer
#define ERR_MEM_LEAK_DETECTED       0x1B // Debug: Memory leak found
#define ERR_MEM_PROTECTED           0x1C // Write to read-only memory
#define ERR_MEM_INVALID_HANDLE      0x1D // Invalid memory handle
#define ERR_MEM_MAPPED_FAILED       0x1E // mmap failed
#define ERR_MEM_CORRUPTION          0x1F // Heap corruption detected

/* =========================================================================
   DOMAIN 0x2X: SECURITY & ACCESS CONTROL
   Desc: Auth, permissions, and cryptography.
   ========================================================================= */
#define ERR_SEC_GENERAL             0x20 // General security error
#define ERR_SEC_UNAUTHORIZED        0x21 // Authentication required (401)
#define ERR_SEC_FOUNDATION          0x22 // [FOUNDATION] Security Breach
#define ERR_SEC_FORBIDDEN           0x23 // Authenticated but denied (403)
#define ERR_SEC_ACCESS_DENIED       0x24 // Filesystem/Resource permission (EACCES)
#define ERR_SEC_INVALID_TOKEN       0x25 // Bad auth token/session
#define ERR_SEC_TOKEN_EXPIRED       0x26 // Token valid but old
#define ERR_SEC_SIGNATURE_BAD       0x27 // Digital signature mismatch
#define ERR_SEC_ENCRYPTION_FAILED   0x28 // Cipher operation failed
#define ERR_SEC_DECRYPTION_FAILED   0x29 // Decipher operation failed
#define ERR_SEC_WEAK_KEY            0x2A // Key strength insufficient
#define ERR_SEC_NO_ENTROPY          0x2B // RNG failed/empty
#define ERR_SEC_LOCKED              0x2C // Resource locked by another user
#define ERR_SEC_TAMPER_DETECTED     0x2D // Integrity check failed
#define ERR_SEC_BAD_CREDENTIALS     0x2E // Wrong username/password
#define ERR_SEC_PRIVILEGE_LOW       0x2F // Root/Admin required

/* =========================================================================
   DOMAIN 0x3X: FILESYSTEM & STORAGE
   Desc: Disk operations, paths, and mounting.
   ========================================================================= */
#define ERR_FS_GENERAL              0x30 // Generic IO error (EIO)
#define ERR_FS_NOT_FOUND            0x31 // File not found (ENOENT)
#define ERR_FS_PATH_INVALID         0x32 // Malformed path
#define ERR_FS_FOUNDATION           0x33 // [FOUNDATION] Disk Failure
#define ERR_FS_ALREADY_EXISTS       0x34 // Create failed, exists (EEXIST)
#define ERR_FS_IS_DIRECTORY         0x35 // Expected file, got dir (EISDIR)
#define ERR_FS_NOT_DIRECTORY        0x36 // Expected dir, got file (ENOTDIR)
#define ERR_FS_NOT_EMPTY            0x37 // Directory not empty
#define ERR_FS_TOO_MANY_OPEN        0x38 // File descriptor limit (EMFILE)
#define ERR_FS_DISK_FULL            0x39 // No space left (ENOSPC)
#define ERR_FS_READ_ONLY            0x3A // File system is read-only
#define ERR_FS_MOUNT_FAILED         0x3B // Mount operation failed
#define ERR_FS_UNMOUNT_FAILED       0x3C // Resource busy, cannot unmount
#define ERR_FS_MEDIA_REMOVED        0x3D // SD/USB pulled out
#define ERR_FS_BAD_DESCRIPTOR       0x3E // Bad FD (EBADF)
#define ERR_FS_SEEK_ERROR           0x3F // Lseek failed (ESPIPE)

/* =========================================================================
   DOMAIN 0x4X: HARDWARE & DRIVERS
   Desc: Low level IO, Buses (I2C/SPI), and GPIO.
   ========================================================================= */
#define ERR_HW_GENERAL              0x40 // Hardware fault
#define ERR_HW_NOT_FOUND            0x41 // Device ID mismatch
#define ERR_HW_NOT_READY            0x42 // Peripheral init incomplete
#define ERR_HW_POWER_LOW            0x43 // Brownout/Battery low
#define ERR_HW_FOUNDATION           0x44 // [FOUNDATION] Bus Lockup
#define ERR_HW_GPIO_FAILURE         0x45 // Pin config error
#define ERR_HW_ADC_ERROR            0x46 // Analog read fail
#define ERR_HW_PWM_ERROR            0x47 // Timer/PWM fail
#define ERR_HW_I2C_NACK             0x48 // I2C No Acknowledge
#define ERR_HW_I2C_ARB_LOST         0x49 // I2C Bus contention
#define ERR_HW_SPI_MODE_ERR         0x4A // SPI Polarity/Phase mismatch
#define ERR_HW_UART_FRAMING         0x4B // UART Baud/Frame error
#define ERR_HW_UART_PARITY          0x4C // UART Parity error
#define ERR_HW_DMA_ERROR            0x4D // Direct Memory Access fail
#define ERR_HW_OVERHEAT             0x4E // Thermal shutdown
#define ERR_HW_SHORT_CIRCUIT        0x4F // Overcurrent detected

/* =========================================================================
   DOMAIN 0x5X: NETWORK & TRANSPORT
   Desc: TCP/IP, Sockets, WiFi, Ethernet.
   [ROOT] 0x55 is the Primordial Root of Connectivity.
   ========================================================================= */
#define ERR_NET_GENERAL             0x50 // Generic Net error
#define ERR_NET_DISCONNECTED        0x51 // Physical link down
#define ERR_NET_HOST_UNREACHABLE    0x52 // Routing failure
#define ERR_NET_DNS_FAILED          0x53 // Name resolution failed
#define ERR_NET_CONNECTION_REFUSED  0x54 // Port closed (ECONNREFUSED)
#define ERR_NET_PRIMORDIAL          0x55 // [PRIMORDIAL] Connection Reset
#define ERR_NET_CONNECTION_ABORTED  0x56 // Software caused abort
#define ERR_NET_ADDRESS_IN_USE      0x57 // Bind failed (EADDRINUSE)
#define ERR_NET_SOCKET_FAILED       0x58 // Socket creation error
#define ERR_NET_BIND_FAILED         0x59 // Interface bind error
#define ERR_NET_LISTEN_FAILED       0x5A // Passive open error
#define ERR_NET_ACCEPT_FAILED       0x5B // Handshake failed
#define ERR_NET_PACKET_TOO_BIG      0x5C // MTU exceeded
#define ERR_NET_NO_ROUTE            0x5D // No gateway
#define ERR_NET_LATENCY_LIMIT       0x5E // Ping/Ack too slow
#define ERR_NET_INTERFACE_DOWN      0x5F // NIC disabled

/* =========================================================================
   DOMAIN 0x6X: PROTOCOLS & WEB
   Desc: Higher level layers (HTTP, MQTT, FTP).
   ========================================================================= */
#define ERR_PROTO_GENERAL           0x60 // Protocol violation
#define ERR_PROTO_BAD_REQUEST       0x61 // Malformed packet (HTTP 400)
#define ERR_PROTO_VERSION_MISMATCH  0x62 // Unsupported version
#define ERR_PROTO_METHOD_NOT_ALLOWED 0x63 // Wrong verb (HTTP 405)
#define ERR_PROTO_HEADER_TOO_LARGE  0x64 // Metadata overflow
#define ERR_PROTO_PAYLOAD_TOO_LARGE 0x65 // Body overflow (HTTP 413)
#define ERR_PROTO_FOUNDATION        0x66 // [FOUNDATION] Parse Failure
#define ERR_PROTO_UNSUPPORTED_TYPE  0x67 // Mime type error (HTTP 415)
#define ERR_PROTO_MISSING_HEADER    0x68 // Required field missing
#define ERR_PROTO_RATE_LIMITED      0x69 // Too many requests (HTTP 429)
#define ERR_PROTO_SERVER_ERROR      0x6A // Remote side failed (HTTP 500)
#define ERR_PROTO_GATEWAY_TIMEOUT   0x6B // Upstream timeout (HTTP 504)
#define ERR_PROTO_HANDSHAKE_FAIL    0x6C // TLS/SSL Handshake
#define ERR_PROTO_CERT_INVALID      0x6D // TLS Cert rejected
#define ERR_PROTO_CRC_MISMATCH      0x6E // Checksum failure
#define ERR_PROTO_UNKNOWN_CMD       0x6F // Command ID unknown

/* =========================================================================
   DOMAIN 0x7X: VALIDATION & DATA
   Desc: Input sanitation, type checking.
   ========================================================================= */
#define ERR_VAL_GENERAL             0x70 // Validation failed
#define ERR_VAL_INVALID_ARG         0x71 // Argument invalid (EINVAL)
#define ERR_VAL_OUT_OF_RANGE        0x72 // Value bounds exceeded
#define ERR_VAL_NULL_ARG            0x73 // Unexpected Null argument
#define ERR_VAL_EMPTY               0x74 // Container empty
#define ERR_VAL_TOO_SHORT           0x75 // String/Data too short
#define ERR_VAL_TOO_LONG            0x76 // String/Data too long
#define ERR_VAL_FOUNDATION          0x77 // [FOUNDATION] Integrity Lost
#define ERR_VAL_PATTERN_MISMATCH    0x78 // Regex/Format mismatch
#define ERR_VAL_INVALID_EMAIL       0x79 // Specific format error
#define ERR_VAL_INVALID_DATE        0x7A // Date/Time logic error
#define ERR_VAL_INVALID_IP          0x7B // IP string malformed
#define ERR_VAL_INVALID_JSON        0x7C // JSON syntax error
#define ERR_VAL_INVALID_XML         0x7D // XML/HTML syntax error
#define ERR_VAL_INVALID_BASE64      0x7E // Decode error
#define ERR_VAL_INVALID_UTF8        0x7F // Encoding error

/* =========================================================================
   DOMAIN 0x8X: STREAMING & IO
   Desc: Pipes, Streams, Buffer manipulation.
   ========================================================================= */
#define ERR_IO_GENERAL              0x80 // General IO failure
#define ERR_IO_EOF                  0x81 // End Of File hit
#define ERR_IO_CLOSED               0x82 // Stream closed
#define ERR_IO_NOT_OPEN             0x83 // Stream not open
#define ERR_IO_WOULD_BLOCK          0x84 // Non-blocking wait
#define ERR_IO_SYNC_FAILED          0x85 // fsync/flush failed
#define ERR_IO_PIPE_BROKEN          0x86 // Writer disconnected (EPIPE)
#define ERR_IO_READ_ERROR           0x87 // Physical read error
#define ERR_IO_FOUNDATION           0x88 // [FOUNDATION] Stream Corruption
#define ERR_IO_WRITE_ERROR          0x89 // Physical write error
#define ERR_IO_SKIP_ERROR           0x8A // Seek/Skip failed
#define ERR_IO_MARK_INVALID         0x8B // Reset to mark failed
#define ERR_IO_COMPRESSION_ERR      0x8C // Gzip/Zlib fail
#define ERR_IO_DECOMPRESSION_ERR    0x8D // Unzip fail
#define ERR_IO_CHECKSUM_ERR         0x8E // Stream hash mismatch
#define ERR_IO_TRUNCATED            0x8F // Unexpected end of data

/* =========================================================================
   DOMAIN 0x9X: PERIPHERAL & HMI
   Desc: Display, Audio, Sensors, Human Interface.
   ========================================================================= */
#define ERR_PERIPH_GENERAL          0x90 // Device error
#define ERR_PERIPH_DISPLAY_INIT     0x91 // LCD/OLED fail
#define ERR_PERIPH_TOUCH_CALIB      0x92 // Touchscreen uncalibrated
#define ERR_PERIPH_AUDIO_MUTE       0x93 // Codec muted/fail
#define ERR_PERIPH_SENSOR_NO_DATA   0x94 // Sensor ready but empty
#define ERR_PERIPH_MOTOR_STALL      0x95 // Stepper/Servo stalled
#define ERR_PERIPH_BATTERY_CRIT     0x96 // Imminent shutdown
#define ERR_PERIPH_KEYBOARD_ERR     0x97 // Matrix scan fail
#define ERR_PERIPH_SD_NO_CARD       0x98 // Slot empty
#define ERR_PERIPH_FOUNDATION       0x99 // [FOUNDATION] Device Halted
#define ERR_PERIPH_CAMERA_INIT      0x9A // Sensor init fail
#define ERR_PERIPH_GPS_NO_FIX       0x9B // No satellite lock
#define ERR_PERIPH_RADIO_JAMMED     0x9C // RF interference
#define ERR_PERIPH_LED_FAIL         0x9D // Indicator error
#define ERR_PERIPH_BUTTON_STUCK     0x9E // Physical button stuck
#define ERR_PERIPH_HAPTIC_FAIL      0x9F // Vibration motor fail

/* =========================================================================
   DOMAIN 0xAX: IPC & MESSAGING
   Desc: Inter-Process Communication, Queues, PubSub.
   [ROOT] 0xAA is the Primordial Inverse of Connectivity (Internal Link).
   ========================================================================= */
#define ERR_IPC_GENERAL             0xA0 // Generic IPC error
#define ERR_IPC_QUEUE_FULL          0xA1 // Message dropped
#define ERR_IPC_QUEUE_EMPTY         0xA2 // No messages
#define ERR_IPC_MUTEX_LOCKED        0xA3 // Lock acquisition failed
#define ERR_IPC_SEMAPHORE_LIMIT     0xA4 // Count exceeded
#define ERR_IPC_SHARED_MEM_ERR      0xA5 // Shmem attach fail
#define ERR_IPC_SUBSCRIBER_ERR      0xA6 // PubSub delivery fail
#define ERR_IPC_TOPIC_INVALID       0xA7 // MQTT/Bus topic bad
#define ERR_IPC_MESSAGE_CORRUPT     0xA8 // Payload integrity
#define ERR_IPC_NO_RECEIVERS        0xA9 // Dead letter
#define ERR_IPC_PRIMORDIAL          0xAA // [PRIMORDIAL] Link Broken
#define ERR_IPC_RPC_FAILED          0xAB // Remote Procedure Call fail
#define ERR_IPC_DESERIALIZE         0xAC // Struct packing error
#define ERR_IPC_SERIALIZE           0xAD // Struct unpacking error
#define ERR_IPC_CHANNEL_CLOSED      0xAE // Comm channel dead
#define ERR_IPC_PRIORITY_INV        0xAF // Priority inversion

/* =========================================================================
   DOMAIN 0xBX: TIME & CHRONOLOGY
   Desc: RTC, Timers, Scheduling logic.
   ========================================================================= */
#define ERR_TIME_GENERAL            0xB0 // Time error
#define ERR_TIME_RTC_STOPPED        0xB1 // Oscillator dead
#define ERR_TIME_NOT_SET            0xB2 // Time is 1970/1900
#define ERR_TIME_SYNCHRONIZATION    0xB3 // NTP/PTP drift too high
#define ERR_TIME_FUTURE_TIMESTAMP   0xB4 // Event from future
#define ERR_TIME_PAST_TIMESTAMP     0xB5 // Event too old
#define ERR_TIME_INVALID_TZ         0xB6 // Timezone bad
#define ERR_TIME_LEAP_SECOND        0xB7 // Calculation error
#define ERR_TIME_TIMER_EXPIRED      0xB8 // Watchdog/Soft timer
#define ERR_TIME_FREQUENCY_ERR      0xB9 // Clock speed wrong
#define ERR_TIME_SLEEP_FAIL         0xBA // Low power mode fail
#define ERR_TIME_FOUNDATION         0xBB // [FOUNDATION] Chrono-Collapse
#define ERR_TIME_WAKEUP_FAIL        0xBC // Resume fail
#define ERR_TIME_CALENDAR_ERR       0xBD // Day/Month math fail
#define ERR_TIME_TIMEOUT_SCALE      0xBE // Overflow in ms calc
#define ERR_TIME_JITTER_EXCESS      0xBF // Real-time violation

/* =========================================================================
   DOMAIN 0xCX: MATH & LOGIC
   Desc: Calculations, Floating point, Logic gates.
   ========================================================================= */
#define ERR_MATH_GENERAL            0xC0 // Math error (EDOM)
#define ERR_MATH_DIV_BY_ZERO        0xC1 // Integer/Float div 0
#define ERR_MATH_OVERFLOW           0xC2 // Int overflow
#define ERR_MATH_UNDERFLOW          0xC3 // Int underflow
#define ERR_MATH_NAN                0xC4 // Not a Number
#define ERR_MATH_INFINITY           0xC5 // Positive/Negative Inf
#define ERR_MATH_DOMAIN             0xC6 // Input outside domain
#define ERR_MATH_RANGE              0xC7 // Result outside range
#define ERR_MATH_PRECISION          0xC8 // Accuracy lost
#define ERR_MATH_SINGULARITY        0xC9 // Matrix non-invertible
#define ERR_MATH_CONVERGENCE        0xCA // Algorithm didn't settle
#define ERR_MATH_DIMENSION          0xCB // Vector size mismatch
#define ERR_MATH_FOUNDATION         0xCC // [FOUNDATION] Logic Contradiction
#define ERR_MATH_SUBSCRIPT          0xCD // Array index logic err
#define ERR_MATH_CAST_INVALID       0xCE // Type conversion loss
#define ERR_MATH_UNINITIALIZED      0xCF // Variable used raw

/* =========================================================================
   DOMAIN 0xDX: DIAGNOSTICS & SYSTEM
   Desc: Debugging, Trace, Assertions.
   ========================================================================= */
#define ERR_DBG_GENERAL             0xD0 // Unknown error
#define ERR_DBG_ASSERTION           0xD1 // Assert(false)
#define ERR_DBG_UNREACHABLE         0xD2 // Code flow error
#define ERR_DBG_NOT_IMPLEMENTED     0xD3 // TODO marker
#define ERR_DBG_STUB                0xD4 // Stub function called
#define ERR_DBG_TEST_FAILED         0xD5 // Unit test fail
#define ERR_DBG_TRACE_FULL          0xD6 // Log buffer wrap
#define ERR_DBG_HOOK_FAILED         0xD7 // Debug hook error
#define ERR_DBG_SYMBOL_MISSING      0xD8 // Linker error (dyn)
#define ERR_DBG_CHECKSUM_ROM        0xD9 // Firmware corrupted
#define ERR_DBG_WATCHDOG            0xDA // WDT reset occurred
#define ERR_DBG_BROWNOUT            0xDB // BOD reset occurred
#define ERR_DBG_POWER_ON            0xDC // Cold boot status
#define ERR_DBG_FOUNDATION          0xDD // [FOUNDATION] System Panic
#define ERR_DBG_STACK_DUMP          0xDE // Stack trace generated
#define ERR_DBG_CORE_DUMP           0xDF // Memory dumped

/* =========================================================================
   DOMAINS 0xE0 - 0xFE: APPLICATION DEFINED
   Desc: Reserved for User Logic. 31 Errors.
   0xFF is ERR_FINALIZED (Reserved).
   ========================================================================= */

// Domain E (Application Space 1)
#define ERR_APP_E0                  0xE0
#define ERR_APP_E1                  0xE1
#define ERR_APP_E2                  0xE2
#define ERR_APP_E3                  0xE3
#define ERR_APP_E4                  0xE4
#define ERR_APP_E5                  0xE5
#define ERR_APP_E6                  0xE6
#define ERR_APP_E7                  0xE7
#define ERR_APP_E8                  0xE8
#define ERR_APP_E9                  0xE9
#define ERR_APP_EA                  0xEA
#define ERR_APP_EB                  0xEB
#define ERR_APP_EC                  0xEC
#define ERR_APP_ED                  0xED
#define ERR_APP_FOUNDATION          0xEE // [FOUNDATION] App Critical
#define ERR_APP_EF                  0xEF

// Domain F (Application Space 2)
#define ERR_APP_F0                  0xF0
#define ERR_APP_F1                  0xF1
#define ERR_APP_F2                  0xF2
#define ERR_APP_F3                  0xF3
#define ERR_APP_F4                  0xF4
#define ERR_APP_F5                  0xF5
#define ERR_APP_F6                  0xF6
#define ERR_APP_F7                  0xF7
#define ERR_APP_F8                  0xF8
#define ERR_APP_F9                  0xF9
#define ERR_APP_FA                  0xFA
#define ERR_APP_FB                  0xFB
#define ERR_APP_FC                  0xFC
#define ERR_APP_FD                  0xFD
#define ERR_APP_FE                  0xFE

// ERR_FINALIZED is 0xFF (Reserved)

/* =========================================================================
   HELPER MACROS FOR ANALYSIS
   ========================================================================= */

// Reserved values used by the protoduino finite state machine
#define ERR_IS_RESERVED(err) \
    ((err) == 0 || (err) == 1 || (err) == 2 || (err) == 3 || (err) == 255)

// Terminal roots
#define ERR_ROOT_INIT      0x00  // 00000000 — genesis, void
#define ERR_ROOT_RUN       0xFF  // 11111111 — saturation, terminal

// Oscillatory roots
#define ERR_ROOT_BEFORE    0x55  // 01010101 — oscillation origin
#define ERR_ROOT_AFTER     0xAA  // 10101010 — oscillation inversion

// Terminal roots: pure endpoints without internal structure
#define ERR_IS_ABSTRACT(err) \
    ((err) == ERR_ROOT_INIT || (err) == ERR_ROOT_RUN)

// Oscillatory roots: structured alternating primitives
#define ERR_IS_MOVEMENT(err) \
    ((err) == ERR_ROOT_BEFORE || (err) == ERR_ROOT_AFTER)

// The 4 primordial error classes of which every other error are its children.
// 0 (00000000), 255 (11111111), 85 (01010101), 170 (10101010)
#define ERR_IS_ROOT(err) \
    (ERR_IS_ABSTRACT(err) || ERR_IS_MOVEMENT(err))

// Direct 12 descendants of the primordial error classes
#define ERR_IS_DOMAIN(err) (!ERR_IS_ROOT(err) && (ERR_IS_ROOT(err_op_center(err))))

// The direct descendants of the DOMAIN error codes
#define ERR_IS_ATOMIC(err) (!(ERR_IS_ROOT(err) || ERR_IS_DOMAIN(err)))

// 8-bit Plane: Split into two 4-bit nibbles
#define ERR_NIBBLE_LEFT(err) (((err) >> 4) & 0x0F)
#define ERR_NIBBLE_RIGHT(err) ((err) & 0x0F)

// Checks if the Left Nibble is equal to the Inverse of the Right Nibble
#define ERR_IS_SHADOW(err) \
    (ERR_NIBBLE_LEFT(err) == (~ERR_NIBBLE_RIGHT(err) & 0xF))

// The first half of a cluster is 4bits and must equal the other 4bits
// e.g. 0x00, 0x11, 0x22 ... 0xFF
#define ERR_IS_TWIN(err) (ERR_NIBBLE_LEFT(err) == ERR_NIBBLE_RIGHT(err))

// Balanced means half of the bits are set (4 out of 8)
#define ERR_IS_BALANCED(err) (err_op_one_count(err) == 4)
#define ERR_IS_UNBALANCED(err) (err_op_one_count(err) != 4)

/**
 * Symmetry Classes (8-bit Matrix of 16x16 byte error codes)
 */

// A balanced diagonal from right top to left bottom
// results in error codes 0x55 (85) and 0xAA (170) => ERR_IS_MOVEMENT(err)
#define ERR_IS_BALANCED_ROOT(err) (err_op_center(err) == err_op_inverse(err))

// A balanced diagonal from right top to left bottom
#define ERR_IS_BALANCED_SHADOW(err) (ERR_IS_BALANCED(err) && ERR_IS_SHADOW(err))

// A balanced circle in the middle of the 16x16 matrix
#define ERR_IS_BALANCED_EDGE(err) (!ERR_IS_BALANCED_ROOT(err) && ERR_IS_BALANCED(err) && !ERR_IS_SHADOW(err))

// An unbalanced diagonal from left top to right bottom
// Only 0x00 and 0xFF
#define ERR_IS_UNBALANCED_ROOT(err) (ERR_IS_ABSTRACT(err) && ERR_IS_TWIN(err))

// Diagonal from left top to right bottom (twin but not 00 or FF)
#define ERR_IS_UNBALANCED_TWIN(err) (!ERR_IS_ABSTRACT(err) && ERR_IS_TWIN(err))

// Extreme imbalance (1 or 7 one's). Outer edge cirlce of the 16x16 matrix.
#define ERR_IS_UNBALANCED_EDGE(err) (err_op_one_count(err) == 1 || err_op_one_count(err) == 7)

// Everything else unbalanced
#define ERR_IS_UNBALANCED_OTHER(err) (!ERR_IS_BALANCED(err) && !ERR_IS_TWIN(err) && !ERR_IS_UNBALANCED_EDGE(err))

CC_ALWAYS_INLINE uint8_t err_op_inverse(uint8_t err) {
    // Inverse the cluster (EEEE DDDD)
    // In 8-bit, we invert the whole byte.
    return ~err;
}

CC_ALWAYS_INLINE uint8_t err_op_reverse(uint8_t err) {
    // Reverse the cluster bits: 7-0
    // Standard SWAR bit reversal for 8 bits
    err = (err & 0xF0) >> 4 | (err & 0x0F) << 4;
    err = (err & 0xCC) >> 2 | (err & 0x33) << 2;
    err = (err & 0xAA) >> 1 | (err & 0x55) << 1;
    return err;
}

CC_ALWAYS_INLINE uint8_t err_op_opposite(uint8_t err) {
    // Swap the nibbles (Left <-> Right)
    // (EEEE DDDD) -> (DDDD EEEE)
    return (ERR_NIBBLE_RIGHT(err) << 4) | ERR_NIBBLE_LEFT(err);
}

CC_ALWAYS_INLINE uint8_t err_op_center(uint8_t err) {
    // Nuclear transformation for 8 bits.
    // Logic: Sliding window of size 4.
    // Right Nibble becomes bits 1,2,3,4 (shifted to 0,1,2,3)
    // Left Nibble becomes bits 3,4,5,6 (shifted to 4,5,6,7)
    // ((err >> 1) & 0x0F) extracts 1,2,3,4 -> 0,1,2,3
    // ((err << 1) & 0xF0) extracts 3,4,5,6 -> 4,5,6,7
    return ((err << 1) & 0xF0) | ((err >> 1) & 0x0F);
}

CC_ALWAYS_INLINE uint8_t err_op_root(uint8_t err) {
    // Convergence tree iteration
    // Typically depth increases with bit width, 8-bit takes 4 steps max.
    while (!ERR_IS_ROOT(err)) err = err_op_center(err);
    return err;
}

/**
 * Get the depth in the nuclear convergence tree.
 * Returns how many center() operations needed to reach the root.
 */
CC_ALWAYS_INLINE uint8_t err_op_depth(uint8_t err) {
  uint8_t d = 0;
  while (!ERR_IS_ROOT(err)) {
      err = err_op_center(err);
      d++;
  }
  return d;
}

static inline uint8_t err_op_one_count(uint8_t err) {
    // Hamming weight for 8 bits
    // This is a SWAR algorithm (SIMD Within A Register) for 32-bit capable CPUs,
    // adapted for 8-bit flow, or simply naive count if compiled without __builtin_popcount
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(err);
#else
    err = (err & 0x55) + ((err >> 1) & 0x55);
    err = (err & 0x33) + ((err >> 2) & 0x33);
    return (err + (err >> 4)) & 0x0F;
#endif
}

static inline uint8_t err_op_distance(uint8_t left, uint8_t right) {
    return err_op_one_count(left ^ right);
}

/*
 * Calculate entropy (Shannon information) of nibbles.
 * Measures balance/chaos: 0 = pure, 1 = perfectly balanced.
 *
 * Formula based on N=8 lines.
 */
CC_ALWAYS_INLINE float32_t err_op_entropy(uint8_t err) {
  uint8_t ones = err_op_one_count(err);
  uint8_t zeros = 8 - ones;

  if (zeros == 0 || ones == 0) return 0.0f;

  float32_t p_ones = ones / 8.0f;
  float32_t p_zeros = zeros / 8.0f;

  return -(p_ones * very_fast_log2(p_ones) + p_zeros * very_fast_log2(p_zeros));
}

/**
 * Calculate balance ratio (one/total lines).
 * Returns value from 0.0 to 1.0.
 */
CC_ALWAYS_INLINE float32_t err_op_balance(uint8_t err) {
  return err_op_one_count(err) / 8.0f;
}

// Identifies which transformation relates two errors
typedef enum {
    ERR_RELATION_DEFAULT = 0,
    ERR_RELATION_CENTER,
    ERR_RELATION_OPPOSITE,
    ERR_RELATION_REVERSED_EQUALS,
    ERR_RELATION_REVERSED,
    ERR_RELATION_INVERTED_EQUALS,
    ERR_RELATION_INVERTED,
} err_relation_t;

err_relation_t err_op_relation(uint8_t left, uint8_t right) {
  if (err_op_center(left) == right)
    return ERR_RELATION_CENTER;
  else if (err_op_opposite(left) == right)
    return ERR_RELATION_OPPOSITE;
  else if (err_op_reverse(left) == right) {
    if (ERR_IS_TWIN(left))
      return ERR_RELATION_REVERSED_EQUALS;
    else
      return ERR_RELATION_REVERSED;
  }
  else if (err_op_inverse(left) == right) {
    if (ERR_IS_TWIN(left))
      return ERR_RELATION_INVERTED_EQUALS;
    else
      return ERR_RELATION_INVERTED;
  }
  else return ERR_RELATION_DEFAULT;
}

#endif // PROTODUINO_ERRORS_H
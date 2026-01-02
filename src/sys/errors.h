#ifndef ERRORS_H_
#define ERRORS_H_

/*
 * ONTOLOGICAL 8-BIT ERROR TAXONOMY v1.0
 * -----------------------------------------------------------------------------
 * Geometry:  16x16 Matrix (0..255) mapped to 4 Attractor Basins.
 * Logic:     Convergence via ((Child << 1) & 0xF0) | ((Child >>> 1) & 0x0F)
 * -----------------------------------------------------------------------------
 */

#include <math.h>
#include <stdint.h>

// =============================================================================
// RESERVED KERNEL CODES (0x00 - 0x03, 0xFF)
// These are defined by the kernel state machine and listed here for reference.
// =============================================================================
#define ERR_SUCCESS      0x00  // Operation completed successfully
#define ERR_YIELDING     0x01  // Process yielded control with value or waiting for event
#define ERR_EXITING      0x02  // Process exited externally
#define ERR_ENDING       0x03  // Process was terminated normally
#define ERR_FINALIZED    0xFF  // System finalized/Shutdown absolute stop!

// =============================================================================
// THE 4 PRIMORDIAL ERROR CLASSES
// =============================================================================

// Terminal roots
#define ERR_ROOT_INIT     0x00 /* 0 dec, 00000000 — genesis, void  */
#define ERR_ROOT_RUN      0xFF /* 255 dec, 11111111 — saturation, terminal */

// Oscillatory roots
#define ERR_ROOT_BEFORE   0x55 /* 85 dec, 01010101 — oscillation origin */
#define ERR_ROOT_AFTER    0xAA /* 170 dec, 10101010 — oscillation inversion */

// =============================================================================
// QUADRANT 1: INIT / LIFECYCLE / SYSTEM STATE
// ROOT: 0x00 (The Void / Success / Origins)
// =============================================================================

// ROOT: 0x00
#define ERR_OK                          ERR_SUCCESS // UNBALANCED_ROOT [Init] Success / No Error

// DOMAIN 1: SYSTEM STATE & PERMISSIONS (Root 0x00 -> Center 0x00)
#define ERR_SYS_STATE                   ERR_YIELDING // UNBALANCED_EDGE [Domain] General System State
// -- Section 1.1: Access Control
#define ERR_SYS_ACCESS                  ERR_EXITING // UNBALANCED_EDGE [Section] Permission Denied
#define ERR_ACCESS_READ                 0x04 // UNBALANCED_EDGE [Leaf] Read Access Forbidden
#define ERR_ACCESS_WRITE                0x05 // UNBALANCED_OTHER [Leaf] Write Access Forbidden
#define ERR_ACCESS_EXEC                 0x84 // UNBALANCED_OTHER [Leaf] Execute Access Forbidden
#define ERR_ACCESS_OWNER                0x85 // UNBALANCED_OTHER [Leaf] Ownership Mismatch
// -- Section 1.2: Handles & Descriptors
#define ERR_SYS_HANDLE                  ERR_ENDING // UNBALANCED_OTHER [Section] Invalid Handle/Descriptor
#define ERR_HANDLE_NULL                 0x06 // UNBALANCED_OTHER [Leaf] Null Pointer/Handle
#define ERR_HANDLE_CLOSED               0x07 // UNBALANCED_OTHER [Leaf] Handle Already Closed
#define ERR_HANDLE_TYPE                 0x86 // UNBALANCED_OTHER [Leaf] Handle Type Mismatch
#define ERR_HANDLE_SHADOW               0x87 // BALANCED_SHADOW  [Leaf] Ghost Handle / Stale Ref
// -- Section 1.3: Processes & Threads
#define ERR_SYS_PROC                    0x82 // UNBALANCED_OTHER [Section] Process/Thread Error
#define ERR_PROC_CANCELLED              0x44 // UNBALANCED_TWIN  [Leaf] Operation Cancelled
#define ERR_PROC_ZOMBIE                 0x45 // UNBALANCED_OTHER [Leaf] Process is Zombie
#define ERR_PROC_ORPHAN                 0xC4 // UNBALANCED_OTHER [Leaf] Parent Process Lost
#define ERR_PROC_INVAL                  0xC5 // BALANCED_EDGE    [Leaf] Invalid System Argument
// -- Section 1.4: Scheduling
#define ERR_SYS_SCHED                   0x83 // UNBALANCED_OTHER [Section] Scheduler/Priority
#define ERR_SCHED_YIELD                 0x46 // UNBALANCED_OTHER [Leaf] Yield Failed
#define ERR_SCHED_PRIORITY              0x47 // BALANCED_EDGE    [Leaf] Priority Inversion
#define ERR_SCHED_QUANTUM               0xC6 // BALANCED_EDGE    [Leaf] Time Slice Expired
#define ERR_SCHED_STARVE                0xC7 // UNBALANCED_OTHER [Leaf] Thread Starvation

// DOMAIN 2: MEMORY & RESOURCES (Root 0x00 -> Center 0x00 -> High Bit)
#define ERR_MEM_DOM                     0x80 // UNBALANCED_EDGE [Domain] Memory Subsystem
// -- Section 2.1: Allocation (Heap)
#define ERR_MEM_ALLOC                   0x40 // UNBALANCED_EDGE [Section] Allocation Failure
#define ERR_HEAP_OOM                    0x20 // UNBALANCED_EDGE [Leaf] Out of Memory
#define ERR_HEAP_FRAGMENT               0x21 // UNBALANCED_OTHER [Leaf] Heap Fragmented
#define ERR_HEAP_CORRUPT                0xA0 // UNBALANCED_OTHER [Leaf] Heap Metadata Corrupt
#define ERR_HEAP_FREE                   0xA1 // UNBALANCED_OTHER [Leaf] Double Free
// -- Section 2.2: Stack & Bounds
#define ERR_MEM_BOUNDS                  0x41 // UNBALANCED_OTHER [Section] Boundary Violation
#define ERR_STACK_OVER                  0x22 // UNBALANCED_TWIN  [Leaf] Stack Overflow
#define ERR_STACK_UNDER                 0x23 // UNBALANCED_OTHER [Leaf] Stack Underflow
#define ERR_BOUNDS_LOWER                0xA2 // UNBALANCED_OTHER [Leaf] Lower Bound Violation
#define ERR_BOUNDS_UPPER                0xA3 // BALANCED_EDGE    [Leaf] Upper Bound Violation
// -- Section 2.3: Paging & Virtual Mem
#define ERR_MEM_VIRT                    0xC0 // UNBALANCED_OTHER [Section] Virtual Memory
#define ERR_PAGE_FAULT                  0x60 // UNBALANCED_OTHER [Leaf] Page Fault
#define ERR_PAGE_NOT_PRESENT            0x61 // UNBALANCED_OTHER [Leaf] Page Not Present
#define ERR_PAGE_PROTECT                0xE0 // UNBALANCED_OTHER [Leaf] Page Protection Violation
#define ERR_PAGE_SHADOW                 0xE1 // BALANCED_SHADOW  [Leaf] Shadow Page Error
// -- Section 2.4: Alignment & Mapping
#define ERR_MEM_MAP                     0xC1 // UNBALANCED_OTHER [Section] Mapping/Alignment
#define ERR_MAP_FAILED                  0x62 // UNBALANCED_OTHER [Leaf] mmap Failed
#define ERR_ALIGN_ADDR                  0x63 // BALANCED_EDGE    [Leaf] Address Misalignment
#define ERR_ALIGN_BUS                   0xE2 // BALANCED_EDGE    [Leaf] Bus Alignment Error
#define ERR_ALIGN_SIZE                  0xE3 // UNBALANCED_OTHER [Leaf] Size Misalignment

// DOMAIN 3: LIFECYCLE & REFLECTION (Root 0x00 -> Center 0x00 -> Mirror)
#define ERR_LIFE_DOM                    0x81 // UNBALANCED_OTHER [Domain] Object Lifecycle
// -- Section 3.1: Initialization
#define ERR_LIFE_INIT                   0x42 // UNBALANCED_OTHER [Section] Initialization
#define ERR_INIT_TIMEOUT                0x24 // UNBALANCED_OTHER [Leaf] Init Timeout
#define ERR_INIT_DEPENDENCY             0x25 // UNBALANCED_OTHER [Leaf] Dependency Missing
#define ERR_INIT_PREMATURE              0xA4 // UNBALANCED_OTHER [Leaf] Premature Initialization
#define ERR_NOT_SUPPORTED               0xA5 // BALANCED_SHADOW  [Leaf] Not Supported
// -- Section 3.2: States
#define ERR_LIFE_STATE                  0x43 // UNBALANCED_OTHER [Section] Invalid State Transition
#define ERR_STATE_UNK                   0x26 // UNBALANCED_OTHER [Leaf] Unknown State
#define ERR_STATE_BALANCED              0x27 // BALANCED_EDGE    [Leaf] Balanced State Violation
#define ERR_STATE_LOCKED                0xA6 // BALANCED_EDGE    [Leaf] Object Locked
#define ERR_STATE_FROZEN                0xA7 // UNBALANCED_OTHER [Leaf] Object Frozen
// -- Section 3.3: Reference Counting
#define ERR_LIFE_REF                    0xC2 // UNBALANCED_OTHER [Section] Reference Counting
#define ERR_REF_ZERO                    0x64 // UNBALANCED_OTHER [Leaf] Ref Count Zero
#define ERR_REF_MAX                     0x65 // BALANCED_EDGE    [Leaf] Ref Count Max
#define ERR_REF_LEAK                    0xE4 // BALANCED_EDGE    [Leaf] Potential Leak
#define ERR_REF_DANGLING                0xE5 // UNBALANCED_OTHER [Leaf] Dangling Pointer
// -- Section 3.4: Garbage Collection
#define ERR_LIFE_GC                     0xC3 // BALANCED_SHADOW  [Section] GC / Cleanup
#define ERR_GC_ROOT                     0x66 // BALANCED_EDGE    [Leaf] Root Not Found
#define ERR_GC_SWEEP                    0x67 // UNBALANCED_OTHER [Leaf] Sweep Failure
#define ERR_GC_COMPACT                  0xE6 // UNBALANCED_OTHER [Leaf] Compaction Failure
#define ERR_GC_MIRROR                   0xE7 // UNBALANCED_OTHER [Leaf] Mirror Object Error

// =============================================================================
// QUADRANT 2: BEFORE / EXTERNAL I/O / HARDWARE
// ROOT: 0x55 (The Oscillation / The Wave / The Wire)
// =============================================================================

// ROOT: 0x55
#define ERR_IO_BUSY                     0x55 // BALANCED_ROOT    [Root] Resource Busy / Retry

// DOMAIN 1: NETWORK & CONNECTIVITY (Root 0x55 -> Center 0x85 (which is Q2 logic))
#define ERR_NET_DOM                     0x2A // UNBALANCED_OTHER [Domain] Networking
// -- Section 1.1: Sockets
#define ERR_NET_SOCK                    0x14 // UNBALANCED_OTHER [Section] Socket Error
#define ERR_SOCK_BIND                   0x08 // UNBALANCED_EDGE  [Leaf] Bind Failed
#define ERR_SOCK_LISTEN                 0x09 // UNBALANCED_OTHER [Leaf] Listen Failed
#define ERR_SOCK_ACCEPT                 0x88 // UNBALANCED_TWIN  [Leaf] Accept Failed
#define ERR_SOCK_CONNECT                0x89 // UNBALANCED_OTHER [Leaf] Connect Failed
// -- Section 1.2: DNS & Resolution
#define ERR_NET_DNS                     0x15 // UNBALANCED_OTHER [Section] DNS/Naming
#define ERR_DNS_NXDOMAIN                0x0A // UNBALANCED_OTHER [Leaf] Domain Not Found
#define ERR_DNS_TIMEOUT                 0x0B // UNBALANCED_OTHER [Leaf] Resolution Timeout
#define ERR_DNS_SERVFAIL                0x8A // UNBALANCED_OTHER [Leaf] Server Failure
#define ERR_DNS_BALANCED                0x8B // BALANCED_EDGE    [Leaf] Load Balancer Error
// -- Section 1.3: Transport (TCP/UDP)
#define ERR_NET_TRANS                   0x94 // UNBALANCED_OTHER [Section] Transport Layer
#define ERR_TRANS_RESET                   0x48 // UNBALANCED_OTHER [Leaf] Connection Reset
#define ERR_TRANS_CLOSED                  0x49 // UNBALANCED_OTHER [Leaf] Connection Closed
#define ERR_TRANS_DROP                    0xC8 // UNBALANCED_OTHER [Leaf] Packet Dropped
#define ERR_TRANS_CONGEST                 0xC9 // BALANCED_EDGE    [Leaf] Congestion
// -- Section 1.4: Protocol (HTTP/MQTT)
#define ERR_NET_PROTO                   0x95 // BALANCED_EDGE    [Section] Application Protocol
#define ERR_PROTO_BAD                     0x4A // UNBALANCED_OTHER [Leaf] Bad Request
#define ERR_PROTO_PROXY                   0x4B // BALANCED_SHADOW  [Leaf] Proxy Error
#define ERR_PROTO_SUB                     0xCA // BALANCED_EDGE    [Leaf] Subscription Failed
#define ERR_PROTO_PUB                     0xCB // UNBALANCED_OTHER [Leaf] Publish Failed

// DOMAIN 2: SIGNALS & TIMING (Root 0x55 -> Balanced Edge)
#define ERR_TIME_DOM                    0x2B // BALANCED_EDGE    [Domain] Timing & Signals
// -- Section 2.1: Interrupts
#define ERR_TIME_IRQ                    0x16 // UNBALANCED_OTHER [Section] Interrupts
#define ERR_IRQ_MASKED                  0x0C // UNBALANCED_OTHER [Leaf] IRQ Masked
#define ERR_IRQ_INTR                    0x0D // UNBALANCED_OTHER [Leaf] Interrupted
#define ERR_IRQ_NESTED                  0x8C // UNBALANCED_OTHER [Leaf] Nested Depth Exceeded
#define ERR_IRQ_BALANCE                 0x8D // BALANCED_EDGE    [Leaf] IRQ Load Balance
// -- Section 2.2: Clocks & Timers
#define ERR_TIME_CLOCK                  0x17 // BALANCED_EDGE    [Section] Clocks
#define ERR_CLK_SKEW                    0x0E // UNBALANCED_OTHER [Leaf] Clock Skew
#define ERR_CLK_JITTER                  0x0F // BALANCED_SHADOW  [Leaf] High Jitter
#define ERR_CLK_EXPIRED                 0x8E // BALANCED_EDGE    [Leaf] Timer Expired
#define ERR_CLK_DRIFT                   0x8F // UNBALANCED_OTHER [Leaf] Clock Drift
// -- Section 2.3: Synchronization
#define ERR_TIME_SYNC                   0x96 // BALANCED_SHADOW  [Section] Sync Primitives
#define ERR_SYNC_WAIT                   0x4C // UNBALANCED_OTHER [Leaf] Wait Failed
#define ERR_SYNC_BARRIER                0x4D // BALANCED_EDGE    [Leaf] Barrier Broken
#define ERR_SYNC_SEM                    0xCC // BALANCED_EDGE    [Leaf] Semaphore Split
#define ERR_SYNC_MUTEX                  0xCD // UNBALANCED_OTHER [Leaf] Mutex Error
// -- Section 2.4: Watchdogs
#define ERR_TIME_WDT                    0x97 // UNBALANCED_OTHER [Section] Watchdog
#define ERR_WDT_BARK                    0x4E // BALANCED_EDGE    [Leaf] Watchdog Warning
#define ERR_WDT_BITE                    0x4F // UNBALANCED_OTHER [Leaf] Watchdog Reset
#define ERR_WDT_EARLY                   0xCE // UNBALANCED_OTHER [Leaf] Kick Too Early
#define ERR_WDT_LATE                    0xCF // UNBALANCED_OTHER [Leaf] Kick Too Late

// DOMAIN 3: PERIPHERALS & DRIVERS (Root 0x55 -> Unbalanced Other)
#define ERR_IO_DOM                      0xAB // UNBALANCED_OTHER [Domain] I/O Subsystem
// -- Section 3.1: Serial (UART/USB)
#define ERR_IO_SERIAL                   0x56 // BALANCED_EDGE    [Section] Serial Bus
#define ERR_SRL_BAUD                    0x2C // UNBALANCED_OTHER [Leaf] Baud Rate Mismatch
#define ERR_SRL_FRAME                   0x2D // BALANCED_SHADOW  [Leaf] Frame Error
#define ERR_SRL_PARITY                  0xAC // BALANCED_EDGE    [Leaf] Parity Fail
#define ERR_SRL_BREAK                   0xAD // UNBALANCED_OTHER [Leaf] Break Condition
// -- Section 3.2: Bus (I2C/SPI)
#define ERR_IO_BUS                      0x57 // UNBALANCED_OTHER [Section] Sync Bus
#define ERR_BUS_NACK                    0x2E // BALANCED_EDGE    [Leaf] I2C NACK
#define ERR_BUS_ARB                     0x2F // UNBALANCED_OTHER [Leaf] Arbitration Lost
#define ERR_BUS_MODE                    0xAE // UNBALANCED_OTHER [Leaf] SPI Mode Mismatch
#define ERR_BUS_OVER                    0xAF // UNBALANCED_OTHER [Leaf] SPI Overrun
// -- Section 3.3: Analog (ADC/DAC)
#define ERR_IO_SIGNAL                   0xD6 // UNBALANCED_OTHER [Section] Analog IO
#define ERR_SIGN_HIGH                   0x6C // BALANCED_EDGE    [Leaf] Saturation High
#define ERR_SIGN_LOW                    0x6D // UNBALANCED_OTHER [Leaf] Saturation Low
#define ERR_SIGN_UNDERRUN               0xEC // UNBALANCED_OTHER [Leaf] DAC Underrun
#define ERR_SIGN_REF                    0xED // UNBALANCED_OTHER [Leaf] Voltage Ref Error
// -- Section 3.4: GPIO & Pin Control
#define ERR_DRV_GPIO                    0xD7 // UNBALANCED_OTHER [Section] GPIO
#define ERR_GPIO_DIR                    0x6E // UNBALANCED_OTHER [Leaf] Direction Error
#define ERR_GPIO_MUX                    0x6F // UNBALANCED_OTHER [Leaf] Muxing Error
#define ERR_GPIO_DEBOUNCE               0xEE // UNBALANCED_TWIN  [Leaf] Debounce Fail
#define ERR_GPIO_LOCKED                 0xEF // UNBALANCED_EDGE  [Leaf] Pin Locked

// =============================================================================
// QUADRANT 3: AFTER / DATA / LOGIC
// ROOT: 0xAA (The Pattern / The Math / Internal)
// =============================================================================

// ROOT: 0xAA
#define ERR_DATA_ROOT                   0xAA // BALANCED_ROOT    [Root] Data Integrity Error

// DOMAIN 1: ENCODING & FORMATS (Root 0xAA -> 0x54)
#define ERR_FMT_DOM                     0x54 // UNBALANCED_OTHER [Domain] Formats
// -- Section 1.1: Text & Strings
#define ERR_FMT_TEXT                    0x28 // UNBALANCED_OTHER [Section] String Encoding
#define ERR_TEXT_ENCODING               0x10 // UNBALANCED_EDGE  [Leaf] Encoding Error
#define ERR_TEXT_EMPTY                  0x11 // UNBALANCED_TWIN  [Leaf] Empty String
#define ERR_TEXT_TRUNC                  0x90 // UNBALANCED_OTHER [Leaf] Truncated
#define ERR_TEXT_FORMAT                 0x91 // UNBALANCED_OTHER [Leaf] Text Format Error
// -- Section 1.2: Structured Data (JSON/XML)
#define ERR_FMT_STRUCT                  0x29 // UNBALANCED_OTHER [Section] Structured Data
#define ERR_STRUCT_PARSE                0x12 // UNBALANCED_OTHER [Leaf] Parse/Syntax Error
#define ERR_STRUCT_TYPE                 0x13 // UNBALANCED_OTHER [Leaf] Type Mismatch
#define ERR_STRUCT_TAG                  0x92 // UNBALANCED_OTHER [Leaf] Tag Mismatch
#define ERR_STRUCT_ATTR                 0x93 // BALANCED_EDGE    [Leaf] Attribute Error
// -- Section 1.3: Binary Formats
#define ERR_FMT_MEDIA                   0xA8 // UNBALANCED_OTHER [Section] Binary Parsing
#define ERR_MEDIA_HEADER                0x50 // UNBALANCED_OTHER [Leaf] Invalid Header
#define ERR_MEDIA_DEPTH                 0x51 // UNBALANCED_OTHER [Leaf] Unsupported Bit Depth
#define ERR_MEDIA_RATE                  0xD0 // UNBALANCED_OTHER [Leaf] Unsupported Sample Rate
#define ERR_MEDIA_CKSUM                 0xD1 // BALANCED_EDGE    [Leaf] Checksum Fail
// -- Section 1.4: Validation
#define ERR_FMT_VALID                   0xA9 // BALANCED_EDGE    [Section] Validation Error
#define ERR_VAL_SCHEMA                  0x52 // UNBALANCED_OTHER [Leaf] Schema Violation
#define ERR_VAL_RANGE                   0x53 // BALANCED_EDGE    [Leaf] Value Out of Range
#define ERR_VAL_PATTERN                 0xD2 // BALANCED_SHADOW  [Leaf] Pattern Mismatch
#define ERR_VAL_REQ                     0xD3 // UNBALANCED_OTHER [Leaf] Required Field Missing

// DOMAIN 2: MATH & LOGIC (Root 0xAA -> 0xD4)
#define ERR_MATH_DOM                    0xD4 // BALANCED_EDGE    [Domain] Computation
// -- Section 2.1: Arithmetic
#define ERR_MATH_CALC                  0x68 // UNBALANCED_OTHER [Section] Arithmetic
#define ERR_CALC_DIV0                   0x30 // UNBALANCED_OTHER [Leaf] Division by Zero
#define ERR_CALC_OVERFLOW               0x31 // UNBALANCED_OTHER [Leaf] Int Overflow
#define ERR_CALC_UNDERFLOW              0xB0 // UNBALANCED_OTHER [Leaf] Int Underflow
#define ERR_CALC_NAN                    0xB1 // BALANCED_EDGE    [Leaf] Result is NaN
// -- Section 2.2: Floating Point
#define ERR_MATH_FP                     0x69 // BALANCED_SHADOW  [Section] Floating Point
#define ERR_FP_INF                      0x32 // UNBALANCED_OTHER [Leaf] Infinity
#define ERR_FP_DENORM                   0x33 // BALANCED_EDGE    [Leaf] Denormalized
#define ERR_FP_PRECISION                0xB2 // BALANCED_EDGE    [Leaf] Precision Loss
#define ERR_FP_ROUND                    0xB3 // UNBALANCED_OTHER [Leaf] Rounding Error
// -- Section 2.3: Cryptographic Math
#define ERR_MATH_CRYPTO                 0xE8 // BALANCED_EDGE    [Section] Crypto Primitives
#define ERR_CRYPTO_CURVE                0x70 // UNBALANCED_OTHER [Leaf] Curve Point Invalid
#define ERR_CRYPTO_PRIME                0x71 // BALANCED_EDGE    [Leaf] Not Prime
#define ERR_CRYPTO_SHADOW_KEY           0xF0 // BALANCED_SHADOW  [Leaf] Weak Key
#define ERR_CRYPTO_PADDING              0xF1 // UNBALANCED_OTHER [Leaf] Padding Error
// -- Section 2.4: Logic Logic
#define ERR_MATH_LOGIC                  0xE9 // UNBALANCED_OTHER [Section] Logic Gates
#define ERR_LOGIC_ASSERT                0x72 // BALANCED_EDGE    [Leaf] Assertion Failed
#define ERR_LOGIC_INVARIANT             0x73 // UNBALANCED_OTHER [Leaf] Invariant Broken
#define ERR_LOGIC_REACH                 0xF2 // UNBALANCED_OTHER [Leaf] Unreachable Code Reached
#define ERR_LOGIC_STATE                 0xF3 // UNBALANCED_OTHER [Leaf] Impossible Logic State

// DOMAIN 3: STORAGE & FILESYSTEM (Root 0xAA -> 0xD5)
#define ERR_STORAGE_DOM                 0xD5 // UNBALANCED_OTHER [Domain] File System
// -- Section 3.1: File Access
#define ERR_STORE_FS                    0x6A // BALANCED_EDGE    [Section] Filesystem Error
#define ERR_FS_NOENT                    0x34 // UNBALANCED_OTHER [Leaf] File Not Found
#define ERR_FS_EXIST                    0x35 // BALANCED_EDGE    [Leaf] File Exists
#define ERR_FS_PERM                     0xB4 // BALANCED_SHADOW  [Leaf] File Locked
#define ERR_FS_FULL                     0xB5 // UNBALANCED_OTHER [Leaf] File Busy
// -- Section 3.2: Attributes
#define ERR_STORE_ATTR                  0x6B // UNBALANCED_OTHER [Section] Inodes/Attrs
#define ERR_ATTR_RO                     0x36 // BALANCED_EDGE    [Leaf] Read Only
#define ERR_ATTR_HIDDEN                 0x37 // UNBALANCED_OTHER [Leaf] Hidden
#define ERR_ATTR_SYMLINK                0xB6 // UNBALANCED_OTHER [Leaf] Symlink Loop
#define ERR_ATTR_NO_SPACE               0xB7 // UNBALANCED_OTHER [Leaf] No Device Space
// -- Section 3.3: Block Device
#define ERR_STORE_BLOCK                 0xEA // UNBALANCED_OTHER [Section] Block Layer
#define ERR_BLK_READ                    0x74 // BALANCED_EDGE    [Leaf] Block Read Error
#define ERR_BLK_WRITE                   0x75 // UNBALANCED_OTHER [Leaf] Block Write Error
#define ERR_BLK_SECTOR                  0xF4 // UNBALANCED_OTHER [Leaf] Bad Sector
#define ERR_BLK_GEOMETRY                0xF5 // UNBALANCED_OTHER [Leaf] Bad Geometry
// -- Section 3.4: Volumes
#define ERR_STORE_VOLUME                0xEB // UNBALANCED_OTHER [Section] Volume Integrity
#define ERR_VOL_DIRTY                   0x76 // UNBALANCED_OTHER [Leaf] Volume Dirty
#define ERR_VOL_MOUNT                   0x77 // UNBALANCED_TWIN  [Leaf] Double Mount
#define ERR_VOL_UNMOUNT                 0xF6 // UNBALANCED_OTHER [Leaf] Unmount Fail
#define ERR_VOL_UNKNOWN                 0xF7 // UNBALANCED_EDGE  [Leaf] Unknown FS

// =============================================================================
// QUADRANT 4: RUN / FATAL / SECURITY / CRITICAL
// ROOT: 0xFF (The End / Saturation / Panic)
// =============================================================================

// ROOT: 0xFF
#define ERR_FATAL                       ERR_FINALIZED // UNBALANCED_ROOT  [Root] Fatal System Failure

// DOMAIN 1: SECURITY & AUTH (Root 0xFF -> 0x7E)
#define ERR_SEC_DOM                     0x7E // UNBALANCED_OTHER [Domain] Security
// -- Section 1.1: Authentication
#define ERR_SEC_AUTH                    0x3C // BALANCED_SHADOW  [Section] Auth Failure
#define ERR_AUTH_FAIL                   0x18 // UNBALANCED_OTHER [Leaf] Login Failed
#define ERR_AUTH_EXPIRED                0x19 // UNBALANCED_OTHER [Leaf] Token Expired
#define ERR_AUTH_SCOPE                  0x98 // UNBALANCED_OTHER [Leaf] Scope Invalid
#define ERR_AUTH_MATCH                  0x99 // BALANCED_EDGE    [Leaf] Credential Replay
// -- Section 1.2: Encryption
#define ERR_SEC_CRYPT                   0x3D // UNBALANCED_OTHER [Section] Encryption
#define ERR_CRYPT_ALGO                  0x1A // UNBALANCED_OTHER [Leaf] Algo Not Supported
#define ERR_CRYPT_KEY                   0x1B // BALANCED_EDGE    [Leaf] Invalid Key
#define ERR_CRYPT_IV                    0x9A // BALANCED_EDGE    [Leaf] Invalid IV
#define ERR_CRYPT_TAG                   0x9B // UNBALANCED_OTHER [Leaf] Tag Mismatch
// -- Section 1.3: Policy
#define ERR_SEC_POLICY                  0xBC // UNBALANCED_OTHER [Section] Policy Violation
#define ERR_POL_DENY                    0x58 // UNBALANCED_OTHER [Leaf] Explicit Deny
#define ERR_POL_QUOTA                   0x59 // BALANCED_EDGE    [Leaf] Quota Exceeded
#define ERR_POL_TIME                    0xD8 // BALANCED_EDGE    [Leaf] Access Time Violation
#define ERR_POL_REJECT                  0xD9 // UNBALANCED_OTHER [Leaf] Access Rejected
// -- Section 1.4: Auditing
#define ERR_SEC_AUDIT                   0xBD // UNBALANCED_OTHER [Section] Audit Failure
#define ERR_AUDIT_LOG                   0x5A // BALANCED_SHADOW  [Leaf] Log Write Failed
#define ERR_AUDIT_FULL                  0x5B // UNBALANCED_OTHER [Leaf] Audit Log Full
#define ERR_AUDIT_TAMPER                0xDA // UNBALANCED_OTHER [Leaf] Tampering Detected
#define ERR_AUDIT_FAIL                  0xDB // UNBALANCED_OTHER [Leaf] Audit System Fail

// DOMAIN 2: DEADLOCK & CONCURRENCY FATAL (Root 0xFF -> 0x7F)
#define ERR_LOCK_DOM                    0x7F // UNBALANCED_EDGE  [Domain] Concurrency Fatal
// -- Section 2.1: Locks
#define ERR_ATOM_LOCK                   0x3E // UNBALANCED_OTHER [Section] Lock Error
#define ERR_LOCK_BUSY                   0x1C // UNBALANCED_OTHER [Leaf] Lock Busy
#define ERR_LOCK_OWNER                  0x1D // BALANCED_EDGE    [Leaf] Not Owner
#define ERR_LOCK_DEAD                   0x9C // BALANCED_EDGE    [Leaf] Deadlock Detected
#define ERR_LOCK_MAX                    0x9D // UNBALANCED_OTHER [Leaf] Max Recursion
// -- Section 2.2: IPC Pipes
#define ERR_ATOM_IPC                    0x3F // UNBALANCED_OTHER [Section] Pipe/Queue
#define ERR_PIPE_BROKEN                 0x1E // BALANCED_SHADOW  [Leaf] Broken Pipe
#define ERR_PIPE_FULL                   0x1F // UNBALANCED_OTHER [Leaf] Pipe Full
#define ERR_MSG_SIZE                    0x9E // UNBALANCED_OTHER [Leaf] Message Too Large
#define ERR_MSG_QUEUE                   0x9F // UNBALANCED_OTHER [Leaf] Queue Destroyed
// -- Section 2.3: Atomic Ops
#define ERR_ATOM_ORDER                  0xBE // UNBALANCED_OTHER [Section] Atomics
#define ERR_ORDER_ACQ                   0x5C // BALANCED_EDGE    [Leaf] Acquire Fail
#define ERR_ORDER_REL                   0x5D // UNBALANCED_OTHER [Leaf] Release Fail
#define ERR_ORDER_BARRIER               0xDC // UNBALANCED_OTHER [Leaf] Barrier Violation
#define ERR_ORDER_RACE                  0xDD // UNBALANCED_TWIN  [Leaf] Race Condition
// -- Section 2.4: Critical Sections
#define ERR_LOCK_CRIT                   0xBF // UNBALANCED_EDGE  [Section] Critical Area
#define ERR_CRIT_ENTER                  0x5E // UNBALANCED_OTHER [Leaf] Enter Fail
#define ERR_CRIT_LEAVE                  0x5F // UNBALANCED_OTHER [Leaf] Leave Fail
#define ERR_CRIT_TIMEOUT                0xDE // UNBALANCED_OTHER [Leaf] Critical Timeout
#define ERR_CRIT_BAIL                   0xDF // UNBALANCED_EDGE  [Leaf] Emergency Bail

// DOMAIN 3: HARDWARE FATAL / PANIC (Root 0xFF -> 0xFE)
#define ERR_HW_DOM                      0xFE // UNBALANCED_EDGE  [Domain] Hardware Panic
// -- Section 3.1: CPU/Core
#define ERR_HW_CPU                      0x7C // UNBALANCED_OTHER [Section] CPU Exception
#define ERR_CPU_ILL                     0x38 // UNBALANCED_OTHER [Leaf] Illegal Instruction
#define ERR_CPU_BUS                     0x39 // BALANCED_EDGE    [Leaf] Bus Error
#define ERR_CPU_TRAP                    0xB8 // BALANCED_EDGE    [Leaf] Trap
#define ERR_CPU_HALT                    0xB9 // UNBALANCED_OTHER [Leaf] Core Halted
// -- Section 3.2: Power
#define ERR_HW_PWR                      0x7D // UNBALANCED_OTHER [Section] Power
#define ERR_PWR_BOR                     0x3A // BALANCED_EDGE    [Leaf] Brownout
#define ERR_PWR_LOW                     0x3B // UNBALANCED_OTHER [Leaf] Voltage Low
#define ERR_PWR_HIGH                    0xBA // UNBALANCED_OTHER [Leaf] Voltage High
#define ERR_PWR_BATT                    0xBB // UNBALANCED_TWIN  [Leaf] Battery Critical
// -- Section 3.3: Memory Hardware
#define ERR_HW_RAM                      0xFC // UNBALANCED_OTHER [Section] RAM/Flash
#define ERR_RAM_ECC                     0x78 // BALANCED_SHADOW  [Leaf] Uncorrectable ECC
#define ERR_RAM_PARITY                  0x79 // UNBALANCED_OTHER [Leaf] RAM Parity
#define ERR_RAM_FAIL                    0xF8 // UNBALANCED_OTHER [Leaf] RAM Test Failed
#define ERR_RAM_LINE                    0xF9 // UNBALANCED_OTHER [Leaf] Address Line Stuck
// -- Section 3.4: System Integrity
#define ERR_SYS_PANIC                   0xFD // UNBALANCED_EDGE  [Section] Kernel Panic
#define ERR_PANIC_GENERAL               0x7A // UNBALANCED_OTHER [Leaf] General Panic
#define ERR_PANIC_STACK                 0x7B // UNBALANCED_OTHER [Leaf] Stack Smashed
#define ERR_PANIC_ASSERT                0xFA // UNBALANCED_OTHER [Leaf] Kernel Assert
#define ERR_PANIC_ABORT                 0xFB // UNBALANCED_EDGE  [Leaf] System Abort

/* =========================================================================
   HELPER MACROS FOR ANALYSIS
   ========================================================================= */

#define ERR_IS_RESERVED(err) \
    ((err) == ERR_SUCCESS || (err) == ERR_YIELDING || (err) == ERR_EXITING || (err) == ERR_ENDING || (err) == ERR_FINALIZED)

// Terminal roots: pure endpoints without internal structure
#define ERR_IS_ABSTRACT(err) \
    ((err) == ERR_ROOT_INIT || (err) == ERR_ROOT_RUN)

// Oscillatory roots: structured alternating primitives
#define ERR_IS_MOVEMENT(err) \
    ((err) == ERR_ROOT_BEFORE || (err) == ERR_ROOT_AFTER)

// 8-bit Plane: Split into two 4-bit nibbles
#define ERR_NIBBLE_LEFT(err) (((err) >> 4) & 0x0F)
#define ERR_NIBBLE_RIGHT(err) ((err) & 0x0F)

// Checks if the Left Nibble is equal to the Inverse of the Right Nibble
#define ERR_IS_SHADOW(err) \
    (ERR_NIBBLE_LEFT(err) == (~ERR_NIBBLE_RIGHT(err) & 0xF))

// The first half of a cluster is 4bits and must equal the other 4bits
// e.g. 0x00, 0x11, 0x22 ... 0xFF
#define ERR_IS_TWIN(err) (ERR_NIBBLE_LEFT(err) == ERR_NIBBLE_RIGHT(err))

#define ERR_IS_MIRROR(err) (err_op_reverse(err) == ((err) & 0xFF))

/**
 *  HIERARCHY FUNCTIONS
 *
 */

// The 4 primordial error classes of which every other error are its children.
// 0 (00000000), 255 (11111111), 85 (01010101), 170 (10101010)
#define ERR_IS_ROOT(err) \
    (ERR_IS_ABSTRACT(err) || ERR_IS_MOVEMENT(err))

// Direct 12 descendants of the primordial error classes
#define ERR_IS_DOMAIN(err) (!ERR_IS_ROOT(err) && (ERR_IS_ROOT(err_op_center(err))))

// Each domain has 4 sections and 12 per root
#define ERR_IS_SECTION(err) (err_op_depth(err) == 2)

// The direct descendants of the DOMAIN error codes
#define ERR_IS_LEAF(err) (err_op_depth(err) == 3)

/**
 * SYMMETRY CLASSES
 * 8-bit Matrix of 16x16 byte error codes
 */

// Balanced means half of the bits are set (4 out of 8)
#define ERR_IS_BALANCED(err) (err_op_one_count(err) == 4)
#define ERR_IS_UNBALANCED(err) (err_op_one_count(err) != 4)

// A balanced diagonal from right top to left bottom
// results in error codes 0x55 (85) and 0xAA (170) => ERR_IS_MOVEMENT(err)
#define ERR_IS_BALANCED_ROOT(err) (err_op_center(err) == err_op_inverse(err))

// A balanced diagonal from right top to left bottom
#define ERR_IS_BALANCED_SHADOW(err) (ERR_IS_BALANCED(err) && ERR_IS_SHADOW(err))

// A balanced circle in the middle of the 16x16 matrix
#define ERR_IS_BALANCED_EDGE(err) (!ERR_IS_BALANCED_ROOT(err) && ERR_IS_BALANCED(err) && !ERR_IS_SHADOW(err))

// An unbalanced diagonal from left top to right bottom
// Only 0x00 and 0xFF
#define ERR_IS_UNBALANCED_ROOT(err) (ERR_IS_ABSTRACT(err))

// Diagonal from left top to right bottom (twin but not 00 or FF)
#define ERR_IS_UNBALANCED_TWIN(err) ((!ERR_IS_ABSTRACT(err)) && ERR_IS_UNBALANCED(err) && ERR_IS_TWIN(err))

// Extreme imbalance (1 or 7 one's). Outer edge cirlce of the 16x16 matrix.
#define ERR_IS_UNBALANCED_EDGE(err) (err_op_one_count(err) == 1 || err_op_one_count(err) == 7)

// Everything else unbalanced
#define ERR_IS_UNBALANCED_OTHER(err) (ERR_IS_UNBALANCED(err) && !ERR_IS_TWIN(err) && !ERR_IS_UNBALANCED_EDGE(err))

static CC_ALWAYS_INLINE uint8_t err_op_inverse(uint8_t err) {
    // Inverse the cluster (EEEE DDDD)
    // In 8-bit, we invert the whole byte.
    return ~err;
}

static CC_ALWAYS_INLINE uint8_t err_op_reverse(uint8_t err) {
    // Reverse the cluster bits: 7-0
    // Standard SWAR bit reversal for 8 bits
    err = (err & 0xF0) >> 4 | (err & 0x0F) << 4;
    err = (err & 0xCC) >> 2 | (err & 0x33) << 2;
    err = (err & 0xAA) >> 1 | (err & 0x55) << 1;
    return err;
}

static CC_ALWAYS_INLINE uint8_t err_op_opposite(uint8_t err) {
    // Swap the nibbles (Left <-> Right)
    // (EEEE DDDD) -> (DDDD EEEE)
    return (ERR_NIBBLE_RIGHT(err) << 4) | ERR_NIBBLE_LEFT(err);
}

static CC_ALWAYS_INLINE uint8_t err_op_center(uint8_t err) {
    // Nuclear transformation for 8 bits.
    // Logic: Sliding window of size 4.
    // Right Nibble becomes bits 1,2,3,4 (shifted to 0,1,2,3)
    // Left Nibble becomes bits 3,4,5,6 (shifted to 4,5,6,7)
    // ((err >> 1) & 0x0F) extracts 1,2,3,4 -> 0,1,2,3
    // ((err << 1) & 0xF0) extracts 3,4,5,6 -> 4,5,6,7
    return ((err << 1) & 0xF0) | ((err >> 1) & 0x0F);
}

static CC_ALWAYS_INLINE uint8_t err_op_root(uint8_t err) {
    // Convergence tree iteration
    // Typically depth increases with bit width, 8-bit takes 4 steps max.
    while (!ERR_IS_ROOT(err)) err = err_op_center(err);
    return err;
}

/**
 * Get the depth in the nuclear convergence tree.
 * Returns how many center() operations needed to reach the root.
 */
static CC_ALWAYS_INLINE uint8_t err_op_depth(uint8_t err) {
  uint8_t d = 0;
  while (!ERR_IS_ROOT(err)) {
      err = err_op_center(err);
      d++;
  }
  return d;
}

static CC_ALWAYS_INLINE uint8_t err_op_one_count(uint8_t err) {
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

static CC_ALWAYS_INLINE uint8_t err_op_distance(uint8_t left, uint8_t right) {
    return err_op_one_count(left ^ right);
}

static CC_ALWAYS_INLINE very_fast_log2(float val) {
  union { float f; uint32_t i; } convert;
  convert.f = val;
  return (float)((convert.i >> 23) - 127);   // only integer part, error up to ~1
}

/*
 * Calculate entropy (Shannon information) of nibbles.
 * Measures balance/chaos: 0 = pure, 1 = perfectly balanced.
 *
 * Formula based on N=8 lines.
 */
static CC_ALWAYS_INLINE float32_t err_op_entropy(uint8_t err) {
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
static CC_ALWAYS_INLINE float32_t err_op_balance(uint8_t err) {
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

static err_relation_t err_op_relation(uint8_t left, uint8_t right) {
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


CC_EXTERN const char *error_to_string(uint8_t err);

#endif // ERRORS_H
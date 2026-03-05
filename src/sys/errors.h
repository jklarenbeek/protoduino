// file: ./src/sys/errors.h
#ifndef __ERRORS_H__
#define __ERRORS_H__

//
// ONTOLOGICAL 8-BIT ERROR TAXONOMY
#define __ERRORS_VERSION__ 3
// -----------------------------------------------------------------------------
// Geometry:  16x16 Matrix (0..255) mapped to 4 Attractor Basins.
// Logic:     Convergence via ((Child << 1) & 0xF0) | ((Child >>> 1) & 0x0F)
// Architecture: 4 ROOTS -> 12 DOMAINS -> 48 SECTIONS -> 192 LEAFS
// Symmetry: TWINS (foundational), SHADOWS (complementary), MIRRORS (bidirectional)
// Inverse mapping: ~error & 0xFF maps to corresponding event code
// -----------------------------------------------------------------------------
//

// =============================================================================
// RESERVED KERNEL CODES (Process State Machine)
// =============================================================================
#define ERR_SUCCESS      0x00  // Operation completed successfully
#define ERR_YIELDING     0x01  // Process yielded control (waiting for event)
#define ERR_EXITING      0x02  // Process exited externally
#define ERR_ENDING       0x03  // Process terminated normally
#define ERR_FINALIZED    0xFF  // System finalized/absolute stop

// =============================================================================
// THE 4 PRIMORDIAL ERROR CLASSES
// =============================================================================
#define ERR_ROOT_INIT     0x00 /* Genesis/Success/Void */
#define ERR_ROOT_RUN      0xFF /* Saturation/Fatal/Terminal */
#define ERR_ROOT_BEFORE   0x55 /* I/O/External/Oscillation */
#define ERR_ROOT_AFTER    0xAA /* Data/Internal/Processing */

// =============================================================================
// QUADRANT 1: INIT / SYSTEM STATE / LIFECYCLE
// ROOT: 0x00 (The Void / Success / Origins)
// Covers: System state, resources, memory, processes, initialization
// =============================================================================

// ROOT: 0x00
#define ERR_OK                          ERR_SUCCESS // IMPULSE [Root] Success state

// -----------------------------------------------------------------------------
// DOMAIN 1.1: SYSTEM STATE & ACCESS CONTROL (0x00 -> 0x01)
// Covers: Permissions, handles, descriptors, processes
// -----------------------------------------------------------------------------
#define ERR_SYS_STATE                   0x01 // IMPULSE [Domain]

// Section 1.1.1: Access & Permissions (0x01 -> 0x02)
#define ERR_SYS_ACCESS                  0x02 // IMPULSE [Section]
#define ERR_ACCESS_DENIED               0x04 // IMPULSE [Leaf] Access denied
#define ERR_ACCESS_READONLY             0x05 // ASYMMETRY [Leaf] Write to readonly
#define ERR_ACCESS_LOCKED               0x84 // ASYMMETRY [Leaf] Resource locked
#define ERR_ACCESS_FORBIDDEN            0x85 // ASYMMETRY [Leaf] Forbidden operation

// Section 1.1.2: Handles & Descriptors (0x01 -> 0x03)
#define ERR_SYS_HANDLE                  0x03 // ASYMMETRY [Section]
#define ERR_HANDLE_INVALID              0x06 // ASYMMETRY [Leaf] Invalid handle
#define ERR_HANDLE_CLOSED               0x07 // ASYMMETRY [Leaf] Handle already closed
#define ERR_HANDLE_TYPE                 0x86 // ASYMMETRY [Leaf] Wrong handle type
#define ERR_HANDLE_SHADOW               0x87 // ANTICODE [Leaf] Stale/ghost handle

// Section 1.1.3: Process Management (0x01 -> 0x82)
#define ERR_SYS_PROC                    0x82 // ASYMMETRY [Section]
#define ERR_PROC_CANCELLED              0x44 // REPEATER [Leaf] Operation cancelled
#define ERR_PROC_KILLED                 0x45 // ASYMMETRY [Leaf] Process killed
#define ERR_PROC_ORPHAN                 0xC4 // ASYMMETRY [Leaf] Orphaned process
#define ERR_PROC_ZOMBIE                 0xC5 // MARGIN [Leaf] Zombie process

// Section 1.1.4: Arguments & Validation (0x01 -> 0x83)
#define ERR_SYS_ARGUMENT                0x83 // ASYMMETRY [Section]
#define ERR_ARG_NULL                    0x46 // ASYMMETRY [Leaf] NULL argument
#define ERR_ARG_INVALID                 0x47 // MARGIN [Leaf] Invalid argument
#define ERR_ARG_RANGE                   0xC6 // MARGIN [Leaf] Arg out of range
#define ERR_ARG_TYPE                    0xC7 // ASYMMETRY [Leaf] Type mismatch

// -----------------------------------------------------------------------------
// DOMAIN 1.2: MEMORY & BUFFERS (0x00 -> 0x80)
// Covers: Heap, stack, buffers, alignment, paging
// -----------------------------------------------------------------------------
#define ERR_MEM_DOM                     0x80 // IMPULSE [Domain]

// Section 1.2.1: Heap Allocation (0x80 -> 0x40)
#define ERR_MEM_HEAP                    0x40 // IMPULSE [Section]
#define ERR_HEAP_OOM                    0x20 // IMPULSE [Leaf] Out of memory
#define ERR_HEAP_FRAGMENT               0x21 // ASYMMETRY [Leaf] Fragmented
#define ERR_HEAP_CORRUPT                0xA0 // ASYMMETRY [Leaf] Heap corrupted
#define ERR_HEAP_DOUBLE_FREE            0xA1 // ASYMMETRY [Leaf] Double free

// Section 1.2.2: Stack & Bounds (0x80 -> 0x41)
#define ERR_MEM_BOUNDS                  0x41 // ASYMMETRY [Section]
#define ERR_STACK_OVERFLOW              0x22 // REPEATER [Leaf] Stack overflow
#define ERR_STACK_UNDERFLOW             0x23 // ASYMMETRY [Leaf] Stack underflow
#define ERR_BOUNDS_LOWER                0xA2 // ASYMMETRY [Leaf] Lower bound
#define ERR_BOUNDS_UPPER                0xA3 // MARGIN [Leaf] Upper bound

// Section 1.2.3: Buffers (0x80 -> 0xC0)
#define ERR_MEM_BUFFER                  0xC0 // ASYMMETRY [Section]
#define ERR_BUF_OVERFLOW                0x60 // ASYMMETRY [Leaf] Buffer overflow
#define ERR_BUF_UNDERFLOW               0x61 // ASYMMETRY [Leaf] Buffer underflow
#define ERR_BUF_FULL                    0xE0 // ASYMMETRY [Leaf] Buffer full
#define ERR_BUF_EMPTY                   0xE1 // ANTICODE [Leaf] Buffer empty

// Section 1.2.4: Alignment & Paging (0x80 -> 0xC1)
#define ERR_MEM_ALIGN                   0xC1 // ASYMMETRY [Section]
#define ERR_ALIGN_ADDR                  0x62 // ASYMMETRY [Leaf] Misaligned address
#define ERR_ALIGN_SIZE                  0x63 // MARGIN [Leaf] Misaligned size
#define ERR_PAGE_FAULT                  0xE2 // MARGIN [Leaf] Page fault
#define ERR_PAGE_PROTECT                0xE3 // ASYMMETRY [Leaf] Page protection

// -----------------------------------------------------------------------------
// DOMAIN 1.3: LIFECYCLE & INITIALIZATION (0x00 -> 0x81)
// Covers: Init sequences, state transitions, reference counting, cleanup
// -----------------------------------------------------------------------------
#define ERR_LIFE_DOM                    0x81 // ASYMMETRY [Domain]

// Section 1.3.1: Initialization (0x81 -> 0x42)
#define ERR_LIFE_INIT                   0x42 // ASYMMETRY [Section]
#define ERR_INIT_FAILED                 0x24 // ASYMMETRY [Leaf] Init failed
#define ERR_INIT_TIMEOUT                0x25 // ASYMMETRY [Leaf] Init timeout
#define ERR_INIT_DEPENDENCY             0xA4 // ASYMMETRY [Leaf] Missing dependency
#define ERR_INIT_NOSUP                  0xA5 // ANTICODE [Leaf] Already initialized

// Section 1.3.2: State Transitions (0x81 -> 0x43)
#define ERR_LIFE_STATE                  0x43 // ASYMMETRY [Section]
#define ERR_STATE_INVALID               0x26 // ASYMMETRY [Leaf] Invalid state
#define ERR_STATE_TRANSITION            0x27 // MARGIN [Leaf] Bad transition
#define ERR_STATE_LOCKED                0xA6 // MARGIN [Leaf] State locked
#define ERR_STATE_FROZEN                0xA7 // ASYMMETRY [Leaf] State frozen

// Section 1.3.3: Reference Counting (0x81 -> 0xC2)
#define ERR_LIFE_REF                    0xC2 // ASYMMETRY [Section]
#define ERR_REF_ZERO                    0x64 // ASYMMETRY [Leaf] Ref count zero
#define ERR_REF_OVERFLOW                0x65 // MARGIN [Leaf] Ref count overflow
#define ERR_REF_LEAK                    0xE4 // MARGIN [Leaf] Memory leak
#define ERR_REF_DANGLING                0xE5 // ASYMMETRY [Leaf] Dangling pointer

// Section 1.3.4: Cleanup & Finalization (0x81 -> 0xC3)
#define ERR_LIFE_CLEAN                  0xC3 // ANTICODE [Section]
#define ERR_CLEAN_FAILED                0x66 // MARGIN [Leaf] Cleanup failed
#define ERR_CLEAN_PARTIAL               0x67 // ASYMMETRY [Leaf] Partial cleanup
#define ERR_CLEAN_BUSY                  0xE6 // ASYMMETRY [Leaf] Resource busy
#define ERR_CLEAN_LEAKED                0xE7 // ASYMMETRY [Leaf] Resource leaked

// =============================================================================
// QUADRANT 2: BEFORE / EXTERNAL I/O / HARDWARE
// ROOT: 0x55 (The Oscillation / The Wire / External Events)
// Covers: Network, peripherals, timing, interrupts, external communication
// =============================================================================

// ROOT: 0x55
#define ERR_IO_BUSY                     0x55 // PARITY [Root] Resource busy

// -----------------------------------------------------------------------------
// DOMAIN 2.1: NETWORK & CONNECTIVITY (0x55 -> 0x2A)
// Covers: Sockets, DNS, protocols, transport layer
// -----------------------------------------------------------------------------
#define ERR_NET_DOM                     0x2A // ASYMMETRY [Domain]

// Section 2.1.1: Sockets & Connections (0x2A -> 0x14)
#define ERR_NET_SOCK                    0x14 // ASYMMETRY [Section]
#define ERR_SOCK_CREATE                 0x08 // IMPULSE [Leaf] Socket create failed
#define ERR_SOCK_BIND                   0x09 // ASYMMETRY [Leaf] Bind failed
#define ERR_SOCK_LISTEN                 0x88 // REPEATER [Leaf] Listen failed
#define ERR_SOCK_ACCEPT                 0x89 // ASYMMETRY [Leaf] Accept failed

// Section 2.1.2: Connection State (0x2A -> 0x15)
#define ERR_NET_CONN                    0x15 // ASYMMETRY [Section]
#define ERR_CONN_REFUSED                0x0A // ASYMMETRY [Leaf] Connection refused
#define ERR_CONN_RESET                  0x0B // ASYMMETRY [Leaf] Connection reset
#define ERR_CONN_CLOSED                 0x8A // ASYMMETRY [Leaf] Connection closed
#define ERR_CONN_TIMEOUT                0x8B // MARGIN [Leaf] Connection timeout

// Section 2.1.3: DNS & Resolution (0x2A -> 0x94)
#define ERR_NET_DNS                     0x94 // ASYMMETRY [Section]
#define ERR_DNS_NXDOMAIN                0x48 // ASYMMETRY [Leaf] Domain not found
#define ERR_DNS_TIMEOUT                 0x49 // ASYMMETRY [Leaf] Resolution timeout
#define ERR_DNS_SERVFAIL                0xC8 // ASYMMETRY [Leaf] Server failure
#define ERR_DNS_CONFIG                  0xC9 // MARGIN [Leaf] DNS config error

// Section 2.1.4: Protocol Layer (0x2A -> 0x95)
#define ERR_NET_PROTO                   0x95 // MARGIN [Section]
#define ERR_PROTO_VERSION               0x4A // ASYMMETRY [Leaf] Protocol version
#define ERR_PROTO_FORMAT                0x4B // ANTICODE [Leaf] Format error
#define ERR_PROTO_SEQUENCE              0xCA // MARGIN [Leaf] Sequence error
#define ERR_PROTO_STATE                 0xCB // ASYMMETRY [Leaf] Protocol state

// -----------------------------------------------------------------------------
// DOMAIN 2.2: TIMING & SIGNALS (0x55 -> 0x2B)
// Covers: Clocks, timers, interrupts, synchronization, watchdogs
// -----------------------------------------------------------------------------
#define ERR_TIME_DOM                    0x2B // MARGIN [Domain]

// Section 2.2.1: Interrupts (0x2B -> 0x16)
#define ERR_TIME_IRQ                    0x16 // ASYMMETRY [Section]
#define ERR_IRQ_DISABLED                0x0C // ASYMMETRY [Leaf] IRQ disabled
#define ERR_IRQ_PENDING                 0x0D // ASYMMETRY [Leaf] IRQ pending
#define ERR_IRQ_NESTED                  0x8C // ASYMMETRY [Leaf] Nested overflow
#define ERR_IRQ_PRIORITY                0x8D // MARGIN [Leaf] Priority error

// Section 2.2.2: Clocks & Timers (0x2B -> 0x17)
#define ERR_TIME_CLOCK                  0x17 // MARGIN [Section]
#define ERR_CLK_NOT_READY               0x0E // ASYMMETRY [Leaf] Clock not ready
#define ERR_CLK_UNSTABLE                0x0F // ANTICODE [Leaf] Clock unstable
#define ERR_CLK_EXPIRED                 0x8E // MARGIN [Leaf] Timer expired
#define ERR_CLK_OVERFLOW                0x8F // ASYMMETRY [Leaf] Timer overflow

// Section 2.2.3: Synchronization (0x2B -> 0x96)
#define ERR_TIME_SYNC                   0x96 // ANTICODE [Section]
#define ERR_SYNC_FAILED                 0x4C // ASYMMETRY [Leaf] Sync failed
#define ERR_SYNC_TIMEOUT                0x4D // MARGIN [Leaf] Sync timeout
#define ERR_SYNC_BARRIER                0xCC // MARGIN [Leaf] Barrier broken
#define ERR_SYNC_LOST                   0xCD // ASYMMETRY [Leaf] Sync lost

// Section 2.2.4: Watchdog (0x2B -> 0x97)
#define ERR_TIME_WDT                    0x97 // ASYMMETRY [Section]
#define ERR_WDT_EXPIRED                 0x4E // MARGIN [Leaf] Watchdog expired
#define ERR_WDT_RESET                   0x4F // ASYMMETRY [Leaf] Watchdog reset
#define ERR_WDT_EARLY                   0xCE // ASYMMETRY [Leaf] Early kick
#define ERR_WDT_CONFIG                  0xCF // ASYMMETRY [Leaf] WDT config error

// -----------------------------------------------------------------------------
// DOMAIN 2.3: PERIPHERALS & DRIVERS (0x55 -> 0xAB)
// Covers: Serial (UART/USB), buses (I2C/SPI), GPIO, analog I/O
// -----------------------------------------------------------------------------
#define ERR_IO_DOM                      0xAB // ASYMMETRY [Domain]

// Section 2.3.1: Serial Communication (0xAB -> 0x56)
#define ERR_IO_SERIAL                   0x56 // MARGIN [Section]
#define ERR_SERIAL_BAUD                 0x2C // ASYMMETRY [Leaf] Baud rate error
#define ERR_SERIAL_FRAME                0x2D // ANTICODE [Leaf] Frame error
#define ERR_SERIAL_PARITY               0xAC // MARGIN [Leaf] Parity error
#define ERR_SERIAL_OVERRUN              0xAD // ASYMMETRY [Leaf] Overrun

// Section 2.3.2: Bus Communication (0xAB -> 0x57)
#define ERR_IO_BUS                      0x57 // ASYMMETRY [Section]
#define ERR_BUS_NACK                    0x2E // MARGIN [Leaf] NACK received
#define ERR_BUS_ARBITRATION             0x2F // ASYMMETRY [Leaf] Arbitration lost
#define ERR_BUS_TIMEOUT                 0xAE // ASYMMETRY [Leaf] Bus timeout
#define ERR_BUS_ERROR                   0xAF // ASYMMETRY [Leaf] Bus error

// Section 2.3.3: Analog I/O (0xAB -> 0xD6)
#define ERR_IO_ANALOG                   0xD6 // ASYMMETRY [Section]
#define ERR_ADC_SATURATED               0x6C // MARGIN [Leaf] ADC saturated
#define ERR_ADC_TIMEOUT                 0x6D // ASYMMETRY [Leaf] ADC timeout
#define ERR_DAC_UNDERRUN                0xEC // ASYMMETRY [Leaf] DAC underrun
#define ERR_DAC_CONFIG                  0xED // ASYMMETRY [Leaf] DAC config

// Section 2.3.4: GPIO & Pins (0xAB -> 0xD7)
#define ERR_IO_GPIO                     0xD7 // ASYMMETRY [Section]
#define ERR_GPIO_CONFIG                 0x6E // ASYMMETRY [Leaf] Config error
#define ERR_GPIO_LOCKED                 0x6F // ASYMMETRY [Leaf] Pin locked
#define ERR_GPIO_STATE                  0xEE // REPEATER [Leaf] Invalid state
#define ERR_GPIO_INTERRUPT              0xEF // IMPULSE [Leaf] IRQ error

// =============================================================================
// QUADRANT 3: AFTER / DATA / LOGIC
// ROOT: 0xAA (The Pattern / The Math / Internal Processing)
// Covers: Parsing, encoding, validation, math, queues, storage
// =============================================================================

// ROOT: 0xAA
#define ERR_DATA_ROOT                   0xAA // PARITY [Root] Data error

// -----------------------------------------------------------------------------
// DOMAIN 3.1: PARSING & ENCODING (0xAA -> 0x54)
// Covers: Text encoding, structured data (JSON/XML), binary formats
// -----------------------------------------------------------------------------
#define ERR_PARSE_DOM                   0x54 // ASYMMETRY [Domain]

// Section 3.1.1: Text & Encoding (0x54 -> 0x28)
#define ERR_PARSE_TEXT                  0x28 // ASYMMETRY [Section]
#define ERR_TEXT_ENCODE                 0x10 // IMPULSE [Leaf] Encoding error
#define ERR_TEXT_DECODE                 0x11 // REPEATER [Leaf] Decode error
#define ERR_TEXT_TRUNC                  0x90 // ASYMMETRY [Leaf] Truncated
#define ERR_TEXT_INVALID                0x91 // ASYMMETRY [Leaf] Invalid text

// Section 3.1.2: Structured Data (0x54 -> 0x29)
#define ERR_PARSE_STRUCT                0x29 // ASYMMETRY [Section]
#define ERR_STRUCT_SYNTAX               0x12 // ASYMMETRY [Leaf] JSON/YAML syntax
#define ERR_STRUCT_TYPE                 0x13 // ASYMMETRY [Leaf] JSON/YAML type error
#define ERR_STRUCT_MALFORMED            0x92 // ASYMMETRY [Leaf] JSON/XML malformed
#define ERR_STRUCT_NAMESPACE            0x93 // MARGIN [Leaf] JSON/XML namespace

// Section 3.1.3: Binary Formats (0x54 -> 0xA8)
#define ERR_PARSE_BINARY                0xA8 // ASYMMETRY [Section]
#define ERR_BIN_HEADER                  0x50 // ASYMMETRY [Leaf] Bad header
#define ERR_BIN_MAGIC                   0x51 // ASYMMETRY [Leaf] Magic mismatch
#define ERR_BIN_VERSION                 0xD0 // ASYMMETRY [Leaf] Version mismatch
#define ERR_BIN_CHECKSUM                0xD1 // MARGIN [Leaf] Checksum fail

// Section 3.1.4: Validation (0x54 -> 0xA9)
#define ERR_PARSE_VALID                 0xA9 // MARGIN [Section]
#define ERR_VALID_SCHEMA                0x52 // ASYMMETRY [Leaf] Schema violation
#define ERR_VALID_CONSTRAINT            0x53 // MARGIN [Leaf] Constraint fail
#define ERR_VALID_PATTERN               0xD2 // ANTICODE [Leaf] Pattern mismatch
#define ERR_VALID_REQUIRED              0xD3 // ASYMMETRY [Leaf] Required missing

// -----------------------------------------------------------------------------
// DOMAIN 3.2: QUEUES & COLLECTIONS (0xAA -> 0xD4)
// Covers: Queue operations, ordering, collection state
// -----------------------------------------------------------------------------
#define ERR_QUEUE_DOM                   0xD4 // MARGIN [Domain]

// Section 3.2.1: Queue State (0xD4 -> 0x68)
#define ERR_QUEUE_STATE                 0x68 // ASYMMETRY [Section]
#define ERR_QUEUE_EMPTY                 0x30 // ASYMMETRY [Leaf] Queue empty
#define ERR_QUEUE_FULL                  0x31 // ASYMMETRY [Leaf] Queue full
#define ERR_QUEUE_OVERFLOW              0xB0 // ASYMMETRY [Leaf] Overflow
#define ERR_QUEUE_UNDERFLOW             0xB1 // MARGIN [Leaf] Underflow

// Section 3.2.2: Queue Operations (0xD4 -> 0x69)
#define ERR_QUEUE_OP                    0x69 // ANTICODE [Section]
#define ERR_QUEUE_ENQUEUE               0x32 // ASYMMETRY [Leaf] Enqueue failed
#define ERR_QUEUE_DEQUEUE               0x33 // MARGIN [Leaf] Dequeue failed
#define ERR_QUEUE_PEEK                  0xB2 // MARGIN [Leaf] Peek failed
#define ERR_QUEUE_CLEAR                 0xB3 // ASYMMETRY [Leaf] Clear failed

// Section 3.2.3: Ordering & Priority (0xD4 -> 0xE8)
#define ERR_QUEUE_ORDER                 0xE8 // MARGIN [Section]
#define ERR_ORDER_SEQUENCE              0x70 // ASYMMETRY [Leaf] Sequence error
#define ERR_ORDER_PRIORITY              0x71 // MARGIN [Leaf] Priority error
#define ERR_ORDER_DUPLICATE             0xF0 // ANTICODE [Leaf] Duplicate entry
#define ERR_ORDER_CONFLICT              0xF1 // ASYMMETRY [Leaf] Order conflict

// Section 3.2.4: Collection State (0xD4 -> 0xE9)
#define ERR_COLLECTION_STATE            0xE9 // ASYMMETRY [Section]
#define ERR_COLL_LOCKED                 0x72 // MARGIN [Leaf] Collection locked
#define ERR_COLL_MODIFIED               0x73 // ASYMMETRY [Leaf] Modified during iteration
#define ERR_COLL_INDEX                  0xF2 // ASYMMETRY [Leaf] Index out of bounds
#define ERR_COLL_KEY                    0xF3 // ASYMMETRY [Leaf] Key not found

// -----------------------------------------------------------------------------
// DOMAIN 3.3: MATH & COMPUTATION (0xAA -> 0xD5)
// Covers: Arithmetic, floating point, logic, crypto primitives
// -----------------------------------------------------------------------------
#define ERR_MATH_DOM                    0xD5 // ASYMMETRY [Domain]

// Section 3.3.1: Arithmetic (0xD5 -> 0x6A)
#define ERR_MATH_ARITH                  0x6A // MARGIN [Section]
#define ERR_ARITH_DIV_ZERO              0x34 // ASYMMETRY [Leaf] Division by zero
#define ERR_ARITH_OVERFLOW              0x35 // MARGIN [Leaf] Overflow
#define ERR_ARITH_UNDERFLOW             0xB4 // ANTICODE [Leaf] Underflow
#define ERR_ARITH_PRECISION             0xB5 // ASYMMETRY [Leaf] Precision loss

// Section 3.3.2: Floating Point (0xD5 -> 0x6B)
#define ERR_MATH_FLOAT                  0x6B // ASYMMETRY [Section]
#define ERR_FLOAT_NAN                   0x36 // MARGIN [Leaf] Result is NaN
#define ERR_FLOAT_INF                   0x37 // ASYMMETRY [Leaf] Result is infinity
#define ERR_FLOAT_DENORM                0xB6 // ASYMMETRY [Leaf] Denormalized
#define ERR_FLOAT_INEXACT               0xB7 // ASYMMETRY [Leaf] Inexact result

// Section 3.3.3: Logic & Assertions (0xD5 -> 0xEA)
#define ERR_MATH_LOGIC                  0xEA // ASYMMETRY [Section]
#define ERR_LOGIC_ASSERT                0x74 // MARGIN [Leaf] Assertion failed
#define ERR_LOGIC_INVARIANT             0x75 // ASYMMETRY [Leaf] Invariant broken
#define ERR_LOGIC_UNREACH               0xF4 // ASYMMETRY [Leaf] Unreachable code
#define ERR_LOGIC_IMPOSSIBLE            0xF5 // ASYMMETRY [Leaf] Impossible state

// Section 3.3.4: Cryptographic Operations (0xD5 -> 0xEB)
#define ERR_MATH_CRYPTO                 0xEB // ASYMMETRY [Section]
#define ERR_CRYPTO_KEY                  0x76 // ASYMMETRY [Leaf] Invalid key
#define ERR_CRYPTO_IV                   0x77 // REPEATER [Leaf] Invalid IV
#define ERR_CRYPTO_TAG                  0xF6 // ASYMMETRY [Leaf] Tag verification
#define ERR_CRYPTO_PADDING              0xF7 // IMPULSE [Leaf] Padding error

// =============================================================================
// QUADRANT 4: RUN / FATAL / SECURITY / CRITICAL
// ROOT: 0xFF (The End / Saturation / Panic)
// Covers: Security, deadlock, hardware failures, critical system errors
// =============================================================================

// ROOT: 0xFF
#define ERR_FATAL                       ERR_FINALIZED // Fatal system failure

// -----------------------------------------------------------------------------
// DOMAIN 4.1: STORAGE & FILESYSTEM (0xFF -> 0x7E)
// Covers: File operations, volumes, block devices, inodes
// -----------------------------------------------------------------------------
#define ERR_STORAGE_DOM                 0x7E // ASYMMETRY [Domain]

// Section 4.1.1: File Operations (0x7E -> 0x3C)
#define ERR_STORAGE_FILE                0x3C // ANTICODE [Section]
#define ERR_FILE_NOT_FOUND              0x18 // ASYMMETRY [Leaf] File not found
#define ERR_FILE_EXISTS                 0x19 // ASYMMETRY [Leaf] File exists
#define ERR_FILE_TOO_LARGE              0x98 // ASYMMETRY [Leaf] File too large
#define ERR_FILE_CORRUPTED              0x99 // MARGIN [Leaf] File corrupted

// Section 4.1.2: File Attributes (0x7E -> 0x3D)
#define ERR_STORAGE_ATTR                0x3D // ASYMMETRY [Section]
#define ERR_ATTR_READONLY               0x1A // ASYMMETRY [Leaf] Read-only
#define ERR_ATTR_HIDDEN                 0x1B // MARGIN [Leaf] Hidden
#define ERR_ATTR_SYSTEM                 0x9A // MARGIN [Leaf] System file
#define ERR_ATTR_DIRECTORY              0x9B // ASYMMETRY [Leaf] Is directory

// Section 4.1.3: Volume & Mount (0x7E -> 0xBC)
#define ERR_STORAGE_VOLUME              0xBC // ASYMMETRY [Section]
#define ERR_VOL_NOT_MOUNTED             0x58 // ASYMMETRY [Leaf] Not mounted
#define ERR_VOL_BUSY                    0x59 // MARGIN [Leaf] Volume busy
#define ERR_VOL_FULL                    0xD8 // MARGIN [Leaf] Volume full
#define ERR_VOL_CORRUPTED               0xD9 // ASYMMETRY [Leaf] Corrupted

// Section 4.1.4: Block Device (0x7E -> 0xBD)
#define ERR_STORAGE_BLOCK               0xBD // ASYMMETRY [Section]
#define ERR_BLK_READ_ERROR              0x5A // ANTICODE [Leaf] Read error
#define ERR_BLK_WRITE_ERROR             0x5B // ASYMMETRY [Leaf] Write error
#define ERR_BLK_BAD_SECTOR              0xDA // ASYMMETRY [Leaf] Bad sector
#define ERR_BLK_HARDWARE                0xDB // ASYMMETRY [Leaf] Hardware error

// -----------------------------------------------------------------------------
// DOMAIN 4.2: SECURITY & AUTHENTICATION (0xFF -> 0x7F)
// Covers: Authentication, encryption, policy, auditing
// -----------------------------------------------------------------------------
#define ERR_SEC_DOM                     0x7F // IMPULSE [Domain]

// Section 4.2.1: Authentication (0x7F -> 0x3E)
#define ERR_SEC_AUTH                    0x3E // ASYMMETRY [Section]
#define ERR_AUTH_FAILED                 0x1C // ASYMMETRY [Leaf] Auth failed
#define ERR_AUTH_EXPIRED                0x1D // MARGIN [Leaf] Token expired
#define ERR_AUTH_REVOKED                0x9C // MARGIN [Leaf] Credentials revoked
#define ERR_AUTH_INSUFFICIENT           0x9D // ASYMMETRY [Leaf] Insufficient rights

// Section 4.2.2: Encryption (0x7F -> 0x3F)
#define ERR_SEC_ENCRYPT                 0x3F // ASYMMETRY [Section]
#define ERR_ENCRYPT_FAILED              0x1E // ANTICODE [Leaf] Encryption failed
#define ERR_DECRYPT_FAILED              0x1F // ASYMMETRY [Leaf] Decryption failed
#define ERR_ENCRYPT_KEY_INVALID         0x9E // ASYMMETRY [Leaf] Invalid key
#define ERR_ENCRYPT_ALGO                0x9F // ASYMMETRY [Leaf] Unsupported algo

// Section 4.2.3: Policy & Access Control (0x7F -> 0xBE)
#define ERR_SEC_POLICY                  0xBE // ASYMMETRY [Section]
#define ERR_POLICY_VIOLATION            0x5C // MARGIN [Leaf] Policy violation
#define ERR_POLICY_DENY                 0x5D // ASYMMETRY [Leaf] Explicit deny
#define ERR_POLICY_QUOTA                0xDC // ASYMMETRY [Leaf] Quota exceeded
#define ERR_POLICY_RATE_LIMIT           0xDD // REPEATER [Leaf] Rate limited

// Section 4.2.4: Auditing & Logging (0x7F -> 0xBF)
#define ERR_SEC_AUDIT                   0xBF // IMPULSE [Section]
#define ERR_AUDIT_FAILED                0x5E // ASYMMETRY [Leaf] Audit failed
#define ERR_AUDIT_FULL                  0x5F // ASYMMETRY [Leaf] Audit log full
#define ERR_AUDIT_TAMPER                0xDE // ASYMMETRY [Leaf] Tamper detected
#define ERR_AUDIT_INTEGRITY             0xDF // IMPULSE [Leaf] Integrity fail

// -----------------------------------------------------------------------------
// DOMAIN 4.3: CONCURRENCY & DEADLOCK (0xFF -> 0xFE)
// Covers: Locks, IPC, atomics, race conditions, critical sections
// -----------------------------------------------------------------------------
#define ERR_DEADLOCK                    0xFE // IMPULSE [Domain]

// Section 4.3.1: Lock Operations (0xFE -> 0x7C)
#define ERR_LOCK_STATE                  0x7C // ASYMMETRY [Section]
#define ERR_LOCK_FAILED                 0x38 // ASYMMETRY [Leaf] Lock failed
#define ERR_LOCK_TIMEOUT                0x39 // MARGIN [Leaf] Lock timeout
#define ERR_LOCK_DEADLOCK               0xB8 // MARGIN [Leaf] Deadlock detected
#define ERR_LOCK_OWNER                  0xB9 // ASYMMETRY [Leaf] Wrong owner

// Section 4.3.2: IPC & Message Passing (0xFE -> 0x7D)
#define ERR_LOCK_IPC                    0x7D // ASYMMETRY [Section]
#define ERR_PIPE_BROKEN                 0x3A // MARGIN [Leaf] Broken pipe
#define ERR_PIPE_FULL                   0x3B // ASYMMETRY [Leaf] Pipe Full
#define ERR_MSG_SIZE                    0xBA // ASYMMETRY [Leaf] Message Too Large
#define ERR_MSG_QUEUE                   0xBB // REPEATER [Leaf] Queue Destroyed

// Section 4.3.3: Atomic Operations (0xFE -> 0xFC)
#define ERR_LOCK_ATOMIC                 0xFC // ASYMMETRY [Section]
#define ERR_ATOMIC_FAILED               0x78 // ANTICODE [Leaf] Atomic op failed
#define ERR_ATOMIC_RACE                 0x79 // ASYMMETRY [Leaf] Race detected
#define ERR_ATOMIC_MEMORY_ORDER         0xF8 // ASYMMETRY [Leaf] Memory order
#define ERR_ATOMIC_CONTENTION           0xF9 // ASYMMETRY [Leaf] High contention

// Section 4.3.4: Critical Sections (0xFE -> 0xFD)
#define ERR_LOCK_CRITICAL               0xFD // IMPULSE [Section]
#define ERR_CRIT_ENTER                  0x7A // ASYMMETRY [Leaf] Enter failed
#define ERR_CRIT_EXIT                   0x7B // ASYMMETRY [Leaf] Exit failed
#define ERR_CRIT_NESTED                 0xFA // ASYMMETRY [Leaf] Nested overflow
#define ERR_CRIT_ABANDONED              0xFB // IMPULSE [Leaf] Abandoned

#endif // __ERRORS_H__
#ifndef __ERRORS_H__
#define __ERRORS_H__

/*
 * ONTOLOGICAL 8-BIT ERROR TAXONOMY v0.2
 * -----------------------------------------------------------------------------
 * Geometry:  16x16 Matrix (0..255) mapped to 4 Attractor Basins.
 * Logic:     Convergence via ((Child << 1) & 0xF0) | ((Child >>> 1) & 0x0F)
 * -----------------------------------------------------------------------------
 */

#include <stdint.h>

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
#define ERR_OK                          ERR_SUCCESS // Success state

// -----------------------------------------------------------------------------
// DOMAIN 1.1: SYSTEM STATE & ACCESS CONTROL (0x00 -> 0x01)
// Covers: Permissions, handles, descriptors, processes
// -----------------------------------------------------------------------------
#define ERR_SYS_STATE                   0x01 // UNBALANCED_EDGE [Domain]

// Section 1.1.1: Access & Permissions (0x01 -> 0x02)
#define ERR_SYS_ACCESS                  0x02 // UNBALANCED_EDGE [Section]
#define ERR_ACCESS_DENIED               0x04 // UNBALANCED_EDGE [Leaf] Access denied
#define ERR_ACCESS_READONLY             0x05 // UNBALANCED_OTHER [Leaf] Write to readonly
#define ERR_ACCESS_LOCKED               0x84 // UNBALANCED_OTHER [Leaf] Resource locked
#define ERR_ACCESS_FORBIDDEN            0x85 // UNBALANCED_OTHER [Leaf] Forbidden operation

// Section 1.1.2: Handles & Descriptors (0x01 -> 0x03)
#define ERR_SYS_HANDLE                  0x03 // UNBALANCED_OTHER [Section]
#define ERR_HANDLE_INVALID              0x06 // UNBALANCED_OTHER [Leaf] Invalid handle
#define ERR_HANDLE_CLOSED               0x07 // UNBALANCED_OTHER [Leaf] Handle already closed
#define ERR_HANDLE_TYPE                 0x86 // UNBALANCED_OTHER [Leaf] Wrong handle type
#define ERR_HANDLE_SHADOW               0x87 // BALANCED_SHADOW [Leaf] Stale/ghost handle

// Section 1.1.3: Process Management (0x01 -> 0x82)
#define ERR_SYS_PROC                    0x82 // UNBALANCED_OTHER [Section]
#define ERR_PROC_CANCELLED              0x44 // UNBALANCED_TWIN [Leaf] Operation cancelled
#define ERR_PROC_KILLED                 0x45 // UNBALANCED_OTHER [Leaf] Process killed
#define ERR_PROC_ORPHAN                 0xC4 // UNBALANCED_OTHER [Leaf] Orphaned process
#define ERR_PROC_ZOMBIE                 0xC5 // BALANCED_EDGE [Leaf] Zombie process

// Section 1.1.4: Arguments & Validation (0x01 -> 0x83)
#define ERR_SYS_ARGUMENT                0x83 // UNBALANCED_OTHER [Section]
#define ERR_ARG_NULL                    0x46 // UNBALANCED_OTHER [Leaf] NULL argument
#define ERR_ARG_INVALID                 0x47 // BALANCED_EDGE [Leaf] Invalid argument
#define ERR_ARG_RANGE                   0xC6 // BALANCED_EDGE [Leaf] Arg out of range
#define ERR_ARG_TYPE                    0xC7 // UNBALANCED_OTHER [Leaf] Type mismatch

// -----------------------------------------------------------------------------
// DOMAIN 1.2: MEMORY & BUFFERS (0x00 -> 0x80)
// Covers: Heap, stack, buffers, alignment, paging
// -----------------------------------------------------------------------------
#define ERR_MEM_DOM                     0x80 // UNBALANCED_EDGE [Domain]

// Section 1.2.1: Heap Allocation (0x80 -> 0x40)
#define ERR_MEM_HEAP                    0x40 // UNBALANCED_EDGE [Section]
#define ERR_HEAP_OOM                    0x20 // UNBALANCED_EDGE [Leaf] Out of memory
#define ERR_HEAP_FRAGMENT               0x21 // UNBALANCED_OTHER [Leaf] Fragmented
#define ERR_HEAP_CORRUPT                0xA0 // UNBALANCED_OTHER [Leaf] Heap corrupted
#define ERR_HEAP_DOUBLE_FREE            0xA1 // UNBALANCED_OTHER [Leaf] Double free

// Section 1.2.2: Stack & Bounds (0x80 -> 0x41)
#define ERR_MEM_BOUNDS                  0x41 // UNBALANCED_OTHER [Section]
#define ERR_STACK_OVERFLOW              0x22 // UNBALANCED_TWIN [Leaf] Stack overflow
#define ERR_STACK_UNDERFLOW             0x23 // UNBALANCED_OTHER [Leaf] Stack underflow
#define ERR_BOUNDS_LOWER                0xA2 // UNBALANCED_OTHER [Leaf] Lower bound
#define ERR_BOUNDS_UPPER                0xA3 // BALANCED_EDGE [Leaf] Upper bound

// Section 1.2.3: Buffers (0x80 -> 0xC0)
#define ERR_MEM_BUFFER                  0xC0 // UNBALANCED_OTHER [Section]
#define ERR_BUF_OVERFLOW                0x60 // UNBALANCED_OTHER [Leaf] Buffer overflow
#define ERR_BUF_UNDERFLOW               0x61 // UNBALANCED_OTHER [Leaf] Buffer underflow
#define ERR_BUF_FULL                    0xE0 // UNBALANCED_OTHER [Leaf] Buffer full
#define ERR_BUF_EMPTY                   0xE1 // BALANCED_SHADOW [Leaf] Buffer empty

// Section 1.2.4: Alignment & Paging (0x80 -> 0xC1)
#define ERR_MEM_ALIGN                   0xC1 // UNBALANCED_OTHER [Section]
#define ERR_ALIGN_ADDR                  0x62 // UNBALANCED_OTHER [Leaf] Misaligned address
#define ERR_ALIGN_SIZE                  0x63 // BALANCED_EDGE [Leaf] Misaligned size
#define ERR_PAGE_FAULT                  0xE2 // BALANCED_EDGE [Leaf] Page fault
#define ERR_PAGE_PROTECT                0xE3 // UNBALANCED_OTHER [Leaf] Page protection

// -----------------------------------------------------------------------------
// DOMAIN 1.3: LIFECYCLE & INITIALIZATION (0x00 -> 0x81)
// Covers: Init sequences, state transitions, reference counting, cleanup
// -----------------------------------------------------------------------------
#define ERR_LIFE_DOM                    0x81 // UNBALANCED_OTHER [Domain]

// Section 1.3.1: Initialization (0x81 -> 0x42)
#define ERR_LIFE_INIT                   0x42 // UNBALANCED_OTHER [Section]
#define ERR_INIT_FAILED                 0x24 // UNBALANCED_OTHER [Leaf] Init failed
#define ERR_INIT_TIMEOUT                0x25 // UNBALANCED_OTHER [Leaf] Init timeout
#define ERR_INIT_DEPENDENCY             0xA4 // UNBALANCED_OTHER [Leaf] Missing dependency
#define ERR_INIT_NOSUP                  0xA5 // BALANCED_SHADOW [Leaf] Already initialized

// Section 1.3.2: State Transitions (0x81 -> 0x43)
#define ERR_LIFE_STATE                  0x43 // UNBALANCED_OTHER [Section]
#define ERR_STATE_INVALID               0x26 // UNBALANCED_OTHER [Leaf] Invalid state
#define ERR_STATE_TRANSITION            0x27 // BALANCED_EDGE [Leaf] Bad transition
#define ERR_STATE_LOCKED                0xA6 // BALANCED_EDGE [Leaf] State locked
#define ERR_STATE_FROZEN                0xA7 // UNBALANCED_OTHER [Leaf] State frozen

// Section 1.3.3: Reference Counting (0x81 -> 0xC2)
#define ERR_LIFE_REF                    0xC2 // UNBALANCED_OTHER [Section]
#define ERR_REF_ZERO                    0x64 // UNBALANCED_OTHER [Leaf] Ref count zero
#define ERR_REF_OVERFLOW                0x65 // BALANCED_EDGE [Leaf] Ref count overflow
#define ERR_REF_LEAK                    0xE4 // BALANCED_EDGE [Leaf] Memory leak
#define ERR_REF_DANGLING                0xE5 // UNBALANCED_OTHER [Leaf] Dangling pointer

// Section 1.3.4: Cleanup & Finalization (0x81 -> 0xC3)
#define ERR_LIFE_CLEAN                  0xC3 // BALANCED_SHADOW [Section]
#define ERR_CLEAN_FAILED                0x66 // BALANCED_EDGE [Leaf] Cleanup failed
#define ERR_CLEAN_PARTIAL               0x67 // UNBALANCED_OTHER [Leaf] Partial cleanup
#define ERR_CLEAN_BUSY                  0xE6 // UNBALANCED_OTHER [Leaf] Resource busy
#define ERR_CLEAN_LEAKED                0xE7 // UNBALANCED_OTHER [Leaf] Resource leaked

// =============================================================================
// QUADRANT 2: BEFORE / EXTERNAL I/O / HARDWARE
// ROOT: 0x55 (The Oscillation / The Wire / External Events)
// Covers: Network, peripherals, timing, interrupts, external communication
// =============================================================================

// ROOT: 0x55
#define ERR_IO_BUSY                     0x55 // BALANCED_ROOT [Root] Resource busy

// -----------------------------------------------------------------------------
// DOMAIN 2.1: NETWORK & CONNECTIVITY (0x55 -> 0x2A)
// Covers: Sockets, DNS, protocols, transport layer
// -----------------------------------------------------------------------------
#define ERR_NET_DOM                     0x2A // UNBALANCED_OTHER [Domain]

// Section 2.1.1: Sockets & Connections (0x2A -> 0x14)
#define ERR_NET_SOCK                    0x14 // UNBALANCED_OTHER [Section]
#define ERR_SOCK_CREATE                 0x08 // UNBALANCED_EDGE [Leaf] Socket create failed
#define ERR_SOCK_BIND                   0x09 // UNBALANCED_OTHER [Leaf] Bind failed
#define ERR_SOCK_LISTEN                 0x88 // UNBALANCED_TWIN [Leaf] Listen failed
#define ERR_SOCK_ACCEPT                 0x89 // UNBALANCED_OTHER [Leaf] Accept failed

// Section 2.1.2: Connection State (0x2A -> 0x15)
#define ERR_NET_CONN                    0x15 // UNBALANCED_OTHER [Section]
#define ERR_CONN_REFUSED                0x0A // UNBALANCED_OTHER [Leaf] Connection refused
#define ERR_CONN_RESET                  0x0B // UNBALANCED_OTHER [Leaf] Connection reset
#define ERR_CONN_CLOSED                 0x8A // UNBALANCED_OTHER [Leaf] Connection closed
#define ERR_CONN_TIMEOUT                0x8B // BALANCED_EDGE [Leaf] Connection timeout

// Section 2.1.3: DNS & Resolution (0x2A -> 0x94)
#define ERR_NET_DNS                     0x94 // UNBALANCED_OTHER [Section]
#define ERR_DNS_NXDOMAIN                0x48 // UNBALANCED_OTHER [Leaf] Domain not found
#define ERR_DNS_TIMEOUT                 0x49 // UNBALANCED_OTHER [Leaf] Resolution timeout
#define ERR_DNS_SERVFAIL                0xC8 // UNBALANCED_OTHER [Leaf] Server failure
#define ERR_DNS_CONFIG                  0xC9 // BALANCED_EDGE [Leaf] DNS config error

// Section 2.1.4: Protocol Layer (0x2A -> 0x95)
#define ERR_NET_PROTO                   0x95 // BALANCED_EDGE [Section]
#define ERR_PROTO_VERSION               0x4A // UNBALANCED_OTHER [Leaf] Protocol version
#define ERR_PROTO_FORMAT                0x4B // BALANCED_SHADOW [Leaf] Format error
#define ERR_PROTO_SEQUENCE              0xCA // BALANCED_EDGE [Leaf] Sequence error
#define ERR_PROTO_STATE                 0xCB // UNBALANCED_OTHER [Leaf] Protocol state

// -----------------------------------------------------------------------------
// DOMAIN 2.2: TIMING & SIGNALS (0x55 -> 0x2B)
// Covers: Clocks, timers, interrupts, synchronization, watchdogs
// -----------------------------------------------------------------------------
#define ERR_TIME_DOM                    0x2B // BALANCED_EDGE [Domain]

// Section 2.2.1: Interrupts (0x2B -> 0x16)
#define ERR_TIME_IRQ                    0x16 // UNBALANCED_OTHER [Section]
#define ERR_IRQ_DISABLED                0x0C // UNBALANCED_OTHER [Leaf] IRQ disabled
#define ERR_IRQ_PENDING                 0x0D // UNBALANCED_OTHER [Leaf] IRQ pending
#define ERR_IRQ_NESTED                  0x8C // UNBALANCED_OTHER [Leaf] Nested overflow
#define ERR_IRQ_PRIORITY                0x8D // BALANCED_EDGE [Leaf] Priority error

// Section 2.2.2: Clocks & Timers (0x2B -> 0x17)
#define ERR_TIME_CLOCK                  0x17 // BALANCED_EDGE [Section]
#define ERR_CLK_NOT_READY               0x0E // UNBALANCED_OTHER [Leaf] Clock not ready
#define ERR_CLK_UNSTABLE                0x0F // BALANCED_SHADOW [Leaf] Clock unstable
#define ERR_CLK_EXPIRED                 0x8E // BALANCED_EDGE [Leaf] Timer expired
#define ERR_CLK_OVERFLOW                0x8F // UNBALANCED_OTHER [Leaf] Timer overflow

// Section 2.2.3: Synchronization (0x2B -> 0x96)
#define ERR_TIME_SYNC                   0x96 // BALANCED_SHADOW [Section]
#define ERR_SYNC_FAILED                 0x4C // UNBALANCED_OTHER [Leaf] Sync failed
#define ERR_SYNC_TIMEOUT                0x4D // BALANCED_EDGE [Leaf] Sync timeout
#define ERR_SYNC_BARRIER                0xCC // BALANCED_EDGE [Leaf] Barrier broken
#define ERR_SYNC_LOST                   0xCD // UNBALANCED_OTHER [Leaf] Sync lost

// Section 2.2.4: Watchdog (0x2B -> 0x97)
#define ERR_TIME_WDT                    0x97 // UNBALANCED_OTHER [Section]
#define ERR_WDT_EXPIRED                 0x4E // BALANCED_EDGE [Leaf] Watchdog expired
#define ERR_WDT_RESET                   0x4F // UNBALANCED_OTHER [Leaf] Watchdog reset
#define ERR_WDT_EARLY                   0xCE // UNBALANCED_OTHER [Leaf] Early kick
#define ERR_WDT_CONFIG                  0xCF // UNBALANCED_OTHER [Leaf] WDT config error

// -----------------------------------------------------------------------------
// DOMAIN 2.3: PERIPHERALS & DRIVERS (0x55 -> 0xAB)
// Covers: Serial (UART/USB), buses (I2C/SPI), GPIO, analog I/O
// -----------------------------------------------------------------------------
#define ERR_IO_DOM                      0xAB // UNBALANCED_OTHER [Domain]

// Section 2.3.1: Serial Communication (0xAB -> 0x56)
#define ERR_IO_SERIAL                   0x56 // BALANCED_EDGE [Section]
#define ERR_SERIAL_BAUD                 0x2C // UNBALANCED_OTHER [Leaf] Baud rate error
#define ERR_SERIAL_FRAME                0x2D // BALANCED_SHADOW [Leaf] Frame error
#define ERR_SERIAL_PARITY               0xAC // BALANCED_EDGE [Leaf] Parity error
#define ERR_SERIAL_OVERRUN              0xAD // UNBALANCED_OTHER [Leaf] Overrun

// Section 2.3.2: Bus Communication (0xAB -> 0x57)
#define ERR_IO_BUS                      0x57 // UNBALANCED_OTHER [Section]
#define ERR_BUS_NACK                    0x2E // BALANCED_EDGE [Leaf] NACK received
#define ERR_BUS_ARBITRATION             0x2F // UNBALANCED_OTHER [Leaf] Arbitration lost
#define ERR_BUS_TIMEOUT                 0xAE // UNBALANCED_OTHER [Leaf] Bus timeout
#define ERR_BUS_ERROR                   0xAF // UNBALANCED_OTHER [Leaf] Bus error

// Section 2.3.3: Analog I/O (0xAB -> 0xD6)
#define ERR_IO_ANALOG                   0xD6 // UNBALANCED_OTHER [Section]
#define ERR_ADC_SATURATED               0x6C // BALANCED_EDGE [Leaf] ADC saturated
#define ERR_ADC_TIMEOUT                 0x6D // UNBALANCED_OTHER [Leaf] ADC timeout
#define ERR_DAC_UNDERRUN                0xEC // UNBALANCED_OTHER [Leaf] DAC underrun
#define ERR_DAC_CONFIG                  0xED // UNBALANCED_OTHER [Leaf] DAC config

// Section 2.3.4: GPIO & Pins (0xAB -> 0xD7)
#define ERR_IO_GPIO                     0xD7 // UNBALANCED_OTHER [Section]
#define ERR_GPIO_CONFIG                 0x6E // UNBALANCED_OTHER [Leaf] Config error
#define ERR_GPIO_LOCKED                 0x6F // UNBALANCED_OTHER [Leaf] Pin locked
#define ERR_GPIO_STATE                  0xEE // UNBALANCED_TWIN [Leaf] Invalid state
#define ERR_GPIO_INTERRUPT              0xEF // UNBALANCED_EDGE [Leaf] IRQ error

// =============================================================================
// QUADRANT 3: AFTER / DATA / LOGIC
// ROOT: 0xAA (The Pattern / The Math / Internal Processing)
// Covers: Parsing, encoding, validation, math, queues, storage
// =============================================================================

// ROOT: 0xAA
#define ERR_DATA_ROOT                   0xAA // BALANCED_ROOT [Root] Data error

// -----------------------------------------------------------------------------
// DOMAIN 3.1: PARSING & ENCODING (0xAA -> 0x54)
// Covers: Text encoding, structured data (JSON/XML), binary formats
// -----------------------------------------------------------------------------
#define ERR_PARSE_DOM                   0x54 // UNBALANCED_OTHER [Domain]

// Section 3.1.1: Text & Encoding (0x54 -> 0x28)
#define ERR_PARSE_TEXT                  0x28 // UNBALANCED_OTHER [Section]
#define ERR_TEXT_ENCODE                 0x10 // UNBALANCED_EDGE [Leaf] Encoding error
#define ERR_TEXT_DECODE                 0x11 // UNBALANCED_TWIN [Leaf] Decode error
#define ERR_TEXT_TRUNC                  0x90 // UNBALANCED_OTHER [Leaf] Truncated
#define ERR_TEXT_INVALID                0x91 // UNBALANCED_OTHER [Leaf] Invalid text

// Section 3.1.2: Structured Data (0x54 -> 0x29)
#define ERR_PARSE_STRUCT                0x29 // UNBALANCED_OTHER [Section]
#define ERR_STRUCT_SYNTAX               0x12 // UNBALANCED_OTHER [Leaf] JSON/YAML syntax
#define ERR_STRUCT_TYPE                 0x13 // UNBALANCED_OTHER [Leaf] JSON/YAML type error
#define ERR_STRUCT_MALFORMED            0x92 // UNBALANCED_OTHER [Leaf] JSON/XML malformed
#define ERR_STRUCT_NAMESPACE            0x93 // BALANCED_EDGE [Leaf] JSON/XML namespace

// Section 3.1.3: Binary Formats (0x54 -> 0xA8)
#define ERR_PARSE_BINARY                0xA8 // UNBALANCED_OTHER [Section]
#define ERR_BIN_HEADER                  0x50 // UNBALANCED_OTHER [Leaf] Bad header
#define ERR_BIN_MAGIC                   0x51 // UNBALANCED_OTHER [Leaf] Magic mismatch
#define ERR_BIN_VERSION                 0xD0 // UNBALANCED_OTHER [Leaf] Version mismatch
#define ERR_BIN_CHECKSUM                0xD1 // BALANCED_EDGE [Leaf] Checksum fail

// Section 3.1.4: Validation (0x54 -> 0xA9)
#define ERR_PARSE_VALID                 0xA9 // BALANCED_EDGE [Section]
#define ERR_VALID_SCHEMA                0x52 // UNBALANCED_OTHER [Leaf] Schema violation
#define ERR_VALID_CONSTRAINT            0x53 // BALANCED_EDGE [Leaf] Constraint fail
#define ERR_VALID_PATTERN               0xD2 // BALANCED_SHADOW [Leaf] Pattern mismatch
#define ERR_VALID_REQUIRED              0xD3 // UNBALANCED_OTHER [Leaf] Required missing

// -----------------------------------------------------------------------------
// DOMAIN 3.2: QUEUES & COLLECTIONS (0xAA -> 0xD4)
// Covers: Queue operations, ordering, collection state
// -----------------------------------------------------------------------------
#define ERR_QUEUE_DOM                   0xD4 // BALANCED_EDGE [Domain]

// Section 3.2.1: Queue State (0xD4 -> 0x68)
#define ERR_QUEUE_STATE                 0x68 // UNBALANCED_OTHER [Section]
#define ERR_QUEUE_EMPTY                 0x30 // UNBALANCED_OTHER [Leaf] Queue empty
#define ERR_QUEUE_FULL                  0x31 // UNBALANCED_OTHER [Leaf] Queue full
#define ERR_QUEUE_OVERFLOW              0xB0 // UNBALANCED_OTHER [Leaf] Overflow
#define ERR_QUEUE_UNDERFLOW             0xB1 // BALANCED_EDGE [Leaf] Underflow

// Section 3.2.2: Queue Operations (0xD4 -> 0x69)
#define ERR_QUEUE_OP                    0x69 // BALANCED_SHADOW [Section]
#define ERR_QUEUE_ENQUEUE               0x32 // UNBALANCED_OTHER [Leaf] Enqueue failed
#define ERR_QUEUE_DEQUEUE               0x33 // BALANCED_EDGE [Leaf] Dequeue failed
#define ERR_QUEUE_PEEK                  0xB2 // BALANCED_EDGE [Leaf] Peek failed
#define ERR_QUEUE_CLEAR                 0xB3 // UNBALANCED_OTHER [Leaf] Clear failed

// Section 3.2.3: Ordering & Priority (0xD4 -> 0xE8)
#define ERR_QUEUE_ORDER                 0xE8 // BALANCED_EDGE [Section]
#define ERR_ORDER_SEQUENCE              0x70 // UNBALANCED_OTHER [Leaf] Sequence error
#define ERR_ORDER_PRIORITY              0x71 // BALANCED_EDGE [Leaf] Priority error
#define ERR_ORDER_DUPLICATE             0xF0 // BALANCED_SHADOW [Leaf] Duplicate entry
#define ERR_ORDER_CONFLICT              0xF1 // UNBALANCED_OTHER [Leaf] Order conflict

// Section 3.2.4: Collection State (0xD4 -> 0xE9)
#define ERR_COLLECTION_STATE            0xE9 // UNBALANCED_OTHER [Section]
#define ERR_COLL_LOCKED                 0x72 // BALANCED_EDGE [Leaf] Collection locked
#define ERR_COLL_MODIFIED               0x73 // UNBALANCED_OTHER [Leaf] Modified during iteration
#define ERR_COLL_INDEX                  0xF2 // UNBALANCED_OTHER [Leaf] Index out of bounds
#define ERR_COLL_KEY                    0xF3 // UNBALANCED_OTHER [Leaf] Key not found

// -----------------------------------------------------------------------------
// DOMAIN 3.3: MATH & COMPUTATION (0xAA -> 0xD5)
// Covers: Arithmetic, floating point, logic, crypto primitives
// -----------------------------------------------------------------------------
#define ERR_MATH_DOM                    0xD5 // UNBALANCED_OTHER [Domain]

// Section 3.3.1: Arithmetic (0xD5 -> 0x6A)
#define ERR_MATH_ARITH                  0x6A // BALANCED_EDGE [Section]
#define ERR_ARITH_DIV_ZERO              0x34 // UNBALANCED_OTHER [Leaf] Division by zero
#define ERR_ARITH_OVERFLOW              0x35 // BALANCED_EDGE [Leaf] Overflow
#define ERR_ARITH_UNDERFLOW             0xB4 // BALANCED_SHADOW [Leaf] Underflow
#define ERR_ARITH_PRECISION             0xB5 // UNBALANCED_OTHER [Leaf] Precision loss

// Section 3.3.2: Floating Point (0xD5 -> 0x6B)
#define ERR_MATH_FLOAT                  0x6B // UNBALANCED_OTHER [Section]
#define ERR_FLOAT_NAN                   0x36 // BALANCED_EDGE [Leaf] Result is NaN
#define ERR_FLOAT_INF                   0x37 // UNBALANCED_OTHER [Leaf] Result is infinity
#define ERR_FLOAT_DENORM                0xB6 // UNBALANCED_OTHER [Leaf] Denormalized
#define ERR_FLOAT_INEXACT               0xB7 // UNBALANCED_OTHER [Leaf] Inexact result

// Section 3.3.3: Logic & Assertions (0xD5 -> 0xEA)
#define ERR_MATH_LOGIC                  0xEA // UNBALANCED_OTHER [Section]
#define ERR_LOGIC_ASSERT                0x74 // BALANCED_EDGE [Leaf] Assertion failed
#define ERR_LOGIC_INVARIANT             0x75 // UNBALANCED_OTHER [Leaf] Invariant broken
#define ERR_LOGIC_UNREACH               0xF4 // UNBALANCED_OTHER [Leaf] Unreachable code
#define ERR_LOGIC_IMPOSSIBLE            0xF5 // UNBALANCED_OTHER [Leaf] Impossible state

// Section 3.3.4: Cryptographic Operations (0xD5 -> 0xEB)
#define ERR_MATH_CRYPTO                 0xEB // UNBALANCED_OTHER [Section]
#define ERR_CRYPTO_KEY                  0x76 // UNBALANCED_OTHER [Leaf] Invalid key
#define ERR_CRYPTO_IV                   0x77 // UNBALANCED_TWIN [Leaf] Invalid IV
#define ERR_CRYPTO_TAG                  0xF6 // UNBALANCED_OTHER [Leaf] Tag verification
#define ERR_CRYPTO_PADDING              0xF7 // UNBALANCED_EDGE [Leaf] Padding error

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
#define ERR_STORAGE_DOM                 0x7E // UNBALANCED_OTHER [Domain]

// Section 4.1.1: File Operations (0x7E -> 0x3C)
#define ERR_STORAGE_FILE                0x3C // BALANCED_SHADOW [Section]
#define ERR_FILE_NOT_FOUND              0x18 // UNBALANCED_OTHER [Leaf] File not found
#define ERR_FILE_EXISTS                 0x19 // UNBALANCED_OTHER [Leaf] File exists
#define ERR_FILE_TOO_LARGE              0x98 // UNBALANCED_OTHER [Leaf] File too large
#define ERR_FILE_CORRUPTED              0x99 // BALANCED_EDGE [Leaf] File corrupted

// Section 4.1.2: File Attributes (0x7E -> 0x3D)
#define ERR_STORAGE_ATTR                0x3D // UNBALANCED_OTHER [Section]
#define ERR_ATTR_READONLY               0x1A // UNBALANCED_OTHER [Leaf] Read-only
#define ERR_ATTR_HIDDEN                 0x1B // BALANCED_EDGE [Leaf] Hidden
#define ERR_ATTR_SYSTEM                 0x9A // BALANCED_EDGE [Leaf] System file
#define ERR_ATTR_DIRECTORY              0x9B // UNBALANCED_OTHER [Leaf] Is directory

// Section 4.1.3: Volume & Mount (0x7E -> 0xBC)
#define ERR_STORAGE_VOLUME              0xBC // UNBALANCED_OTHER [Section]
#define ERR_VOL_NOT_MOUNTED             0x58 // UNBALANCED_OTHER [Leaf] Not mounted
#define ERR_VOL_BUSY                    0x59 // BALANCED_EDGE [Leaf] Volume busy
#define ERR_VOL_FULL                    0xD8 // BALANCED_EDGE [Leaf] Volume full
#define ERR_VOL_CORRUPTED               0xD9 // UNBALANCED_OTHER [Leaf] Corrupted

// Section 4.1.4: Block Device (0x7E -> 0xBD)
#define ERR_STORAGE_BLOCK               0xBD // UNBALANCED_OTHER [Section]
#define ERR_BLK_READ_ERROR              0x5A // BALANCED_SHADOW [Leaf] Read error
#define ERR_BLK_WRITE_ERROR             0x5B // UNBALANCED_OTHER [Leaf] Write error
#define ERR_BLK_BAD_SECTOR              0xDA // UNBALANCED_OTHER [Leaf] Bad sector
#define ERR_BLK_HARDWARE                0xDB // UNBALANCED_OTHER [Leaf] Hardware error

// -----------------------------------------------------------------------------
// DOMAIN 4.2: SECURITY & AUTHENTICATION (0xFF -> 0x7F)
// Covers: Authentication, encryption, policy, auditing
// -----------------------------------------------------------------------------
#define ERR_SEC_DOM                     0x7F // UNBALANCED_EDGE [Domain]

// Section 4.2.1: Authentication (0x7F -> 0x3E)
#define ERR_SEC_AUTH                    0x3E // UNBALANCED_OTHER [Section]
#define ERR_AUTH_FAILED                 0x1C // UNBALANCED_OTHER [Leaf] Auth failed
#define ERR_AUTH_EXPIRED                0x1D // BALANCED_EDGE [Leaf] Token expired
#define ERR_AUTH_REVOKED                0x9C // BALANCED_EDGE [Leaf] Credentials revoked
#define ERR_AUTH_INSUFFICIENT           0x9D // UNBALANCED_OTHER [Leaf] Insufficient rights

// Section 4.2.2: Encryption (0x7F -> 0x3F)
#define ERR_SEC_ENCRYPT                 0x3F // UNBALANCED_OTHER [Section]
#define ERR_ENCRYPT_FAILED              0x1E // BALANCED_SHADOW [Leaf] Encryption failed
#define ERR_DECRYPT_FAILED              0x1F // UNBALANCED_OTHER [Leaf] Decryption failed
#define ERR_ENCRYPT_KEY_INVALID         0x9E // UNBALANCED_OTHER [Leaf] Invalid key
#define ERR_ENCRYPT_ALGO                0x9F // UNBALANCED_OTHER [Leaf] Unsupported algo

// Section 4.2.3: Policy & Access Control (0x7F -> 0xBE)
#define ERR_SEC_POLICY                  0xBE // UNBALANCED_OTHER [Section]
#define ERR_POLICY_VIOLATION            0x5C // BALANCED_EDGE [Leaf] Policy violation
#define ERR_POLICY_DENY                 0x5D // UNBALANCED_OTHER [Leaf] Explicit deny
#define ERR_POLICY_QUOTA                0xDC // UNBALANCED_OTHER [Leaf] Quota exceeded
#define ERR_POLICY_RATE_LIMIT           0xDD // UNBALANCED_TWIN [Leaf] Rate limited

// Section 4.2.4: Auditing & Logging (0x7F -> 0xBF)
#define ERR_SEC_AUDIT                   0xBF // UNBALANCED_EDGE [Section]
#define ERR_AUDIT_FAILED                0x5E // UNBALANCED_OTHER [Leaf] Audit failed
#define ERR_AUDIT_FULL                  0x5F // UNBALANCED_OTHER [Leaf] Audit log full
#define ERR_AUDIT_TAMPER                0xDE // UNBALANCED_OTHER [Leaf] Tamper detected
#define ERR_AUDIT_INTEGRITY             0xDF // UNBALANCED_EDGE [Leaf] Integrity fail

// -----------------------------------------------------------------------------
// DOMAIN 4.3: CONCURRENCY & DEADLOCK (0xFF -> 0xFE)
// Covers: Locks, IPC, atomics, race conditions, critical sections
// -----------------------------------------------------------------------------
#define ERR_LOCK_DOM                    0xFE // UNBALANCED_EDGE [Domain]

// Section 4.3.1: Lock Operations (0xFE -> 0x7C)
#define ERR_LOCK_STATE                  0x7C // UNBALANCED_OTHER [Section]
#define ERR_LOCK_FAILED                 0x38 // UNBALANCED_OTHER [Leaf] Lock failed
#define ERR_LOCK_TIMEOUT                0x39 // BALANCED_EDGE [Leaf] Lock timeout
#define ERR_LOCK_DEADLOCK               0xB8 // BALANCED_EDGE [Leaf] Deadlock detected
#define ERR_LOCK_OWNER                  0xB9 // UNBALANCED_OTHER [Leaf] Wrong owner

// Section 4.3.2: IPC & Message Passing (0xFE -> 0x7D)
#define ERR_LOCK_IPC                    0x7D // UNBALANCED_OTHER [Section]
#define ERR_PIPE_BROKEN                 0x3A // BALANCED_EDGE [Leaf] Broken pipe
#define ERR_PIPE_FULL                   0x3B // UNBALANCED_OTHER [Leaf] Pipe Full
#define ERR_MSG_SIZE                    0xBA // UNBALANCED_OTHER [Leaf] Message Too Large
#define ERR_MSG_QUEUE                   0xBB // UNBALANCED_TWIN [Leaf] Queue Destroyed

// Section 4.3.3: Atomic Operations (0xFE -> 0xFC)
#define ERR_LOCK_ATOMIC                 0xFC // UNBALANCED_OTHER [Section]
#define ERR_ATOMIC_FAILED               0x78 // BALANCED_SHADOW [Leaf] Atomic op failed
#define ERR_ATOMIC_RACE                 0x79 // UNBALANCED_OTHER [Leaf] Race detected
#define ERR_ATOMIC_MEMORY_ORDER         0xF8 // UNBALANCED_OTHER [Leaf] Memory order
#define ERR_ATOMIC_CONTENTION           0xF9 // UNBALANCED_OTHER [Leaf] High contention

// Section 4.3.4: Critical Sections (0xFE -> 0xFD)
#define ERR_LOCK_CRITICAL               0xFD // UNBALANCED_EDGE [Section]
#define ERR_CRIT_ENTER                  0x7A // UNBALANCED_OTHER [Leaf] Enter failed
#define ERR_CRIT_EXIT                   0x7B // UNBALANCED_OTHER [Leaf] Exit failed
#define ERR_CRIT_NESTED                 0xFA // UNBALANCED_OTHER [Leaf] Nested overflow
#define ERR_CRIT_ABANDONED              0xFB // UNBALANCED_EDGE [Leaf] Abandoned

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
static CC_ALWAYS_INLINE float err_op_entropy(uint8_t err) {
  uint8_t ones = err_op_one_count(err);
  uint8_t zeros = 8 - ones;

  if (zeros == 0 || ones == 0) return 0.0f;

  float p_ones = ones / 8.0f;
  float p_zeros = zeros / 8.0f;

  return -(p_ones * very_fast_log2(p_ones) + p_zeros * very_fast_log2(p_zeros));
}

/**
 * Calculate balance ratio (one/total lines).
 * Returns value from 0.0 to 1.0.
 */
static CC_ALWAYS_INLINE float err_op_balance(uint8_t err) {
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
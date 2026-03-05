// file: ./src/dbg/errors.c

/*
 * If the S macro is not provided elsewhere, provide the
 * compile-time selector: tiny (short) vs verbose messages.
 */
#include <protoduino.h>

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

const char *error_to_string(uint8_t err) {
  switch (err) {
#if 0
        // =====================================================================
        // QUADRANT 1: INIT / LIFECYCLE / SYSTEM STATE
        // ROOT: 0x00 (The Void / Success)
        // =====================================================================

        // --- ROOT ---
        case ERR_ROOT_INIT:       return S("P_OK", "Success (Void)");
        // ERR_SUCCESS is 0x00, duplicate of ERR_ROOT_INIT
        // ERR_OK is 0x00, duplicate of ERR_ROOT_INIT

        // --- KERNEL FLOW STATES (Reserved) ---
        // case ERR_YIELDING:        return S("P_YIELD", "Process Yielded / Waiting");
        // case ERR_EXITING:         return S("P_EXIT", "Process Exiting");
        // case ERR_ENDING:          return S("P_END", "Process Terminated Normally");

        // --- DOMAIN 1.1: SYSTEM STATE (0x01) ---
        case ERR_SYS_STATE:       return S("E_SYS", "System State Subsystem");

        // Section 1.1.1: Access Control
        case ERR_SYS_ACCESS:      return S("E_ACCESS", "Access Control Error");
        case ERR_ACCESS_DENIED:   return S("E_DENIED", "Access Denied");
        case ERR_ACCESS_READONLY: return S("E_RO", "Read Only");
        case ERR_ACCESS_LOCKED:   return S("E_LOCKED", "Access Locked");
        case ERR_ACCESS_FORBIDDEN:return S("E_FORBIDDEN", "Forbidden Operation");

        // Section 1.1.2: Handles & Descriptors
        case ERR_SYS_HANDLE:      return S("E_HANDLE", "Handle Error");
        case ERR_HANDLE_INVALID:  return S("E_INVHANDLE", "Invalid Handle");
        case ERR_HANDLE_CLOSED:   return S("E_CLOSED", "Handle Closed");
        case ERR_HANDLE_TYPE:     return S("E_HTYPE", "Handle Type Mismatch");
        case ERR_HANDLE_SHADOW:   return S("E_HSHADOW", "Stale Handle");

        // Section 1.1.3: Processes & Threads
        case ERR_SYS_PROC:        return S("E_PROC", "Process Error");
        case ERR_PROC_CANCELLED:  return S("E_CANCEL", "Cancelled");
        case ERR_PROC_KILLED:     return S("E_KILL", "Process Killed");
        case ERR_PROC_ORPHAN:     return S("E_ORPHAN", "Orphaned Process");
        case ERR_PROC_ZOMBIE:     return S("E_ZOMBIE", "Zombie Process");

        // Section 1.1.4: Arguments & Validation
        case ERR_SYS_ARGUMENT:    return S("E_ARG", "Argument Error");
        case ERR_ARG_NULL:        return S("E_NULL", "Null Argument");
        case ERR_ARG_INVALID:     return S("E_INVAL", "Invalid Argument");
        case ERR_ARG_RANGE:       return S("E_RANGE", "Argument Out of Range");
        case ERR_ARG_TYPE:        return S("E_ATYPE", "Argument Type Mismatch");

        // --- DOMAIN 1.2: MEMORY & RESOURCES (0x80) ---
        case ERR_MEM_DOM:         return S("E_MEM", "Memory Subsystem");

        // Section 1.2.1: Allocation (Heap)
        case ERR_MEM_HEAP:        return S("E_HEAP", "Heap Error");
        case ERR_HEAP_OOM:        return S("E_OOM", "Out of Memory");
        case ERR_HEAP_FRAGMENT:   return S("E_FRAG", "Heap Fragmented");
        case ERR_HEAP_CORRUPT:    return S("E_HCORRUPT", "Heap Corrupted");
        case ERR_HEAP_DOUBLE_FREE:return S("E_DBLFREE", "Double Free");

        // Section 1.2.2: Stack & Bounds
        case ERR_MEM_BOUNDS:      return S("E_BOUNDS", "Bounds Error");
        case ERR_STACK_OVERFLOW:  return S("E_STKOVF", "Stack Overflow");
        case ERR_STACK_UNDERFLOW: return S("E_STKUDF", "Stack Underflow");
        case ERR_BOUNDS_LOWER:    return S("E_BLOW", "Lower Bound Violation");
        case ERR_BOUNDS_UPPER:    return S("E_BUP", "Upper Bound Violation");

        // Section 1.2.3: Buffers
        case ERR_MEM_BUFFER:      return S("E_BUF", "Buffer Error");
        case ERR_BUF_OVERFLOW:    return S("E_BUFOVF", "Buffer Overflow");
        case ERR_BUF_UNDERFLOW:   return S("E_BUFUDF", "Buffer Underflow");
        case ERR_BUF_FULL:        return S("E_BUFFULL", "Buffer Full");
        case ERR_BUF_EMPTY:       return S("E_BUFEMPTY", "Buffer Empty");

        // Section 1.2.4: Alignment & Paging
        case ERR_MEM_ALIGN:       return S("E_ALIGN", "Alignment Error");
        case ERR_ALIGN_ADDR:      return S("E_ADDRALIGN", "Address Misaligned");
        case ERR_ALIGN_SIZE:      return S("E_SZALIGN", "Size Misaligned");
        case ERR_PAGE_FAULT:      return S("E_PGFAULT", "Page Fault");
        case ERR_PAGE_PROTECT:    return S("E_PGPROT", "Page Protection Violation");

        // --- DOMAIN 1.3: LIFECYCLE & INITIALIZATION (0x81) ---
        case ERR_LIFE_DOM:        return S("E_LIFE", "Lifecycle Subsystem");

        // Section 1.3.1: Initialization
        case ERR_LIFE_INIT:       return S("E_INIT", "Initialization Error");
        case ERR_INIT_FAILED:     return S("E_INITFAIL", "Init Failed");
        case ERR_INIT_TIMEOUT:    return S("E_INITTO", "Init Timeout");
        case ERR_INIT_DEPENDENCY: return S("E_DEP", "Missing Dependency");
        case ERR_INIT_NOSUP:      return S("E_NOSUP", "Not Supported");

        // Section 1.3.2: State Transitions
        case ERR_LIFE_STATE:      return S("E_STATE", "State Error");
        case ERR_STATE_INVALID:   return S("E_INVSTATE", "Invalid State");
        case ERR_STATE_TRANSITION:return S("E_TRANS", "Bad Transition");
        case ERR_STATE_LOCKED:    return S("E_STLOCK", "State Locked");
        case ERR_STATE_FROZEN:    return S("E_STFROZEN", "State Frozen");

        // Section 1.3.3: Reference Counting
        case ERR_LIFE_REF:        return S("E_REF", "Reference Error");
        case ERR_REF_ZERO:        return S("E_REF0", "Ref Count Zero");
        case ERR_REF_OVERFLOW:    return S("E_REFOVF", "Ref Count Overflow");
        case ERR_REF_LEAK:        return S("E_REFLEAK", "Ref Leak");
        case ERR_REF_DANGLING:    return S("E_DANGLING", "Dangling Reference");

        // Section 1.3.4: Cleanup & Finalization
        case ERR_LIFE_CLEAN:      return S("E_CLEAN", "Cleanup Error");
        case ERR_CLEAN_FAILED:    return S("E_CLEANFAIL", "Cleanup Failed");
        case ERR_CLEAN_PARTIAL:   return S("E_PARTIAL", "Partial Cleanup");
        case ERR_CLEAN_BUSY:      return S("E_CBUSY", "Cleanup Busy");
        case ERR_CLEAN_LEAKED:    return S("E_CLEAK", "Resource Leaked");


        // =====================================================================
        // QUADRANT 2: BEFORE / EXTERNAL I/O / HARDWARE
        // ROOT: 0x55 (The Oscillation / The Wave / The Wire)
        // =====================================================================

        // --- ROOT ---
        case ERR_ROOT_BEFORE:     return S("E_BUSY", "Resource Busy / Retry");
        // ERR_IO_BUSY is 0x55, duplicate of ERR_ROOT_BEFORE

        // --- DOMAIN 2.1: NETWORK & CONNECTIVITY (0x2A) ---
        case ERR_NET_DOM:         return S("E_NET", "Network Subsystem");

        // Section 2.1.1: Sockets & Connections
        case ERR_NET_SOCK:        return S("E_SOCK", "Socket Error");
        case ERR_SOCK_CREATE:     return S("E_SCREATE", "Socket Create Failed");
        case ERR_SOCK_BIND:       return S("E_BIND", "Bind Failed");
        case ERR_SOCK_LISTEN:     return S("E_LISTEN", "Listen Failed");
        case ERR_SOCK_ACCEPT:     return S("E_ACCEPT", "Accept Failed");

        // Section 2.1.2: Connection State
        case ERR_NET_CONN:        return S("E_CONN", "Connection Error");
        case ERR_CONN_REFUSED:    return S("E_REFUSED", "Connection Refused");
        case ERR_CONN_RESET:      return S("E_RESET", "Connection Reset");
        case ERR_CONN_CLOSED:     return S("E_CCLOSED", "Connection Closed");
        case ERR_CONN_TIMEOUT:    return S("E_CTO", "Connection Timeout");

        // Section 2.1.3: DNS & Resolution
        case ERR_NET_DNS:         return S("E_DNS", "DNS Error");
        case ERR_DNS_NXDOMAIN:    return S("E_NXDOM", "Domain Not Found");
        case ERR_DNS_TIMEOUT:     return S("E_DNSTO", "DNS Timeout");
        case ERR_DNS_SERVFAIL:    return S("E_SERVFAIL", "Server Failure");
        case ERR_DNS_CONFIG:      return S("E_DNSCFG", "DNS Config Error");

        // Section 2.1.4: Protocol Layer
        case ERR_NET_PROTO:       return S("E_PROTO", "Protocol Error");
        case ERR_PROTO_VERSION:   return S("E_PVER", "Protocol Version");
        case ERR_PROTO_FORMAT:    return S("E_PFMT", "Protocol Format Error");
        case ERR_PROTO_SEQUENCE:  return S("E_PSEQ", "Sequence Error");
        case ERR_PROTO_STATE:     return S("E_PSTATE", "Protocol State Error");

        // --- DOMAIN 2.2: SIGNALS & TIMING (0x2B) ---
        case ERR_TIME_DOM:        return S("E_TIME", "Timing Subsystem");

        // Section 2.2.1: Interrupts
        case ERR_TIME_IRQ:        return S("E_IRQ", "Interrupt Error");
        case ERR_IRQ_DISABLED:    return S("E_IRQDIS", "IRQ Disabled");
        case ERR_IRQ_PENDING:     return S("E_IRQPEND", "IRQ Pending");
        case ERR_IRQ_NESTED:      return S("E_IRQNEST", "Nested Overflow");
        case ERR_IRQ_PRIORITY:    return S("E_IRQPRIO", "Priority Error");

        // Section 2.2.2: Clocks & Timers
        case ERR_TIME_CLOCK:      return S("E_CLOCK", "Clock Error");
        case ERR_CLK_NOT_READY:   return S("E_CLKNRDY", "Clock Not Ready");
        case ERR_CLK_UNSTABLE:    return S("E_CLKUNST", "Clock Unstable");
        case ERR_CLK_EXPIRED:     return S("E_CLKEXP", "Timer Expired");
        case ERR_CLK_OVERFLOW:    return S("E_CLKOVF", "Timer Overflow");

        // Section 2.2.3: Synchronization
        case ERR_TIME_SYNC:       return S("E_SYNC", "Sync Error");
        case ERR_SYNC_FAILED:     return S("E_SYNCF", "Sync Failed");
        case ERR_SYNC_TIMEOUT:    return S("E_SYNCTO", "Sync Timeout");
        case ERR_SYNC_BARRIER:    return S("E_BARRIER", "Barrier Broken");
        case ERR_SYNC_LOST:       return S("E_SYNCL", "Sync Lost");

        // Section 2.2.4: Watchdogs
        case ERR_TIME_WDT:        return S("E_WDT", "Watchdog Error");
        case ERR_WDT_EXPIRED:     return S("E_WDTEXP", "Watchdog Expired");
        case ERR_WDT_RESET:       return S("E_WDTRST", "Watchdog Reset");
        case ERR_WDT_EARLY:       return S("E_WDTEARLY", "Early Kick");
        case ERR_WDT_CONFIG:      return S("E_WDTCFG", "WDT Config Error");

        // --- DOMAIN 2.3: PERIPHERALS & DRIVERS (0xAB) ---
        case ERR_IO_DOM:          return S("E_IO", "I/O Subsystem");

        // Section 2.3.1: Serial Communication
        case ERR_IO_SERIAL:       return S("E_SERIAL", "Serial Error");
        case ERR_SERIAL_BAUD:     return S("E_BAUD", "Baud Rate Error");
        case ERR_SERIAL_FRAME:    return S("E_FRAME", "Frame Error");
        case ERR_SERIAL_PARITY:   return S("E_PARITY", "Parity Error");
        case ERR_SERIAL_OVERRUN:  return S("E_OVERRUN", "Overrun Error");

        // Section 2.3.2: Bus Communication
        case ERR_IO_BUS:          return S("E_BUS", "Bus Error");
        case ERR_BUS_NACK:        return S("E_NACK", "NACK Received");
        case ERR_BUS_ARBITRATION: return S("E_ARBLOST", "Arbitration Lost");
        case ERR_BUS_TIMEOUT:     return S("E_BUSTO", "Bus Timeout");
        case ERR_BUS_ERROR:       return S("E_BUSERR", "Bus Error");

        // Section 2.3.3: Analog I/O
        case ERR_IO_ANALOG:       return S("E_ANALOG", "Analog Error");
        case ERR_ADC_SATURATED:   return S("E_ADCSAT", "ADC Saturated");
        case ERR_ADC_TIMEOUT:     return S("E_ADCTO", "ADC Timeout");
        case ERR_DAC_UNDERRUN:    return S("E_DACUR", "DAC Underrun");
        case ERR_DAC_CONFIG:      return S("E_DACCFG", "DAC Config");

        // Section 2.3.4: GPIO & Pins
        case ERR_IO_GPIO:         return S("E_GPIO", "GPIO Error");
        case ERR_GPIO_CONFIG:     return S("E_GPIOCFG", "GPIO Config Error");
        case ERR_GPIO_LOCKED:     return S("E_GPIOLCK", "Pin Locked");
        case ERR_GPIO_STATE:      return S("E_GPIOST", "Invalid GPIO State");
        case ERR_GPIO_INTERRUPT:  return S("E_GPIOINT", "GPIO Interrupt Error");


        // =====================================================================
        // QUADRANT 3: AFTER / DATA / LOGIC
        // ROOT: 0xAA (The Pattern / The Math)
        // =====================================================================

        // --- ROOT ---
        case ERR_ROOT_AFTER:      return S("E_DATA", "Data Integrity Root");

        // --- DOMAIN 3.1: ENCODING & FORMATS (0x54) ---
        case ERR_PARSE_DOM:       return S("E_FMT", "Format & Encoding");

        // Section 3.1.1: Text & Strings
        case ERR_PARSE_TEXT:      return S("E_TEXT", "Parsing Error");
        case ERR_TEXT_ENCODE:     return S("E_ENC", "Encoding Error");
        case ERR_TEXT_DECODE:     return S("E_DEC", "Decoding Error");
        case ERR_TEXT_TRUNC:      return S("E_TRUNC", "String Truncated");
        case ERR_TEXT_INVALID:    return S("E_FMT", "Text Format Error");

        // Section 3.1.2: Structured Data
        case ERR_PARSE_STRUCT:    return S("E_STRUCT", "Structured Data Error");
        case ERR_STRUCT_SYNTAX:   return S("E_PARSE", "Syntax Error");
        case ERR_STRUCT_TYPE:     return S("E_JTYPE", "Type Mismatch");
        case ERR_STRUCT_MALFORMED:return S("E_FORM", "Malformed Error");
        case ERR_STRUCT_NAMESPACE:return S("E_NAME", "Namespace Error");

        // Section 3.1.3: Media/Binary
        case ERR_PARSE_BINARY:    return S("E_MEDIA", "Media Format Error");
        case ERR_BIN_HEADER:      return S("E_HDR", "Invalid Header");
        case ERR_BIN_MAGIC:       return S("E_MAGC", "Magic mismatch");
        case ERR_BIN_VERSION:     return S("E_VERS", "Version mismatch");
        case ERR_BIN_CHECKSUM:    return S("E_CKSUM", "Checksum Mismatch");

        // Section 3.1.4: Validation
        case ERR_PARSE_VALID:     return S("E_VALID", "Validation Error");
        case ERR_VALID_SCHEMA:    return S("E_SCHEMA", "Schema Violation");
        case ERR_VALID_CONSTRAINT:return S("E_CONSTR", "Constraint failure");
        case ERR_VALID_PATTERN:   return S("E_PAT", "Pattern Mismatch");
        case ERR_VALID_REQUIRED:  return S("E_REQ", "Required Field Missing");

        // --- DOMAIN 3.2: COLLECTIONS & QUEUES ---
        case ERR_QUEUE_DOM:       return S("E_QUEUE", "Math Subsystem");

        // Section 3.2.1: Queue State (0xD4 -> 0x68)
        case ERR_QUEUE_STATE:     return S("E_QSTATE", "Queue State Error");
        case ERR_QUEUE_EMPTY:     return S("E_QEMPTY", "Queue empty");
        case ERR_QUEUE_FULL:      return S("E_QFULL", "Queue full");
        case ERR_QUEUE_OVERFLOW:  return S("E_QOVER", "Queue Overflow");
        case ERR_QUEUE_UNDERFLOW: return S("E_QUNDER", "Queue Underflow");

        // Section 3.2.2: Queue Operations (0xD4 -> 0x69)
        case ERR_QUEUE_OP:        return S("E_QOP", "Queue Operation Error");
        case ERR_QUEUE_ENQUEUE:   return S("E_QENQ", "Enqueue failed");
        case ERR_QUEUE_DEQUEUE:   return S("E_QDEQ", "Dequeue failed");
        case ERR_QUEUE_PEEK:      return S("E_QPEEK", "Peek failed");
        case ERR_QUEUE_CLEAR:     return S("E_QCLR", "Clear failed");

        // Section 3.2.3: Ordering & Priority (0xD4 -> 0xE8)
        case ERR_QUEUE_ORDER:     return S("E_QORD", "Queue Order Error");
        case ERR_ORDER_SEQUENCE:  return S("E_QSEQ", "Sequence error");
        case ERR_ORDER_PRIORITY:  return S("E_QPRIO", "Priority error");
        case ERR_ORDER_DUPLICATE: return S("E_QDUP", "Duplicate entry");
        case ERR_ORDER_CONFLICT:  return S("E_QCONF", "Order conflict");

        // Section 3.2.4: Collection State (0xD4 -> 0xE9)
        case ERR_COLLECTION_STATE:return S("E_COLLECT", "Collection Error");
        case ERR_COLL_LOCKED:     return S("E_CLCK", "Collection locked");
        case ERR_COLL_MODIFIED:   return S("E_CMOD", "Modified during iteration");
        case ERR_COLL_INDEX:      return S("E_CIDX", "Index out of bounds");
        case ERR_COLL_KEY:        return S("E_CKEY", "Key not found");

        // --- DOMAIN 3.3: MATH & ALGORITHMS (0xAA -> 0xD5) ---
        case ERR_MATH_DOM:        return S("E_MATH", "Math Subsystem");

        // Section 3.3.1: Arithmetic (0xD5 -> 0x6A)
        case ERR_MATH_ARITH:      return S("E_INT", "Arithmetic Error");
        case ERR_ARITH_DIV_ZERO:  return S("E_DIV0", "Division");
        case ERR_ARITH_OVERFLOW:  return S("E_IOVF", "Overflow");
        case ERR_ARITH_UNDERFLOW: return S("E_IUDF", "Underflow");
        case ERR_ARITH_PRECISION: return S("E_IPREC", "Precision loss");

        // Section 3.3.2: Floating Point (0xD5 -> 0x6B)
        case ERR_MATH_FLOAT:      return S("E_FLOAT", "Floating Point Error");
        case ERR_FLOAT_NAN:       return S("E_NAN", "Not a Number");
        case ERR_FLOAT_INF:       return S("E_INF", "Infinity");
        case ERR_FLOAT_DENORM:    return S("E_DENORM", "Denormalized");
        case ERR_FLOAT_INEXACT:   return S("E_INEXACT", "Inexact Result");

        // Section 3.3.3: Logic & Assertions (0xD5 -> 0xEA)
        case ERR_MATH_LOGIC:      return S("E_LOGIC", "Logic Error");
        case ERR_LOGIC_ASSERT:    return S("E_ASSERT", "Assertion Failed");
        case ERR_LOGIC_INVARIANT: return S("E_INVAR", "Invariant Broken");
        case ERR_LOGIC_UNREACH:   return S("E_UNREACH", "Unreachable Code");
        case ERR_LOGIC_IMPOSSIBLE:return S("E_IMPOSS", "Impossible State");

        // Section 3.3.4: Cryptographic Operations (0xD5 -> 0xEB)
        case ERR_MATH_CRYPTO:     return S("E_CRYPTO", "Cryptographic Error");
        case ERR_CRYPTO_KEY:      return S("E_CKEY", "Invalid Key");
        case ERR_CRYPTO_IV:       return S("E_CIV", "Invalid IV");
        case ERR_CRYPTO_TAG:      return S("E_CTAG", "Tag Verification Failed");
        case ERR_CRYPTO_PADDING:  return S("E_CPADD", "Padding Error");

        // =====================================================================
        // QUADRANT 4: FINAL / SECURITY / FATAL
        // ROOT: 0xFF (The End / Saturation)
        // =====================================================================

        // --- ROOT ---
        case ERR_ROOT_RUN:        return S("E_FATAL", "Fatal System Failure");
        // ERR_FATAL is 0xFF, duplicate of ERR_ROOT_RUN
        // ERR_FINALIZED is 0xFF, duplicate of ERR_ROOT_RUN

        // --- DOMAIN 4.1: STORAGE & FILESYSTEM (0xFF -> 0x7E) ---
        case ERR_STORAGE_DOM:     return S("E_STORAGE", "Storage Subsystem");

        // Section 4.1.1: File Operations
        case ERR_STORAGE_FILE:    return S("E_FILE", "File Error");
        case ERR_FILE_NOT_FOUND:  return S("E_FNOENT", "File Not Found");
        case ERR_FILE_EXISTS:     return S("E_FEXIST", "File Exists");
        case ERR_FILE_TOO_LARGE:  return S("E_FTOOLARGE", "File Too Large");
        case ERR_FILE_CORRUPTED:  return S("E_FCORRUPT", "File Corrupted");

        // Section 4.1.2: File Attributes
        case ERR_STORAGE_ATTR:    return S("E_ATTR", "Attribute Error");
        case ERR_ATTR_READONLY:   return S("E_ATTRRO", "Read Only");
        case ERR_ATTR_HIDDEN:     return S("E_ATTRHDN", "Hidden");
        case ERR_ATTR_SYSTEM:     return S("E_ATTRSYS", "System File");
        case ERR_ATTR_DIRECTORY:  return S("E_ATTRDIR", "Is Directory");

        // Section 4.1.3: Volume & Mount
        case ERR_STORAGE_VOLUME:  return S("E_VOLUME", "Volume Error");
        case ERR_VOL_NOT_MOUNTED: return S("E_NOTMNT", "Not Mounted");
        case ERR_VOL_BUSY:        return S("E_VBUSY", "Volume Busy");
        case ERR_VOL_FULL:        return S("E_VFULL", "Volume Full");
        case ERR_VOL_CORRUPTED:   return S("E_VCORRUPT", "Volume Corrupted");

        // Section 4.1.4: Block Device
        case ERR_STORAGE_BLOCK:   return S("E_BLK", "Block Device Error");
        case ERR_BLK_READ_ERROR:  return S("E_BLKRD", "Block Read Error");
        case ERR_BLK_WRITE_ERROR: return S("E_BLKWR", "Block Write Error");
        case ERR_BLK_BAD_SECTOR:  return S("E_BADSECT", "Bad Sector");
        case ERR_BLK_HARDWARE:    return S("E_BLKHW", "Hardware Error");

        // --- DOMAIN 4.2: SECURITY & AUTHENTICATION (0xFF -> 0x7F) ---
        case ERR_SEC_DOM:         return S("E_SEC", "Security Subsystem");

        // Section 4.2.1: Authentication
        case ERR_SEC_AUTH:        return S("E_AUTH", "Authentication Error");
        case ERR_AUTH_FAILED:     return S("E_AUTHF", "Auth Failed");
        case ERR_AUTH_EXPIRED:    return S("E_AUTHEXP", "Token Expired");
        case ERR_AUTH_REVOKED:    return S("E_AUTHREV", "Credentials Revoked");
        case ERR_AUTH_INSUFFICIENT:return S("E_AUTHINS", "Insufficient Rights");

        // Section 4.2.2: Encryption
        case ERR_SEC_ENCRYPT:     return S("E_ENCRYPT", "Encryption Error");
        case ERR_ENCRYPT_FAILED:  return S("E_ENCF", "Encryption Failed");
        case ERR_DECRYPT_FAILED:  return S("E_DECF", "Decryption Failed");
        case ERR_ENCRYPT_KEY_INVALID:return S("E_INVKEY", "Invalid Key");
        case ERR_ENCRYPT_ALGO:    return S("E_ALGO", "Unsupported Algorithm");

        // Section 4.2.3: Policy & Access Control
        case ERR_SEC_POLICY:      return S("E_POLICY", "Policy Error");
        case ERR_POLICY_VIOLATION:return S("E_POLVIO", "Policy Violation");
        case ERR_POLICY_DENY:     return S("E_DENY", "Explicit Deny");
        case ERR_POLICY_QUOTA:    return S("E_QUOTA", "Quota Exceeded");
        case ERR_POLICY_RATE_LIMIT:return S("E_RATELIM", "Rate Limited");

        // Section 4.2.4: Auditing & Logging
        case ERR_SEC_AUDIT:       return S("E_AUDIT", "Audit Error");
        case ERR_AUDIT_FAILED:    return S("E_AUDITF", "Audit Failed");
        case ERR_AUDIT_FULL:      return S("E_AFULL", "Audit Log Full");
        case ERR_AUDIT_TAMPER:    return S("E_TAMPER", "Tamper Detected");
        case ERR_AUDIT_INTEGRITY: return S("E_AINTEG", "Integrity Failure");

        // --- DOMAIN 4.3: CONCURRENCY & ATOMICS (0xFF -> 0xFE) ---
        case ERR_LOCK_DOM:        return S("E_LOCK", "Concurrency Subsystem");

        // Section 4.3.1: Lock Operations
        case ERR_LOCK_STATE:      return S("E_LSTATE", "Lock State Error");
        case ERR_LOCK_FAILED:     return S("E_LOCKF", "Lock Failed");
        case ERR_LOCK_TIMEOUT:    return S("E_LOCKTO", "Lock Timeout");
        case ERR_LOCK_DEADLOCK:   return S("E_DEADLOCK", "Deadlock Detected");
        case ERR_LOCK_OWNER:      return S("E_LOWNER", "Wrong Owner");

        // Section 4.3.2: IPC & Message Passing
        case ERR_LOCK_IPC:        return S("E_IPC", "IPC Error");
        case ERR_PIPE_BROKEN:     return S("E_BROKEN", "Broken Pipe");
        case ERR_PIPE_FULL:       return S("E_PFULL", "Pipe Full");
        case ERR_MSG_SIZE:        return S("E_SIZE", "Message Too Large");
        case ERR_MSG_QUEUE:       return S("E_QUEUE", "Queue Destroyed");

        // Section 4.3.3: Atomic Operations
        case ERR_LOCK_ATOMIC:     return S("E_ATOMIC", "Atomic Error");
        case ERR_ATOMIC_FAILED:   return S("E_ATOMF", "Atomic Op Failed");
        case ERR_ATOMIC_RACE:     return S("E_RACE", "Race Detected");
        case ERR_ATOMIC_MEMORY_ORDER:return S("E_MEMORD", "Memory Order");
        case ERR_ATOMIC_CONTENTION:return S("E_CONTEND", "High Contention");

        // Section 4.3.4: Critical Sections
        case ERR_LOCK_CRITICAL:   return S("E_CRIT", "Critical Section Error");
        case ERR_CRIT_ENTER:      return S("E_ENTER", "Enter Failed");
        case ERR_CRIT_EXIT:       return S("E_EXIT", "Exit Failed");
        case ERR_CRIT_NESTED:     return S("E_CNEST", "Nested Overflow");
        case ERR_CRIT_ABANDONED:  return S("E_ABAND", "Abandoned");
#endif
  // --- DEFAULT ---
  default:
    return S("UNK", "Unknown Ontology Code");
  }
}

#ifdef __cplusplus
}
#endif
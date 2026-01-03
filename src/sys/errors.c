// file: ./src/sys/errors.c
#include <protoduino-config.h>
#include <cc.h>
#include "errors.h"
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
        // =====================================================================
        // QUADRANT 1: INIT / LIFECYCLE / SYSTEM STATE
        // ROOT: 0x00 (The Void / Success)
        // =====================================================================

        // --- ROOT ---
        case ERR_ROOT_INIT:       return S("P_OK", "Success (Void)");
        // ERR_ROOT_INIT is 0x00, which is also ERR_SUCCESS
        // case ERR_SUCCESS:      (Duplicate of ERR_ROOT_INIT)

        // --- KERNEL FLOW STATES (Reserved) ---
        case ERR_YIELDING:        return S("P_YIELD", "Process Yielded / Waiting");
        case ERR_EXITING:         return S("P_EXIT", "Process Exiting");
        case ERR_ENDING:          return S("P_END", "Process Terminated Normally");

        // --- DOMAIN 1: SYSTEM STATE (0x01) ---
        // case ERR_SYS_STATE:    (Duplicate of ERR_YIELDING 0x01)

        // Section 1.1: Access Control
        // case ERR_SYS_ACCESS:   (Duplicate of ERR_EXITING 0x02)
        case ERR_ACCESS_READ:     return S("E_READ", "Read Access Forbidden");
        case ERR_ACCESS_WRITE:    return S("E_WRITE", "Write Access Forbidden");
        case ERR_ACCESS_EXEC:     return S("E_EXEC", "Execute Access Forbidden");
        case ERR_ACCESS_OWNER:    return S("E_OWNER", "Ownership Mismatch");

        // Section 1.2: Handles & Descriptors
        // case ERR_SYS_HANDLE:   (Duplicate of ERR_ENDING 0x03)
        case ERR_HANDLE_NULL:     return S("E_NULL", "Null Pointer/Handle");
        case ERR_HANDLE_CLOSED:   return S("E_CLOSED", "Handle Already Closed");
        case ERR_HANDLE_TYPE:     return S("E_TYPE", "Handle Type Mismatch");
        case ERR_HANDLE_SHADOW:   return S("E_SHADOW", "Stale Handle Reference");

        // Section 1.3: Processes & Threads
        case ERR_SYS_PROC:        return S("E_PROC", "Process Subsystem Error");
        case ERR_PROC_CANCELLED:  return S("E_FORK", "Operation Cancelled");
        case ERR_PROC_ZOMBIE:     return S("E_ZOMBIE", "Process is Zombie");
        case ERR_PROC_ORPHAN:     return S("E_ORPHAN", "Parent Process Lost");
        case ERR_PROC_INVAL:      return S("E_INVAL", "Invalid System Argument");

        // Section 1.4: Scheduling
        case ERR_SYS_SCHED:       return S("E_SCHED", "Scheduler Error");
        case ERR_SCHED_YIELD:     return S("E_YIELD", "Yield Failed");
        case ERR_SCHED_PRIORITY:  return S("E_PRIO", "Priority Inversion");
        case ERR_SCHED_QUANTUM:   return S("E_QUANTUM", "Time Slice Expired");
        case ERR_SCHED_STARVE:    return S("E_STARVE", "Thread Starvation");

        // --- DOMAIN 2: MEMORY & RESOURCES (0x80) ---
        case ERR_MEM_DOM:         return S("E_MEM", "Memory Subsystem");

        // Section 2.1: Allocation (Heap)
        case ERR_MEM_ALLOC:       return S("E_ALLOC", "Allocation Failure");
        case ERR_HEAP_OOM:        return S("E_OOM", "Out of Memory");
        case ERR_HEAP_FRAGMENT:   return S("E_FRAG", "Heap Fragmented");
        case ERR_HEAP_CORRUPT:    return S("E_CORRUPT", "Heap Metadata Corrupt");
        case ERR_HEAP_FREE:       return S("E_DBLFREE", "Double Free Detected");

        // Section 2.2: Stack & Bounds
        case ERR_MEM_BOUNDS:      return S("E_BOUNDS", "Memory Boundary Violation");
        case ERR_STACK_OVER:      return S("E_OVER", "Stack Overflow");
        case ERR_STACK_UNDER:     return S("E_UNDER", "Stack Underflow");
        case ERR_BOUNDS_LOWER:    return S("E_LOWER", "Lower Bound Violation");
        case ERR_BOUNDS_UPPER:    return S("E_UPPER", "Upper Bound Violation");

        // Section 2.3: Paging & Virtual Mem
        case ERR_MEM_VIRT:        return S("E_VIRT", "Virtual Memory Error");
        case ERR_PAGE_FAULT:      return S("E_FAULT", "Page Fault");
        case ERR_PAGE_NOT_PRESENT:return S("E_MISS", "Page Not Present");
        case ERR_PAGE_PROTECT:    return S("E_PROT", "Page Protection Violation");
        case ERR_PAGE_SHADOW:     return S("E_PAGE", "Shadow Page Error");

        // Section 2.4: Alignment & Mapping
        case ERR_MEM_MAP:         return S("E_MAP", "Memory Mapping Error");
        case ERR_MAP_FAILED:      return S("E_MMAP", "mmap Failed");
        case ERR_ALIGN_ADDR:      return S("E_ALIGN", "Address Misalignment");
        case ERR_ALIGN_BUS:       return S("E_BUS_ALGN", "Bus Alignment Error");
        case ERR_ALIGN_SIZE:      return S("E_SZ_ALGN", "Size Misalignment");

        // --- DOMAIN 3: LIFECYCLE & REFLECTION (0x81) ---
        case ERR_LIFE_DOM:        return S("E_LIFE", "Lifecycle Subsystem");

        // Section 3.1: Initialization
        case ERR_LIFE_INIT:       return S("E_INIT", "Initialization Error");
        case ERR_INIT_TIMEOUT:    return S("E_TO", "Init Timeout");
        case ERR_INIT_DEPENDENCY: return S("E_DEP", "Dependency Missing");
        case ERR_INIT_PREMATURE:  return S("E_PREMATURE", "Premature Initialization");
        case ERR_NOT_SUPPORTED:     return S("E_NOSUP", "Not Supported");

        // Section 3.2: States
        case ERR_LIFE_STATE:      return S("E_STATE", "Invalid State Transition");
        case ERR_STATE_UNK:       return S("E_UNKNOWN", "Unknown State");
        case ERR_STATE_BALANCED:  return S("E_ST_BAL", "Balanced State Violation");
        case ERR_STATE_LOCKED:    return S("E_LOCKED", "Object Locked");
        case ERR_STATE_FROZEN:    return S("E_FROZEN", "Object Frozen");

        // Section 3.3: Reference Counting
        case ERR_LIFE_REF:        return S("E_REF", "Reference Counting Error");
        case ERR_REF_ZERO:        return S("E_REF_0", "Ref Count Zero");
        case ERR_REF_MAX:         return S("E_REF_MAX", "Ref Count Max");
        case ERR_REF_LEAK:        return S("E_LEAK", "Potential Leak Detected");
        case ERR_REF_DANGLING:    return S("E_DANGLE", "Dangling Pointer");

        // Section 3.4: Garbage Collection
        case ERR_LIFE_GC:         return S("E_GC", "Garbage Collection Error");
        case ERR_GC_ROOT:         return S("E_GC_ROOT", "GC Root Not Found");
        case ERR_GC_SWEEP:        return S("E_GC_SWEEP", "GC Sweep Failure");
        case ERR_GC_COMPACT:      return S("E_GC_COMP", "GC Compaction Failure");
        case ERR_GC_MIRROR:       return S("E_GC_MIR", "Mirror Object Error");


        // =====================================================================
        // QUADRANT 2: BEFORE / EXTERNAL I/O / HARDWARE
        // ROOT: 0x55 (The Oscillation / The Wave / The Wire)
        // =====================================================================

        // --- ROOT ---
        case ERR_ROOT_BEFORE:     return S("E_BUSY", "Resource Busy / Retry");
        // ERR_IO_BUSY is 0x55, duplicate of ERR_ROOT_BEFORE

        // --- DOMAIN 1: NETWORK & CONNECTIVITY (0x2A) ---
        case ERR_NET_DOM:         return S("E_NET", "Network Subsystem");

        // Section 1.1: Sockets
        case ERR_NET_SOCK:        return S("E_SOCK", "Socket Error");
        case ERR_SOCK_BIND:       return S("E_BIND", "Bind Failed");
        case ERR_SOCK_LISTEN:     return S("E_LISTEN", "Listen Failed");
        case ERR_SOCK_ACCEPT:     return S("E_ACCEPT", "Accept Failed");
        case ERR_SOCK_CONNECT:    return S("E_CONN", "Connect Failed");

        // Section 1.2: DNS & Resolution
        case ERR_NET_DNS:         return S("E_NAME", "DNS Error");
        case ERR_DNS_NXDOMAIN:    return S("E_NXDOMAIN", "Domain Not Found");
        case ERR_DNS_TIMEOUT:     return S("E_DNS_TO", "Resolution Timeout");
        case ERR_DNS_SERVFAIL:    return S("E_SERVFAIL", "DNS Server Failure");
        case ERR_DNS_BALANCED:    return S("E_DNS_LB", "Load Balancer Error");

        // Section 1.3: Transport
        case ERR_NET_TRANS:       return S("E_TRANS", "Transport Layer Error");
        case ERR_TRANS_RESET:     return S("E_RST", "Connection Reset");
        case ERR_TRANS_CLOSED:    return S("E_CLOSE", "Connection Closed");
        case ERR_TRANS_DROP:      return S("E_DROP", "Packet Dropped");
        case ERR_TRANS_CONGEST:   return S("E_CONGEST", "Network Congestion");

        // Section 1.4: Protocol
        case ERR_NET_PROTO:       return S("E_PROTO", "Protocol Error");
        case ERR_PROTO_BAD:       return S("E_BAD", "Bad Request");
        case ERR_PROTO_PROXY:     return S("E_PROXY", "Proxy Error");
        case ERR_PROTO_SUB:       return S("E_SUB", "Subscription Failed");
        case ERR_PROTO_PUB:       return S("E_PUB", "Publish Failed");

        // --- DOMAIN 2: SIGNALS & TIMING (0x2B) ---
        case ERR_TIME_DOM:        return S("E_SIGNAL", "Timing Subsystem");

        // Section 2.1: Interrupts
        case ERR_TIME_IRQ:        return S("E_IRQ", "Interrupt Error");
        case ERR_IRQ_MASKED:      return S("E_MASKED", "IRQ Masked");
        case ERR_IRQ_INTR:        return S("E_INTR", "Interrupted");
        case ERR_IRQ_NESTED:      return S("E_NESTED", "Nested Depth Exceeded");
        case ERR_IRQ_BALANCE:     return S("E_IRQ_BAL", "IRQ Load Balance Error");

        // Section 2.2: Clocks & Timers
        case ERR_TIME_CLOCK:      return S("E_CLK", "Clock Error");
        case ERR_CLK_SKEW:        return S("E_SKEW", "Clock Skew");
        case ERR_CLK_JITTER:      return S("E_JITTER", "High Jitter");
        case ERR_CLK_EXPIRED:     return S("E_EXPIRE", "Timer Expired");
        case ERR_CLK_DRIFT:       return S("E_DRIFT", "Clock Drift");

        // Section 2.3: Synchronization
        case ERR_TIME_SYNC:       return S("TIME_SYNC", "Sync Primitive Error");
        case ERR_SYNC_WAIT:       return S("E_WAIT", "Wait Failed");
        case ERR_SYNC_BARRIER:    return S("E_BARRIER", "Barrier Broken");
        case ERR_SYNC_SEM:        return S("E_SEM", "Semaphore Twin Error");
        case ERR_SYNC_MUTEX:      return S("E_MUTEX", "Mutex Deadlock/Error");

        // Section 2.4: Watchdogs
        case ERR_TIME_WDT:        return S("E_WDT", "Watchdog Error");
        case ERR_WDT_BARK:        return S("E_BARK", "Watchdog Warning");
        case ERR_WDT_BITE:        return S("E_BITE", "Watchdog Reset");
        case ERR_WDT_EARLY:       return S("E_EARLY", "Kick Too Early");
        case ERR_WDT_LATE:        return S("E_LATE", "Kick Too Late");

        // --- DOMAIN 3: I/O & DRIVERS ---
        // Based on logic, 0xAB is likely ERR_IO_DOM or similar, inferred from children
        case ERR_IO_DOM:          return S("E_IO", "I/O Subsystem");

        // Section 3.1: Serial (UART)
        case ERR_IO_SERIAL:       return S("E_SERIAL", "Serial Driver Error");
        case ERR_SRL_BAUD:        return S("E_BAUD", "Baud Rate Mismatch");
        case ERR_SRL_FRAME:       return S("E_FRAME", "Framing Error");
        case ERR_SRL_PARITY:      return S("E_PARITY", "Parity Error");
        case ERR_SRL_BREAK:       return S("E_BREAK", "Break Condition");

        // Section 3.2: Bus (I2C/SPI)
        case ERR_IO_BUS:          return S("E_BUSDRV", "Bus Driver Error");
        case ERR_BUS_NACK:        return S("E_NACK", "I2C NACK");
        case ERR_BUS_ARB:         return S("E_ARB", "I2C Arbitration Lost");
        case ERR_BUS_MODE:        return S("E_MODE", "SPI Mode Mismatch");
        case ERR_BUS_OVER:        return S("E_OVER", "SPI Overrun");

        // Section 3.3: Analog (ADC/DAC)
        case ERR_IO_SIGNAL:       return S("E_ANALOG", "Analog Driver Error");
        case ERR_SIGN_HIGH:       return S("E_SATHIGH", "ADC Saturation High");
        case ERR_SIGN_LOW:        return S("E_SATLOW", "ADC Saturation Low");
        case ERR_SIGN_UNDERRUN:   return S("E_URUN", "DAC Underrun");
        case ERR_SIGN_REF:        return S("E_VREF", "Voltage Ref Error");

        // Section 3.4: GPIO
        case ERR_DRV_GPIO:        return S("DRV_GPIO", "GPIO Error");
        case ERR_GPIO_DIR:        return S("E_DIR", "Direction Error");
        case ERR_GPIO_MUX:        return S("E_MUX", "Muxing Error");
        case ERR_GPIO_DEBOUNCE:   return S("E_DEBOUNCE", "Debounce Fail");
        case ERR_GPIO_LOCKED:     return S("E_PINLCK", "Pin Locked");


        // =====================================================================
        // QUADRANT 3: AFTER / DATA / LOGIC
        // ROOT: 0xAA (The Pattern / The Math)
        // =====================================================================

        // --- ROOT ---
        case ERR_ROOT_AFTER:      return S("E_DATA", "Data Integrity Root");

        // --- DOMAIN 1: ENCODING & FORMATS (0x54) ---
        case ERR_FMT_DOM:         return S("E_FMT", "Format & Encoding");

        // Section 1.1: Text & Strings
        case ERR_FMT_TEXT:        return S("E_TEXT", "String Encoding");
        case ERR_TEXT_ENCODING:   return S("E_ENC", "Encoding Error (UTF-8)");
        case ERR_TEXT_DECODING:   return S("E_DEC", "Decoding Error");
        case ERR_TEXT_TRUNC:      return S("E_TRUNC", "String Truncated");
        case ERR_TEXT_FORMAT:     return S("E_FMT", "Text Format Error");

        // Section 1.2: JSON/XML
        case ERR_FMT_STRUCT:      return S("E_STRUCT", "Structured Data Error");
        case ERR_STRUCT_PARSE:    return S("E_PARSE", "Parse Error");
        case ERR_STRUCT_TYPE:     return S("E_JTYPE", "Type Mismatch");
        case ERR_STRUCT_TAG:      return S("E_TAG", "Tag Mismatch");
        case ERR_STRUCT_ATTR:     return S("E_ATTR", "Attribute Error");

        // Section 1.3: Media/Binary
        case ERR_FMT_MEDIA:       return S("E_MEDIA", "Media Format Error");
        case ERR_MEDIA_HEADER:    return S("E_HDR", "Invalid Header");
        case ERR_MEDIA_DEPTH:     return S("E_DEPTH", "Unsupported Bit Depth");
        case ERR_MEDIA_RATE:      return S("E_RATE", "Unsupported Sample Rate");
        case ERR_MEDIA_CKSUM:     return S("E_CKSUM", "Checksum Mismatch");

        // Section 1.4: Validation
        case ERR_FMT_VALID:       return S("E_VALID", "Validation Error");
        case ERR_VAL_SCHEMA:      return S("E_SCHEMA", "Schema Violation");
        case ERR_VAL_RANGE:       return S("E_RANGE", "Value Out of Range");
        case ERR_VAL_PATTERN:     return S("E_PAT", "Pattern Mismatch");
        case ERR_VAL_REQ:         return S("E_REQ", "Required Field Missing");

        // --- DOMAIN 2: MATH & LOGIC (0xAA -> 0xD4) ---
        case ERR_MATH_DOM:        return S("E_MATH", "Math Subsystem");

        // Section 2.1: Arithmetic
        case ERR_MATH_CALC:       return S("E_CALC", "Arithmetic Error");
        case ERR_CALC_DIV0:       return S("E_DIV0", "Division by Zero");
        case ERR_CALC_OVERFLOW:   return S("E_OVF", "Integer Overflow");
        case ERR_CALC_UNDERFLOW:  return S("E_UDF", "Integer Underflow");
        case ERR_CALC_NAN:        return S("E_NAN", "Result is NaN");

        // Section 2.2: Floating Point
        case ERR_MATH_FP:         return S("E_FP", "Floating Point Error");
        case ERR_FP_INF:          return S("E_INF", "Infinity");
        case ERR_FP_DENORM:       return S("E_DENORM", "Denormalized Number");
        case ERR_FP_PRECISION:    return S("E_PREC", "Precision Loss");
        case ERR_FP_ROUND:        return S("E_ROUND", "Rounding Error");

        // Section 2.3: Cryptographic Math
        case ERR_MATH_CRYPTO:     return S("E_CRYP", "Crypto Math Error");
        case ERR_CRYPTO_CURVE:    return S("E_CURVE", "Curve Point Invalid");
        case ERR_CRYPTO_PRIME:    return S("E_PRIME", "Not Prime");
        case ERR_CRYPTO_SHADOW_KEY: return S("E_WEAK", "Weak Key Detected");
        case ERR_CRYPTO_PADDING:  return S("E_PAD", "Padding Error");

        // Section 2.4: Logic
        case ERR_MATH_LOGIC:      return S("E_LOGIC", "Logic Error");
        case ERR_LOGIC_ASSERT:    return S("E_ASSERT", "Assertion Failed");
        case ERR_LOGIC_INVARIANT: return S("E_INVAR", "Invariant Violated");
        case ERR_LOGIC_REACH:     return S("E_REACH", "Unreachable Code Reached");
        case ERR_LOGIC_STATE:     return S("E_LSTATE", "Impossible Logic State");

        // --- DOMAIN 3: DATA & STORAGE ---
        case ERR_STORAGE_DOM:     return S("E_STORE", "Data Subsystem");

        // Section 3.1: File System
        case ERR_STORE_FS:        return S("E_FS", "Filesystem Error");
        case ERR_FS_NOENT:        return S("E_NOENT", "File Not Found");
        case ERR_FS_EXIST:        return S("E_EXIST", "File Exists");
        case ERR_FS_PERM:         return S("E_PERM", "FS Permission Denied");
        case ERR_FS_FULL:         return S("E_FULL", "Disk Full");

        // Section 3.2: Attributes
        case ERR_STORE_ATTR:      return S("E_SATTR", "Inodes/Attrs");
        case ERR_ATTR_RO:         return S("E_RO", "Read Only");
        case ERR_ATTR_HIDDEN:     return S("E_HDN", "Hidden");
        case ERR_ATTR_SYMLINK:    return S("E_LINK", "Symlink Loop");
        case ERR_ATTR_NO_SPACE:   return S("E_NS", "No Device Space");

        // Section 3.3: Block Device
        case ERR_STORE_BLOCK:     return S("E_BLOCK", "Block Layer");
        case ERR_BLK_READ:        return S("E_BREAD", "Block Read Error");
        case ERR_BLK_WRITE:       return S("E_BWRITE", "Block Write Error");
        case ERR_BLK_SECTOR:      return S("E_SECTOR", "Bad Sector");
        case ERR_BLK_GEOMETRY:    return S("E_GEOM", "Bad Geometry");

        // Section 3.4: Volume
        case ERR_STORE_VOLUME:    return S("E_VOL", "Conversion Error");
        case ERR_VOL_DIRTY:       return S("E_DIRTY", "Endianness Mismatch");
        case ERR_VOL_MOUNT:       return S("E_MNT", "Size Conversion Error");
        case ERR_VOL_UNMOUNT:     return S("E_UMNT", "Data Loss in Conversion");
        case ERR_VOL_UNKNOWN:     return S("E_UNKW", "Type Conversion Failed");


        // =====================================================================
        // QUADRANT 4: FINAL / SECURITY / FATAL
        // ROOT: 0xFF (The End / Saturation)
        // =====================================================================

        // --- ROOT ---
        case ERR_ROOT_RUN:        return S("E_FATAL", "Fatal System Failure");
        // ERR_FATAL is 0xFF, duplicate of ERR_ROOT_RUN
        // ERR_FINALIZED is 0xFF, duplicate

        // --- DOMAIN 1: SECURITY & AUTH (0x7E) ---
        case ERR_SEC_DOM:         return S("E_SEC", "Security Subsystem");

        // Section 1.1: Authentication
        case ERR_SEC_AUTH:        return S("E_AUTH", "Authentication Failure");
        case ERR_AUTH_FAIL:       return S("E_LOGIN", "Login Failed");
        case ERR_AUTH_EXPIRED:    return S("E_EXPIRE", "Token Expired");
        case ERR_AUTH_SCOPE:      return S("E_SCOPE", "Scope Invalid");
        case ERR_AUTH_MATCH:      return S("E_REPLAY", "Credential Replay Detected");

        // Section 1.2: Encryption
        case ERR_SEC_CRYPT:       return S("E_CRYPT", "Encryption Error");
        case ERR_CRYPT_ALGO:      return S("E_ALGO", "Algo Not Supported");
        case ERR_CRYPT_KEY:       return S("E_KEY", "Invalid Key");
        case ERR_CRYPT_IV:        return S("E_IV", "Invalid IV");
        case ERR_CRYPT_TAG:       return S("E_TAG", "Tag Mismatch");

        // Section 1.3: Access (Policies)
        case ERR_SEC_POLICY:      return S("E_POL", "Policy Violation");
        case ERR_POL_DENY:        return S("E_DENY", "Explicit Deny");
        case ERR_POL_QUOTA:       return S("E_QUOTA", "Quota Exceeded");
        case ERR_POL_TIME:        return S("E_TIME", "Access Time Violation");
        case ERR_POL_REJECT:       return S("E_REJECT", "Access Rejected");

        // Section 1.4: Auditing
        case ERR_SEC_AUDIT:       return S("E_AUDIT", "Audit Failure");
        case ERR_AUDIT_LOG:       return S("E_LOG", "Log Write Failed");
        case ERR_AUDIT_FULL:      return S("E_AFULL", "Audit Log Full");
        case ERR_AUDIT_TAMPER:    return S("E_TAMPER", "Tampering Detected");
        case ERR_AUDIT_FAIL:      return S("E_AFAIL", "Audit System Fail");

        // --- DOMAIN 2: CONCURRENCY & ATOMICS ---
        case ERR_LOCK_DOM:        return S("E_ATOM", "Concurrency Fatal");

        // Section 2.1: Locks
        case ERR_ATOM_LOCK:       return S("E_LCK", "Lock Error");
        case ERR_LOCK_BUSY:       return S("E_LBUSY", "Lock Busy");
        case ERR_LOCK_OWNER:      return S("E_LOWNER", "Lock Ownership Error");
        case ERR_LOCK_DEAD:       return S("E_LDEAD", "Deadlock Detected");
        case ERR_LOCK_MAX:        return S("E_LMAX", "Max Recursion");

        // Section 2.2: IPC Pipes
        case ERR_ATOM_IPC:        return S("E_IPC", "Pipe/Queue Error");
        case ERR_PIPE_BROKEN:     return S("E_BROKEN", "Broken Pipe");
        case ERR_PIPE_FULL:       return S("E_PFULL", "Pipe Full");
        case ERR_MSG_SIZE:        return S("E_SIZE", "Message Too Large");
        case ERR_MSG_QUEUE:       return S("E_QUEUE", "Queue Destroyed");

        // Section 2.3: Ordering/Fences
        case ERR_ATOM_ORDER:      return S("E_ORDER", "Ordering Error");
        case ERR_ORDER_ACQ:       return S("E_ACQ", "Acquire Fail");
        case ERR_ORDER_REL:       return S("E_REL", "Release Fail");
        case ERR_ORDER_BARRIER:   return S("E_BAR", "Barrier Violation");
        case ERR_ORDER_RACE:      return S("E_RACE", "Race Condition Detected");

        // Section 2.4: Critical Sections
        case ERR_LOCK_CRIT:       return S("E_CRI", "Critical Section Error");
        case ERR_CRIT_ENTER:      return S("E_ENTER", "Enter Critical Fail");
        case ERR_CRIT_LEAVE:      return S("E_LEAVE", "Leave Critical Fail");
        case ERR_CRIT_TIMEOUT:    return S("E_CTO", "Critical Timeout");
        case ERR_CRIT_BAIL:       return S("E_BAIL", "Emergency Bail");

        // --- DOMAIN 3: HARDWARE FATAL (0xFE) ---
        case ERR_HW_DOM:          return S("E_HW", "Hardware Panic");

        // Section 3.1: CPU
        case ERR_HW_CPU:          return S("E_CPU", "CPU Exception");
        case ERR_CPU_ILL:         return S("E_ILL", "Illegal Instruction");
        case ERR_CPU_BUS:         return S("E_BUS", "Bus Error");
        case ERR_CPU_TRAP:        return S("E_TRAP", "Trap");
        case ERR_CPU_HALT:        return S("E_HALT", "CPU Halted");

        // Section 3.2: Power
        case ERR_HW_PWR:          return S("E_PWR", "Power Failure");
        case ERR_PWR_BOR:         return S("E_BOR", "Brown Out Reset");
        case ERR_PWR_LOW:         return S("E_LOW", "Voltage Low");
        case ERR_PWR_HIGH:        return S("E_HIGH", "Voltage High");
        case ERR_PWR_BATT:        return S("E_BATT", "Battery Critical");

        // Section 3.3: Memory
        case ERR_HW_RAM:          return S("E_RAM", "RAM Failure");
        case ERR_RAM_ECC:         return S("E_ECC", "ECC Error");
        case ERR_RAM_PARITY:      return S("E_RPAR", "RAM Parity");
        case ERR_RAM_FAIL:        return S("E_RFAIL", "RAM Test Failed");
        case ERR_RAM_LINE:        return S("E_LINE", "Address Line Stuck");

        // Section 3.4: System Integrity
        case ERR_SYS_PANIC:       return S("E_SYS", "Kernel Panic");
        case ERR_PANIC_GENERAL:   return S("E_GNRL", "General Panic");
        case ERR_PANIC_STACK:     return S("E_STCK", "Stack Smashe");
        case ERR_PANIC_ASSERT:    return S("E_ASSERT", "Kernel Assert");
        case ERR_PANIC_ABORT:     return S("E_ABORT", "System Abort");

        // --- DEFAULT ---
        default:
            return S("UNK", "Unknown Ontology Code");
    }
}

#ifdef __cplusplus
}
#endif
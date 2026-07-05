
// file: ./src/sys/events.h
#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <stdint.h>

//
// ONTOLOGICAL 8-BIT EVENT TAXONOMY
#define __EVENTS_VERSION__ 4
// -----------------------------------------------------------------------------
// Geometry:  16x16 Matrix (0..255) mapped to 4 Attractor Basins.
// Logic:     Convergence via ((Child << 1) & 0xF0) | ((Child >> 1) & 0x0F)
// Architecture: 4 ROOTS -> 12 DOMAINS -> 48 SECTIONS -> 192 LEAFS
// Symmetry: TWINS (foundational), SHADOWS (complementary), MIRRORS
// (bidirectional) Inverse mapping: ~event & 0xFF maps to corresponding error
// code in errors.h
// Duality: event = ~error & 0xFF  (bitwise inversion connects the two spaces)
// Quadrant mapping:
//   Q1 events (0x00) <-> Q4 errors (0xFF)  [INIT <-> RUN]
//   Q2 events (0x55) <-> Q3 errors (0xAA)  [BEFORE <-> AFTER]
//   Q3 events (0xAA) <-> Q2 errors (0x55)  [AFTER <-> BEFORE]
//   Q4 events (0xFF) <-> Q1 errors (0x00)  [RUN <-> INIT]
// User input: scalar (key/button) cast to process_data_t directly;
//             vector (motion/touch/gesture) wrapped in ipc_msg_t* from pool.
//
// This header is VALIDATED against the ontology math by
// tools/ontology-sync.js: every symmetry/hierarchy annotation in the
// comments below is checked, and the string table
// src/dbg/events-strings.inc is generated from the descriptions.
// After editing, run: node tools/ontology-sync.js
//
// v4: domain headers renamed to their event-side semantics
//     (EVENT_PARSE_DOM -> EVENT_SIGNAL_DOM, EVENT_QUEUE_DOM ->
//      EVENT_TIMING_DOM, EVENT_DEADLOCK_DOM -> EVENT_PSTATE_DOM);
//     values are unchanged.
// -----------------------------------------------------------------------------
//

// =============================================================================
// RESERVED KERNEL CODES (Process State Machine — shared with errors.h)
// =============================================================================
#define EVENT_NONE                                                             \
  0x00 // No event / null                 => ~ERR_FINALIZED(0xFF)
#define EVENT_INIT                                                             \
  0x01 // Process init event              => ~ERR_DEADLOCK(0xFE)
#define EVENT_POLL                                                             \
  0x02 // Process poll request            => ~ERR_LOCK_CRITICAL(0xFD)
#define EVENT_EXIT                                                             \
  0x03 // Process exit event              => ~ERR_LOCK_ATOMIC(0xFC)
#define EVENT_ERROR                                                            \
  0xFF // Error occurred (carries error)  => ~ERR_SUCCESS(0x00)

// Shared reserved aliases (same value as reserved codes above, named for
// inverse clarity):
#define EVENT_SYS_ACCESS_EV                                                    \
  0xFD // IMPULSE  Dual of ERR_SYS_ACCESS(0x02)  [=EVENT_POLL]
#define EVENT_SYS_HANDLE_EV                                                    \
  0xFC // ASYMMETRY Dual of ERR_SYS_HANDLE(0x03) [=EVENT_EXIT]

// Convenience aliases used by process.h
#define PROCESS_EVENT_NONE EVENT_NONE
#define PROCESS_EVENT_INIT EVENT_INIT
#define PROCESS_EVENT_POLL EVENT_POLL
#define PROCESS_EVENT_EXIT EVENT_EXIT
#define PROCESS_EVENT_ERROR EVENT_ERROR

// =============================================================================
// QUADRANT 1: INIT / SYSTEM LIFECYCLE (root=0x00)
// ROOTs dual: Q4 errors (security/storage/concurrency/lock)
// Semantics: process lifecycle, memory, init, IPC, auth
// =============================================================================

// =============================================================================
// DOMAIN Q1.1: PROCESS LIFECYCLE & CONCURRENCY  (0x00 -> 0x01 RESERVED)
// Dual of Q4/Domain-4.3: ERR_DEADLOCK domain (lock, IPC, atomic, critical)
// =============================================================================

// Section Q1.1.1: Lock & Access Events  (0x01 -> 0x02 RESERVED)
// Dual of Sec-4.3.4: ERR_LOCK_CRITICAL (atomic/critical section errors)
#define EVENT_LOCK_ACQ 0x04   // IMPULSE   Resource/lock acquired
#define EVENT_LOCK_REL 0x05   // ASYMMETRY Resource/lock released
#define EVENT_CRIT_ENTER 0x84 // ASYMMETRY Critical section entered
#define EVENT_CRIT_EXIT 0x85  // ASYMMETRY Critical section exited
// Inverse Q4 leaves of this section (duals of 0x04,0x05,0x84,0x85):
#define EVENT_ACCESS_REVOKE                                                    \
  0xFB // IMPULSE   Access/lock forcibly revoked   [~ERR_ACCESS_DENIED]
#define EVENT_ACCESS_ALLOW                                                     \
  0xFA // ASYMMETRY Read-only resource made writable [~ERR_ACCESS_READONLY]
#define EVENT_CRIT_UNLOCK                                                      \
  0x7B // ASYMMETRY Lock forcibly released           [~ERR_ACCESS_LOCKED]
#define EVENT_CRIT_PERMIT                                                      \
  0x7A // ASYMMETRY Forbidden op now permitted        [~ERR_ACCESS_FORBIDDEN]

// Section Q1.1.2: Handle & Descriptor Events  (0x01 -> 0x03 RESERVED)
// Dual of Sec-4.3.3: ERR_LOCK_ATOMIC (atomic ops errors)
#define EVENT_HANDLE_OPEN 0x06  // ASYMMETRY File handle / fd opened
#define EVENT_HANDLE_CLOSE 0x07 // ASYMMETRY File handle / fd closed
#define EVENT_HANDLE_READY 0x86 // ASYMMETRY Handle ready for I/O
#define EVENT_HANDLE_WAKE 0x87  // ANTICODE  Stale/dormant handle resumed
// Inverse Q4 leaves of this section (duals of 0x06,0x07,0x86,0x87):
#define EVENT_HANDLE_VALID                                                     \
  0xF9 // ASYMMETRY Handle validated/re-opened       [~ERR_HANDLE_INVALID]
#define EVENT_HANDLE_REOPEN                                                    \
  0xF8 // ASYMMETRY Closed handle reopened           [~ERR_HANDLE_CLOSED]
#define EVENT_HANDLE_RETYPE                                                    \
  0x79 // ASYMMETRY Handle type corrected/cast       [~ERR_HANDLE_TYPE]
#define EVENT_HANDLE_FRESH                                                     \
  0x78 // ANTICODE  Fresh handle replaced stale one   [~ERR_HANDLE_SHADOW]

// Section Q1.1.3: IPC & Pipe Events  (0x01 -> 0x82)
// Dual of Sec-4.3.2: ERR_LOCK_IPC (pipe/msg errors)
#define EVENT_IPC 0x82        // ASYMMETRY [Section header]
#define EVENT_PIPE_READY 0x44 // REPEATER  Pipe has data ready
#define EVENT_PIPE_OPEN 0x45  // ASYMMETRY Pipe connection opened
#define EVENT_PIPE_CLOSE 0xC4 // ASYMMETRY Pipe connection closed
#define EVENT_PIPE_FLUSH 0xC5 // MARGIN    Pipe data flushed

// Section Q1.1.4: Process State Events  (0x01 -> 0x83)
// Dual of Sec-4.3.1: ERR_LOCK_STATE (lock state errors)
#define EVENT_PROC_STATE 0x83   // ASYMMETRY [Section header]
#define EVENT_PROC_STARTED 0x46 // ASYMMETRY Process scheduled/started
#define EVENT_PROC_STOPPED 0x47 // MARGIN    Process removed/stopped
#define EVENT_PROC_PAUSED 0xC6  // MARGIN    Process execution paused
#define EVENT_PROC_RESUMED 0xC7 // ASYMMETRY Process execution resumed

// =============================================================================
// DOMAIN Q1.2: MEMORY & BUFFERS  (0x00 -> 0x80)
// Dual of Q4/Domain-4.2: ERR_SEC_DOM (auth/encrypt/policy/audit)
// =============================================================================

// Section Q1.2.1: Heap / Memory Allocation Events  (0x80 -> 0x40)
// Dual of Sec-4.2.4: ERR_SEC_AUDIT (audit errors)
#define EVENT_MEM_DOM 0x80       // IMPULSE   [Domain header]
#define EVENT_MEM_ALLOC_SEC 0x40 // IMPULSE   [Section header]
#define EVENT_MEM_ALLOC 0x20     // IMPULSE   Memory block allocated
#define EVENT_MEM_FREE 0x21      // ASYMMETRY Memory block freed
#define EVENT_MEM_COMPACT 0xA0   // ASYMMETRY Memory compacted/defraged
#define EVENT_MEM_GC 0xA1        // ASYMMETRY GC cycle completed

// Section Q1.2.2: Stack & Buffer Events  (0x80 -> 0x41)
// Dual of Sec-4.2.3: ERR_SEC_POLICY (policy/quota errors)
#define EVENT_BUF_SEC 0x41   // ASYMMETRY [Section header]
#define EVENT_BUF_ALLOC 0x22 // REPEATER  Buffer allocated
#define EVENT_BUF_FREE 0x23  // ASYMMETRY Buffer freed
#define EVENT_BUF_READY 0xA2 // ASYMMETRY Buffer ready for write
#define EVENT_BUF_FLUSH 0xA3 // MARGIN    Buffer flushed to stream

// Section Q1.2.3: Encrypt & Decrypt Events  (0x80 -> 0xC0)
// Dual of Sec-4.2.2: ERR_SEC_ENCRYPT (encryption errors)
#define EVENT_ENC_SEC 0xC0       // ASYMMETRY [Section header]
#define EVENT_ENCRYPT_BEGIN 0x60 // ASYMMETRY Encryption operation started
#define EVENT_ENCRYPT_DONE 0x61  // ASYMMETRY Encryption operation complete
#define EVENT_DECRYPT_BEGIN 0xE0 // ASYMMETRY Decryption operation started
#define EVENT_DECRYPT_DONE 0xE1  // ANTICODE  Decryption operation complete

// Section Q1.2.4: Authentication Events  (0x80 -> 0xC1)
// Dual of Sec-4.2.1: ERR_SEC_AUTH (auth failure errors)
#define EVENT_AUTH_SEC 0xC1     // ASYMMETRY [Section header]
#define EVENT_AUTH_REQUEST 0x62 // ASYMMETRY Auth challenge issued
#define EVENT_AUTH_OK 0x63      // MARGIN    Authentication succeeded
#define EVENT_AUTH_REVOKE 0xE2  // MARGIN    Auth token revoked
#define EVENT_AUTH_EXPIRE 0xE3  // ASYMMETRY Auth token expired

// =============================================================================
// DOMAIN Q1.3: LIFECYCLE & INITIALIZATION  (0x00 -> 0x81)
// Dual of Q4/Domain-4.1: ERR_STORAGE_DOM (file/volume/block errors)
// =============================================================================

// Section Q1.3.1: System Init Events  (0x81 -> 0x42)
// Dual of Sec-4.1.4: ERR_STORAGE_BLOCK (block device errors)
#define EVENT_LIFE_DOM 0x81    // ASYMMETRY [Domain header] MIRROR
#define EVENT_INIT_SEC 0x42    // ASYMMETRY [Section header] MIRROR
#define EVENT_SYS_READY 0x24   // ASYMMETRY System/module ready MIRROR
#define EVENT_MODULE_INIT 0x25 // ASYMMETRY Module initialized
#define EVENT_DEP_OK 0xA4      // ASYMMETRY Dependency satisfied
#define EVENT_REINIT 0xA5      // ANTICODE  Module reinitialized MIRROR

// Section Q1.3.2: State Transition Events  (0x81 -> 0x43)
// Dual of Sec-4.1.3: ERR_STORAGE_VOLUME (volume/mount errors)
#define EVENT_STATE_SEC 0x43    // ASYMMETRY [Section header]
#define EVENT_STATE_ENTER 0x26  // ASYMMETRY FSM state entered
#define EVENT_STATE_EXIT 0x27   // MARGIN    FSM state exited
#define EVENT_STATE_STABLE 0xA6 // MARGIN    FSM state stabilized
#define EVENT_STATE_CHANGE 0xA7 // ASYMMETRY FSM state transition

// Section Q1.3.3: Reference Counting Events  (0x81 -> 0xC2)
// Dual of Sec-4.1.2: ERR_STORAGE_ATTR (file attribute errors)
#define EVENT_REF_SEC 0xC2     // ASYMMETRY [Section header]
#define EVENT_REF_INC 0x64     // ASYMMETRY Reference count incremented
#define EVENT_REF_DEC 0x65     // MARGIN    Reference count decremented
#define EVENT_REF_ZERO 0xE4    // MARGIN    Reference count reached zero
#define EVENT_REF_RELEASE 0xE5 // ASYMMETRY Object fully released

// Section Q1.3.4: Cleanup & Finalization Events  (0x81 -> 0xC3)
// Dual of Sec-4.1.1: ERR_STORAGE_FILE (file not found / corrupt)
// ANTICODE section — balanced, complementary, handshake-style
#define EVENT_CLEAN_SEC 0xC3     // ANTICODE  [Section header] SHADOW+MIRROR
#define EVENT_CLEANUP_DONE 0x66  // MARGIN    Cleanup completed TWIN+MIRROR
#define EVENT_RESOURCE_REL 0x67  // ASYMMETRY Resource released
#define EVENT_RESOURCE_CLR 0xE6  // ASYMMETRY Resource cleared/reset
#define EVENT_RESOURCE_FREE 0xE7 // ASYMMETRY Resource freed MIRROR

// =============================================================================
// QUADRANT 2: BEFORE / EXTERNAL I/O (root=0x55)
// ROOTs dual: Q3 errors (parsing/queues/math)
// Semantics: networking, IPC, user input, bus/serial, timers, ordering
// =============================================================================
#define EVENT_SYNC 0x55 // PARITY    [Root] External I/O oscillation

// =============================================================================
// DOMAIN Q2.1: NETWORKING & SOCKETS  (0x55 -> 0x2A)
// Dual of Q3/Domain-3.3: ERR_MATH_DOM (arithmetic/float/logic/crypto)
// =============================================================================

// Section Q2.1.1: Socket Lifecycle Events  (0x2A -> 0x14)
// Dual of Sec-3.3.4: ERR_MATH_CRYPTO (key/IV/tag/padding errors)
#define EVENT_NET_DOM 0x2A     // ASYMMETRY [Domain header]
#define EVENT_SOCK_SEC 0x14    // ASYMMETRY [Section header]
#define EVENT_SOCK_BOUND 0x08  // IMPULSE   Socket bound to port
#define EVENT_SOCK_LISTEN 0x09 // ASYMMETRY Socket entered listening state
#define EVENT_SOCK_ACCEPT 0x88 // REPEATER  Incoming connection accepted TWIN
#define EVENT_SOCK_OPEN 0x89   // ASYMMETRY Outbound connection established

// Section Q2.1.2: Connection State Events  (0x2A -> 0x15)
// Dual of Sec-3.3.3: ERR_MATH_LOGIC (assert/invariant/unreachable)
#define EVENT_CONN_SEC 0x15    // ASYMMETRY [Section header]
#define EVENT_CONN_ACTIVE 0x0A // ASYMMETRY Connection is active
#define EVENT_CONN_RESET 0x0B  // ASYMMETRY Connection reset by peer
#define EVENT_CONN_CLOSE 0x8A  // ASYMMETRY Connection closed
#define EVENT_CONN_KEEP 0x8B   // MARGIN    Keepalive acknowledged

// Section Q2.1.3: DNS & Resolution Events  (0x2A -> 0x94)
// Dual of Sec-3.3.2: ERR_MATH_FLOAT (NaN/INF/denorm/inexact)
#define EVENT_DNS_SEC 0x94    // ASYMMETRY [Section header]
#define EVENT_DNS_QUERY 0x48  // ASYMMETRY DNS query sent
#define EVENT_DNS_REPLY 0x49  // ASYMMETRY DNS reply received
#define EVENT_DNS_CACHED 0xC8 // ASYMMETRY DNS result from cache
#define EVENT_DNS_OK 0xC9     // MARGIN    DNS resolution complete

// Section Q2.1.4: Protocol & Channel Events  (0x2A -> 0x95)
// Dual of Sec-3.3.1: ERR_MATH_ARITH (division/overflow/underflow/precision)
// MARGIN section — balanced operational states
#define EVENT_PROTO_SEC 0x95    // MARGIN    [Section header]
#define EVENT_PROTO_SHAKE 0x4A  // ASYMMETRY Protocol handshake initiated
#define EVENT_PROTO_ACK 0x4B    // ANTICODE  Protocol step acknowledged
#define EVENT_CHANNEL_UP 0xCA   // MARGIN    Channel/session established
#define EVENT_CHANNEL_DOWN 0xCB // ASYMMETRY Channel/session closed

// =============================================================================
// DOMAIN Q2.2: TIMING, ORDERING & QUEUES  (0x55 -> 0x2B)
// Dual of Q3/Domain-3.2: ERR_QUEUE_DOM (queue/collection/ordering errors)
// MARGIN domain — high entropy, dynamic runtime
// =============================================================================

// Section Q2.2.1: Collection Events  (0x2B -> 0x16)
// Dual of Sec-3.2.4: ERR_COLLECTION_STATE (lock/modified/index/key errors)
#define EVENT_TIME_DOM 0x2B    // MARGIN    [Domain header]
#define EVENT_COLL_SEC 0x16    // ASYMMETRY [Section header]
#define EVENT_COLL_ADD 0x0C    // ASYMMETRY Item added to collection
#define EVENT_COLL_REMOVE 0x0D // ASYMMETRY Item removed from collection
#define EVENT_COLL_UPDATE 0x8C // ASYMMETRY Item in collection updated
#define EVENT_COLL_READY 0x8D  // MARGIN    Collection ready for access

// Section Q2.2.2: Ordering & Priority Events  (0x2B -> 0x17)
// Dual of Sec-3.2.3: ERR_QUEUE_ORDER (sequence/priority/duplicate/conflict)
// MARGIN section
#define EVENT_ORDER_SEC 0x17    // MARGIN    [Section header]
#define EVENT_PRIO_SET 0x0E     // ASYMMETRY Priority assigned
#define EVENT_PRIO_ADJ 0x0F     // ANTICODE  Priority adjusted SHADOW
#define EVENT_ORDER_DONE 0x8E   // MARGIN    Ordering/sort complete
#define EVENT_ORDER_CHANGE 0x8F // ASYMMETRY Order changed/requeued

// Section Q2.2.3: Message Queue Events  (0x2B -> 0x96)
// Dual of Sec-3.2.2: ERR_QUEUE_OP (enqueue/dequeue/peek/clear errors)
// ANTICODE section — shadow+balanced, handshake semantics
#define EVENT_MSGQ_SEC 0x96 // ANTICODE  [Section header] SHADOW
#define EVENT_MSG_PUSH                                                         \
  0x4C // ASYMMETRY Message pushed to queue; also dual of ERR_QUEUE_CLEAR(0xB3)
#define EVENT_MSG_POP 0x4D   // MARGIN    Message popped from queue
#define EVENT_MSG_ACK 0xCC   // MARGIN    Message acknowledged TWIN
#define EVENT_MSG_FLUSH 0xCD // ASYMMETRY Message queue flushed

// Section Q2.2.4: Timer & Frame Events  (0x2B -> 0x97)
// Dual of Sec-3.2.1: ERR_QUEUE_STATE (empty/full/overflow/underflow)
#define EVENT_TIMER_SEC 0x97   // ASYMMETRY [Section header]
#define EVENT_TIMER 0x4E       // MARGIN    Timer fired
#define EVENT_INTERVAL 0x4F    // ASYMMETRY Periodic interval tick
#define EVENT_HEARTBEAT 0xCE   // ASYMMETRY Heartbeat / health-check tick
#define EVENT_FRAME_BEGIN 0xCF // ASYMMETRY Frame / cycle start

// =============================================================================
// DOMAIN Q2.3: PERIPHERALS, BUS & USER INPUT  (0x55 -> 0xAB)
// Dual of Q3/Domain-3.1: ERR_PARSE_DOM (text/struct/binary/validation errors)
// =============================================================================

// Section Q2.3.1: Serial Communication Events  (0xAB -> 0x56)
// Dual of Sec-3.1.4: ERR_PARSE_VALID (schema/constraint/pattern/required)
// MARGIN section
#define EVENT_IO_DOM 0xAB       // ASYMMETRY [Domain header]
#define EVENT_SERIAL_SEC 0x56   // MARGIN    [Section header]
#define EVENT_SERIAL_RX 0x2C    // ASYMMETRY UART / serial data received
#define EVENT_SERIAL_TX 0x2D    // ANTICODE  UART / serial data sent SHADOW
#define EVENT_SERIAL_OPEN 0xAC  // MARGIN    Serial port opened
#define EVENT_SERIAL_CLOSE 0xAD // ASYMMETRY Serial port closed

// Section Q2.3.2: Bus Communication Events  (0xAB -> 0x57)
// Dual of Sec-3.1.3: ERR_PARSE_BINARY (header/magic/version/checksum)
#define EVENT_BUS_SEC 0x57   // ASYMMETRY [Section header]
#define EVENT_BUS_MATCH 0x2E // MARGIN    Bus slave addressed / matched
#define EVENT_BUS_START 0x2F // ASYMMETRY Bus START condition
#define EVENT_BUS_RX 0xAE    // ASYMMETRY Bus data received
#define EVENT_BUS_TX 0xAF    // ASYMMETRY Bus transmit buffer sent

// Section Q2.3.3: Text & Command Input Events  (0xAB -> 0xD6)
// Dual of Sec-3.1.2: ERR_PARSE_STRUCT (JSON/YAML syntax/type/malformed)
#define EVENT_INPUT_SEC 0xD6  // ASYMMETRY [Section header]
#define EVENT_CMD_RECV 0x6C   // MARGIN    CLI/AT/RPC command received
#define EVENT_TEXT_RECV 0x6D  // ASYMMETRY UTF-8 text string received
#define EVENT_TEXT_START 0xEC // ASYMMETRY IME composition started
#define EVENT_TEXT_END 0xED   // ASYMMETRY IME composition ended

// Section Q2.3.4: Keyboard & Char Events  (0xAB -> 0xD7)
// Dual of Sec-3.1.1: ERR_PARSE_TEXT (encode/decode/trunc/invalid)
// Note: key value encoded as (process_data_t)(uintptr_t)keycode — no alloc
#define EVENT_KEY_SEC 0xD7    // ASYMMETRY [Section header]
#define EVENT_KEY_DOWN 0x6E   // ASYMMETRY Physical key pressed
#define EVENT_KEY_UP 0x6F     // ASYMMETRY Physical key released
#define EVENT_KEY_REPEAT 0xEE // REPEATER  Key auto-repeat event TWIN
#define EVENT_KEY_ALT 0xEF    // IMPULSE   Non-printable / alternate key

// =============================================================================
// QUADRANT 3: AFTER / DATA FLOW (root=0xAA)
// ROOTs dual: Q2 errors (networking/timing/peripherals)
// Semantics: GPIO, ADC, bus completion, sync, IRQ, network data
// =============================================================================
#define EVENT_FLOW 0xAA // PARITY    [Root] Internal data flow

// =============================================================================
// DOMAIN Q3.1: SIGNAL DETECTION & ANALOG  (0xAA -> 0x54)
// Dual of Q2/Domain-2.3: ERR_IO_DOM (serial/bus/analog/GPIO errors)
// =============================================================================

// Section Q3.1.1: GPIO Signal Events  (0x54 -> 0x28)
// Dual of Sec-2.3.4: ERR_IO_GPIO (config/locked/state/interrupt errors)
#define EVENT_SIGNAL_DOM 0x54   // ASYMMETRY [Domain header]
#define EVENT_GPIO_SEC 0x28     // ASYMMETRY [Section header]
#define EVENT_GPIO_RISING 0x10  // IMPULSE   GPIO rising edge detected
#define EVENT_GPIO_FALLING 0x11 // REPEATER  GPIO falling edge detected TWIN
#define EVENT_GPIO_CHANGE 0x90  // ASYMMETRY GPIO level change (both edges)
#define EVENT_GPIO_MODE 0x91    // ASYMMETRY GPIO pin mode changed

// Section Q3.1.2: Analog I/O Events  (0x54 -> 0x29)
// Dual of Sec-2.3.3: ERR_IO_ANALOG (ADC saturated/timeout, DAC underrun)
#define EVENT_ADC_SEC 0x29       // ASYMMETRY [Section header]
#define EVENT_ADC_READY 0x12     // ASYMMETRY ADC result ready to read
#define EVENT_SAMPLE_TICK 0x13   // ASYMMETRY Sample timer tick
#define EVENT_DAC_DONE 0x92      // ASYMMETRY DAC output complete
#define EVENT_CONV_COMPLETE 0x93 // MARGIN    ADC/DAC conversion complete

// Section Q3.1.3: Bus Transfer Completion Events  (0x54 -> 0xA8)
// Dual of Sec-2.3.2: ERR_IO_BUS (NACK/arbitration/timeout/bus error)
#define EVENT_SDATA_SEC 0xA8   // ASYMMETRY [Section header]
#define EVENT_BUS_STOP 0x50    // ASYMMETRY Bus STOP condition
#define EVENT_BUS_TX_DONE 0x51 // ASYMMETRY Bus transmit acknowledged
#define EVENT_BUS_RX_DONE 0xD0 // ASYMMETRY Bus receive complete
#define EVENT_BUS_DONE 0xD1    // MARGIN    Bus transaction complete

// Section Q3.1.4: Device Data Events  (0x54 -> 0xA9)
// Dual of Sec-2.3.1: ERR_IO_SERIAL (baud/frame/parity/overrun errors)
// MARGIN section
#define EVENT_DATA_SEC 0xA9     // MARGIN    [Section header]
#define EVENT_DEVICE_DATA 0x52  // ASYMMETRY Device data available
#define EVENT_DEVICE_READY 0x53 // MARGIN    Device ready for next op
#define EVENT_DATA_VALID 0xD2   // ANTICODE  Data validated / checked SHADOW
#define EVENT_DATA_RECV 0xD3    // ASYMMETRY Data received and buffered

// =============================================================================
// DOMAIN Q3.2: TIMING, SYNC & IRQ  (0xAA -> 0xD4)
// Dual of Q2/Domain-2.2: ERR_TIME_DOM (interrupt/clock/sync/watchdog errors)
// MARGIN domain
// =============================================================================

// Section Q3.2.1: Watchdog & Power Events  (0xD4 -> 0x68)
// Dual of Sec-2.2.4: ERR_TIME_WDT (expired/reset/early/config errors)
#define EVENT_TIMING_DOM 0xD4   // MARGIN    [Domain header]
#define EVENT_WDT_SEC 0x68      // ASYMMETRY [Section header]
#define EVENT_WDT_KICK 0x30     // ASYMMETRY Watchdog kicked / refreshed
#define EVENT_WDT_ARMED 0x31    // ASYMMETRY Watchdog armed
#define EVENT_WDT_DISARMED 0xB0 // ASYMMETRY Watchdog disarmed
#define EVENT_POWER_OK 0xB1     // MARGIN    System power stable

// Section Q3.2.2: Synchronization Events  (0xD4 -> 0x69)
// Dual of Sec-2.2.3: ERR_TIME_SYNC (failed/timeout/barrier/lost errors)
// ANTICODE section — shadow+balanced, handshake/acknowledge semantics
#define EVENT_SYNC_SEC 0x69     // ANTICODE  [Section header] SHADOW
#define EVENT_SYNC_DONE 0x32    // ASYMMETRY Synchronization complete
#define EVENT_SYNC_LOCK 0x33    // MARGIN    Phase / clock locked TWIN
#define EVENT_BARRIER_PASS 0xB2 // MARGIN    Barrier passed
#define EVENT_PHASE_LOCK 0xB3   // ASYMMETRY Phase lock achieved

// Section Q3.2.3: Clock & Frame Events  (0xD4 -> 0xE8)
// Dual of Sec-2.2.2: ERR_TIME_CLOCK (not-ready/unstable/expired/overflow)
// MARGIN section
#define EVENT_CLK_SEC 0xE8    // MARGIN    [Section header]
#define EVENT_CLK_READY 0x70  // ASYMMETRY Clock source ready
#define EVENT_CLK_STABLE 0x71 // MARGIN    Clock stable / locked
#define EVENT_FRAME_END 0xF0  // ANTICODE  Frame / cycle ended SHADOW
#define EVENT_FRAME_SYNC 0xF1 // ASYMMETRY Frame synchronised

// Section Q3.2.4: Interrupt Events  (0xD4 -> 0xE9)
// Dual of Sec-2.2.1: ERR_TIME_IRQ (disabled/pending/nested/priority errors)
#define EVENT_IRQ_SEC 0xE9   // ASYMMETRY [Section header]
#define EVENT_IRQ_FIRED 0x72 // MARGIN    Hardware interrupt triggered
#define EVENT_IRQ_ACK 0x73   // ASYMMETRY Interrupt acknowledged
#define EVENT_IRQ_DONE 0xF2  // ASYMMETRY Interrupt service routine done
#define EVENT_IRQ_CLEAR 0xF3 // ASYMMETRY Interrupt flag cleared

// =============================================================================
// DOMAIN Q3.3: NETWORK DATA & CHANNEL CONTROL  (0xAA -> 0xD5)
// Dual of Q2/Domain-2.1: ERR_NET_DOM (socket/conn/DNS/protocol errors)
// =============================================================================

// Section Q3.3.1: Data Plane Events  (0xD5 -> 0x6A)
// Dual of Sec-2.1.4: ERR_NET_PROTO (version/format/sequence/state errors)
// MARGIN section
#define EVENT_NET_DATA_DOM 0xD5   // ASYMMETRY [Domain header]
#define EVENT_NETDATA_SEC 0x6A    // MARGIN    [Section header]
#define EVENT_ECHO_REPLY 0x34     // ASYMMETRY Ping / echo response received
#define EVENT_RECV_READY 0x35     // MARGIN    Receive buffer has data
#define EVENT_HANDSHAKE_DONE 0xB4 // ANTICODE  TLS/WS handshake complete SHADOW
#define EVENT_SEND_READY 0xB5     // ASYMMETRY Send buffer has free space

// Section Q3.3.2: Network Control Events  (0xD5 -> 0x6B)
// Dual of Sec-2.1.3: ERR_NET_DNS (nxdomain/timeout/servfail/config errors)
#define EVENT_NETCTRL_SEC 0x6B    // ASYMMETRY [Section header]
#define EVENT_SOCKET_RESOLVE 0x36 // MARGIN    Socket DNS resolved
#define EVENT_OOB_DATA 0x37       // ASYMMETRY Out-of-band (urgent) data arrived
#define EVENT_SOCKET_FRAME 0xB6 // ASYMMETRY UDP packet / WS data frame arrived
#define EVENT_SOCKET_KEEP 0xB7  // ASYMMETRY Socket keepalive received

// Section Q3.3.3: Connection State Events  (0xD5 -> 0xEA)
// Dual of Sec-2.1.2: ERR_NET_CONN (refused/reset/closed/timeout errors)
#define EVENT_NETSTATE_SEC 0xEA    // ASYMMETRY [Section header]
#define EVENT_SOCKET_DISCONN 0x74  // MARGIN    Connection closed by peer
#define EVENT_SOCKET_SHUTDOWN 0x75 // ASYMMETRY Half-close (shutdown) initiated
#define EVENT_SOCKET_ERROR 0xF4    // ASYMMETRY Network error (data=error_code)
#define EVENT_NET_DOWN 0xF5        // ASYMMETRY Network interface down

// Section Q3.3.4: Channel & Application Events  (0xD5 -> 0xEB)
// Dual of Sec-2.1.1: ERR_NET_SOCK (create/bind/listen/accept errors)
#define EVENT_CHANNEL_SEC 0xEB  // ASYMMETRY [Section header]
#define EVENT_CHANNEL_REQ 0x76  // ASYMMETRY HTTP/WS request received
#define EVENT_CHANNEL_RESP 0x77 // REPEATER  HTTP/WS response received TWIN
#define EVENT_CHANNEL_UPG 0xF6  // ASYMMETRY Protocol upgrade (HTTP->WS)
#define EVENT_OOB_RECV 0xF7     // IMPULSE   Out-of-band receipt confirmed

// =============================================================================
// QUADRANT 4: RUN / STORAGE / SECURITY (root=0xFF)
// ROOTs dual: Q1 errors (system state/memory/lifecycle/access)
// Semantics: file/volume, auth, process running, arg validation
// =============================================================================

// =============================================================================
// DOMAIN Q4.1: STORAGE & FILESYSTEM  (0xFF -> 0x7E)
// Dual of Q1/Domain-1.3: ERR_LIFE_DOM (init/state/refcount/cleanup errors)
// =============================================================================

// Section Q4.1.1: File Operation Events  (0x7E -> 0x3C)
// Dual of Sec-1.3.4: ERR_LIFE_CLEAN (cleanup/partial/busy/leaked errors)
// ANTICODE+MIRROR section — handshake, bidirectional
#define EVENT_STORAGE_DOM 0x7E // ASYMMETRY [Domain header] MIRROR
#define EVENT_FILE_SEC 0x3C    // ANTICODE  [Section header] SHADOW+MIRROR
#define EVENT_FILE_OPEN 0x18   // ASYMMETRY File opened MIRROR
#define EVENT_FILE_CLOSE 0x19  // ASYMMETRY File closed
#define EVENT_FILE_DELETE 0x98 // ASYMMETRY File deleted
#define EVENT_FILE_RENAME 0x99 // MARGIN    File renamed TWIN+MIRROR

// Section Q4.1.2: File Attribute Events  (0x7E -> 0x3D)
// Dual of Sec-1.3.3: ERR_LIFE_REF (ref-zero/overflow/leak/dangling errors)
#define EVENT_ATTR_SEC 0x3D    // ASYMMETRY [Section header]
#define EVENT_ATTR_READ 0x1A   // ASYMMETRY File attributes read
#define EVENT_ATTR_WRITE 0x1B  // MARGIN    File attributes written
#define EVENT_ATTR_CHANGE 0x9A // MARGIN    File attributes changed
#define EVENT_ATTR_WATCH 0x9B  // ASYMMETRY File attribute watch registered

// Section Q4.1.3: Volume & Mount Events  (0x7E -> 0xBC)
// Dual of Sec-1.3.2: ERR_LIFE_STATE (invalid/transition/locked/frozen errors)
#define EVENT_VOL_SEC 0xBC     // ASYMMETRY [Section header]
#define EVENT_VOL_MOUNT 0x58   // ASYMMETRY Volume mounted
#define EVENT_VOL_SYNC 0x59    // MARGIN    Volume flushed/synced
#define EVENT_VOL_UNMOUNT 0xD8 // MARGIN    Volume unmounted
#define EVENT_VOL_TRIM 0xD9    // ASYMMETRY Volume TRIM/erase issued

// Section Q4.1.4: Block Device Events  (0x7E -> 0xBD)
// Dual of Sec-1.3.1: ERR_LIFE_INIT (failed/timeout/dependency/nosup errors)
// MIRROR section
#define EVENT_BLK_SEC 0xBD    // ASYMMETRY [Section header] MIRROR
#define EVENT_BLK_READ 0x5A   // ANTICODE  Block read complete SHADOW+MIRROR
#define EVENT_BLK_WRITE 0x5B  // ASYMMETRY Block write complete
#define EVENT_BLK_ERASE 0xDA  // ASYMMETRY Block erased
#define EVENT_BLK_VERIFY 0xDB // ASYMMETRY Block verify complete MIRROR

// =============================================================================
// DOMAIN Q4.2: SECURITY & POLICY  (0xFF -> 0x7F)
// Dual of Q1/Domain-1.2: ERR_MEM_DOM (heap/stack/buffer/align errors)
// IMPULSE domain
// =============================================================================

// Section Q4.2.1: Authentication Result Events  (0x7F -> 0x3E)
// Dual of Sec-1.2.4: ERR_MEM_ALIGN (addr/size/pagefault/protect errors)
#define EVENT_SEC_DOM 0x7F      // IMPULSE   [Domain header]
#define EVENT_AUTH2_SEC 0x3E    // ASYMMETRY [Section header]
#define EVENT_AUTH_GRANT 0x1C   // ASYMMETRY Auth/access granted
#define EVENT_AUTH_DENY_EV 0x1D // MARGIN    Auth/access denied event
#define EVENT_CERT_OK 0x9C      // MARGIN    Certificate/credential verified
#define EVENT_CERT_ISSUE 0x9D   // ASYMMETRY Certificate/credential issued

// Section Q4.2.2: Encryption Result Events  (0x7F -> 0x3F)
// Dual of Sec-1.2.3: ERR_MEM_BUFFER (overflow/underflow/full/empty errors)
#define EVENT_CRYPT_SEC 0x3F   // ASYMMETRY [Section header]
#define EVENT_SIGN_DONE 0x1E   // ANTICODE  Signing operation complete SHADOW
#define EVENT_VERIFY_OK 0x1F   // ASYMMETRY Signature verified
#define EVENT_KEY_DERIVED 0x9E // ASYMMETRY Key derivation complete
#define EVENT_HASH_DONE 0x9F   // ASYMMETRY Hash computation complete

// Section Q4.2.3: Policy & Quota Events  (0x7F -> 0xBE)
// Dual of Sec-1.2.2: ERR_MEM_BOUNDS (stack ov/underflow, bounds errors)
#define EVENT_POLICY_SEC 0xBE   // ASYMMETRY [Section header]
#define EVENT_POLICY_OK 0x5C    // MARGIN    Policy check passed
#define EVENT_QUOTA_OK 0x5D     // ASYMMETRY Quota / rate-limit OK
#define EVENT_RATE_OK 0xDC      // ASYMMETRY Rate limit not exceeded
#define EVENT_POLICY_RESET 0xDD // REPEATER  Policy counters reset TWIN

// Section Q4.2.4: Audit & Logging Events  (0x7F -> 0xBF)
// Dual of Sec-1.2.1: ERR_MEM_HEAP (OOM/fragment/corrupt/double-free errors)
// IMPULSE section
#define EVENT_AUDIT_SEC 0xBF   // IMPULSE   [Section header]
#define EVENT_AUDIT_WRITE 0x5E // ASYMMETRY Audit log entry written
#define EVENT_AUDIT_DONE 0x5F  // ASYMMETRY Audit cycle complete
#define EVENT_LOG_SYNC 0xDE    // ASYMMETRY Audit log flushed/synced
#define EVENT_LOG_OK 0xDF      // IMPULSE   Log integrity verified

// =============================================================================
// DOMAIN Q4.3: PROCESS STATE & ARGUMENT VALIDITY  (0xFF -> 0xFE)
// Dual of Q1/Domain-1.1: ERR_SYS_STATE (access/handle/proc/arg errors)
// IMPULSE domain
// =============================================================================

// Section Q4.3.1: Argument & Validation Events  (0xFE -> 0x7C)
// Dual of Sec-1.1.4: ERR_SYS_ARGUMENT (null/invalid/range/type errors)
#define EVENT_PSTATE_DOM 0xFE        // IMPULSE   [Domain header]
#define EVENT_ARG_SEC 0x7C           // ASYMMETRY [Section header]
#define EVENT_LOCK_OK 0x38           // ASYMMETRY Lock operation succeeded
#define EVENT_LOCK_TIMEOUT_EV 0x39   // MARGIN   Lock acquired after wait
#define EVENT_DEADLOCK_RESOLVED 0xB8 // MARGIN Deadlock resolved
#define EVENT_OWNER_CHANGED 0xB9     // ASYMMETRY Lock owner changed

// Section Q4.3.2: Process State Machine Events  (0xFE -> 0x7D)
// Dual of Sec-1.1.3: ERR_SYS_PROC (cancelled/killed/orphan/zombie errors)
#define EVENT_PSCHED_SEC 0x7D    // ASYMMETRY [Section header]
#define EVENT_PROC_ALIVE 0x3A    // MARGIN    Process is alive/running
#define EVENT_PROC_IDLE 0x3B     // ASYMMETRY Process is idle/waiting
#define EVENT_MSG_SENT 0xBA      // ASYMMETRY IPC message sent
#define EVENT_MSG_DELIVERED 0xBB // REPEATER  IPC message delivered TWIN

// Section Q4.3.3: Atomic & Race Events  (0xFE -> 0xFC=RESERVED=EVENT_EXIT)
// Dual of Sec-1.1.2: ERR_SYS_HANDLE (handle errors)
// Note: section header 0xFC is shared with reserved EVENT_EXIT
#define EVENT_ATOMIC_OK 0x87 // ANTICODE  Atomic CAS succeeded (see also Q1.1.2)
// Note: leaf codes here overlap with Q1.1.2 leaves when expanded — these are
// the SAME leaf codes viewed from the opposite direction (events driving guard
// checks). They are intentionally aliased below for clarity in different
// contexts:
#define EVENT_HANDLE_ACQ                                                       \
  0x87 // ANTICODE  Handle atomically acquired (=EVENT_HANDLE_WAKE)
#define EVENT_HANDLE_REL                                                       \
  0x86 // ASYMMETRY Handle atomically released (=EVENT_HANDLE_READY)
#define EVENT_CAS_DONE                                                         \
  0x07 // ASYMMETRY Compare-and-swap succeeded (=EVENT_HANDLE_CLOSE)
#define EVENT_CAS_RETRY                                                        \
  0x06 // ASYMMETRY Compare-and-swap retry (=EVENT_HANDLE_OPEN)

// Section Q4.3.4: Access Control Events  (0xFE -> 0xFD=RESERVED=EVENT_POLL)
// Dual of Sec-1.1.1: ERR_SYS_ACCESS (denied/readonly/locked/forbidden errors)
// Note: section header 0xFD is shared with reserved EVENT_POLL
// These leaf codes alias Q1.1.1 leaves viewed from the "granted access"
// perspective:
#define EVENT_ACCESS_GRANTED 0x04 // IMPULSE   Access granted (=EVENT_LOCK_ACQ)
#define EVENT_ACCESS_OK_EV                                                     \
  0x05 // ASYMMETRY Access check passed (=EVENT_LOCK_REL)
#define EVENT_UNLOCK_DONE                                                      \
  0x84                       // ASYMMETRY Resource unlocked (=EVENT_CRIT_ENTER)
#define EVENT_PERMIT_OK 0x85 // ASYMMETRY Permission verified (=EVENT_CRIT_EXIT)

// =============================================================================
// CONTROLLER / SENSOR: Rich input (use ipc_msg_t* payload, not event code)
// post as EVENT_MOTION / EVENT_TOUCH / EVENT_BUTTON with ipc_msg_t* data
// ipc_msg_t.type disambiguates the sub-type (AXIS, BALL, HAT, FINGER, etc.)
// ipc_msg_t.argv[0] carries the coordinate/sensor struct pointer
//
// ALIASING CONTRACT: the input aliases below intentionally reuse codes
// from other sections (e.g. EVENT_BUTTON == EVENT_BUS_RX). A subscriber
// CANNOT tell the two apart by code alone, so an alias may only be
// posted to a process that does not also consume its aliased meaning:
//  - input aliases (EVENT_CHAR/BUTTON/MOTION/...) are posted to
//    UI/TUI-class processes, which never talk to a raw bus directly;
//  - bus/queue events go to driver-class processes, which never
//    receive user input.
// If a process must handle both worlds, disambiguate via the payload
// (ipc_msg_t.type) or split the process. The same rule applies to the
// Q4.3 access/atomic aliases of the Q1.1 lock/handle leaves.
// =============================================================================

// User input event types (scalar values cast directly to process_data_t):
#define EVENT_CHAR                                                             \
  0x6D // UTF-8 / rune16 char (=EVENT_TEXT_RECV, data=codepoint)
#define EVENT_BUTTON 0xAE // Button/finger DOWN  (=EVENT_BUS_RX, data=button_id)
#define EVENT_BUTTON_UP                                                        \
  0xAF // Button/finger UP    (=EVENT_BUS_TX, data=button_id)
#define EVENT_BUTTON_DBL                                                       \
  0xEC // Button double-click (=EVENT_TEXT_START, data=button_id)

// User input event types (vector data via ipc_msg_t*):
#define EVENT_MOTION                                                           \
  0xED // Axis/Ball/Hat moved  (=EVENT_TEXT_END, data=ipc_msg_t*)
#define EVENT_MOUSE                                                            \
  0x6C // Mouse moved          (=EVENT_CMD_RECV, data=ipc_msg_t*)
#define EVENT_WHEEL                                                            \
  0xCC // Wheel/scroll turned  (=EVENT_MSG_ACK,  data=ipc_msg_t*)
#define EVENT_TOUCH                                                            \
  0xCD // Multi-touch event    (=EVENT_MSG_FLUSH, data=ipc_msg_t*)
#define EVENT_GESTURE                                                          \
  0xCE // Gesture detected     (=EVENT_HEARTBEAT, data=ipc_msg_t*)
#define EVENT_SENSOR                                                           \
  0xCF // Sensor value changed (=EVENT_FRAME_BEGIN, data=ipc_msg_t*)
#define EVENT_CALIBRATE                                                        \
  0x4D // Sensor calibration   (=EVENT_MSG_POP,  data=ipc_msg_t*)

// =============================================================================
// SHUTDOWN / IDLE helpers
// =============================================================================
#define EVENT_IDLE_ENTER 0xB1 // System entered idle  (=EVENT_POWER_OK)
#define EVENT_SHUTDOWN 0xB8   // System shutdown req  (=EVENT_DEADLOCK_RESOLVED)
#define EVENT_PRIO_ADJ_EV                                                      \
  0x0F // Priority adjusted    (=EVENT_PRIO_ADJ ANTICODE)

// =============================================================================
// DUALITY MACROS
// =============================================================================
#define EVENT_TO_ERROR(ev) ((uint8_t)(~(ev) & 0xFF))
#define ERROR_TO_EVENT(err) ((uint8_t)(~(err) & 0xFF))

#endif // __EVENTS_H__
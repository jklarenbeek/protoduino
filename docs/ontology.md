# Protoduino Event-Error Ontology

## A Mathematical Framework for Embedded System State Representation Integrated with Protothreads and Kernel Scheduler

**Version:** 1.2
**Author:** jklarenbeek@gmail.com
**Date:** July 2026

> **Consistency note:** all constant names and values in this document
> refer to the real headers `src/sys/events.h` and `src/sys/errors.h`.
> The math is claimed by `docs/ontology-claim.js`, proven exhaustively by
> `docs/ontology-proof.csv`, and enforced against the headers by
> `tools/ontology-sync.js` (run `node tools/ontology-sync.js --check`).

---

## Table of Contents

1. [Overview](#overview)
2. [Theoretical Foundation](#theoretical-foundation)
3. [Mathematical Properties](#mathematical-properties)
4. [Generation Recipe](#generation-recipe)
5. [Hierarchical Structure](#hierarchical-structure)
6. [Symmetry Classes](#symmetry-classes)
7. [Event-Error Duality](#event-error-duality)
8. [Practical Applications](#practical-applications)
9. [Implementation Guide](#implementation-guide)
10. [Advanced Topics](#advanced-topics)
11. [Validation Checklist](#validation-checklist)
12. [References & Further Reading](#references--further-reading)

---

## Overview

The Protoduino Event-Error Ontology is a **mathematically rigorous, self-consistent 8-bit taxonomy** that models the complete lifecycle of embedded systems through complementary event and error spaces, fully integrated with Protothreads v2 for lightweight cooperative multitasking and the Protoduino kernel scheduler for event-driven process management. Unlike traditional error code schemes that grow organically, this system leverages **group theory, information theory, and bitwise symmetry operations** to create a predictable, analyzable framework for system state representation, where events trigger scheduler actions and errors propagate through Protothread exceptions.

### The Heatmap: Why This Ontology Exists

The design goal of the ontology is **observability on the smallest possible budget**. Because every code is one byte and `error = ~event`, a running system can simply *stream its raw event and error codes* (over UART, a pipe, or a radio) and the receiving side can render them as a **16×16 heatmap** over time:

- Each code is a cell in the 16×16 matrix (high nibble = row, low nibble = column).
- An event and its failure mode are **point-symmetric through the center** of the map (`~x` flips every bit, so cell *(r, c)* maps to *(15−r, 15−c)*).
- A healthy system lights up its event basins; a failing system lights up the *mirrored* cells. Drift between the two halves is visible at a glance, without decoding a single string.
- The hierarchy (ROOT → DOMAIN → SECTION → LEAF) means related activity clusters spatially, and the symmetry classes predict *where* activity should appear (BALANCED codes in steady-state runtime, IMPULSE codes at boot/shutdown edges).

This is why the taxonomy fits an ATtiny: with `ERRORS_CONF_STRINGS 0` no string table is compiled at all — the device streams bytes and the host does the interpretation using `docs/ontology-proof.csv`. Larger systems can enable flash-resident string tables (tiny or verbose) for a local TUI, but the heatmap workflow needs neither.

### Key Innovations

- **Dual Ontology**: Events and errors exist as inverse pairs (`~event & 0xFF = error`), with events posted via the scheduler's `process_post()` and errors raised in Protothreads using `PT_RAISE` or `PT_THROW`.
- **Hierarchical Depth**: 4-level tree structure (ROOT → DOMAIN → SECTION → LEAF), mapping to Protothread states and scheduler lifecycles.
- **Symmetry Preservation**: TWINS, SHADOWS, MIRRORS provide structural invariants, ensuring consistent behavior in Protothread finalization (`PT_FINALLY`) and scheduler error logging (`PROCESS_EVENT_ERROR`).
- **Information Theoretic**: Shannon entropy and Hamming distance encode semantic relationships, aiding in scheduler priority decisions and Protothread error recovery.
- **Finite State Automaton**: Natural mapping to FSM transitions and error recovery, with macros like `PROCESS_WAIT_EVENT_UNTIL` handling scheduler-dispatched events.

### Why This Matters

Traditional error handling treats errors as exceptions—afterthoughts to normal flow. This ontology treats **events and errors as dual manifestations of the same underlying process**, seamlessly integrated with Protoduino's components. When `EVENT_DEVICE_DATA (0x52)` fails in a Protothread, it raises `ERR_SERIAL_OVERRUN (0xAD)` where `0xAD = ~0x52` — data arrived but could not be consumed — triggering scheduler logging via `PROCESS_EVENT_ERROR`. This isn't coincidence—it's **ontological necessity**, ensuring cooperative scheduling and exception propagation in resource-constrained environments like AVR/Arduino.

---

## Theoretical Foundation

### Ontology vs. Taxonomy

**Ontology** defines the fundamental nature and relationships of entities within a domain. **Taxonomy** organizes these entities into hierarchical classifications.

The Protoduino system is:
- **Ontologically grounded** in bitwise operations representing state transformations, aligned with Protothread control flow and scheduler event dispatching.
- **Taxonomically structured** through a 4-level hierarchy mapping to system lifecycle phases, from Protothread initialization (`PT_INIT`) to finalization (`PT_FINAL`).

### Philosophical Basis

The framework rests on three axioms, adapted to Protoduino's Protothreads and scheduler:

1. **Duality Axiom**: Every event has a corresponding failure mode (error), where scheduler-posted events (`process_post`) can trigger Protothread errors (`PT_ERROR`), enabling graceful recovery in cooperative tasks.
2. **Hierarchy Axiom**: System states decompose into ROOT → DOMAIN → SECTION → LEAF, mirroring Protothread nesting (e.g., `PT_SPAWN` for child threads) and scheduler process hierarchies.
3. **Symmetry Axiom**: Structural relationships encode semantic relationships, ensuring invariants in scheduler polls (`PROCESS_EVENT_POLL`) and Protothread symmetries (e.g., MIRRORS for stable states).

These aren't arbitrary—they reflect **how embedded systems actually work** in Protoduino:
- Initialization fails differently than shutdown (QUADRANT symmetry), with errors propagating via `PT_RAISE` to scheduler logs.
- Communication errors differ from scheduling errors (DOMAIN separation), handled via IPC pipes/messages in the scheduler.
- Sensor saturation is the inverse of sensor reading (EVENT-ERROR duality), triggering `PT_THROW` in a Protothread and `process_poll` for recovery.

### Information-Theoretic Perspective

Each 8-bit code is a **256-state discrete space** where:
- **Hamming distance** measures semantic similarity, useful for Protothread error clustering in scheduler recovery strategies.
- **Shannon entropy** quantifies information content, guiding scheduler priority (high-entropy states for runtime flow).
- **Bit operations** represent state transformations, integrated with Protothread line counters (`lc_t`) and scheduler event queues.

Example:
```
EVENT_NONE:    0x00 = 00000000  (entropy: 0.0, absolute certainty) → No event / queue sentinel.
EVENT_SYNC:    0x55 = 01010101  (entropy: 1.0, maximum oscillation) → Q2 root: external I/O flow.
EVENT_ERROR:   0xFF = 11111111  (entropy: 0.0, absolute certainty) → Terminal error event (dual of ERR_SUCCESS).
ERR_DATA_ROOT: 0xAA = 10101010  (entropy: 1.0, balanced failure)   → Q3 root: internal data errors. (Note: 0xAA = ~0x55, preserving duality and maximum entropy.)
```

All event-error pairs must use distinct codes via inversion to avoid contextual ambiguity in scheduler posting vs. Protothread raising.

The system begins and ends with **zero entropy** (deterministic states) and achieves **maximum entropy** during runtime (balanced operational states), aligning with scheduler's poll-event loop.

### Formal Notation and Definitions

Let \( x \in \{0, \dots, 255\} \) represent an 8-bit code.

- **Nibbles**: High nibble \( h(x) = (x \gg 4) \land 0x0F \); low nibble \( l(x) = x \land 0x0F \).
- **Hamming Weight**: \( w(x) = \) number of 1-bits in \( x \), also denoted as popcount(x).
- **Shannon Entropy**: \( e(x) = -p_1 \log_2 p_1 - p_0 \log_2 p_0 \), where \( p_1 = w(x)/8 \), \( p_0 = 1 - p_1 \). If \( p_1 = 0 \) or 1, \( e(x) = 0 \). (Runtime note: since entropy depends only on the popcount, `err_op_entropy()` returns the nine possible values as exact flash constants — no runtime log2.)
- **Hamming Distance**: \( d(x, y) = w(x \oplus y) \).
- **ROOT**: Codes where repeated center operations converge to themselves (depth 0).
- **DOMAIN**: Direct children of ROOTS (depth 1).
- **SECTION**: Depth 2.
- **LEAF**: Depth 3.
- **TWIN**: \( h(x) = l(x) \).
- **SHADOW**: \( h(x) = \neg l(x) \land 0x0F \).
- **MIRROR**: reverse(x) = x.
- **BALANCED**: \( w(x) = 4 \), \( e(x) \approx 1.0 \).
- **UNBALANCED**: \( w(x) \neq 4 \).
- **PARITY**: center(x) = \neg x.
- **ANTICODE**: BALANCED and SHADOW. *(Theorem: ANTICODE ≡ SHADOW — every SHADOW is balanced by construction, since \( h(x) = \neg l(x) \) forces exactly 4 ones. The BALANCED term is kept for definitional symmetry.)*
- **MARGIN**: BALANCED, not PARITY, not SHADOW.
- **ABSOLUTE**: ROOTS 0x00 or 0xFF.
- **REPEATER**: UNBALANCED TWIN, not ABSOLUTE.
- **IMPULSE**: \( w(x) = 1 \) or 7.
- **ASYMMETRY**: UNBALANCED, not TWIN, not IMPULSE.
- **Reserved Codes**: ERR_SUCCESS (0x00), ERR_YIELDING (0x01), ERR_EXITING (0x02), ERR_ENDING (0x03), ERR_FINALIZED (0xFF). These are necessary for Protothreads in the Protoduino framework: 0x00 for successful completion, 0x01 for PT_YIELD, 0x02 for PT_EXIT, 0x03 for PT_END, 0xFF for finalization. These are shared duals for events.

See `src/dbg/errors.h` for the C implementations (the code values themselves live in `src/sys/errors.h` — two different files that share a base name).

---

## Mathematical Properties

### Core Operations

The ontology defines five fundamental bitwise operations, used in Protoduino for state transitions in Protothreads and scheduler error handling:

#### 1. Inverse Operation
\[ \neg x = \sim x \land 0xFF \]
**Semantic meaning**: Failure mode of a process, mapping scheduler events to Protothread errors. Precondition: x is 8-bit. Postcondition: \( \neg (\neg x) = x \); maps each quadrant to its **dual** quadrant (root(\( \neg x \)) = \( \neg \)root(x): Q1↔Q4, Q2↔Q3).
**Example**: \( \neg \) EVENT_INIT (0x01) = ERR_DEADLOCK (0xFE), used in `PT_THROW(self, ERR_DEADLOCK)`. Exhaustive: For x=0x00, \( \neg x = 0xFF \); for x=0x55, \( \neg x = 0xAA \). Edge: x=0xFF, \( \neg x = 0x00 \) (ROOT duality).

Initialization's inverse is deadlock—a system that fails to initialize holds its resources forever, triggering scheduler `PROCESS_EVENT_ERROR`.

#### 2. Reverse Operation (Bit Reflection)
\[ \text{reverse}(x) = \] swap bits 7-0 (e.g., 0bABCD_EFGH → 0bHGFE_DCBA).
**Semantic meaning**: Symmetrical counterpart, for mirrored states in Protothread iterators (`PT_FOREACH`). Precondition: x is 8-bit. Postcondition: reverse(reverse(x)) = x; preserves balance (w(reverse(x)) = w(x)).
**Example**: reverse(0x01) = 0x80 (IMPULSE to IMPULSE). Exhaustive: For MIRRORS like 0x00, reverse(x)=x. Edge: Non-MIRRORS like 0x02 → 0x40.

#### 3. Opposite Operation (Nibble Swap)
\[ \text{opposite}(x) = (l(x) \ll 4) \lor h(x) \]
**Semantic meaning**: Complementary perspective, for quadrant swaps in scheduler broadcasts. Precondition: x is 8-bit. Postcondition: opposite(opposite(x))=x; may change root quadrant.
**Example**: opposite(0x01)=0x10 (INIT to AFTER). Exhaustive: For TWINS like 0x00, opposite(x)=x. Edge: 0x55 → 0x55 (PARITY invariant).

#### 4. Center Operation (Nuclear Convergence)
\[ \text{center}(x) = ((x \ll 1) \land 0xF0) \lor ((x \gg 1) \land 0x0F) \]
**Semantic meaning**: Parent in hierarchy, for ascending to DOMAIN/ROOT in error recovery. Precondition: x is 8-bit. Postcondition: Iterating converges to ROOT; depth decreases by 1.
**Example**: center(0x01)=0x00 (DOMAIN to ROOT). Exhaustive: Max depth=3; ROOTS fixed (center(0x00)=0x00). Edge: 0xFF → 0xFF.

#### 5. Root Operation
\[ \text{root}(x) = \] iterate center until ROOT.
**Semantic meaning**: Quadrant (semantic basin), for grouping related states in FSMs. Precondition: x is 8-bit. Postcondition: One of 4 ROOTS; inversion maps to the dual quadrant (root(~x) = ~root(x)).
**Example**: root(0x01)=0x00 (Q1: INIT). Exhaustive: 64 codes per ROOT (256/4). Edge: Non-convergent impossible by design (proven exhaustively in `ontology-proof.csv`, max depth 3).

See `src/dbg/errors.h` for C implementations; these operations apply identically to events — `src/dbg/events.h` provides the `EV_IS_*` / `ev_op_*` aliases.

---

## Generation Recipe

This section provides a prescriptive algorithm to generate the ontology from first principles. In this repository the loop is closed by three artifacts:

- **Claim** — `docs/ontology-claim.js`: the ontology math (operations, hierarchy, symmetry classes) as an importable ES module.
- **Proof** — `docs/ontology-proof.csv`: the exhaustive, deterministic (timestamp-free) classification of all 256 codes, generated from the claim.
- **Enforcement** — `tools/ontology-sync.js`: parses `src/sys/errors.h` and `src/sys/events.h`, validates every annotation and the duality against the claim, regenerates the proof CSV and the flash string tables (`src/dbg/errors-strings.inc`, `src/dbg/events-strings.inc`). `--check` mode fails CI when anything drifts.

The taxonomy *names* remain hand-authored in the headers (naming is a semantic act), but nothing about them can silently disagree with the math.

### Algorithm

1. **Enumerate Codes**: Loop x from 0 to 255.
2. **Compute Operations**: For each x, calculate inverse, reverse, opposite, center, root, depth (iterate center), distance (Hamming to 0), ones (popcount), entropy (Shannon via log2), binary string.
3. **Classify Hierarchy**: ROOT if depth=0; DOMAIN if depth=1 and root(parent)=ROOT; SECTION if depth=2; LEAF if depth=3.
4. **Classify Symmetry**: Apply definitions (e.g., TWIN if h(x)==l(x); BALANCED if ones==4).
5. **Assign Semantics**: Based on quadrant (root(x)): Q1 (0x00)=INIT; Q2 (0x55)=BEFORE; Q3 (0xAA)=AFTER; Q4 (0xFF)=RUN. Append RESERVED for 0x00,0x01,0x02,0x03,0xFF.
6. **Output CSV**: Columns: hex,classes,hierarchy,symmetry,inverse,opposite,reverse,parent,depth,distance,ones,entropy,bin,value.
7. **Validate Headers & Generate Tables**: Parse the hand-authored headers, check every annotation against the computed classes, and emit the flash string tables. Mirror macros/functions live in `src/dbg/events.h` (EV_IS_ROOT = ERR_IS_ROOT).

Pseudocode (Python-like for LLM/script):
```python
import math
def generate_ontology():
    data = []
    for x in range(256):
        inv = (~x & 0xFF)
        rev = reverse_bits(x)  # Implement SWAR reversal
        opp = ((x & 0x0F) << 4) | ((x >> 4) & 0x0F)
        cen = ((x << 1) & 0xF0) | ((x >> 1) & 0x0F)
        rt = root(x)  # Iterate cen to fixed point
        dep = depth(x)  # Count iterations
        dist = bin(x).count('1')  # To 0, or generalize
        ones = bin(x).count('1')
        p1 = ones / 8.0
        ent = 0 if p1 in (0,1) else - (p1 * math.log2(p1) + (1-p1) * math.log2(1-p1))
        classes = classify_classes(x)  # e.g., 'RESERVED|INIT' if x in [0,1,2,3,255] and rt==0
        hierarchy = classify_hierarchy(dep)
        symmetry = classify_symmetry(x, ones)
        bin_str = f'{x:08b}'
        data.append({'hex': f'0x{x:02X}', 'classes': classes, ...})  # Fill all
    write_csv('ontology-proof.csv', data)
    generate_headers(data)  # Enums, macros from data
```

Validate output against the checklist below, or simply run `node tools/ontology-sync.js --check`.

---

## Hierarchical Structure

The 256 codes partition into:
- 4 ROOTS (fixed points).
- 12 DOMAINS (direct children).
- 48 SECTIONS.
- 192 LEAFS.

Quadrants (per ROOT):
- Q1 (0x00): INIT (low entropy, deterministic boot).
- Q2 (0x55): BEFORE (high entropy, preparatory flow).
- Q3 (0xAA): AFTER (high entropy, post-action).
- Q4 (0xFF): RUN (low entropy, shutdown).

See `ontology-proof.csv` for exhaustive listing.

```mermaid
graph TD
    ROOT --> DOMAIN
    DOMAIN --> SECTION
    SECTION --> LEAF
```

---

## Symmetry Classes

- **Balanced (Popcount=4)**: High entropy for dynamic states (e.g., runtime flow in scheduler loops). Count: C(8,4)=70.
- **Unbalanced**: Low entropy for static phases (e.g., init/shutdown in Protothreads). Count: 256-70=186.
- **Parity**: Center == inverse, for movable states (e.g., 0x55/0xAA roots). Count: 2 (ROOTs).
- **Anticode**: Balanced + shadow, for handshakes (e.g., message recv in IPC). Count: 16.
- **Margin**: Balanced non-parity non-shadow, for boundaries (e.g., priority adjust in scheduler). Count: 70 - (parity + anticode count).
- **Absolute**: Abstract roots (0x00/0xFF), for absolute certainties in system start/end. Count: 2.
- **Repeater**: Unbalanced twin non-abstract, for repeating patterns in iterators. Count: 16-4=12 (non-ROOT TWINS).
- **Impulse**: 1 or 7 ones, for impulse events (e.g., boot start in scheduler). Count: C(8,1)+C(8,7)=16.
- **Asymmetry**: Unbalanced non-twin non-impulse, for general cases. Count: 186 - (repeater + impulse + absolute).

Examples tie to framework: TWINS for stable processes, SHADOWS for pipe wake callbacks.

```mermaid
graph LR
    BALANCED --> PARITY
    BALANCED --> ANTICODE
    BALANCED --> MARGIN
    UNBALANCED --> ABSOLUTE
    UNBALANCED --> REPEATER
    UNBALANCED --> IMPULSE
    UNBALANCED --> ASYMMETRY
```

Exhaustive Examples: 0x55 (BALANCED|PARITY, entropy=1.0); 0x3C (BALANCED|ANTICODE|SHADOW|MIRROR); 0x00 (UNBALANCED|ABSOLUTE|TWIN|MIRROR). Edge: 0x01 (UNBALANCED|IMPULSE, ones=1); 0xFE (UNBALANCED|IMPULSE, ones=7).

Metrics: 16 TWINS (4 per quadrant); 16 SHADOWS; 16 MIRRORS; entropy ranges 0.0-1.0; verify: sum(ones over all x)= 256*4=1024 (average 4 ones/code).

---

## Event-Error Duality

Duality ensures every scheduler event has a Protothread error counterpart:
- Event `process_post(proc, EVENT_DEVICE_DATA, data)` succeeds or fails to `PT_THROW(self, ERR_SERIAL_OVERRUN)`.
- Errors post to logger via scheduler's `PROCESS_EVENT_ERROR`, allowing recovery in parent threads (`PT_CATCHANY`).

This duality models Protoduino's cooperative nature: Events drive forward (yields/polls), errors reverse (finalization/exit). Formal: For event e, error = \neg e; root(\neg e) = \neg root(e) (dual quadrant); d(e, \neg e)=8 (max distance, full inversion).

Exhaustive Examples: EVENT_NONE (0x00) ↔ ERR_FINALIZED (0xFF); EVENT_DEVICE_DATA (0x52) ↔ ERR_SERIAL_OVERRUN (0xAD). Edge: the five reserved codes (0x00–0x03, 0xFF) are deliberately *shared* between both spaces — they are process states, not postable events or throwable errors.

```mermaid
sequenceDiagram
    participant Scheduler
    participant Protothread
    Scheduler->>Protothread: process_post(EVENT_DEVICE_DATA)
    alt Success
        Protothread-->>Scheduler: PT_WAITING
    else Failure
        Protothread->>Scheduler: PT_THROW(ERR_SERIAL_OVERRUN)
        Scheduler->>Logger: PROCESS_EVENT_ERROR
    end
```

---

## Practical Applications

### 1. Error Propagation in Protothreads

Use duality for chained recovery:
```c
PT_BEGIN(self);
PT_WAIT_UNTIL(self, ev == EVENT_DEVICE_DATA);
if (overrun) PT_RAISE(self, ERR_SERIAL_OVERRUN);  // Inverse of the event
PT_CATCHANY(self) { /* Recover */ }
PT_END(self);
```

Scheduler handles via `call_process` and error posting.

### 2. Scheduler Event Handling

Post events hierarchically:
```c
process_post(&device_proc.base, EVENT_DEVICE_DATA, data);
if (overrun) process_error(pt_process, PROCESS_EVENT_ERROR, ERR_SERIAL_OVERRUN);
```

### 3. FSM Design

Use quadrants for states: Init (Q1) → I/O (Q2) → Parsing (Q3) → Shutdown (Q4).

### 4. Diagnostic Tools & the Heatmap

Stream raw codes and render them as the 16×16 heatmap (see Overview): events and their failure modes occupy point-symmetric cells, so systemic failure shows up as activity migrating to the mirrored half of the map. Hamming distance clusters similar errors in logs; entropy spikes flag chaotic flows.

### 5. Code Compression

Compress high-frequency codes (ROOTs/DOMAINS) in scheduler queues.

### 6. Distributed System Coordination

Nodes share ontology for events/errors in IPC pipes/messages.

### Mapping to Software Flow

- Init: Low-entropy ROOT events for PT_INIT.
- Runtime: High-entropy BALANCED for PT_YIELD.
- Error: UNBALANCED for PT_THROW.
- Analysis: Trace entropy spikes (>0.9) for bottlenecks; use distance for transition costs in FSMs.

---

## Implementation Guide

### Event Handler Pattern

*(Illustrative sketch — a 256-entry registry costs RAM and suits larger targets, not ATtiny-class devices.)*

```c
// event_handler.c

#include <protoduino.h>
#include <dbg/errors.h>
#include <sys/process.h>
#include <sys/ipc.h>

typedef uint8_t (*EventHandler)(uint8_t event, void *context);

typedef struct {
    uint8_t event_code;
    EventHandler handler;
    uint8_t error_fallback;  // = ~event_code automatically
} EventRegistration;

// Global event registry
static EventRegistration event_registry[256];

void register_event_handler(uint8_t event, EventHandler handler) {
    event_registry[event].event_code = event;
    event_registry[event].handler = handler;
    event_registry[event].error_fallback = ~event & 0xFF;
}

uint8_t dispatch_event(uint8_t event, void *context) {
    if (event_registry[event].handler == NULL) {
        // No handler registered → automatic error
        return event_registry[event].error_fallback;
    }

    uint8_t result = event_registry[event].handler(event, context);

    // If handler returns non-zero, it's an error code
    if (result != 0x00) {
        // Verify error is in same semantic space
        uint8_t expected_error = ~event & 0xFF;
        if (err_op_root(result) != err_op_root(expected_error)) {
            // Handler returned error from wrong quadrant
            // → Log warning, use canonical error
            log_warning("Handler for 0x%02X returned wrong error 0x%02X",
                        event, result);
            return expected_error;
        }
    }

    return result;
}
```

### FSM Integration

```c
// state_machine.c

#include <protoduino.h>
#include <sys/process.h>

typedef struct {
    uint8_t current_state;
    uint8_t event_received;
    uint8_t next_state;
    uint8_t (*guard)(void);  // Optional guard condition
} Transition;

typedef struct {
    const char *name;
    Transition *transitions;
    size_t num_transitions;
} StateMachine;

uint8_t fsm_step(StateMachine *fsm, uint8_t event) {
    for (size_t i = 0; i < fsm->num_transitions; i++) {
        Transition *t = &fsm->transitions[i];

        if (t->current_state == fsm->current_state &&
            t->event_received == event) {

            // Check guard condition
            if (t->guard != NULL && !t->guard()) {
                continue;  // Guard failed, try next transition
            }

            // Transition allowed
            uint8_t old_state = fsm->current_state;
            fsm->current_state = t->next_state;

            log_transition(fsm->name, old_state, event, t->next_state);
            return 0x00;  // Success
        }
    }

    // No valid transition found → error
    uint8_t error = ~event & 0xFF;
    log_error(fsm->name, fsm->current_state, event, error);
    return error;
}
```

### Testing Strategy

```c
// test_symmetry.c

#include <protoduino.h>
#include <dbg/errors.h>
#include <assert.h>

void test_twin_property() {
    // All ROOTs must be TWINs
    assert(ERR_IS_TWIN(0x00));
    assert(ERR_IS_TWIN(0x55));
    assert(ERR_IS_TWIN(0xAA));
    assert(ERR_IS_TWIN(0xFF));

    // Each quadrant has exactly 4 TWINs
    uint8_t twin_counts[4] = {0, 0, 0, 0};
    for (uint16_t i = 0; i < 256; i++) {
        if (ERR_IS_TWIN(i)) {
            uint8_t q = err_op_root(i) >> 6;  // Quadrant index
            twin_counts[q]++;
        }
    }
    for (int q = 0; q < 4; q++) {
        assert(twin_counts[q] == 4);
    }
}

void test_duality() {
    // Verify event-error inverses
    assert(err_op_inverse(EVENT_NONE) == ERR_FINALIZED);
    assert(err_op_inverse(EVENT_DEVICE_DATA) == ERR_SERIAL_OVERRUN);
    assert(EVENT_TO_ERROR(EVENT_LOCK_ACQ) == ERR_CRIT_ABANDONED);
    // exhaustively checked by: node tools/ontology-sync.js --check
}

void test_scheduler_integration() {
    // Simulate scheduler with ontology error
    struct process test_proc;
    process_post(&test_proc, EVENT_DEVICE_DATA, NULL);
    // Assert error posts PROCESS_EVENT_ERROR with inverse
}
```

### Where Everything Lives

| Concern | File |
|---|---|
| Event codes (`EVENT_*`) + `EVENT_TO_ERROR`/`ERROR_TO_EVENT` | `src/sys/events.h` |
| Error codes (`ERR_*`), reserved kernel codes | `src/sys/errors.h` |
| Error-side ops: `ERR_IS_*` macros, `err_op_*` inline functions | `src/dbg/errors.h` |
| Event-side ops: `EV_IS_*` / `ev_op_*` (aliases — one implementation) | `src/dbg/events.h` |
| `error_to_string()` — flash-resident (PROGMEM), config-gated | `src/dbg/errors.c` + generated `errors-strings.inc` |
| `event_to_string()` — flash-resident (PROGMEM), config-gated | `src/dbg/events.c` + generated `events-strings.inc` |
| String/config knob `ERRORS_CONF_STRINGS` (0 = none, 1 = tiny, 2 = verbose) | `src/sys/errors.conf.h` |
| Claim (math) / Proof (CSV) / Enforcement (validator + generator) | `docs/ontology-claim.js` / `docs/ontology-proof.csv` / `tools/ontology-sync.js` |

Storage rule: **every string is flash-resident** (`PSTR()`/PROGMEM on AVR) — the taxonomy never costs SRAM. `error_to_string()`/`event_to_string()` return flash addresses; print them with `print_P()`. With `ERRORS_CONF_STRINGS 0` no table is compiled at all and both functions return NULL — stream the raw codes and decode host-side.

Throwability rule: reserved codes 0x00–0x03 and 0xFF coincide with `ptstate_t` and must never be passed to `PT_RAISE`/`PT_THROW`; 0x04 (`ERR_ACCESS_DENIED`) doubles as the generic `PT_ERROR` sentinel. See the header comment in `src/sys/errors.h`.

---

## Advanced Topics

### 1. Ontology in Distributed Systems

Use duality for multi-node coordination via scheduler IPC: Events in pipes trigger remote Protothread errors.

### 2. Performance Optimization

High-entropy codes for frequent scheduler events; low-entropy for rare Protothread finals.

### 3. Security Implications

Reserve LEAFs for crypto errors; symmetries for tamper detection in scheduler queues.

### 4. Host-Side Monitoring

The streamed-code heatmap (see Overview) works for any target size: devices with no TUI at all stream bytes, and the host renders activity vs. failure as two point-symmetric halves of the 16×16 map. Entropy and Hamming distance make useful secondary axes (color/intensity) for spotting chaotic flows and clustered failures over time.

### 5. Compression Schemes

ROOT/DOMAIN: 2-bit encoding; SECTION: 4-bit; LEAF: No compression. Aligns with scheduler message pools.

### 6. Integration with Pipes

Duality in streaming: Event for data ready (`process_poll`), error for overflow (`PT_THROW`).

---

## Validation Checklist

All of these are enforced automatically by `node tools/ontology-sync.js --check` (run it after any header edit; CI runs it on every push):

- Exactly 4 ROOTS, 12 DOMAINS, 48 SECTIONS, 192 LEAFs.
- All ROOTS are TWINS; 16 TWINS total (4 per quadrant).
- 16 SHADOWS, 16 MIRRORS, 16 ANTICODEs (ANTICODE ≡ SHADOW).
- 70 BALANCED codes (C(8,4)=70).
- Entropy: 0.0 for pure (ones=0 or 8); 1.0 for balanced.
- Duality: For all x, inverse(inverse(x))=x; root(inverse(x))=inverse(root(x)) (dual quadrant).
- Every value 0x00–0xFF has both an `ERR_*` and an `EVENT_*` name; every non-reserved pair is a strict inverse.
- Every symmetry/hierarchy annotation in the header comments matches the computed class.
- Hierarchy: For all x, depth(x) ≤ 3; center decreases depth by exactly 1; roots are fixed points.
- Reserved: 0x00,0x01,0x02,0x03,0xFF marked RESERVED and shared between both spaces.
- Sum of ones over all codes: 1024 (average 4).
- Generated artifacts (proof CSV, string tables) are byte-identical to the checked-in copies.

---

## References & Further Reading

- Group Theory: Klein four-group for ROOT symmetries.
- Information Theory: Shannon's original paper for entropy.
- Protothreads v2: See `protothreads.md`.
- Scheduler: See `scheduler.md`.
- Unified IPC Layer: See `ipc.md`.
- Claim: `docs/ontology-claim.js` (the math, JS port of `src/dbg/errors.h`).
- Proof: `docs/ontology-proof.csv` (deterministic, generated from the claim).
- Enforcement: `tools/ontology-sync.js` (validates the headers, regenerates proof + string tables; `--check` for CI).
- Duality spot-check: `docs/verify-duality.js`.
- Naming source material: `common-events.md`, `common-errors.md` (surveys of event/error vocabularies across platforms).

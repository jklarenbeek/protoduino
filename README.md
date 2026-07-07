# Protoduino

**A Lightweight Operating Framework for AVR Microcontrollers**

Protoduino transforms Arduino/AVR boards into sophisticated embedded systems using a cooperative, event-driven architecture. Built on an enhanced **Protothreads v2** core, it provides professional-grade multitasking, IPC, and error handling for resource-constrained environments (e.g., ATmega328P with 2KB SRAM).

---

### ⚠️ LEGAL DISCLAIMER / AVISO LEGAL

Users in the following jurisdictions are **STRICTLY PROHIBITED** from using or reading this project due to age verification laws for operating systems:
- 🇧🇷 Brasil (a partir de 17 de maio de 2026)
- 🇺🇸 California, New York, Illinois, Colorado

*Please see the full disclaimer in the [LICENSE](./LICENSE) or previous versions for details.*

---

## 🚀 Core Components

Protoduino is structured into several modular layers, each with dedicated documentation and implementation:

### 1. [Process Scheduler](./docs/scheduler.md) (Kernel)
A tiny, deterministic cooperative scheduler that manages process lifecycles and event dispatching.
- **Implementation**: [`src/sys/process.h`](./src/sys/process.h), [`src/sys/process.c`](./src/sys/process.c)
- **Features**: Priority-based scheduling, zero-malloc design, ISR-safe event posting.

### 2. [Protothreads v2](./docs/protothreads.md) (Multitasking)
An enhanced version of Adam Dunkels' stackless coroutines, adding native exception handling.
- **Implementation**: [`src/sys/pt.h`](./src/sys/pt.h)
- **Features**: `PT_RAISE`, `PT_CATCH`, and `PT_FINALLY` for robust error management without stack overhead.

### 3. [Inter-Process Communication](./docs/ipc.md) (IPC)
Flexible primitives for data exchange between processes and ISRs.
- **Implementation**: [`src/sys/ipc.h`](./src/sys/ipc.h), [`src/sys/pt/pipe.h`](./src/sys/pt/pipe.h)
- **Mechanisms**: Structured **Messages** (RPC-style) and Streaming **Pipes** (ring buffers).

### 4. [Event-Error Ontology](./docs/ontology.md)
A mathematically rigorous 8-bit taxonomy for system states, events, and errors.
- **Implementation**: [`src/sys/errors.h`](./src/sys/errors.h), [`src/sys/events.h`](./src/sys/events.h)
- **Reference**: [Common Error Codes](./docs/common-errors.md), [Common Events](./docs/common-events.md)

### 5. [Unicode & UTF-8 Support](./docs/unicode.md)
Professional text handling ported from Plan 9, suitable for internationalized displays. UTF-8 everywhere, flash-resident classification tables, verified on-target under protosim.
- **Implementation**: [`src/lib/text/utf8.h`](./src/lib/text/utf8.h), [`src/lib/text/rune16.h`](./src/lib/text/rune16.h), [`src/lib/text/rune32.h`](./src/lib/text/rune32.h)

---

## 🛠️ Quick Start

### 1. Define your process
```cpp
#include <protoduino.h>

// 1. Define process type with persistent state
PROCESS_DEFINE(blink, "Blinker", 2,
    uint8_t pin;
    uint16_t delay_ms;
);

// 2. Create an instance
PROCESS_INSTANCE(blink, blink_proc);

// 3. Implementation
PROCESS_THREAD(blink, ev, data) {
    blink_t *self = PROCESS_SELF(blink);
    PROCESS_BEGIN();

    self->pin = LED_BUILTIN;
    self->delay_ms = 1000;
    pinMode(self->pin, OUTPUT);

    while(1) {
        digitalWrite(self->pin, HIGH);
        PROCESS_WAIT_EVENT_UNTIL(millis() % (self->delay_ms * 2) > self->delay_ms);

        digitalWrite(self->pin, LOW);
        PROCESS_WAIT_EVENT_UNTIL(millis() % (self->delay_ms * 2) < self->delay_ms);
    }

    PROCESS_END();
}
```

### 2. Run the Kernel
```cpp
void setup() {
    process_init(NULL); // Initialize scheduler
    process_start(&blink_proc.base, NULL);
}

void loop() {
    process_run(); // Drive the cooperative engine
}
```

---

## 🔬 Debugging with Protosim

For a seamless development experience, Protoduino is deeply integrated with [**Protosim**](https://github.com/jklarenbeek/protosim), a cycle-accurate AVR simulator built on `simavr`. Protosim allows you to debug, profile, and analyze your firmware entirely in software without physical hardware.

### Why use Protosim?
- **Cycle-Exact Profiling**: Identify bottlenecks by measuring the exact CPU cycles spent in each function.
- **Symbolic Breakpoints**: Set breakpoints on C/C++ function names (e.g., `process_run`) instead of memory addresses.
- **Variable Watches**: Monitor the internal state of your processes in real-time.
- **Virtual UART**: Automatically bridges the AVR UART to a local PTY (Linux/macOS) or TCP port (Windows) for serial interaction.
- **GDB Integration**: Connect `avr-gdb` directly to the simulator for source-level step-debugging.

### Getting Started with Simulation
Most examples in this repository include a `.ino` file. These folders also define a virtual circuit (e.g., an Arduino Uno) that loads your compiled `.hex` or `.elf` file.

1. **Install Protosim**: Follow the instructions at [jklarenbeek/protosim](https://github.com/jklarenbeek/protosim).
2. **Compile your sketch**: Ensure you generate an `.elf` file (contains debugging symbols).
3. **Launch the simulator**:
   ```bash
   # Run a simulation using an example circuit
   protosim examples/01-pt-wait-one-basic/01-pt-wait-one-basic.elf
   ```
4. **Connect to UART**: On Windows, open PuTTY and connect to `127.0.0.1:4000` (Telnet) to see your serial output.

---

## 📚 Documentation Index

| File | Description |
|------|-------------|
| [**Scheduler**](./docs/scheduler.md) | Kernel architecture, priority, and lifecycle. |
| [**Protothreads v2**](./docs/protothreads.md) | Exception handling, yielding, and state machines. |
| [**IPC**](./docs/ipc.md) | Deep dive into Messages and Pipes. |
| [**Ontology**](./docs/ontology.md) | The mathematical framework for error/event duality. |
| [**Common Errors**](./docs/common-errors.md) | Comprehensive list of ontological error codes. |
| [**Common Events**](./docs/common-events.md) | Comprehensive list of ontological event codes. |
| [**Module Loader**](./docs/loader.md) | *Experimental* runtime code loading. |
| [**Unicode**](./docs/unicode.md) | UTF-8 codec strictness, rune16/rune32 tables, `PROTODUINO_UNICODE_LEVEL`. |
| [**mcurses**](./docs/mcurses.md) | Lean terminal output driver — VT/ANSI escapes over an `ipc_pipe_t`. |
| [**TUI**](./docs/tui.md) | Damage-tracked, Clay-inspired terminal UI layout engine (output side). |
| [**TUI Roadmap**](./docs/tui-roadmap.md) | *Future* Option C: full virtual-terminal cell buffer + diff rendering. |
| [**ANSI Parser**](./docs/ansi.md) | Near-full ANSI/VT input parser (DEC VT500 state machine, key decoding). |

---

## 📂 Project Structure

- `src/sys/`: Core kernel, protothreads, and IPC implementation.
- `src/lib/`: Utility libraries (UTF-8, math, ring buffers).
- `src/cpu/avr/`: Hardware-specific UART and clock drivers.
- `examples/`: Step-by-step tutorials from basic to advanced.
- `docs/`: In-depth technical references and specifications.

---

## 🤝 Contributing & License

Protoduino is an open-source project. See the [LICENSE](./LICENSE) file for distribution rights and jurisdictional restrictions.

*Developed by [jklarenbeek](https://github.com/jklarenbeek).*

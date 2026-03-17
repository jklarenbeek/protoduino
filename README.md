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

### 5. Unicode & UTF-8 Support
Professional text handling ported from Plan 9, suitable for internationalized displays.
- **Implementation**: [`src/lib/text/utf8.h`](./src/lib/text/utf8.h), [`src/lib/text/rune16.h`](./src/lib/text/rune16.h)

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
    process_start(&blink_proc.base);
}

void loop() {
    process_run(); // Drive the cooperative engine
}
```

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

# Protoduino Unified IPC Layer

### Messages • Pipes • Fixed-Size Pools • Zero-Copy • ISR-Safe • Deterministic

---

## Introduction

The Protoduino **IPC Layer** provides two complementary, lightweight inter-process communication mechanisms designed for small microcontrollers:

1. **Structured Messages**
   A typed, small fixed-argument packet (`ipc_msg_t`) suitable for commands, requests, responses, and notifications.

2. **Streaming Pipes**
   A zero-copy byte ring buffer (`ipc_pipe_t`) designed for fast streaming (UART, sensors, audio, logging, etc.).

Both designs are:

* **deterministic** (O(1) operations)
* **zero-malloc** (fixed pool or static buffers)
* **ISR-safe**
* **atomic** (via `CC_ATOMIC_RESTORE()` from `cc.h`)
* **scheduler-independent**
* tailored for **8-bit AVR / low-RAM** systems
* fully integrated with your **Error Taxonomy (`errors.h`)**

The IPC layer does **not** depend on the Protoduino scheduler.
Integration is performed through a **user-supplied wake callback**.

---

# 1. Message IPC

Messages are small, fixed-size packets that behave similarly to microkernel RPC (Remote Procedure Calls). They allow processes to exchange multi-argument structured information.

---

## 1.1 Message Structure: `ipc_msg_t`

```c
typedef struct ipc_msg {
    uint8_t type;                         // application-defined message type
    uint8_t argc;                         // number of valid argv entries
    void *argv[IPC_MSG_MAX_ARGS];         // lightweight arguments (pointers only)
} ipc_msg_t;
```

Properties:

* **Small fixed footprint** (default: 4 arguments)
* **Pointers only** (zero deep-copy)
* Suitable for:
  * Command requests
  * System calls
  * Notifications
  * Error reports
  * Driver/API calls
  * Filesystem operations
* Argument buffer ownership is explicit and user-controlled
  (e.g., pool-allocated, static, ROM, stack—your choice)

---

## 1.2 Message Pools (`ipc_pool_t`)

Messages are allocated from a **fixed-size pool** with zero fragmentation:

```c
struct ipc_pool {
    void    *free_list;     // simple free-list stack
    uint8_t *buffer;        // contiguous block of memory
    size_t   block_size;
    uint16_t n_blocks;
    uint16_t free_count;
};
```

This pool is used not only for messages (`ipc_msg_t`) but also for any small buffers or descriptors.

### Initialization

```c
ipc_pool_init(&pool, pool_buffer, sizeof(ipc_msg_t), 8);
```

Where:

* `pool_buffer` is a statically allocated block:
  `sizeof(ipc_msg_t) * N`
* `N` determines maximum simultaneous message allocations.

### Allocation

```c
ipc_msg_t *m = ipc_msg_alloc_from_pool(&pool);
if (!m) {
    // allocation failed (pool empty)
}
```

### Freeing

```c
ipc_msg_free_to_pool(&pool, m);
```

All operations are wrapped in `CC_ATOMIC_RESTORE()`.

This makes the pool:

* **ISR-safe**
* **deterministically O(1)**
* **predictable under pressure**

---

## 1.3 Initializing Messages

```c
ipc_msg_init(m, type, argc, argv);
```

Or incrementally:

```c
ipc_msg_set_arg(m, 0, pointer);
ipc_msg_set_arg(m, 1, pointer);
```

Your scheduler will later deliver messages as:

```c
PROCESS_EVENT_MSG, m
```

(we implement this in `process.c` later)

---

# 2. Streaming Pipes

Pipes provide **high-speed, low-latency byte streaming** without allocating a message per byte (which would be too slow and RAM-heavy).

A pipe is a classic **ring buffer**:

```c
typedef struct ipc_pipe {
    uint8_t     *buf;
    size_t       size;
    size_t       head;      // writer
    size_t       tail;      // reader
    ipc_wake_cb_t wake_cb;
    void        *wake_ctx;
} ipc_pipe_t;
```

Pipes are ideal for:

* UART RX/TX
* Sensor data
* File streaming
* Logging
* Real-time packet I/O

---

## 2.1 Wake Callbacks

The pipe is scheduler-agnostic. Instead of calling Protoduino API directly, it uses a callback:

```c
typedef void (*ipc_wake_cb_t)(void *ctx);
```

Examples:

* `process_poll(reader_proc)`
* `process_post(reader_proc, PROCESS_EVENT_POLL, NULL)`
* A custom ISR-safe event trigger

### Setup Example

```c
ipc_pipe_init(
    &pipe,
    pipe_buffer,
    sizeof(pipe_buffer),
    process_wake_cb,
    (void *)reader_process
);
```

Where `process_wake_cb` is implemented in the kernel as scheduler glue.

---

# 2.2 Writing to a Pipe

```c
size_t written = ipc_pipe_write(&pipe, data, len);
```

Characteristics:

* Never blocks
* Writes as much as fits
* Zero copy
* If pipe transitions from **empty → non-empty**, the wake callback fires

Errors handled via return length (0 = overflow).

---

# 2.3 Reading from a Pipe

```c
uint8_t buf[32];
size_t n = ipc_pipe_read(&pipe, buf, sizeof(buf));
```

Reads up to requested bytes
(returns 0 if empty).

---

# 2.4 Atomicity and ISR-safety

All pointer and head/tail modifications are wrapped in:

```
CC_ATOMIC_RESTORE() { ... }
```

This ensures:

* No race between ISR and kernel
* No partial modifications
* No corrupted pointers or ring state

---

# 3. Integration Model

The IPC layer is intentionally **scheduler-agnostic**.

This means:

* Pipes do not know about processes
* Message pools do not know about event queues
* `process.c` remains lean, small, and focused

Your scheduler will attach pipes to processes later.

---

# 3.1 Example Integration with Protoduino Processes

Provide a callback:

```c
void ipc_process_wake_cb(void *ctx)
{
    struct process *p = (struct process *)ctx;
    process_poll(p);       // or process_post(p, PROCESS_EVENT_POLL, NULL);
}
```

Initialize pipe:

```c
ipc_pipe_init(&uart_rx_pipe, uart_rx_buf, 64, ipc_process_wake_cb, uart_process);
```

Now the `uart_process()` wakes whenever bytes arrive.

---

# 4. Error Handling

All IPC operations use the [Error Taxonomy](taxonomy.md).

Examples:

| Condition      | IPC function returns                                    |
| -------------- | ------------------------------------------------------- |
| Bad pointers   | `ERR_SYS_INVAL`                                         |
| Pool exhausted | NULL (caller may map to `ERR_RES_EXHAUSTED`)            |
| Pipe full      | returns 0 bytes (caller may signal `ERR_PIPE_OVERFLOW`) |
| Bad args       | `ERR_SYS_INVAL`                                         |

This ensures consistent error handling across:

* kernel
* IPC
* protothreads
* drivers
* modules

---

# 5. Determinism & Performance Characteristics

| Operation          | Complexity | Notes                  |
| ------------------ | ---------- | ---------------------- |
| Alloc/free message | O(1)       | Fixed pool free-list   |
| Write pipe         | O(n)       | Bounded by buffer size |
| Read pipe          | O(n)       | Same                   |
| Wake callback      | O(1)       | Scheduler-defined      |
| Message init       | O(argc)    | Typically 1–4          |

Memory overhead:

* `ipc_msg_t`: ~6 bytes + 4 pointers
  (based on `IPC_MSG_MAX_ARGS` = 4)
* Pipe: exactly N bytes of buffer + 2 indices

No dynamic heap. No fragmentation.

---

# 6. Design Philosophy

The Protoduino IPC Layer follows **three core principles**:

---

## 6.1 Separate Structured Messages from Streaming Pipes

This is the same distinction UNIX makes:

### Messages

* Self-contained
* Semantic
* Small control packets
* RPC-like
* Delivered via queue

### Pipes

* Raw bytes
* High throughput
* No semantic meaning per byte
* Delivered via polling or events

Trying to unify both in one structure makes everything slower.

---

## 6.2 Zero Copy = Zero Overhead

All IPC interactions avoid copying unless *you* explicitly choose to copy.

---

## 6.3 Scheduler-Agnostic by Design

Allows:

* reuse in multiple schedulers
* reuse in bootloader environments
* aggressive unit testing outside kernel
* multi-context integration (thread + ISR)

---

# 7. Examples

---

## 7.1 Structured Message Example

```c
ipc_msg_t *m = ipc_msg_alloc_from_pool(&msg_pool);
void *argv[2] = { (void*)"/dev/uart0", (void*)115200 };

ipc_msg_init(m, MSG_UART_INIT, 2, argv);

/* deliver using the protoduino scheduler:
    process_post(uart_process, PROCESS_EVENT_MSG, m);
*/
```

---

## 7.2 Pipe Stream Example

UART ISR:

```c
ISR(USART_RX_vect) {
    uint8_t b = UDR0;
    ipc_pipe_write(&uart_rx_pipe, &b, 1);
}
```

UART process:

```c
PROCESS_THREAD(uart_process, ev, data)
{
    PROCESS_BEGIN();

    uint8_t buf[16];

    while (1) {
        PROCESS_WAIT_EVENT();   // PROCESS_EVENT_POLL arrives on data

        if (ev == PROCESS_EVENT_POLL) {
            size_t n = ipc_pipe_read(&uart_rx_pipe, buf, sizeof(buf));
            for (size_t i = 0; i < n; i++)
                handle_byte(buf[i]);
        }
    }

    PROCESS_END();
}
```

---

# 8. Usage examples & integration hints

## 1. Creating a message pool + sending a typed message

```c
#include "ipc.h"

/* statically allocate pool memory: 8 messages of size sizeof(ipc_msg_t) */
static uint8_t msgpool_buf[sizeof(ipc_msg_t) * 8];
static struct ipc_pool msgpool;

void setup_ipc(void) {
    ipc_pool_init(&msgpool, msgpool_buf, sizeof(ipc_msg_t), 8);
}

/* allocate and send message (scheduler integration left to user) */
void send_hello(void) {
    ipc_msg_t *m = ipc_msg_alloc_from_pool(&msgpool);
    if (!m) {
        /* pool exhausted */
        return;
    }
    void *args[1] = { (void*)"world" };
    ipc_msg_init(m, 1 /* type */, 1, args);
    /* now post to a process: process_post(dest, PROCESS_EVENT_MSG, m); */
}
```

## 2. Creating a pipe and wiring it to your process scheduler

```c
#include "ipc.h"

/* buffer for pipe */
static uint8_t uart_pipe_buf[64];
static ipc_pipe_t uart_pipe;

/* wake callback example that integrates with process_poll():
 * In your process layer implement a function `void process_wake_poll(void *ctx)`
 * that calls process_poll((struct process*)ctx). Then pass that as wake_cb.
 */
void setup_pipe(struct process *reader_proc) {
    ipc_pipe_init(&uart_pipe, uart_pipe_buf, sizeof(uart_pipe_buf),
                  /*wake_cb=*/process_poll_wake_cb,
                  /*wake_ctx=*/(void*)reader_proc);
}

/* writer pushes bytes */
void uart_rx_isr_push(const uint8_t *data, size_t len) {
    ipc_pipe_write(&uart_pipe, data, len);
}

/* reader (in its process) does on poll:
   uint8_t buf[16];
   while (ipc_pipe_read(&uart_pipe, buf, sizeof(buf)) > 0) { ... }
*/
```

Note: process_poll_wake_cb() is a small wrapper you would implement in your glue code that simply calls process_poll(reader_proc) or process_post(reader_proc, PROCESS_EVENT_POLL, NULL). We purposely don't include process.h here so the IPC remains scheduler-agnostic.

---
# 8. TODO: Debugging Aids

`ipc_msg_to_string()` produces human-readable output:

```
MSG(type=3 argc=2) [0]=0x0042 [1]=0x1F20
```

Useful when debugging on host builds.

---

# Conclusion

The Protoduino **Unified IPC Layer** provides:

* A **microkernel-grade messaging system**
* A **zero-copy high-speed pipe**
* Deterministic real-time allocations
* Full ISR-safety
* Power-failure hardened atomicity (via `CC_ATOMIC_RESTORE`)
* Seamless integration with the error taxonomy
* Full scheduler independence

This IPC architecture brings Protoduino to the level of modern embedded RTOSes—while remaining under **2 KB of code**, fully static, and suitable for **ATmega and smaller controllers**.

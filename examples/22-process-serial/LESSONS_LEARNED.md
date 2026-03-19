# Lessons Learned — 22-process-serial

*Notes from the session that implemented and debugged the serial process driver.*

---

## 1. Ring buffers need a clear invariant — and it must be documented

The `process.c` event queue used a "head = last-written" convention: `enqueue` computed `next = (head+1)%N`, wrote to `events[next]`, then set `head = next`. Dequeue read `events[tail]` and advanced tail. With `head == tail == 0` at boot, the first enqueue wrote to slot 1 but the first dequeue read slot 0 (blank). This wasted one dequeue cycle and, critically, made the last-enqueued event unreachable whenever exactly 2 events were queued at startup — the scenario every two-process application hits.

The comment said `event_head` = "next write slot (exclusive)" which was misleading — it was actually the *last-written* slot. The mismatch between the comment and the real invariant made the bug invisible during review.

**Lesson**: write-at-head → advance-head is the standard, self-consistent ring buffer. Always document which slot head/tail point to (read-ready vs write-ready), and have at least one unit test that starts exactly two processes and checks both INIT events are dispatched.

---

## 2. `PROCESS_WAIT_EVENT_UNTIL` does not yield when the condition is already true

`PT_YIELD_UNTIL(pt, cond)` saves the LC then immediately checks the condition — if it is already true, execution falls through without returning to the scheduler. In the echo_upper loop:

```c
while (1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL || ev == PROCESS_EVENT_INIT);
    …
}
```

On the INIT dispatch `ev` *is* `PROCESS_EVENT_INIT`, so the condition is true and the thread loops back instantly, spinning forever without ever returning `PT_ISRUNNING`. The scheduler is starved.

**Lesson**: use `PROCESS_WAIT_EVENT()` (unconditional yield) at the top of a service loop and then examine `ev` inside the body. Reserve `PROCESS_WAIT_EVENT_UNTIL(cond)` for cases where you know the condition is currently false and you are genuinely waiting for it to become true.

---

## 3. Connecting a pipe at run-time requires updating the wake callback

`SERIAL_PROCESSN_CONNECT` wired `pipein` and `pipeout` but left `rx_pipe.wake_cb = NULL`. The UART RX ISR called `ipc_pipe_write(&rx_pipe, …)` which transitions the pipe from empty to non-empty — the very condition that is supposed to trigger a wake. Because `wake_cb` was NULL, no `process_poll` fired and the consumer process slept forever.

The fix was to set `wake_cb = process_ipc_wake` and `wake_ctx = consuming_process` inside the CONNECT macro. The CONNECT call is the exact moment the driver learns *which* process to wake, so it is also the right place to install the callback.

**Lesson**: whenever you hand a pipe to a consumer at run-time, immediately update `wake_cb`/`wake_ctx` to point at that consumer. A pipe with `NULL` wake callback is fine for polling-only use, but ISR-driven producers require it.

---

## 4. Coverage reports are powerful, but silence on PROGMEM data sections is expected

The 0% coverage shown for `_ZL23process_type_echo_upper` (PROGMEM type descriptor) and `serial_process0_name` (PROGMEM string) was initially alarming — it looked as if `process_new` was never accessing the type descriptor. In reality, code coverage in protosim tracks *instruction* execution; `pgm_read_byte` / `pgm_read_ptr` reads from flash data sections do not increment instruction coverage for the data symbols. Those 0% entries are expected and normal.

**Lesson**: distinguish between 0% on a *code* symbol (function never called — a real problem) and 0% on a *data* symbol in PROGMEM (normal, since data access is recorded against the `pgm_read_*` instruction, not the data address).

---

## 5. Protosim's TCP UART injection requires the XON flag to be set first

On Windows, protosim bridges UART0 to a TCP port. Incoming TCP bytes are only forwarded to the simulated UART when `p->xon == 1` — set by `uart_com_xon_hook` which fires when the simulated firmware enables RXEN in UCSRB. After that a 1 kHz timer injects one byte every 16 000 simulated cycles into the UART RX register.

This means:
- You must call `process_run()` / let the simulation advance past the UART `open()` call before sending TCP bytes; otherwise the bytes queue in the TCP thread's FIFO and are never injected.
- 500M simulated cycles (~31 s of AVR time) gives plenty of margin for roundtrip echo at 9 600 baud (≈ 1 ms / byte * 16 000 cycles / ms = 16 000 cycles / byte injected every 16 000 cycles ≈ 1 byte / injection tick).

**Lesson**: in timing-sensitive protosim TCP tests, let the simulation advance at least a few hundred thousand cycles (one timer tick = 16 000 cycles) after UART open before sending test data. A 0.1–0.3 s `time.sleep()` before `sendall()` is sufficient at typical desktop speeds.

---

## 6. Protothread `__LINE__`-based LC labels break inside function-like macros

The protothread local-continuation mechanism (lc-switch) inserts `case __LINE__:` labels inside a `switch` to save and restore the execution point. When the thread body is wrapped in a `#define` macro, all `PROCESS_WAIT_EVENT` calls in the macro share the same `__LINE__` values across every instantiation of the macro in different `.c` files, producing duplicate `case` labels.

This forced the design of `serial_private.h` to be a raw `#include` file (not a function-like macro), so each `.c` file that includes it gets its own independent line-number namespace.

**Lesson**: never put protothread body code (anything containing `PT_YIELD`, `PT_WAIT_UNTIL`, `PROCESS_BEGIN`, etc.) inside a C preprocessor macro that will be instantiated more than once. Use `#include` of a template header instead.

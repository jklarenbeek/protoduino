# 22-process-serial — Serial Process Driver Example

## What

This example shows how to use the `sys/process/serial` device driver, which wraps each hardware UART on the ATmega2560 as a **protoduino process**. The driver exposes bidirectional byte streams through a pair of `ipc_pipe_t` pipelines so that any application process can read from and write to the UART without ever touching ISR registration, ring buffers or UART registers directly.

The sketch implements a classic **echo-upper** terminal: every character you type is converted to uppercase and sent back.

---

## Why

Raw UART access on AVR typically requires manually writing ISRs, managing ring buffers, and coordinating `on_rx_complete` / `on_tx_complete` callbacks. In a protoduino application that already uses the cooperative scheduler, this creates two separate "worlds" — ISR-driven hardware and event-driven processes — that need careful synchronisation.

The serial device process bridges that gap:

| Without serial process driver | With serial process driver |
|-------------------------------|----------------------------|
| Manual ISR + ring buffer | `process_new(&process_type_serial_process0, ...)` |
| Custom `on_rx_complete` hook | `SERIAL_PROCESS0_CONNECT(pt_process)` |
| Polling or callbacks for TX drain | `process_poll(&serial_process0_proc.base)` |
| Tight coupling to hardware | Portable byte-stream interface |

---

## How

### Architecture

```
  UART0 RX ISR
      │  (byte)
      ▼
  serial_process0_on_rx()          (ISR-safe callback)
      │  ipc_pipe_write → rx_pipe
      │  process_ipc_wake → process_poll(echo_upper)
      ▼
  echo_upper (wakes on POLL)
      │  ipc_pipe_read(pipein)  ← same rx_pipe
      │  uppercase byte
      │  ipc_pipe_write(pipeout) → tx_pipe
      │  process_poll(serial_process0)
      ▼
  serial_process0 (wakes on POLL)
      │  ipc_pipe_read(tx_pipe)
      │  serial0_write8(byte) → serial TX ring buffer
      │  serial0_flush()
      ▼
  UART0 UDRE ISR → wire → TCP port 4000 (protosim)
```

### Key API

| Call | Purpose |
|------|---------|
| `process_new(&process_type_serial_process0, &serial_process0_proc)` | Start the UART0 device process (opens UART, initialises pipes) |
| `SERIAL_PROCESS0_CONNECT(pt_process)` | Wire `pipein`/`pipeout` to the serial device **and** set `rx_pipe.wake_cb` so the consumer is polled on every incoming byte |
| `ipc_pipe_available(PROCESS_PIPEIN())` | Check how many bytes are waiting in the RX pipe |
| `ipc_pipe_read(PROCESS_PIPEIN(), &b, 1)` | Read one byte from RX |
| `ipc_pipe_write(PROCESS_PIPEOUT(), &b, 1)` | Write one byte to TX |
| `process_poll(&serial_process0_proc.base)` | Tell the serial device to flush the TX pipe to hardware |

### Baud rate

`process_new` zero-initialises the instance struct, so `baud` is 0 after the call. The driver defaults to `PROCESS_SERIAL_DEFAULT_BAUD` (9600) when it sees `baud == 0`. To use a different rate, set `serial_process0_proc.baud` **inside** the application process INIT handler — at that point the struct has already been initialised by the scheduler.

### Start order

`serial_process0` must be started **before** `echo_upper` so its pipes are ready when `SERIAL_PROCESS0_CONNECT` runs during echo_upper's INIT event.

---

## Building

```pwsh
# From the protoduino root
arduino-cli compile --fqbn arduino:avr:mega --library . `
    --output-dir build/22-process-serial `
    examples/22-process-serial/22-process-serial.ino
```

Flash size: ~4736 bytes (1.8% of the Mega's 253952 bytes)  
RAM: 466 bytes global variables

## Simulation

```pwsh
C:\path\to\protosim\bin\protosim.exe `
    build\22-process-serial\22-process-serial.ino.elf `
    -m atmega2560 -f 16000000 --max-steps 500000000

# Then connect:
putty.exe -telnet 127.0.0.1 4000
# Type "hello", press Enter → see "HELLO" echoed back
```

## Extending

| Goal | Change |
|------|--------|
| Different baud rate | Set `baud` field inside your process INIT, before calling `SERIAL_PROCESS0_CONNECT` |
| Second UART | Use `SERIAL_PROCESS1_CONNECT`, `process_type_serial_process1`, `serial_process1_proc` |
| Bridge two UARTs | Start both serial devices; in a bridge process, `CONNECT` to both and forward bytes between their pipes |
| Line buffering | Accumulate bytes in the echo loop until `'\r'` before writing to pipeout |

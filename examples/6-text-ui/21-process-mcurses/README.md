# 21-process-mcurses

## Overview
This sketch demonstrates the use of the `mcurses` minimal VT100 library alongside the `protoduino` cooperative scheduler (`sys/process.h`).

### The "Why"
Building terminal UIs on microcontrollers often degenerates into spaghetti code full of blocking `delay()`, messy global display state, and non-reentrant UI functions. 

This example proves that you can run **two completely decoupled processes** (`dashboard` and `cmdline`) sharing the same VT100 terminal asynchronously. 
- Neither process uses blocking calls like `delay()`.
- They communicate purely via memory-safe IPC pipes (`ipc_pipe_t`).
- The `mcurses_t` state machine holds all screen rendering state in a single struct, eliminating globals.

### How It Works
1. **Pipes**: We establish two byte-pipes, `tx_pipe` and `rx_pipe`, bound to the hardware UART interrupts.
2. **Dashboard Process**: Owns the `mcurses_t` context. It renders the static layout and updates statistics incrementally via a soft 1-second timer event. It also listens for `PROCESS_EVENT_MSG` events to print strings to the scrollable log area.
3. **Cmdline Process**: Non-blocking line editor. It polls `getch_ex()`. If a completed string is submitted by the user (Enter), it fires a `PROCESS_EVENT_MSG` internally pointing to the string. The dashboard consumes the event and renders the command to the log.

## How to Build and Test

### 1. Compile
Build using `arduino-cli` against the root `protoduino` library.
```bash
arduino-cli compile --fqbn arduino:avr:uno --library ../../ --output-dir build .
```

### 2. Simulate with protosim
To run the firmware in the cycle-accurate simulator:
```bash
protosim build\21-process-mcurses.ino.elf -m atmega328p -f 16000000 --max-steps 50000000
```
*The simulation will expose a virtual UART on `TCP 4000`.*

### 3. Connect a Terminal
While `protosim` is running, connect a VT100-compatible TCP client. For example, using PuTTY on Windows:
```bash
putty.exe -telnet 127.0.0.1 4000
```
Or use a basic Python socket script to read the serial stream manually.

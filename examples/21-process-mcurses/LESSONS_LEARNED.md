# Debugging Experiences and Lessons Learned

During the development and testing of this example under the `protosim` emulator, several subtle bugs within the architecture and C++ conventions were isolated and fixed. 

These are the lessons documented from the test session:

## 1. The `process_start` vs `process_new` Trap
**Symptom**: The firmware compiled successfully but exhibited a catastrophic hardware reset loop under simulation. Specifically, it crashed and reset in under 300 microseconds (roughly 4000 CPU cycles).
**Root Cause**: The `sys/process.h` scheduler macros heavily obscure lifecycle initialization. 
We declared processes using `PROCESS_INSTANCE(name, var)`, which expands into a zero-initialized static struct. The sketch then invoked `process_start(&instance.base)`. However, `process_start` natively *does not* populate the `.thread` function pointer inside the struct; it assumes the struct is pre-populated. When the scheduler eventually ticked the process, it executed `icall` on an absolute `0x0000` (NULL), hitting the AVR Reset Vector and wiping out the system.
**Solution**: Always use `process_new(&process_type_descriptor, &instance)` when hydrating `PROCESS_INSTANCE` structs. `process_new` safely copies the metadata, pointers, and priority configurations directly from Flash/PROGMEM templates into the RAM instance before queuing it on the scheduler.

## 2. Invalid Linkage from Static/Extern Mismatches
**Symptom**: `undefined reference to rx_error_count` during Linker (`ld`) phase.
**Root Cause**: A variable was declared deep in the file as `static uint32_t rx_error_count = 0;`. Code positioned above it attempted a forward-declaration via `extern uint32_t rx_error_count;`. Because `static` confines linkage strictly to internal translation unit visibility, the linker could not reconcile the generic `extern` request, terminating the build.
**Solution**: Scope scoping correctly by elevating the `static` declaration sequentially *above* its consumers to rely on sequential compiler visibility instead of hacking around it with `extern`.

## 3. Emulation Trace Profiling is King
When facing "silent" cyclic reboots—where the UART hasn't even initialized yet—classical `printf()` debugging is useless. 
**Lesson**: Using `protosim` with instruction traces (`-t 100`) combined with `avr-nm` and `avr-objdump` allows reconstructing the exact branch, jump, and variable state milliseconds before memory corruption or bad jumps execute. Without cycle-accurate PC tracing, identifying an `icall` to `0x0000` embedded in a library macro expansion would be near-impossible context blind.

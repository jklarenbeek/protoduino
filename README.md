# Protoduino

An extremely small operating framework using an updated version of the [protothreads](http://dunkels.com/adam/pt/about.html) library created by Adam Dunkels. Over time it will include more system tools to handle different types of operations that can come in handy when dealing with sensors, processing and viewing at the same time using a multithreaded environment for single core processors.

For now it contains protothread v2.0, an utf8 and unicode processing library taken from [plan9port](https://github.com/9fans/plan9port) and adapted to work with unicode on arduino with a low footprint.

## Protothreads V2

Protothreads V2 is an updated version of the v1.4 version currently available at their website. It starts with an additional routine in the `lc.h` library which expands on the `LC_SET()` function called `LC_RET()`. This enables the insertion of an extra command before the state machine enters its next state. It is used in the `PT_WAIT_ONE()` function which block the protothread for one cycle only. This is really helpfull in long loops or slow processing of data.

It also replaces the `PT_YIELD_FLAG` with the `PT_ERROR_STATE` local variable which is used to store the intermediate state of any child process that is called from a protothread. It can also be set by raising an error with `PT_RAISE()` from the running protothread which can be catched by the `PT_CATCH()` or `PT_CATCHANY()` clause. This enables fully functional exception handling with a protothread.

There are also some additional iterator helper functions like `PT_FOREACH()` and `PT_ENDEACH()` which expects a `PT_YIELDED` state from a child thread to come into action.

The examples included with this protoduino library should explain more about the inner workings of the additional functionality.

### Breaking changes

Another major change with the protothreads library is what happens when a protothread exits or ends. The v1.x library will reset the state to its original beginning state before it leaves the thread. This is **NOT** the case in V2 where the protothread will stay stuck at its ending state. Subsequent calls to a stopped protothread will therefor always result in the same `PT_EXITED`, `PT_ENDED` or `PT_ERROR` state that it last left to the caller.

This change is clearly visible in the examples that come with this library.

### Things todo.

TODO

## UTF8 and UNICODE16 modules

TODO



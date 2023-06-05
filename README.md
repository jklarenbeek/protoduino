# Protoduino

Protoduino is an open-source operating framework designed for Arduino boards. It is built upon an enhanced version of the [protothreads](http://dunkels.com/adam/pt/about.html) library created by Adam Dunkels. Protothreads are lightweight, stackless threads that provide a convenient way to write event-driven code on resource-constrained systems.

The protoduino framework extends the functionality of protothreads by incorporating additional features. These include an UTF-8 and Unicode processing library, which is a port of the library found in the Plan 9 operating system ([plan9port](https://github.com/9fans/plan9port)). This library allows for the manipulation and handling of UTF-8 encoded text and Unicode characters.

In addition to the Unicode processing library, Protoduino aims to provide various system tools to handle different types of operations that can be useful when working with sensors, processing data, and displaying information simultaneously. The framework is designed to enable a multithreaded environment on single-core processors, allowing for more efficient utilization of system resources.

It's important to note that protoduino is a work in progress, meaning that it's still being developed and expanded over time. As a result, more features and system tools are likely to be added to enhance its capabilities further.

If you are interested in using protoduino, you can find the source code and documentation in the GitHub repository: https://github.com/jklarenbeek/protoduino.

## **Protothreads v2**

Protothreads v2 is an enhanced version of the protothreads library, designed to provide lightweight cooperative multitasking and native exception handling capabilities for embedded systems. It builds upon the features of protothreads v1.4 while introducing several new features and improvements.

### **Differences from Protothreads v1.4**

#### - **Enum-based State**

Protothreads v2 wraps the returned state of a protothread into an enum called `ptstate_t`. Depending on the state of the protothread, it returns `PT_WAITING`, `PT_YIELDING`, `PT_EXITED`, `PT_ENDED`, which are also present in protothreads v1.4. In addition, protothreads v2 extends the state enumeration with `PT_ERROR` and `PT_FINALIZING`. This allows protothreads in v2 to support lightweight exception handling natively.

#### - **Reinitialization Requirement**

Protothreads v2 breaks from the state machine behavior of protothreads v1.4. In v2, the protothread's state is not automatically reinitialized to the beginning of the protothread when it exits, ends or throws. Instead, the caller is responsible for reinitializing the protothread state to restart it. This however is done automatically for a child protothread when using the `PT_SPAWN` or `PT_FOREACH` macros.

#### - **Usage of PT_SCHEDULE Macro**

The `PT_SCHEDULE` macro in protothreads v2 must only be used within a protothread. If used outside a protothread, the `PT_ISRUNNING` macro should be used to test if the protothread is still running.

#### - **Exception Handling**

Protothreads v2 introduces native exception handling capabilities. The `PT_ERROR` state enables a protothread to support lightweight exception handling without the explicit declaration of a `PT_TRY` macro; The `PT_BEGIN` macro is enough. A protothread in v2 can raise an exception using `PT_RAISE` and catch the exception using `PT_CATCHANY` or `PT_CATCH`. When an exception is handled within the protothread, the thread can gracefully exit, restart, or throw an error to the parent thread using the `PT_THROW` or `PT_RETHROW` macros. Unless you know the what and the why, it is advised, that the latter macros should only be used within a `PT_CATCHANY` or `PT_CATCH` control block. See the `08-pt-try-catch.ini` example for its usage.

#### - **Error Handling**

When a spawned protothread using the `PT_SPAWN` or `PT_FOREACH` macros throw an error, the parent thread can handle the error using the `PT_ONERROR` macro. The `PT_ONERROR` macro requires the returned state of a protothread to deside whether or not an error occured, which is by design stored in the local variable `PT_ERROR_STATE` and should be used in the majority of cases when using `PT_ONERROR`. The `PT_ERROR_STATE` local variable replaces the `PT_YIELD_FLAG` variable used in protothreads v1.4 and therefor doesn't add any overhead using error handling. See the `08-pt-try-catch.ini` example for its usage.

#### - **Iterator Protothreads**

Protothreads v2 includes additional macros `PT_FOREACH` and `PT_ENDEACH`, which allow for easy handling of iterator protothreads. These macros can be used in combination with the `PT_YIELD` or `PT_YIELD_UNTIL` macros. See the `07-pt-yield-foreach.ino` example for its usage.

#### - **Code Restrictions**

Within the protothread, normal code can be used, with a few restrictions. These restrictions are similar with protothreads v1.4, i.e. watch out with local variables, but also warns you for example, with the use of a `PT_YIELD` statement which should preferably not be placed in a `PT_CATCHANY`, `PT_CATCH`, or `PT_FINALLY` block.

#### - **PT_FINALLY Clause**

Protothreads v2 introduces the `PT_FINALLY` clause, which can be called by the parent thread using the `PT_FINAL` macro. This provides a way to gracefully handle the shutdown of a protothread, serving as a substitute for the `PROCESS_EXITHANDLER` macro in Contiki-OS.

### **Compatibility**

Protothreads v2 is an almost backward-compatible library that preserves the functionality of protothreads v1.4 while introducing new features. Existing code written using protothreads v1.4 should work with protothreads v2 with minimal modifications.

Protothreads v2 differs to v1 in that it is the responsibility of the caller to reset the state of a protothread. This is automatically done, when you spawn a child protothread with the `PT_SPAWN` or `PT_FOREACH` macros. Looking at Contiki-OS, which uses protothread v1 at its foundation, I could not detect any immediate problems in its `process.h` file, which is the ultimate parent. Since protoduino and protothread v2, haven't been tested in conjunction with Contiki-OS, there isn't much to say about compatability.

## **UTF8 and UNICODE16 String Library**

Protoduino includes a powerful string library that provides various functions for manipulating and working with strings. The string library is designed to handle UTF-8 encoded strings and provides support for Unicode characters.

### **Features**

The string library offers the following features:

#### - **UTF-8 Support**

The library fully supports UTF-8 encoded strings, allowing you to work with multi-byte characters and perform operations such as string concatenation, searching, comparison, and copying. It ensures proper handling and manipulation of UTF-8 characters, ensuring the integrity of the encoded data.

#### - **UNICODE 16 Compatibility**

Protoduino also includes a Rune16 module, which is a port of the C string library forked from Plan9's `string.h` implementation. This module provides additional functions specifically designed for working with 16-bit Unicode characters, referred to as `rune16_t`.

#### - **Arduino Stream Integration**

The string library seamlessly integrates with the Stream class, allowing you to easily read and write strings from different sources, such as serial communication, files, or custom data streams. This integration enables efficient handling of strings in various input/output scenarios.

#### - **Doxygen Documentation**

The string library is extensively documented using Doxygen notation, making it easy to understand the available functions, their parameters, and return values. The Doxygen documentation provides detailed explanations and usage examples, aiding developers in utilizing the library effectively.

### **Getting Started**

To start using the string library in your protoduino project, follow these steps:

1. Include the necessary header files in your source code, such as `utf/utf8.h`, `utf/rune16.h`, and `utf/utf8-stream.h`.

2. Make sure to link the string library during the compilation process to include the required functions and symbols.

3. Refer to the Doxygen documentation for the string library to explore the available functions and their usage. The documentation provides comprehensive explanations, parameter details, and usage examples.

4. Use the provided functions to manipulate and work with strings in your protoduino project. Take advantage of the UTF-8 support and Rune16 compatibility to handle strings efficiently, ensuring proper encoding and compatibility with Unicode characters.

For more information and detailed usage examples, refer to the complete documentation available in the `examples/` directory.

# Protoduino

Protoduino is an open-source operating framework designed for Arduino boards. It is built upon an enhanced version of the [protothreads](http://dunkels.com/adam/pt/about.html) library created by Adam Dunkels. Protothreads are lightweight, stackless threads that provide a convenient way to write event-driven code on resource-constrained systems.

The Protoduino framework extends the functionality of Protothreads by incorporating additional features. These include an UTF-8 and Unicode processing library, which is a port of the library found in the Plan 9 operating system ([plan9port](https://github.com/9fans/plan9port)). This library allows for the manipulation and handling of UTF-8 encoded text and Unicode characters.

In addition to the Unicode processing library, Protoduino aims to provide various system tools to handle different types of operations that can be useful when working with sensors, processing data, and displaying information simultaneously. The framework is designed to enable a multithreaded environment on single-core processors, allowing for more efficient utilization of system resources.

It's important to note that Protoduino is a work in progress, meaning that it's still being developed and expanded over time. As a result, more features and system tools are likely to be added to enhance its capabilities further.

If you are interested in using Protoduino, you can find the source code and documentation in the GitHub repository: https://github.com/jklarenbeek/protoduino.

## Protothreads v2

Protothreads v2 is an enhanced version of the Protothreads library, designed to provide lightweight cooperative multitasking capabilities for embedded systems. It builds upon the features of Protothreads v1.4 while introducing several new features and improvements.

### Differences from Protothreads v1.4

- **Enum-based State**: Protothreads v2 wraps the returned state of a protothread into an enum called `ptstate_t`. Depending on the state of the protothread, it returns `PT_WAITING`, `PT_YIELDING`, `PT_EXITED`, `PT_ENDED`, which are also present in Protothreads v1.4. In addition, Protothreads v2 extends the state enumeration with `PT_ERROR` and `PT_FINALIZING`. This allows protothreads in v2 to support lightweight exception handling natively.

- **Reinitialization Requirement**: Protothreads v2 breaks from the state machine behavior of Protothreads v1.4. In v2, the protothread's state is not automatically reinitialized to the beginning of the protothread. Instead, the caller is responsible for reinitializing the protothread state.

- **Usage of PT_SCHEDULE Macro**: The `PT_SCHEDULE` macro in Protothreads v2 must only be used within a protothread. If used outside a protothread, the `PT_ISRUNNING` macro should be used to test if the protothread is still running.

- **Exception Handling**: Protothreads v2 introduces native exception handling capabilities. The `PT_ERROR` state enables a protothread to support lightweight exception handling without the explicit declaration of a `PT_TRY` macro. A protothread in v2 can raise an exception using `PT_RAISE` and catch the exception using `PT_CATCHANY` or `PT_CATCH`. When an exception is handled within the protothread, the thread can gracefully exit, restart, or throw an error to the parent thread using the `PT_THROW` or `PT_RETHROW` macros. Note that the latter macros should only be used within a `PT_CATCHANY` or `PT_CATCH` block.

- **Error Handling in Parent Threads**: When a spawned protothread throws an error, the parent thread can handle the error using the `PT_ONERROR` macro.

- **Iterator Protothreads**: Protothreads v2 includes additional macros `PT_FOREACH` and `PT_ENDEACH`, which allow for easy handling of iterator protothreads. These macros can be used in combination with the `PT_YIELD` or `PT_YIELD_UNTIL` macros.

- **Code Restrictions**: Within the protothread, normal code can be used, with a few restrictions. For example, a `PT_YIELD` statement cannot be placed in a `PT_CATCHANY`, `PT_CATCH`, or `PT_FINALLY` block.

- **PT_FINALLY Clause**: Protothreads v2 introduces the `PT_FINALLY` clause, which can be called by the parent thread using `PT_FINAL`. This provides a way to gracefully handle the shutdown of a protothread, serving as a substitute for the `PROCESS_EXITHANDLER` macro in Contiki-OS.

### Compatibility

Protothreads v2 is a backward-compatible library that preserves the functionality of Protothreads v1.4 while introducing new features. Existing code written using Protothreads v1.4 should work with Protothreads v2 with minimal modifications.

For detailed usage instructions and code examples, please refer to the [Protothreads v2 GitHub repository](https://github

### Things todo.

- add PT_PROCESS default message handler
- add process kernel to handle events and signals.

## UTF8 and UNICODE16 modules

TODO

## TODO

Let's think this through, step by step, based on everything above and generate a readme.md document, about the differences between protothreads v1.4 at http://dunkels.com/adam/pt/index.html and protothreads v2 at https://github.com/jklarenbeek/protoduino/tree/main/src, based on everything posted in this chat and use the the following differences that explains the protothreads v2:
- Protothreads v2 wraps the returned state of a protothread into an enum called `ptstate_t`. Depending on the state of the protothread, it returns `PT_WAITING`, `PT_YIELDING`, `PT_EXITED`, `PT_ENDED` on which protothreads v2 extends it with `PT_ERROR` and `PT_FINALIZING`. 
- The protothread v2 breaks with the protothread v1.4 state machine in that it not automatically reinitializes it state to the beginning of the protothread, but instead requires the caller to reinitialize it.
- The `PT_SCHEDULE` macro MUST only be used within a protothread. When outside of a protothread one MUST use the `PT_ISRUNNING` macro to test if the protothread is still running.
- The `PT_ERROR` state enables a protothread to support lightweight exception handling natively, without the excplicit declaration of a `PT_TRY` macro. This allowes a protothread to raise an exception with `PT_RAISE` and catch the exception with `PT_CATCHANY` or `PT_CATCH`. 
- When an exception is handled within the protothread, the thread can gracefully exit, restart or throws an error to the parent thread with the `PT_THROW` or `PT_RETHROW` macros. Take care that the latter macros can only be used within a `PT_CATCHANY` or `PT_CATCH` block.
- When a spawned protothread has thrown an error the parent thread can handle the error with the `PT_ONERROR` macro.
- Protothreads v2 includes an additional `PT_FOREACH` and `PT_ENDEACH` macros that allowes for easy handling iterator protothread using the `PT_YIELD` or `PT_YIELD_UNTIL` macros.
- Within the protothread, you can use perfectly normal code, with a few restrictions - you can't put a `PT_YIELD` statement in a `PT_CATCHANY`, `PT_CATCH` or `PT_FINALLY` block. 
- A protothread supports the `PT_FINALLY` block which can be called by the parent using the `PT_FINAL`. This is a substitude for the `PROCESS_EXITHANDLER` macro defined and used in contiki-os which tries to pave the way for handling a gracefull shutdown of a protothread.


## Doxygen

Create a Doxygen configuration file:

Create a file named Doxyfile in the same directory as the pt.h file.

Open the Doxyfile in a text editor.

Add the following lines to the Doxyfile:

```makefile
PROJECT_NAME = "Protothreads Library"
INPUT = pt.h
OUTPUT_DIRECTORY = ./docs
JAVADOC_AUTOBRIEF = YES
```

Save and close the Doxyfile.

Run Doxygen:

Open a terminal or command prompt.

Navigate to the directory containing the Doxyfile file.

Run the command doxygen Doxyfile.


Review the generated documentation:

After running Doxygen, a new directory named docs should be created.
Open the docs directory and locate the generated HTML files.
Open the index.html file in a web browser to view the documentation.
The above steps will generate Doxygen documentation for the pt.h file in HTML format. You can customize the Doxygen configuration file (Doxyfile) to adjust various settings, such as output format, styling, and additional configuration options.

Note: It's important to ensure that Doxygen is installed on your system before running the above steps. Doxygen is an open-source documentation generator available for various platforms. You can download it from the official Doxygen website (https://www.doxygen.nl/download.html) and follow the installation instructions specific to your operating system.




Was this response better or worse?
Better
Worse
Same




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

Protothreads v2 wraps the returned state of a protothread into an enum called `ptstate_t`. Depending on the state of the protothread, it returns `PT_WAITING`, `PT_YIELDED`, `PT_EXITED`, `PT_ENDED`, which are also present in protothreads v1.4. In addition, protothreads v2 extends the state enumeration with `PT_ERROR` and `PT_FINALIZED`. 

```cpp
enum ptstate_t : uint8_t
{
  PT_WAITING = 0,
  PT_YIELDED = 1,
  PT_EXITED = 2,
  PT_ENDED = 3,
  PT_ERROR = 4, // or anything higher.
  PT_FINALIZED = 255
};
```

Because of the state extensions in protothreads v2, we now have the capability to add some new and exciting constructs. An example is the finally control block and the error encoding into the state that allows exception handling. This comes without adding any memory overhead in comparison to protothreads v1.4. But before we get into this, lets consider the following macros in the `iterator()` function below. If they seem familiar, its because that most of the different state values that are returned from the iterator are in part already present in protothreads v1.4.

```cpp
#include <protoduino.h>
#include <dbg/examples.h>

static ptstate_t iterator(struct pt * self)
{
  static uint8_t v;

  PT_BEGIN(self);
  forever: while(1)
  {
    v = random(0,8);
    if (v == 0)
      PT_WAIT_ONE(self); // returns PT_WAITING
    else if (v == 1)
      PT_YIELD(self); // returns PT_YIELDED
    else if (v == 2)
      PT_RESTART(self); // returns PT_WAITING
    else if (v == 3)
      PT_EXIT(self); // returns PT_EXITED
    else if (v == 4)
      break;
    else if (v == 5)
      PT_RAISE(self, PT_ERROR + v); // returns PT_WAITING
    else
      PT_THROW(self, PT_ERROR + v); // returns PT_ERROR + v
  }
  PT_END(self); // returns PT_FINALIZED
}

static struct pt it1;

void setup()
{
  print_setup();
  PT_INIT(&it1);
}

void loop()
{
  print_state(iterator(&it1), "void loop()");
  print_count++;
  delay(1000);
}
```

#### - **Reinitialization Requirement**

Protothreads v2 breaks from the state machine control flow that made protothreads v1.4. When a protothread in v2 has exited, ended or throws an error, the protothread is blocked until it is explicitly finalized. Protothreads v2 introduces the ultimate final state `PT_FINALIZED` of a protothread, after which nothing else should occur as a side effect of the state of a protothread.

The next example show's what that means for the state of a protothread:

```cpp
#include <protoduino.h>
#include <dbg/examples.h>

static ptstate_t protothread(struct pt *self)
{
  PT_BEGIN(self);

  PT_WAIT_ONE(self);

  PT_END(self);
}

static struct pt pt1;

void setup()
{
  print_setup();
  PT_INIT(&pt1);
}

void loop()
{
    print_state(protothread(&pt1), "< protothread");
    print_count++;
    delay(2000);
}
```

#### - **Finalizing Protothreads**

Protothreads v2 introduces the `PT_FINALIZED` protothread state. This new state is member of the `ptstate_t` enum as an integral part of how v2 introduces new behaviour into the FSM of protothreads; when in v1.4 the end of a protothread is reached, using the `PT_END()` macro, a `PT_ENDED` state is returned to the caller. This is not the case with protothreads v2. Protothreads v2 returns at the `PT_END()` of a protothread, a `PT_FINALIZED` state. 

To intercept the control flow and its behaviour of a protothread and return a `PT_ENDED` state, protothreads v2 introduces the `PT_FINALLY()` control block.

```cpp
#include <protoduino.h>
#include <dbg/examples.h>

static ptstate_t protothread(struct pt *self)
{
  //static uint8_t v;
  PT_BEGIN(self);

  PT_WAIT_ONE(self); // returns PT_WAITING

  PT_FINALLY(self) // returns PT_ENDED
  {
    print_line("PT_FINALLY() protothread");
  }
  PT_END(self); // returns PT_FINALIZED
}

static struct pt pt1;

void setup()
{
  print_setup();
  PT_INIT(&pt1);
}

void loop()
{
  print_count++;
  delay(1000);

  ptstate_t state = protothread(&pt1);
  print_state(state, pt1.lc, "< protothread");
  if (state == PT_FINALIZED || PT_ISRUNNING(state))
    return;
  
  PT_FINAL(&pt1);
}
```

In this simple example we execute a protothread and wait for its state to return, in order to determine whether or not we should finalize it by calling the `PT_FINAL()` macro.
Working with different compilers I came to conclude that whenever you want to define a local variable in order to hold some value, it is better for protothreads v2 to declare a static variable above all; I have seen stuff done by compilers or the like that can realy create unpredictable behaviour.


#### - **Exception Handling**

Protothreads v2 introduces native exception handling capabilities. It's a little bit different from the exception handling that we acquire in languages like `C++`, `C#` or `JAVA` for example. This is because of the nature of the `PT_ERROR` state, which enables a protothread to support lightweight exception handling. By encoding an error code into the state when raising an exception, protothreads v2 includes some new constructs that allow for easy and natural implementation of otherwise complex structures like this. A protothread in v2 raises the exception using the `PT_RAISE()` macro and handles the exception using the `PT_CATCHANY()` or `PT_CATCH()` macros. When raising an exception the protothread MUST implement at least a `PT_CATCHANY()` control block. When an exception is handled within the protothread, the thread can gracefully exit, restart, or throw an error. Some generic errors are already defined in `<sys/errors.h>`, but they offer an alternative idea to an abstraction of all that programming in C has to offer. Therefor this document needs an overhaul, to more accuratelly explain the situation in C, pretty soon.


```cpp
#include <protoduino.h>
#include <dbg/examples.h>

static ptstate_t protothread(struct pt *self)
{
  //static uint8_t v;
  PT_BEGIN(self);

  PT_WAIT_ONE(self); // returns PT_WAITING

  PT_RAISE(self, (PT_ERROR + random(0, 63)));

  print_line("UNREACHABLE protothread");

  PT_CATCHANY(self)
  {
    print_error("PT_CATCHANY() protothread", PT_ERROR_STATE);
  }
  PT_END(self); // returns PT_FINALIZED
}

static struct pt pt1;

void setup()
{
  print_setup();
  PT_INIT(&pt1);
}

void loop()
{
  print_count++;
  delay(1000);

  ptstate_t state = protothread(&pt1);
  print_state(state, pt1.lc, "< protothread");
  if (state == PT_FINALIZED || PT_ISRUNNING(state))
    return;
  
  PT_FINAL(&pt1);
}
```

In contrary to the finalizing control block in the previous chapter, we now have a protothread that is capable of handing errors by using the `PT_RAISE()` and `PT_CATCHANY()` macros without the parent thread intervening to do so. Try removing the `PT_RAISE()` macro and notice that the `PT_CATCHANY()` control block is called with `255` as the error code, which is equal to the `PT_FINALIZED` state.

For a more advanced example, see the `20-pt-basic-term.ino` sketch in the `examples/` directory.

#### - **Error Handling**

When a spawned protothread using the `PT_SPAWN` or `PT_FOREACH` macros throw an error, the calling parent thread handles the error of the child protothread, using the `PT_ONERROR` macro and raises an exception automatically. The only macro that does not handle an error automatically is the `PT_WAIT_THREAD` macro. With this macro, the errors should be handled manually. The `PT_ONERROR` macro requires the returned state of a protothread to decide whether or not an error occured, which is by design stored in the local variable `PT_ERROR_STATE` and should be used in the majority of cases when using the `PT_ONERROR` macro. The `PT_ERROR_STATE` local variable replaces the local `PT_YIELD_FLAG` variable used in protothreads v1.4 and therefor doesn't add any overhead using error or exception handling. 

See the `08-pt-try-catch.ini` example for its usage:

```cpp
#include <protoduino.h>
#include <dbg/examples.h>

static ptstate_t child(struct pt *self)
{
  PT_BEGIN(self);
  
  PT_WAIT_ONE(self);

  if (random(0, 2))
    PT_THROW(self, PT_ERROR + random(0, 63));

  PT_END(self);
}

static ptstate_t protothread(struct pt *self)
{
  static struct pt it1;

  PT_BEGIN(self);

  PT_INIT(&it1);
  PT_WAIT_THREAD(self, child(&it1));
  PT_ONERROR(PT_ERROR_STATE)
    PT_RAISE(self, PT_ERROR_STATE);
  
  PT_CATCHANY(self)
  {
    print_line("PT_CATCHANY() protothread");
  }
  PT_FINALLY(self)
  {
    print_line("PT_FINALLY() protothread");
  }

  PT_END(self);
}

static struct pt pt1;

void setup()
{
  print_setup();
  PT_INIT(&pt1);
}

void loop()
{
  print_count++;
  delay(1000);

  ptstate_t state = protothread(&pt1);
  print_state(state, pt1.lc, "< protothread");
  if (state == PT_FINALIZED || PT_ISRUNNING(state))
    return;
  
  PT_FINAL(&pt1);
}
```

This example shows the propper use of the PT_CATCHANY() and PT_FINALLY() control blocks and is the verbose version of the alternative using the `PT_SPAWN()` macro below.

```cpp
#include <protoduino.h>
#include <dbg/examples.h>

static ptstate_t child(struct pt *self)
{
  PT_BEGIN(self);
  
  PT_WAIT_ONE(self);

  if (random(0, 2))
    PT_THROW(self, PT_ERROR + random(0, 63));

  PT_END(self);
}

static ptstate_t protothread(struct pt *self)
{
  static struct pt it1;

  PT_BEGIN(self);

  PT_SPAWN(self, &it1, child(&it1));
  
  PT_CATCHANY(self)
  {
    print_line("PT_CATCHANY() protothread");
  }
  PT_FINALLY(self)
  {
    print_line("PT_FINALLY() protothread");
  }

  PT_END(self);
}

static struct pt pt1;

void setup()
{
  print_setup();
  PT_INIT(&pt1);
}

void loop()
{
  print_count++;
  delay(1000);

  ptstate_t state = protothread(&pt1);
  print_state(state, pt1.lc, "< protothread");
  if (state == PT_FINALIZED || PT_ISRUNNING(state))
    return;
  
  PT_FINAL(&pt1);
}
```

#### - **Iterators and Protothreads**

Protothreads v2 makes a more distinct use of the `PT_WAITING` and `PT_YIELDED` states and includes the additional macros `PT_FOREACH` and `PT_ENDEACH`, which allow for easy handling of iterator protothreads. These macros can be used in combination with the `PT_YIELD` or `PT_YIELD_UNTIL` macros.

See the `07-pt-yield-foreach.ino` example for its usage:

```cpp
#include <protoduino.h>
#include <dbg/examples.h>

struct ptyield
{
    lc_t lc;
    uint8_t value;
};

static ptstate_t iterator(struct ptyield *self, uint8_t max)
{  
  PT_BEGIN(self);

  forever: while(1) {

    PT_WAIT_ONE(self);

    self->value = random(0, 255);

    PT_YIELD(self);

    if (self->value >= max)
      PT_EXIT(self);
    
  }

  PT_END(self);
}

static ptstate_t protothread(struct pt *self)
{
  static struct ptyield it1;

  PT_BEGIN(self);

  PT_FOREACH(self, &it1, iterator(&it1, 240))
  {
    print_line("Iterator yielded value:", it1.value);

    PT_WAIT_ONE(self);
  }
  PT_ENDEACH(self);

  PT_END(self);
}

static struct pt pt1;

void setup()
{
  print_setup();
  PT_INIT(&pt1);
}

void loop()
{
  print_count++;
  delay(1000);

  ptstate_t state = protothread(&pt1);
  print_state(state, pt1.lc, "< protothread");
  if (state == PT_FINALIZED || PT_ISRUNNING(state))
    return;
  
  PT_FINAL(&pt1);
}
```

#### - **Scheduling Protothreads**

The `PT_SCHEDULE` macro in protothreads v2, sets the local `PT_ERROR_STATE` variable and must be used within a protothread only. If used outside a protothread, the `PT_ISRUNNING` macro should be used to schedule and test if the protothread is still running.

```cpp
#include <protoduino.h>
#include <dbg/examples.h>

static ptstate_t protothread(struct pt *self)
{
  PT_BEGIN(self);

  forever: while(1)
  {
    print_line("forever: protothread");
    delay(1000);

    if (random(0, 2))
      PT_WAIT_ONE(self);
    
    if (random(0, 2))
      PT_THROW(self, PT_ERROR);
  }

  PT_END(self);
}

static ptstate_t main_driver(struct pt *self)
{
  static struct pt pt1;

  PT_BEGIN(self);
  
  PT_INIT(&pt1);
  while(PT_SCHEDULE(protothread(&pt1)))
  {
    print_state(PT_ERROR_STATE, "< protothread 1");
    delay(1000);
  }
  PT_ONERROR(PT_ERROR_STATE)
  {
    print_error("protothread ONERROR", PT_ERROR_STATE);
    PT_EXIT(self);
  }

  PT_END(self);
}

void setup()
{
  print_setup();
}

void loop()
{
  static struct pt pt1;
  static ptstate_t state;

  PT_INIT(&pt1);
  while(PT_ISRUNNING(main_driver(&pt1)))
  {
    print_state(state, pt1.lc, "main_driver ISRUNNING");
    delay(1000); // arduino delay a second
  }
  print_count++;
}
```

#### - **Code Restrictions**

Protothreads v2 is code compatible with v1.4 for the most part. The restrictions are similar with protothreads v1.4 (i.e. watch out with local variables), but also warns you for example, with the use of a `PT_YIELD` statement, which should preferably not be placed in a `PT_CATCHANY`, `PT_CATCH`, or `PT_FINALLY` block.

### **Compatibility**

Protothreads v2 is an almost backward-compatible library that preserves the functionality of protothreads v1.4 while introducing new features. Existing code written using protothreads v1.4 should work with protothreads v2 with minimal modifications.

Although not tested, going through the source code of `Contiki-OS` this doesn't seem to be a blocking change. If you have evidence of blocking changes within Contiki-OS please take up the issue in the issue's page of github.

Protothreads v2 differs to v1 in that it is the responsibility of the caller to reset the state of a protothread. This is automatically done, when you spawn a child protothread with the `PT_SPAWN` or `PT_FOREACH` macros. Looking at Contiki-OS, which uses protothread v1 at its foundation, I could not detect any immediate problems in its `process.h` file, which is the ultimate parent. Since protoduino and protothread v2, haven't been tested in conjunction with Contiki-OS, there isn't much to say about compatability.

@todo Protothreads v2 does not implement `lc-addr.h` for now and for several reasons.

### **Getting Started**

To start using the protothreads library in your protoduino project, follow these steps:

1. Download the protoduino library from the [GitHub repository](https://github.com/jklarenbeek/protoduino).

2. Extract the downloaded ZIP file to a convenient location on your computer.

3. Open the Arduino IDE or your preferred development environment.

4. In the IDE, navigate to **Sketch > Include Library > Add .ZIP Library**.

5. Browse to the extracted Protothreads v2 library folder and select it.

6. Click the **Open** button to install the library.

7. Include the necessary header files in your source code, such as `<protoduino.h>` and `<sys/pt.h>`

For more information and detailed usage examples, refer to the complete documentation available in the `examples/` directory.

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

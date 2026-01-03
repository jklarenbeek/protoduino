// file: ./src/sys/serial/SerialClass.hpp

#ifndef __SerialClass_HPP__
#define __SerialClass_HPP__
#include "Stream.h"
#include "../serial.h"
#include <stdbool.h>

/**
 * @brief Arduino-compatible Serial class wrapper for Protoduino's C99 serial implementation
 *
 * @details
 * This header provides a C++ class (Serial0Class) that mimics the standard Arduino `Serial` object,
 * allowing existing Arduino sketches to compile and run on Protoduino with minimal or no changes.
 * The class inherits from `Stream` (and through it from `Print`), implementing the core methods expected
 * by the Arduino API: begin(), end(), available(), peek(), read(), flush(), availableForWrite(), write(), etc.
 *
 * Protoduino's serial communication is implemented primarily in C99 (see serial.h and the underlying uart.h).
 * This approach ensures maximum portability, smaller code size, and easier integration into non-Arduino environments.
 * SerialClass.hpp acts as a thin compatibility layer that wraps the C99 functions (e.g., serial0_open(), serial0_write8())
 * into a familiar C++ object.
 *
 * @note Current status and limitations
 * - Only Serial0 (corresponding to UART0) is currently implemented as Serial0Class.
 * - Serial1, Serial2, and Serial3 (when the hardware has additional UARTs) are **not yet provided**.
 *   Implementing them is straightforward and should follow the same pattern:
 *   either by duplicating the class for each instance (Serial1Class, etc.) or,
 *   preferably, by creating a macro-templated reusable header similar to serial_private_header.h.
 *   This is marked as a TODO for future completeness.
 *
 * @note Fallback mechanism
 * If the preprocessor symbol `USE_ARDUINO_HARDWARESERIAL` is defined via `protoduino-config.h` and before including this file,
 * the entire Protoduino SerialClass implementation is skipped, allowing the sketch to fall back
 * to the original Arduino core's HardwareSerial class. This is useful for hybrid projects or
 * when specific features of the official implementation are required.
 *
 * @warning
 * This file should only be included in C++ sketches that need Arduino-style `Serial`.
 * Pure C projects should use the functions declared in serial.h directly.
 */

#ifndef USE_ARDUINO_HARDWARESERIAL

class Serial0Class : public Stream
{

  public:
    inline void begin(uint32_t baud) { serial0_open(baud); }
    inline void begin(uint32_t baud, uint8_t config) { serial0_openex(baud, config); }
    inline void end() { serial0_close(); }

    inline int available(void) { return serial0_read_available(); }
    inline int peek(void) { return serial0_peek8(); }
    inline int read(void) { return serial0_read8(); }

    inline void flush(void) { while(serial0_flush() > 0); }

    inline int availableForWrite(void) { return serial0_write_available(); }

    inline size_t write(const uint8_t n)
    {
      while(serial0_write8(n) == 0)
        serial0_flush();
      return 1;
    }

    // TODO: the following four functions are faulty, but has a low priority right now
    inline size_t write(uint32_t n) { return write((const uint8_t)n); }
    inline size_t write(int32_t n) { return write((const uint8_t)n); }
    inline size_t write(uint16_t n) { return write((const uint8_t)n); }
    inline size_t write(int16_t n) { return write((const uint8_t)n); }

    using Print::write; // pull in write(str) and write(buf, size) from Print

    operator bool() { return true; } // TODO: Why?

};

static Serial0Class CSerial0 = Serial0Class();

#define SerialLine CSerial0

#else

#include <Arduino.h>
#define SerialLine Serial

#endif

#endif
// file: ./src/sys/tmpl/SerialClass.hpp

#ifndef __SerialClass_HPP__
#define __SerialClass_HPP__
#include "Stream.h"
#include "../serial.h"
#include <stdbool.h>

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

    inline size_t write(uint32_t n) { return write((const uint8_t)n); }
    inline size_t write(int32_t n) { return write((const uint8_t)n); }
    inline size_t write(uint16_t n) { return write((const uint8_t)n); }
    inline size_t write(int16_t n) { return write((const uint8_t)n); }

    using Print::write; // pull in write(str) and write(buf, size) from Print

    operator bool() { return true; } // TODO: Why?

};

static Serial0Class CSerial0 = Serial0Class();

#define SerialLine CSerial0
#endif

#endif
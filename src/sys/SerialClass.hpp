#ifndef __SerialClass_HPP__
#define __SerialClass_HPP__
#include "Stream.h"
#include <stdbool.h>

extern "C" {
  typedef bool (*serial_onrecieved_fn)(uint_fast8_t);
}

class Serial0Class : public Stream
{

  public:
    virtual void onRecieved(const serial_onrecieved_fn callback);

    virtual void begin(uint32_t baud);
    virtual void begin(uint32_t, uint8_t);
    virtual void end();

    virtual int available(void);
    virtual int peek(void);
    virtual int read(void);

    virtual int_fast32_t read16(void);
    virtual int_fast32_t read24(void);
    virtual int_fast32_t read32(void);

    virtual int flushOne(void);
    virtual void flush(void);

    virtual int availableForWrite(void);

    virtual size_t write(const uint8_t);

    inline size_t write(uint32_t n) { return write((const uint8_t)n); }
    inline size_t write(int32_t n) { return write((const uint8_t)n); }
    inline size_t write(uint16_t n) { return write((const uint8_t)n); }
    inline size_t write(int16_t n) { return write((const uint8_t)n); }

    using Print::write; // pull in write(str) and write(buf, size) from Print

    virtual uint_fast8_t write16(const uint_fast16_t data);
    virtual uint_fast8_t write24(const uint_fast32_t data);
    virtual uint_fast8_t write32(const uint_fast32_t data);

    operator bool() { return true; } // TODO: Why?

};

Serial0Class Serial0 = Serial0Class();

#endif
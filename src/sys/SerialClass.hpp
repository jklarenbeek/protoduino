#ifndef __SerialClass_HPP__
#define __SerialClass_HPP__

#include "pt.h"
#include "Stream.h"
void delay(unsigned long ms);

class Serial0Class : public Stream
{

  public:
    virtual void begin(unsigned long baud);
    virtual void begin(unsigned long, uint8_t);
    virtual void end();
    virtual ptstate_t end(struct pt *pt);
    virtual int available(void);
    virtual int peek(void);
    virtual int read(void);
    virtual int availableForWrite(void);
    virtual int flushex(void);
    virtual ptstate_t flush(struct pt *pt);
    inline void flush(void) 
    { 
      int val = 0;
      int cnt = 0;
      while((val = flushex()) > 0)
      {
        cnt += val;
        delay(1);
      }
    
      return cnt;
    }

    virtual size_t write(const uint8_t);

    inline size_t write(unsigned long n) { return write((const uint8_t)n); }
    inline size_t write(long n) { return write((const uint8_t)n); }
    inline size_t write(unsigned int n) { return write((const uint8_t)n); }
    inline size_t write(int n) { return write((const uint8_t)n); }

    using Print::write; // pull in write(str) and write(buf, size) from Print

    operator bool() { return true; } // TODO: Why?

};

#endif
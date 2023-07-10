#ifndef __PROTODUINO_CONFIG_H__
#define __PROTODUINO_CONFIG_H__


// flag to force using the default Arduino UART ISR vectors
//#define USE_ARDUINO_HARDWARESERIAL


#if !defined(SERIAL_RX_BUFFER_SIZE)
#if ((RAMEND - RAMSTART) < 1023)
#define SERIAL_RX_BUFFER_SIZE 16
#else
#define SERIAL_RX_BUFFER_SIZE 64
#endif
#endif

#if !defined(SERIAL_TX_BUFFER_SIZE)
#if ((RAMEND - RAMSTART) < 1023)
#define SERIAL_TX_BUFFER_SIZE 16
#else
#define SERIAL_TX_BUFFER_SIZE 64
#endif
#endif


#endif
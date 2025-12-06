#ifndef __SERIAL_CONF_H__
#define __SERIAL_CONF_H__

// flag to force using the default Arduino UART ISR vectors
// #define USE_ARDUINO_HARDWARESERIAL

// flag to get a count of rx errors or buffer overflows
// this feature will consume an extra 8 bytes per serial device.
#define SERIAL_REGISTER_ERRORS

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
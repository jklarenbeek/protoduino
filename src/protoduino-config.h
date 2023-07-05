#ifndef __PROTODUINO_CONFIG_H__
#define __PROTODUINO_CONFIG_H__


// flag to force using the default Arduino UART ISR vectors
//#define USE_ARDUINO_HARDWARESERIAL

#ifndef SERIAL_BUFFER_RX_SIZE
#define SERIAL_BUFFER_RX_SIZE 8
#endif
#ifndef SERIAL_BUFFER_TX_SIZE
#define SERIAL_BUFFER_TX_SIZE 8
#endif

#endif
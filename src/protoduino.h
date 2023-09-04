#ifndef __PROTODUINO_H__
#define __PROTODUINO_H__

#include "protoduino-config.h"
#ifdef USE_ARDUINO_HARDWARESERIAL
#include <Arduino.h>
#define SerialLine Serial
#endif

#include <stdbool.h>

#include "cc.h"
#include "sys/pt.h"
#include "sys/errors.h"


#endif
#if defined (__AVR_ATmega328P__) || defined(__AVR_ATmega328__) || defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega88__)
#include "cpu/avr/clock.h"

#else

#error "clock: Unsupported Platform!"

#endif
#if defined(__AVR__)
#include "cpu/avr/clock.h"

#elif defined(CC_ARCH_RP2) || (defined(PICO_RP2040) && PICO_RP2040) || (defined(PICO_RP2350) && PICO_RP2350) || defined(ARDUINO_ARCH_RP2040)
#include "../cpu/rp2/clock.h"

#else

#error "clock: Unsupported Platform!"

#endif

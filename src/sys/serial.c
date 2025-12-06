#include "serial.h"
// ---------------------------------------------------------------------------
// Helper: Print a CC_PROGMEM string (F-macro style)
// ---------------------------------------------------------------------------
void print_P(const char *s)
{
    if (!s) return;
    char c;
    while ((c = pgm_read_byte(s++)) != '\0') {
        while (serial0_write8((uint_fast8_t)c) == 0) {
            serial0_flush();
        }
    }
}

// ---------------------------------------------------------------------------
// Helper: Print a regular RAM string
// ---------------------------------------------------------------------------
void print(const char *s)
{
    if (!s) return;
    char c;
    while ((c = *s++) != '\0') {
        while (serial0_write8((uint_fast8_t)c) == 0) {
            serial0_flush();
        }
    }
}

// ---------------------------------------------------------------------------
// Helper: Print Unsigned Integer (saves flash over printf)
// ---------------------------------------------------------------------------
void print_dec(uint8_t val) {
    char numbuf[4];
    uint8_t i = 0;
    if (val == 0) {
        print("0");
        return;
    }
    while (val > 0 && i < sizeof(numbuf) - 1) {
        numbuf[i++] = '0' + (val % 10);
        val /= 10;
    }
    // Print reverse
    while (i--) {
        while (serial0_write8(numbuf[i]) == 0) serial0_flush();
    }
}

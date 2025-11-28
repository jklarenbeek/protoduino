// file: ./src/sys/logger.c

#include "../protoduino.h"
#include <avr/pgmspace.h>
#include "process.h"
#include "errors.h"
#include "serial.h" // C interface to serial0_ functions

// #include <WString.h>

// ---------------------------------------------------------------------------
// Helper: Print a CC_PROGMEM string (F-macro style) using serial0_write8
// ---------------------------------------------------------------------------
static void print_P(const char *s)
{
    if (!s) return;

    char c;
    while ((c = pgm_read_byte(s++)) != '\0')
    {
        while (serial0_write8((uint_fast8_t)c) == 0)
        {
            serial0_flush(); // wait until space available
        }
    }
}

// ---------------------------------------------------------------------------
// Helper: Print a regular RAM string
// ---------------------------------------------------------------------------
static void print(const char *s)
{
    if (!s) return;

    char c;
    while ((c = *s++) != '\0')
    {
        while (serial0_write8((uint_fast8_t)c) == 0)
        {
            serial0_flush();
        }
    }
}

// ---------------------------------------------------------------------------
// Process Implementation
// ---------------------------------------------------------------------------
static void init_logger(void) {
      serial0_open(9600);
}

PROCESS(error_logger_process /*, &init_logger */, "Error Logger", 0);

PROCESS_THREAD(error_logger_process, ev, data)
{
    PROCESS_BEGIN();

    if (ev == PROCESS_EVENT_INIT)
    {
        print_P(F("[ErrorLogger] Ready\r\n"));
        serial0_flush();
    }

    while (1)
    {
        PROCESS_WAIT_EVENT();

        if (ev == PROCESS_EVENT_ERROR)
        {
            struct error_info
            {
                struct process *source;
                uint8_t code;
            } *info = (struct error_info *)data;

            print_P(PSTR("[ERROR] "));

            if (info->source && PROCESS_NAME_STRING(info->source))
            {
                print(PROCESS_NAME_STRING(info->source));
            }
            else
            {
                print_P(PSTR("<unknown process>"));
            }

            print_P(PSTR(": "));

            // Print error code as decimal
            char numbuf[8];
            uint8_t code = info->code;
            uint8_t i = 0;
            do
            {
                numbuf[i++] = '0' + (code % 10);
                code /= 10;
            } while (code && i < sizeof(numbuf));
            while (i--)
            {
                while (serial0_write8(numbuf[i]) == 0)
                    serial0_flush();
            }

            print_P(PSTR(" ("));
            const char *msg = error_to_string(info->code);
            if (msg)
            {
                print_P(msg); // error_to_string returns CC_PROGMEM pointer
            }
            else
            {
                print_P(PSTR("Unknown error"));
            }
            print_P(PSTR(")\r\n"));

            serial0_flush(); // Ensure full line is sent
        }

        PROCESS_YIELD(); // back to while(1)
    }

    PROCESS_END();
}
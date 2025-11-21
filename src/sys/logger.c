// file: ./src/sys/logger.c

#include "../protoduino.h"
#include <avr/pgmspace.h>
#include "process.h"
#include "errors.h"
#include "serial.h" // C interface to serial0_ functions
#include "dsc.h"

// ---------------------------------------------------------------------------
// DSC and Process Definition
// ---------------------------------------------------------------------------

PROCESS(error_logger_process, "Error Logger", 0);

DSC(error_logger_dsc,
    "Default Error Logger",
    "logger.prg",
    error_logger_process); // Note: 4-argument version from dsc.h


// ---------------------------------------------------------------------------
// Helper: Print a PROGMEM string (F-macro style) using serial0_write8
// ---------------------------------------------------------------------------
static void print_P(const char *s)
{
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
    if (!s)
        return;
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

PROCESS_THREAD(error_logger_process, ev, data)
{
    PROCESS_BEGIN();

    if (ev == PROCESS_EVENT_INIT)
    {
        print_P(PSTR("[ErrorLogger] Ready\r\n"));
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
                print_P((const char *)msg); // error_to_string returns PROGMEM pointer
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
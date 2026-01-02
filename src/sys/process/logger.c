// file: ./src/sys/logger.c

#include "logger.h"
#include "../ipc.h"
#include "../serial.h"

// ---------------------------------------------------------------------------
// Process Implementation
// ---------------------------------------------------------------------------

PROCESS(error_logger_process, "Error Logger", 0);

PROCESS_THREAD(error_logger_process, ev, data)
{
    PROCESS_BEGIN();

    // Initialization moved inside the thread for better portability
    if (ev == PROCESS_EVENT_INIT)
    {
        serial0_open(9600);
        print_P(PSTR("[Log] Ready\r\n"));
        serial0_flush();
    }

    while (1)
    {
        PROCESS_WAIT_EVENT();

        // 1. Handle Standard Errors
        if (ev == PROCESS_EVENT_ERROR)
        {
            // Use the definition from process.h, do not redefine
            struct error_info *info = (struct error_info *)data;

            print_P(PSTR("[ERR] Src:"));
            print(PROCESS_NAME_STRING(info->source));

            print_P(PSTR(" Code:"));
            print_dec(info->code);

            print_P(PSTR(" ("));
            // Assuming error_to_string is defined in errors.h and returns PROGMEM ptr
            const char *msg = error_to_string(info->code);
            if (msg)
            {
                print_P(msg);
            }
            else
            {
                print_P(PSTR("Unknown"));
            }
            print_P(PSTR(")\r\n"));
            serial0_flush();
        }
        // 2. Handle Message Leaks (Patch from previous analysis)
        else if (ev == PROCESS_EVENT_MSG_LEAK)
        {
            ipc_msg_t *m = (ipc_msg_t *)data;

            print_P(PSTR("[LEAK] IPC Msg Type:"));
            if (m)
            {
                print_dec(m->type);
                print_P(PSTR(" ArgC:"));
                print_dec(m->argc);

                // TODO: If you have a global pool, you can recover memory here:
                // extern struct ipc_pool my_system_pool;
                // ipc_msg_free_to_pool(&my_system_pool, m);
            }
            else
            {
                print_P(PSTR("NULL"));
            }
            print_P(PSTR("\r\n"));
            serial0_flush();
        }
    }

    PROCESS_END();
}
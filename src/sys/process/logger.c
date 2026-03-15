/* file: ./src/sys/logger.c */

#include "logger.h"
#include "../ipc.h"
#include "../serial.h"

PROCESS_DEFINE(error_logger_process, "Error Logger", 0, );
struct process error_logger_process;

PROCESS_THREAD(error_logger_process, ev, data)
{
  PROCESS_BEGIN();

  if (ev == PROCESS_EVENT_INIT)
  {
    if (data == 0)
      serial0_open(9600);
    else
      serial0_open(data);

    print_P(PSTR("[Log] Ready\r\n"));
    serial0_flush();
  }

  while (1)
  {
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_MSG)
    {
      ipc_msg_t *m = (ipc_msg_t *)data;
      print_P(PSTR("[LEAK] IPC Msg Type:"));
      if (m)
      {
        print_dec(m->type);
        print_P(PSTR(" ArgC:"));
        print_dec(m->argc);
      }
      else
      {
        print_P(PSTR("NULL"));
      }
      print_P(PSTR("\r\n"));
      serial0_flush();
    }
    else
    {
      struct error_info *info = (struct error_info *)data;
      print_P(PSTR("[ERR] Src:"));
      print_P(PROCESS_NAME_STRING(info->source));
      print_P(PSTR(" Code:"));
      print_dec(info->error);
      print_P(PSTR(" ("));
      const char *msg = error_to_string(info->error);
      if (msg) print_P(msg);
      else print_P(PSTR("Unknown"));
      print_P(PSTR(")\r\n"));
      serial0_flush();
    }
  }
  PROCESS_END();
}

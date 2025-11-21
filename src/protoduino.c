// file: ./src/protoduino.c

#include "protoduino.h"
#include "sys/process.h"

void protoduino_start() {
  process_init();
  // TODO: setup default process_set_error_sink(&error_logger_process);

  procinit_init();           // starts processes defined with PROCINIT
  autostart_start();          // starts processes defined with AUTOSTART_PROCESSES
}

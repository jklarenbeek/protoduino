// file: ./src/protoduino.c

#include "protoduino.h"
#include "sys/process.h"
#include "sys/logger.h"

void protoduino_start() {
  process_init(&error_logger_process);

  procinit_init();           // starts processes defined with PROCINIT
  autostart_start();          // starts processes defined with AUTOSTART_PROCESSES
}

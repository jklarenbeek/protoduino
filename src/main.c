// file: ./src/main.c

#include "main.h"
#include "sys/logger.h"

#if PROCESS_CONF_AUTO_IPC_POOL
// Define the static buffer and the global pool structure instance.
// The size is calculated at compile time based on IPC_CONF_MSG_BLOCK_COUNT.
static uint8_t sys_pool_buffer[sizeof(ipc_msg_t) * IPC_CONF_MSG_BLOCK_COUNT];
struct ipc_pool g_sys_msg_pool;
#endif

struct process *system_log_process = NULL;

void protoduino_start() {

#if PROCESS_CONF_AUTO_IPC_POOL
  // 1. Initialize the global IPC message pool.
  ipc_pool_init(
      &g_sys_msg_pool,
      sys_pool_buffer,
      sizeof(ipc_msg_t),
      IPC_CONF_MSG_BLOCK_COUNT
  );
#endif

  // 2. Initialize the scheduler and link the error logger.
  // The error logger can now safely rely on g_sys_msg_pool if it handles leaks.
  struct process *logger = system_log_process = NULL
    ? error_logger_process
    : system_log_process;

  process_init(&logger);

  procinit_init();           // starts processes defined with PROCINIT
  autostart_start();          // starts processes defined with AUTOSTART_PROCESSES
}
// file: ./src/main.h

#ifndef __MAIN_H__
#define __MAIN_H__

#include "protoduino.h"
#include "sys/process.h"
#include "sys/ipc.h"    // Required for ipc_pool_init and ipc_msg_t size

struct process *system_log_process;

/* ------------------------------------------------------------------ */
/* Auto-configuration for System-Wide IPC Message Pool                */
/* ------------------------------------------------------------------ */

// 1. Enable/Disable the automatic global IPC message pool setup.
// If enabled (1), protoduino will allocate memory and initialize the pool.
#ifndef PROCESS_CONF_AUTO_IPC_POOL
#define PROCESS_CONF_AUTO_IPC_POOL 1
#endif

#if PROCESS_CONF_AUTO_IPC_POOL
// 2. Define the size of the pool (in number of ipc_msg_t blocks).
// Set this based on your application's concurrency and memory budget.
#ifndef IPC_CONF_MSG_BLOCK_COUNT
#define IPC_CONF_MSG_BLOCK_COUNT 8 // Default size: 8 messages
#endif
#endif

#if PROCESS_CONF_AUTO_IPC_POOL
// Expose the single, globally defined message pool instance.
// This is the pool used by the system and available to application code.
extern struct ipc_pool g_sys_msg_pool;
#endif

CC_EXTERN void protoduino_start(void);

#endif // __PROTODUINO_H__
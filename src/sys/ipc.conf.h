// file: ./src/sys/ipc.conf.h

#ifndef __IPC_CONF_H__
#define __IPC_CONF_H__

/* Maximum number of argv slots in ipc_msg_t. Keep small for AVR. */
#ifndef IPC_MSG_MAX_ARGS
#define IPC_MSG_MAX_ARGS 4
#endif

#endif

// file: ./src/sys/process/pipelines.h

#ifndef __PROCESS_PIPELINES_H__
#define __PROCESS_PIPELINES_H__

#include <protoduino.h>
#include "../pt/pipe.h"
#include "../process.h"

// process IPC pipes helpers
#define PROCESS_STDIN()  (PROCESS_CURRENT()->stdin)
#define PROCESS_STDOUT() (PROCESS_CURRENT()->stdout)

#define PROCESS_PIPE_READ(buf, max_len, read_len_ptr) \
  PT_PIPE_READ_EX((PROCESS_CURRENT())->pt, PROCESS_STDIN(), buf, max_len, read_len_ptr)

#define PROCESS_PIPE_WRITE_ATOMIC(buf, len) \
  PT_PIPE_WRITE_ATOMIC_EX((PROCESS_CURRENT())->pt, PROCESS_STDOUT(), buf, len)

#define PROCESS_PIPE_WRITE(buf, len) \
  PT_PIPE_WRITE_EX((PROCESS_CURRENT())->pt, PROCESS_STDOUT(), buf, len, (PROCESS_CURRENT())->written)

#define PROCESS_PIPE_WRITE_BATCH(buf, len) \
  PT_PIPE_WRITE_BATCH_EX((PROCESS_CURRENT())->pt, PROCESS_STDOUT(), buf, len, (PROCESS_CURRENT())->written)

CC_EXTERN void process_ipc_wake(void *ctx);

#endif /* __PROCESS_PIPELINES_H__ */
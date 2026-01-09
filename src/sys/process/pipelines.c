// file: ./src/sys/process/pipelines.c

#include "pipelines.h"

void process_ipc_wake(void *ctx)
{
  struct process *p = (struct process *)ctx;
  if (p)
    process_poll(p);
}

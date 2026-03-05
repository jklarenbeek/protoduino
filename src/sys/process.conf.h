#ifndef __PROCESS_CONF_H__
#define __PROCESS_CONF_H__

/* ------------------------------------------------------------------ */
/* Configuration (override in protoduino_config.h)                    */
/* ------------------------------------------------------------------ */

#ifndef PROCESS_CONF_MAX_ARGS
#define PROCESS_CONF_MAX_ARGS 4
#endif

#ifndef PROCESS_CONF_ERROR_POOL_SIZE
#define PROCESS_CONF_ERROR_POOL_SIZE 4
#endif

#ifndef PROCESS_CONF_EVENT_QUEUE_SIZE
#define PROCESS_CONF_EVENT_QUEUE_SIZE 4
#endif

#ifndef PROCESS_CONF_EVENT_INBOX
#define PROCESS_CONF_EVENT_INBOX 0
#endif

#ifndef PROCESS_CONF_INBOX_SIZE
#define PROCESS_CONF_INBOX_SIZE 4
#endif

#ifndef PROCESS_CONF_PIPELINES
#define PROCESS_CONF_PIPELINES 1
#endif

#ifndef PROCESS_CONF_PIPELINES_MAX_STAGES
#define PROCESS_CONF_PIPELINES_MAX_STAGES 3
#endif

#ifndef PROCESS_CONF_PIPE_BUFFER_SIZE
#define PROCESS_CONF_PIPE_BUFFER_SIZE PT_CONF_PIPE_BUFFER_SIZE
#endif

#endif /* __PROCESS_CONF_H__ */
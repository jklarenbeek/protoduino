#ifndef __PROCESS_CONF_H__
#define __PROCESS_CONF_H__

/* ------------------------------------------------------------------ */
/* Configuration (override in protoduino_config.h)                    */
/* ------------------------------------------------------------------ */

#ifndef PROCESS_CONF_EVENT_QUEUE_SIZE
#define PROCESS_CONF_EVENT_QUEUE_SIZE 8
#endif

#ifndef PROCESS_CONF_PER_PROCESS_INBOX
#define PROCESS_CONF_PER_PROCESS_INBOX 0
#endif

#ifndef PROCESS_CONF_INBOX_SIZE
#define PROCESS_CONF_INBOX_SIZE 4
#endif

#ifndef PROCESS_CONF_INBOX_POINTERS
#define PROCESS_CONF_INBOX_POINTERS 0
#endif


#endif
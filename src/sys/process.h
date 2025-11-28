// file: ./src/sys/process.h

#ifndef PROCESS_H_
#define PROCESS_H_

#include "pt.h"        // Protothread v2: ptstate_t, PT_THREAD, PT_BEGIN, PT_END, PT_ISERROR
#include "lc.h"        // Local Continuations
#include "errors.h"    // Embedded Error Taxonomy

// --------------------------------------------------------------------------
// Typedefs
// --------------------------------------------------------------------------

typedef uint8_t process_event_t;
typedef void * process_data_t;
typedef uint8_t process_prio_t;  /* 0 = highest priority */
typedef uint8_t process_num_events_t;

// Function pointer for the process thread
typedef ptstate_t (*process_thread_t)(
    struct pt *process_pt,
    process_event_t process_event,
    process_data_t data
);

// Process Runtime States
typedef enum
{
  PROCESS_STATE_NONE    = 0, /* process is not doing anything */
  PROCESS_STATE_INIT    = 1, /* process is initializing, not listening to events */
  PROCESS_STATE_RUNNING = 3, /* process is running */
  PROCESS_STATE_CALLED  = 4, /* process has been called twice, not listening to events */
  PROCESS_STATE_EXITING = 5, /* process is finalizing, not listening to events */
} psstate_t;

// Process Default Events

#define PROCESS_EVENT_NONE          0
#define PROCESS_EVENT_INIT          50
#define PROCESS_EVENT_EXIT          51
#define PROCESS_EVENT_POLL          53
#define PROCESS_EVENT_ERROR         54
#define PROCESS_EVENT_MAX           70


// Structure for passing error information
struct error_info
{
    struct process *source;
    uint8_t code;
};

// --------------------------------------------------------------------------
// Core Process Structure
// --------------------------------------------------------------------------

struct process {
  struct process *next;          /* Pointer to next process */

#if !PROCESS_CONF_NO_PROCESS_NAMES
  const char *name;              /* Process name stored in flash */
#endif

  process_prio_t prio : 2;           /* Priority (0=Highest) */
  psstate_t state : 3;           /* Runtime state */
  uint8_t needspoll : 1;         /* Poll flag */
  uint8_t reserved_flags : 2;

  struct pt pt;                  /* Protothread internal state */
  process_thread_t thread;       /* The thread function */
};

// --------------------------------------------------------------------------
// Macros
// --------------------------------------------------------------------------

#define PROCESS_THREAD(name, ev, data) \
  static ptstate_t process_thread_##name( \
    struct pt *pt_process, \
    process_event_t ev, \
    process_data_t data)

#define PROCESS(name, strname, priority) \
  static const char process_name_##name[] CC_PROGMEM = strname; \
  PROCESS_THREAD(name, ev, data); \
  struct process name = { \
    NULL, \
    process_name_##name, \
    priority, \
    PROCESS_STATE_NONE, \
    0, 0, \
    { 0 }, \
    process_thread_##name \
  }

#define PROCESS_EXTERN(name) extern struct process name

#if !PROCESS_CONF_NO_PROCESS_NAMES
#define PROCESS_NAME_STRING(p) ((const char *)(p == NULL ? F("NULL") : ((p)->name ? (p)->name : F(""))))
#else
#define PROCESS_NAME_STRING(p) ((const char *)(p == NULL ? F("NULL") : F("")))
#endif

// Context Helpers
#define PROCESS_BEGIN()             PT_BEGIN(pt_process)
#define PROCESS_END()               PT_END(pt_process)
#define PROCESS_EXIT()              PT_EXIT(pt_process)
#define PROCESS_WAIT_EVENT()        PT_YIELD(pt_process)
#define PROCESS_WAIT_EVENT_UNTIL(c) PT_YIELD_UNTIL(pt_process, c)

// --------------------------------------------------------------------------
// API
// --------------------------------------------------------------------------

void process_init(struct process *error_logger);
void process_start(struct process *p);
void process_exit(struct process *p);
void process_run(void);
void process_post(struct process *p, process_event_t ev, process_data_t data);
void process_poll(struct process *p);

#endif /* PROCESS_H_ */
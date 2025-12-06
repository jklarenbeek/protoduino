// file: ./src/sys/process.h
#ifndef PROCESS_H_
#define PROCESS_H_

/*
 * process.h - protoduino process scheduler header
 *
 * See design notes in repository README. This header defines the
 * process struct, macros and scheduler API.
 *
 * Config knobs are overridable in ./src/protoduino_config.h
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "../cc.h"      /* CC_ATOMIC_RESTORE() / CC_ATOMIC_FORCEON() */
#include "errors.h"     /* ERR_* */
#include "pt.h"         /* ptstate_t, PT_* macros */

/* ------------------------------------------------------------------ */
/* Basic types & events                                               */
/* ------------------------------------------------------------------ */
typedef uint8_t process_event_t;
typedef void * process_data_t;
typedef uint8_t process_prio_t;
typedef uint8_t process_num_events_t;

/* Process thread prototype (unchanged) */
typedef ptstate_t (*process_thread_t)(
    struct pt *process_pt,
    process_event_t process_event,
    process_data_t data
);

/* Process runtime state (kept in process struct) */
typedef enum {
    PROCESS_STATE_NONE = 0,
    PROCESS_STATE_INIT = 1,
    PROCESS_STATE_RUNNING = 3,
    PROCESS_STATE_CALLED = 4,
    PROCESS_STATE_EXITING = 5,
} psstate_t;

/* Standard events */
#define PROCESS_EVENT_NONE       0
#define PROCESS_EVENT_INIT       50
#define PROCESS_EVENT_EXIT       51
#define PROCESS_EVENT_POLL       53
#define PROCESS_EVENT_ERROR      54
#define PROCESS_EVENT_MSG        60  /* intended to carry ipc_msg_t* */
#define PROCESS_EVENT_MSG_LEAK   61  /* A process exited while holding this message data (ipc_msg_t*) */
#define PROCESS_EVENT_PIPE_CTRL  62

/* Error information structure */

struct error_info {
    struct process *source;    /* process that generated the error */
    uint8_t code;              /* raw ptstate_t error code (>= PT_ERROR) */
};

/* -- process struct -------------------------------------------------- */

struct process {
    struct process *next;

#if !defined(PROCESS_CONF_NO_PROCESS_NAMES) && defined(__AVR__)
    const char *name; /* pointer to PROGMEM string (pointer in RAM) */
#elif !defined(PROCESS_CONF_NO_PROCESS_NAMES)
    const char *name;
#endif

    process_prio_t prio;
    psstate_t state : 3;
    uint8_t needspoll : 1;
    uint8_t reserved_flags : 4;
    struct pt pt;
    process_thread_t thread;

#if PROCESS_CONF_PER_PROCESS_INBOX
#if PROCESS_CONF_INBOX_POINTERS
    process_event_t inbox_ev[PROCESS_CONF_INBOX_SIZE];
    process_data_t  inbox_data[PROCESS_CONF_INBOX_SIZE];
#else
    struct { process_event_t ev; process_data_t data; } inbox[PROCESS_CONF_INBOX_SIZE];
#endif
    uint8_t inbox_head;
    uint8_t inbox_tail;
#endif
};

/* Proc thread / declaration macros */
#define PROCESS_THREAD(name, ev, data) \
  static ptstate_t process_thread_##name( \
    struct pt *pt_process, \
    process_event_t ev, \
    process_data_t data)

#if defined(__AVR__)
#include <avr/pgmspace.h>
#define PROCESS(name, strname, priority) \
  static const char process_name_##name[] PROGMEM = strname; \
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
#else
#define PROCESS(name, strname, priority) \
  static const char process_name_##name[] = strname; \
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
#endif

#define PROCESS_EXTERN(name) extern struct process name

#if !defined(PROCESS_CONF_NO_PROCESS_NAMES)
#define PROCESS_NAME_STRING(p) ((const char *)(p == NULL ? (const char*)"NULL" : ((p)->name ? (const char*)(p)->name : (const char*)"")))
#else
#define PROCESS_NAME_STRING(p) ((const char *)(p == NULL ? (const char*)"NULL" : (const char*)""))
#endif

/* Protothread helpers */
#define PROCESS_BEGIN()             PT_BEGIN(pt_process)
#define PROCESS_END()               PT_END(pt_process)
#define PROCESS_EXIT()              PT_EXIT(pt_process)
#define PROCESS_WAIT_EVENT()        PT_YIELD(pt_process)
#define PROCESS_WAIT_EVENT_UNTIL(c) PT_YIELD_UNTIL(pt_process, c)

/* ------------------------------------------------------------------ */
/* Scheduler API                                                      */
/* ------------------------------------------------------------------ */

/* Initialize scheduler. Pass an optional error_logger process (may be NULL) */
void process_init(struct process *error_logger);

/* Register and start a process (sends INIT) */
void process_start(struct process *p);

/* Stop and remove a process from scheduler */
void process_exit(struct process *p);

/* Game-loop scheduler: handle polls first if poll_requested, otherwise handle exactly one event */
void process_run(void);

/* Post event to global queue (atomic). Returns 1 on success, 0 if queue full.
 * p == NULL => broadcast.
 * Safe to call from ISR (uses CC_ATOMIC_RESTORE()).
 */
int process_post(struct process *p, process_event_t ev, process_data_t data);

/* Alias for ISR explicitness (same behavior as process_post) */
int process_post_from_isr(struct process *p, process_event_t ev, process_data_t data);

/* Request a poll for a process (sets needspoll and poll_requested) */
void process_poll(struct process *p);

/* Convenience: report error to configured logger (if any) */
void process_report_error(struct process *src, uint8_t code);

/* End of header */
#endif /* PROCESS_H_ */

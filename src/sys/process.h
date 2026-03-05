// file: ./src/sys/process.h
#ifndef __PROCESS_H__
#define __PROCESS_H__

/*
 * process.h - protoduino process scheduler header
 *
 * See design notes (./docs/scheduler.md). This header defines the
 * process struct, macros and scheduler API.
 *
 * Config knobs are overridable in ./src/protoduino_config.h
 */

#include <protoduino.h>
#if PROCESS_CONF_PIPELINES
#include "process/pipelines.h"
#endif

/* ------------------------------------------------------------------ */
/* Basic types & events                                               */
/* ------------------------------------------------------------------ */
typedef uint8_t process_event_t;
typedef void *process_data_t;
typedef uint8_t process_prio_t;
typedef uint8_t process_num_events_t;

/* Process thread prototype (unchanged) */
typedef ptstate_t (*process_thread_t)(struct pt *process_pt,
                                      process_event_t process_event,
                                      process_data_t data);
typedef struct process_args {
  uint8_t argc;
  void *argv[PROCESS_CONF_MAX_ARGS];
} process_args_t;

/* Process runtime state (kept in process struct) */
typedef enum psstate_t {
  PROCESS_STATE_NONE = 0,
  PROCESS_STATE_INIT = 1,
  PROCESS_STATE_RUNNING = 2,
  PROCESS_STATE_CALLED = 3,
  PROCESS_STATE_EXITING = 4, /* process in finalizing state*/
} psstate_t;

/* Standard events: TODO renumber in accordance with the protoduino Event/Error
 * Ontology */
#define PROCESS_EVENT_NONE EVENT_NONE   /* 0 => ERR_FINALIZED = 0xFF */
#define PROCESS_EVENT_INIT EVENT_INIT   /* 1 => ERR_DEADLOCK = 0xFE */
#define PROCESS_EVENT_POLL EVENT_POLL   /* 2 => ERR_LOCK_CRITICAL = 0xFD */
#define PROCESS_EVENT_EXIT EVENT_EXIT   /* 3 => ERR_LOCK_ATOMIC = 0xFC */
#define PROCESS_EVENT_ERROR EVENT_ERROR /* 4 => ERR_CRIT_ABANDONED = 0xFB */
#define PROCESS_EVENT_MSG EVENT_MSGQ_SEC
#define PROCESS_EVENT_PIPE EVENT_PIPE /* ERR_PIPE_BROKEN */

/* Error information structure */
struct error_info {
  struct process *source; /* process that generated the error */
  uint8_t severity : 3; /* Non = 0/Debug = 1/Flow/Verbose/Info/Warn/Error/Fatal
                           => Least signifant 3 bits */
  uint8_t reserved : 5;
  uint8_t event; /* the event that triggered the error (e.g. PROCESS_EVENT_MSG) */
  uint8_t error; /* raw ptstate_t error code (>= PT_ERROR) */
};

/* -- process struct -------------------------------------------------- */
#ifndef PROCESS_CONF_NO_PROCESS_NAMES
#define PROCESS_NAME_STRING(p)                                                 \
  (p == NULL ? PSTR("NULL") : ((p)->name ? (p)->name : PSTR("EMPTY")))
#else
#define PROCESS_NAME_STRING(p) (p == NULL ? PSTR("NULL") : PSTR("EMPTY"))
#endif

struct process {
  struct process *next;

#ifndef PROCESS_CONF_NO_PROCESS_NAMES
  const char *name;
#endif

  process_prio_t prio;

  psstate_t state : 3;
  uint8_t is_service : 1; /* this is a process that can only be started once!*/
  uint8_t needs_poll : 1;
  uint8_t has_stdin : 1;
  uint8_t has_stdout : 1;
  uint8_t reserved_flag : 1;

  struct pt pt;
  process_thread_t thread;

#if PROCESS_CONF_PIPELINES
  ipc_pipe_t *stdin;
  ipc_pipe_t *stdout;
  size_t written;
#endif

#if PROCESS_CONF_EVENT_INBOX
  struct {
    process_event_t ev;
    process_data_t data;
  } inbox[PROCESS_CONF_INBOX_SIZE];
  uint8_t inbox_head;
  uint8_t inbox_tail;
#endif

}; /* struct process */

/* TODO: how do we deal with these extensions PROCESS_CONF_PIPELINES AND
 * PROCESS_CONF_EVENT_INBOX? */
#ifndef PROCESS_CONF_NO_PROCESS_NAMES
#if PROCESS_CONF_PIPELINES
#if PROCESS_CONF_EVENT_INBOX
#define PROCESS_INSTANCE(name, priority)                                       \
  struct process name = {                                                      \
      NULL, /* next */                                                         \
      process_name_##name,                                                     \
      priority,                                                                \
      PROCESS_STATE_NONE,                                                      \
      0,                                                                       \
      0,                                                                       \
      0,                                                                       \
      0,                                                                       \
      {0}, /* struct pt */                                                     \
      process_thread_##name,                                                   \
      NULL, /* stdin */                                                        \
      NULL, /* stdout */                                                       \
      0,    /* written */                                                      \
      {0},  /* struct inbox[] */                                               \
      0,    /* inbox_head */                                                   \
      0     /* inbox_tail */                                                   \
  }
#else /* no event inbox */
#define PROCESS_INSTANCE(name, priority)                                       \
  struct process name = {                                                      \
      NULL, /* next */                                                         \
      process_name_##name,                                                     \
      priority,                                                                \
      PROCESS_STATE_NONE,                                                      \
      0,                                                                       \
      0,                                                                       \
      0,                                                                       \
      0,                                                                       \
      {0}, /* struct pt */                                                     \
      process_thread_##name,                                                   \
      NULL, /* stdin */                                                        \
      NULL, /* stdout */                                                       \
      0     /* written */                                                      \
  }
#endif
#else /* no pipelines */
#define PROCESS_INSTANCE(name, priority)                                       \
  struct process name = {NULL, /* next */                                      \
                         process_name_##name,                                  \
                         priority,                                             \
                         PROCESS_STATE_NONE,                                   \
                         0,                                                    \
                         0,                                                    \
                         0,                                                    \
                         0,                                                    \
                         {0}, /* struct pt */                                  \
                         process_thread_##name}

#endif
#else /* no process names */
#define PROCESS_INSTANCE(name, priority)                                       \
  struct process name = {NULL, /* next */                                      \
                         priority, PROCESS_STATE_NONE, 0, 0, 0, 0,             \
                         {0}, /* struct pt */                                  \
                         process_thread_##name
}
#endif

/* Proc thread / declaration macros */
#define PROCESS_THREAD_EX(name, pttype, ev, data)                              \
  static ptstate_t process_thread_##name(                                      \
      pttype *pt_process, process_event_t ev, process_data_t data)

#define PROCESS_THREAD(name, ev, data)                                         \
  PROCESS_THREAD_EX(name, struct pt, ev, data)

#define PROCESS(name, caption, priority)                                       \
  static const char process_name_##name[] CC_PROGMEM = caption;                \
  PROCESS_THREAD(name, ev, data);                                              \
  PROCESS_INSTANCE(name, priority)

#define PROCESS_EXTERN(name) extern struct process name

#define PROCESS_CURRENT() process_current
CC_EXTERN struct process *process_current;

/* Protothread helpers */
#define PROCESS_BEGIN() PT_BEGIN(pt_process)
#define PROCESS_END() PT_END(pt_process)
#define PROCESS_EXIT() PT_EXIT(pt_process)
#define PROCESS_WAIT_EVENT() PT_YIELD(pt_process)
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

/* Game-loop scheduler: handle polls first if poll_requested, otherwise handle
 * exactly one event */
void process_run(void);

/* Callback type for process iteration */
typedef void (*process_iterate_cb_t)(struct process *p);

/* Lookup active process by name with length limit. */
struct process *process_lookup_n(const char *name_segment, size_t len);

/* Post event to global queue (atomic). Returns 1 on success, 0 if queue full.
 * p == NULL => broadcast.
 * Safe to call from ISR (uses CC_ATOMIC_RESTORE()).
 */
int process_post(struct process *p, process_event_t ev, process_data_t data);

/* Alias for ISR explicitness (same behavior as process_post) */
int process_post_from_isr(struct process *p, process_event_t ev,
                          process_data_t data);

/* Request a poll for a process (sets needs_poll and poll_requested) */
void process_poll(struct process *p);

/* Convenience: report error to configured logger (if any) */
void process_debug(struct process *src, process_event_t ev, uint8_t code);
void process_flow(struct process *src, process_event_t ev, uint8_t code);
void process_verbose(struct process *src, process_event_t ev, uint8_t code);
void process_info(struct process *src, process_event_t ev, uint8_t code);
void process_warn(struct process *src, process_event_t ev, uint8_t code);
void process_error(struct process *src, process_event_t ev, uint8_t code);
void process_fatal(struct process *src, process_event_t ev, uint8_t code);

/* End of header */
#endif /* __PROCESS_H__ */

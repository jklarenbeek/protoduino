// file:./src/sys/process.h

#if CC_NO_VA_ARGS
#error "C compiler must support __VA_ARGS__ macro"
#endif

/*
 * This file is part of the Protoduino operating framework.
 */

/**
 * \addtogroup sys
 * @{
 */

/**
 * \defgroup process Protoduino Process Kernel
 *
 * The Protoduino process kernel provides lightweight, event-driven
 * cooperative multitasking for tiny IoT devices. It supports static
 * and dynamic (ELF32-loaded) processes, priorities, polling, IPC via
 * events, and basic piping.
 *
 * @{
 */

/**
 * \file
 * Header file for the Protoduino process kernel.
 */

#ifndef PROCESS_H_
#define PROCESS_H_

#include "protoduino.h"
#include "sys/pt.h"
#include "sys/timer.h"
#include <avr/interrupt.h>  /* For cli/sei ISR safety */

/* Configuration Defaults */
#ifndef PROCESS_CONF_NUMEVENTS
#define PROCESS_CONF_NUMEVENTS 32  /* Must be power of 2 for mask efficiency */
#endif

/* Compile-time sanity check: EVENT queue size must be a power of two
 * so that the implementation may safely use bit-masking: index & (N-1).
 * If this triggers, pick the nearest power-of-two (e.g. 16, 32, 64).
 */
#if (PROCESS_CONF_NUMEVENTS & (PROCESS_CONF_NUMEVENTS - 1)) != 0
#error "PROCESS_CONF_NUMEVENTS must be a power of two"
#endif

#define EVENT_MASK (PROCESS_CONF_NUMEVENTS - 1)

#ifndef PROCESS_CONF_MAX_PROCESSES
#define PROCESS_CONF_MAX_PROCESSES 20
#endif

#ifndef PROCESS_CONF_PRIOS
#define PROCESS_CONF_PRIOS 4  /* 0=highest, 3=idle */
#endif

#ifndef PROCESS_CONF_STATS
#define PROCESS_CONF_STATS 0
#endif

#ifndef PROCESS_CONF_NO_PROCESS_NAMES
#define PROCESS_CONF_NO_PROCESS_NAMES 0
#endif

/* Types */
typedef uint8_t process_event_t;
typedef void * process_data_t;
typedef uint8_t process_prio_t;  /* 0=highest */
typedef uint8_t process_num_events_t;

#define PROCESS_EVENT_NONE          0x00  /* No event / sentinel */
#define PROCESS_EVENT_INIT          0x01  /* Sent once when the process starts */
#define PROCESS_EVENT_POLL          0x02  /* Internal poll request (needspoll flag) */
#define PROCESS_EVENT_EXIT          0x03  /* Instruct process to exit gracefully */
#define PROCESS_EVENT_CONTINUE      0x00  /* Resume after pause (process_continue) */
#define PROCESS_EVENT_MSG           0x05  /* Generic IPC message event */
#define PROCESS_EVENT_EXITED        0x06  /* Child or piped process has exited */
#define PROCESS_EVENT_TIMER         0x07  /* Timer fired for this process */
#define PROCESS_EVENT_LOADED        0x08  /* Dynamic module successfully loaded */
#define PROCESS_EVENT_UNLOADED      0x09  /* Dynamic module unloaded / memory released */
#define PROCESS_EVENT_ERROR         0x0A  /* A process or protothread reported an error */
#define PROCESS_EVENT_YIELDED       0x0B  /* Process yielded a value (iterator semantics) */
#define PROCESS_EVENT_PIPE_CONNECT  0x0C  /* A pipe between processes was established */
#define PROCESS_EVENT_PIPE_DATA     0x0D  /* Data sent through an established pipe */
#define PROCESS_EVENT_PIPE_CLOSE    0x0E  /* Pipe has been closed by either endpoint */
#define PROCESS_EVENT_PIPE_ERROR    0x0F  /* Pipe fault (cycle, broken link, invalid op) */
#define PROCESS_EVENT_COM           0x10  /* TODO: ? I am not sure if we need this */
#define PROCESS_EVENT_SERVICE_REMOVED 0x11 /* TODO: ? I am not sure if we need this  */
#define PROCESS_EVENT_MAX           0x7F  /* User events start after this value */

#define PROCESS_BROADCAST NULL
#define PROCESS_ZOMBIE ((struct process *)0x1)

/* Process States */
#define PROCESS_STATE_NONE     0 /* Process is initialized */
#define PROCESS_STATE_RUNNING  1 /* Process is running */
#define PROCESS_STATE_CALLED   2 /* Process is waiting */
#define PROCESS_STATE_PAUSED   3 /* Process is paused */
#define PROCESS_STATE_EXITING  0 /* Process is exiting */

/**
 * \typedef process_thread_t
 *
 * A process thread function is the "main" event handler for a Protoduino
 * process. Every process runs as a protothread (PT), so the handler must
 * return a ptstate_t.
 *
 * Parameters:
 *   - self : pointer to the process instance
 *   - ev   : the event being delivered to the process
 *   - data : optional data associated with the event
 *
 * Every PROCESS_THREAD() macro expands into a function of this type.
 */
typedef ptstate_t (*process_thread_t)(
    struct process *self,
    process_event_t ev,
    process_data_t data
);

/* Process Structure */
struct process {
  struct pt pt;                  /* Protothread internal state */

#if !PROCESS_CONF_NO_PROCESS_NAMES
  const char PROGMEM *name;      /* Process name stored in flash */
#endif

  process_prio_t prio;           /* Scheduling priority (0 = highest) */
  uint8_t state : 3;             /* Current runtime state */
  uint8_t needspoll : 1;         /* Flag indicating poll request */
  uint8_t reserved_flags : 4;    /* Reserved for future extensions */


  struct process *next;          /* Next process in the priority list */
  struct process *pipe_to;       /* Piped output destination */

  /**
   * Pointer to the process' protothread handler.
   *
   * This function is called whenever an event is dispatched to the
   * process. It implements cooperative multitasking via protothreads.
   */
  process_thread_t thread;
};

#if !PROCESS_CONF_NO_PROCESS_NAMES
#define PROCESS_NAME_STRING(p) ((p)->name ? (p)->name : "")
#else
#define PROCESS_NAME_STRING(p) ""
#endif

/* Event Queue Entry */
struct event_data {
  struct process *p;  /* Destination, NULL=broadcast */
  process_event_t ev;
  process_data_t data;
};

/* For error sink – sends both source process and code */
struct error_report {
  struct process *source;
  ptstate_t      code;
};

/**
 * Get a pointer to the currently running process.
 *
 * This macro get a pointer to the currently running
 * process. Typically, this macro is used to post an event to the
 * current process with process_post().
 *
 * \hideinitializer
 */
CC_EXTERN struct process *process_current;

/* Error sink: the kernel can forward uncaught process exceptions to a
 * single, optional error-handler process. Use process_set_error_sink()
 * to register an error sink (e.g. a logger process). If NULL, no sink.
 */
CC_EXTERN void process_set_error_sink(struct process *sink);

/* Public API */
CC_EXTERN void process_init(void);
CC_EXTERN int process_start(struct process *p, process_data_t data);
CC_EXTERN void process_exit(struct process *p);
CC_EXTERN int process_is_running(struct process *p);
CC_EXTERN int process_pause(struct process *p);
CC_EXTERN int process_continue(struct process *p);

CC_EXTERN int process_post(struct process *p, process_event_t ev, process_data_t data);

/**
 * \brief Synchronously post event and wait for processing
 *
 * NOTE: This function is declared but not implemented. Use process_call() instead
 * for synchronous invocation, or process_post() for async event delivery.
 */
// CC_EXTERN void process_post_synch(struct process *p, process_event_t ev, process_data_t data);
CC_EXTERN void process_call(struct process *p, process_event_t ev, process_data_t data);

CC_EXTERN void process_poll(struct process *p);
// CC_EXTERN void process_poll_isr(struct process *p);  /* ISR-safe version */


CC_EXTERN process_event_t process_alloc_event(void);

CC_EXTERN int process_run(void);  /* Run scheduler once, returns pending events */
CC_EXTERN int process_nevents(void);  /* Pending events + polls */

// 6. Add process iteration helpers declaration (already implemented but not declared)
CC_EXTERN void process_foreach(void (*callback)(struct process *));
CC_EXTERN uint8_t process_count_active(void);

CC_EXTERN struct process *process_find(const char *name);

/* Piping API */
CC_EXTERN int process_pipe_connect(struct process *src, struct process *dst);
CC_EXTERN int process_pipe_close(struct process *src);
CC_EXTERN int process_pipe_send(struct process *src, process_data_t data);  /* Convenience for PIPE_DATA */

/* Macros for Process Definition */
#define PROCESS_THREAD(name, ev, data) \
  ptstate_t process_thread_##name(struct process *self, process_event_t ev, process_data_t data)

/**
 * Define a process.
 *
 * \param name The variable name of the process structure.
 * \param strname The string representation of the process' name.
 * \param prio The priority (0=highest).
 */
#if !PROCESS_CONF_NO_PROCESS_NAMES
#define PROCESS(name, strname, prio) \
  static const char process_name_##name[] PROGMEM = strname; \
  PROCESS_THREAD(name, ev, data); \
  struct process name = { \
    NULL, \
    process_name_##name, \
    prio, \
    PROCESS_STATE_NONE, \
    0, 0, /* needspoll, reserved_flags */ \
    NULL, /* next */ \
    NULL, /* pipe_to */ \
    process_thread_##name \
  }
#else
  PROCESS_THREAD(name, ev, data); \
  struct process name = { \
    NULL, \
    prio, \
    PROCESS_STATE_NONE, \
    0, 0, /* needspoll, reserved_flags */ \
    NULL, /* next */ \
    NULL, /* pipe_to */ \
    process_thread_##name \
  }
#endif

#define PROCESS_NAME(name) extern struct process name

/* Hooks for Process-Specific Handlers */
#define PROCESS_POLLHANDLER(handler) if(ev == PROCESS_EVENT_POLL) { handler; }
#define PROCESS_ON_INIT(handler) if(ev == PROCESS_EVENT_INIT) { handler; }
#define PROCESS_ON_EXIT(handler) if(ev == PROCESS_EVENT_EXIT) { handler; }

/* Global Process List */
CC_EXTERN struct process *process_list;

#define PROCESS_LIST() process_list

/* Win32 Port Hooks (Define in platform-specific code) */
#ifdef PROCESS_WIN32_PORT
/* e.g., #define PROCESS_WIN32_PORT 1 in config */
CC_EXTERN void process_win32_init(void);  /* Hook for stdio simulation */
CC_EXTERN void process_win32_poll(void);  /* Simulate hardware polls */
#else
#define process_win32_init()
#define process_win32_poll()
#endif

/**
 * Wait for a specific event in a process thread
 */
#define PROCESS_WAIT_EVENT() \
  PROCESS_YIELD()

#define PROCESS_WAIT_EVENT_UNTIL(condition) \
  do { \
    PROCESS_YIELD(); \
  } while(!(condition))

/** @} */
/** @} */

/**
 * \brief Define array of processes to initialize
 *
 * Usage:
 * \code
 * PROCINIT(&timer_process, &serial_process, &network_process);
 * \endcode
 */
CC_EXTERN const struct process * const procinit[];

#define PROCINIT(...) \
  const struct process * const procinit[] = { __VA_ARGS__, NULL };

/**
 * \brief Initialize all processes defined with PROCINIT
 *
 * This should be called early in system initialization,
 * after process_init() but before the main loop.
 */
CC_EXTERN void procinit_init(void);

/**
 * \brief Define array of processes to start after initialize
 *
 * Usage:
 * \code
 * AUTOSTART_PROCESSES(&shell_process, &chat_process);
 * \endcode
 */
CC_EXTERN struct process * const autostart_processes[];
#define AUTOSTART_PROCESSES(...) \
  struct process * const autostart_processes[] = { __VA_ARGS__, NULL };

CC_EXTERN void autostart_start(void);
CC_EXTERN void autostart_exit(void);


#endif /* PROCESS_H_ */
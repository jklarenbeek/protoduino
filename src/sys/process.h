// file: ./src/sys/process.h
#ifndef __PROCESS_H__
#define __PROCESS_H__

/*
 * process.h  –  protoduino cooperative process scheduler
 * ========================================================
 *
 * QUICK-START (the 80% case)
 * --------------------------
 *
 *   // 1. Declare a process type with any per-instance state you need:
 *   PROCESS_DEFINE(blink, "blink", 2,
 *     uint8_t  led_pin;
 *     uint16_t interval_ms;
 *   );
 *
 *   // 2. Create one static instance:
 *   PROCESS_INSTANCE(blink, blink_proc);
 *
 *   // 3. Write the thread body:
 *   PROCESS_THREAD(blink, ev, data) {
 *     blink_t *self = PROCESS_SELF(blink);
 *     PROCESS_BEGIN();
 *     self->led_pin    = 13;
 *     self->interval_ms = 500;
 *     while (1) {
 *       PROCESS_WAIT_EVENT();
 *       // respond to ev / data …
 *     }
 *     PROCESS_END();
 *   }
 *
 *   // 4. Start it:
 *   process_start(&blink_proc.base, NULL);
 *
 *   // 5. Drive the scheduler from loop():
 *   void loop() { process_run(); }
 *
 *
 * SCALING DOWN (no user fields, no names, no pipelines)
 * ------------------------------------------------------
 *   Override in protoduino_config.h:
 *     #define PROCESS_CONF_NO_PROCESS_NAMES  1
 *     #define PROCESS_CONF_PIPELINES         0
 *     #define PROCESS_CONF_EVENT_INBOX       0
 *     #define PROCESS_CONF_EVENT_QUEUE_SIZE  4
 *
 *   For zero-overhead processes with no user state:
 *     PROCESS(heartbeat, "hb", 3);
 *     process_start(&heartbeat, NULL);    // heartbeat is struct process directly
 *
 *
 * SCALING UP (pools of processes)
 * --------------------------------
 *   PROCESS_POOL(blink, blink_pool, 4);
 *
 *   blink_t *b = blink_pool_alloc();
 *   if (b) process_new(&process_type_blink, b);
 *
 *
 * STRUCT LAYOUT RULES (read once, never forget)
 * ----------------------------------------------
 *   struct pt          is the protothread engine: { lc_t lc; }
 *   struct process     is the scheduler base:     { struct pt pt; ... }
 *   process_XYZ_t      is the concrete type:      { struct process base; ... }
 *
 *   All three are castable to each other via C99 §6.7.2.1 because the first
 *   member chain is always the same.  The scheduler holds struct process *
 *   exclusively.  Inside a thread body use PROCESS_SELF(name) to get back
 *   to your concrete type.
 *
 *
 * CONFIG KNOBS (override in protoduino_config.h, never here)
 * -----------------------------------------------------------
 *   PROCESS_CONF_EVENT_QUEUE_SIZE   ring-buffer capacity (default 4)
 *   PROCESS_CONF_ERROR_POOL_SIZE    rotating error slots  (default 4)
 *   PROCESS_CONF_MAX_ARGS           argv[] size           (default 4)
 *   PROCESS_CONF_EVENT_INBOX        per-process inbox     (default 0)
 *   PROCESS_CONF_INBOX_SIZE         inbox depth           (default 4)
 *   PROCESS_CONF_PIPELINES          IPC pipe support      (default 1)
 *   PROCESS_CONF_PIPELINES_MAX_STAGES                     (default 3)
 *   PROCESS_CONF_NO_PROCESS_NAMES   strip name strings    (default undef)
 *
 * See process_conf.h for defaults.
 */

#include <protoduino.h>
#if PROCESS_CONF_PIPELINES > 0
#include "pt/pipe.h"
#endif

/* ------------------------------------------------------------------ */
/* §1  Basic types                                                     */
/* ------------------------------------------------------------------ */

typedef uint8_t  process_event_t;
typedef void    *process_data_t;
typedef uint8_t  process_prio_t;
typedef uint8_t  process_num_events_t;

/** Variable-argument bundle passed via process_data_t when needed. */
typedef struct process_args {
  uint8_t argc;
  void   *argv[PROCESS_CONF_MAX_ARGS];
} process_args_t;

/** Runtime lifecycle state stored in every process struct (3-bit field). */
typedef enum psstate_t {
  PROCESS_STATE_NONE    = 0, /**< slot is free / not registered          */
  PROCESS_STATE_INIT    = 1, /**< started, INIT event not yet processed  */
  PROCESS_STATE_RUNNING = 2, /**< currently executing in call_process()  */
  PROCESS_STATE_CALLED  = 3, /**< suspended, waiting for next event/poll */
  PROCESS_STATE_EXITING = 4, /**< running PT_FINALLY / cleanup blocks    */
} psstate_t;

/** Process kind: multi-instance process or singleton device/driver. */
typedef enum pstype_t {
  PROCESS_TYPE_PROCESS = 0, /**< multiple instances allowed              */
  PROCESS_TYPE_DEVICE  = 1, /**< singleton: only one instance at a time  */
} pstype_t;

/* ------------------------------------------------------------------ */
/* §2  Standard events                                                 */
/* ------------------------------------------------------------------ */

#define PROCESS_EVENT_NONE  EVENT_NONE      /* 0x00 – no event / idle        */
#define PROCESS_EVENT_INIT  EVENT_INIT      /* 0x01 – sent once on start     */
#define PROCESS_EVENT_POLL  EVENT_POLL      /* 0x02 – poll request           */
#define PROCESS_EVENT_EXIT  EVENT_EXIT      /* 0x03 – process is exiting     */
#define PROCESS_EVENT_ERROR EVENT_ERROR     /* 0xFF – error condition        */
#define PROCESS_EVENT_MSG   EVENT_MSGQ_SEC  /* 0x96 – directed message       */
#define PROCESS_EVENT_PIPE  EVENT_PIPE_READY /* 0x44 – pipe data ready       */

/* ------------------------------------------------------------------ */
/* §3  Error / logging payload                                         */
/* ------------------------------------------------------------------ */

/**
 * Passed as process_data_t to the error-logger process.
 * Layout is stable – logger consumers depend on it.
 */
struct error_info {
  struct process *source;    /**< process that triggered the log entry    */
  uint8_t severity  : 3;    /**< 0=none 1=debug 2=flow 3=verbose
                                  4=info 5=warn 6=error 7=fatal           */
  uint8_t reserved  : 5;
  uint8_t event;             /**< event active when log entry was created */
  uint8_t error;             /**< raw ptstate_t error code                */
};

/* ------------------------------------------------------------------ */
/* §4  Process name helpers (PROGMEM-aware)                            */
/* ------------------------------------------------------------------ */

#ifndef PROCESS_CONF_NO_PROCESS_NAMES
/** Safe PROGMEM name accessor – returns PSTR literal for NULL or unnamed. */
#  define PROCESS_NAME_STRING(p) \
     ((p) == NULL ? PSTR("NULL") : ((p)->name ? (p)->name : PSTR("EMPTY")))
#else
#  define PROCESS_NAME_STRING(p) \
     ((p) == NULL ? PSTR("NULL") : PSTR("EMPTY"))
#endif

/* ------------------------------------------------------------------ */
/* §6  process_thread_t  –  the one true thread signature              */
/* ------------------------------------------------------------------ */

/**
 * Every process thread function has this exact signature.
 *
 * pt_process  –  struct process * for this instance.
 *                Cast to your concrete type via PROCESS_SELF(name).
 *                &pt_process->pt  is the protothread state for PT_BEGIN/END.
 * ev          –  the event that woke this invocation.
 * data        –  optional payload (may be NULL).
 */
typedef ptstate_t (*process_thread_t)(struct process  *pt_process,
                                      process_event_t  ev,
                                      process_data_t   data);

/* ------------------------------------------------------------------ */
/* §5  struct process  –  the scheduler base type                      */
/* ------------------------------------------------------------------ */

/**
 * struct process  –  base scheduler record.
 *
 * FIRST MEMBER IS struct pt.  This makes the following pointer casts
 * valid via C99 §6.7.2.1 common-initial-sequence:
 *
 *   (struct pt *)p       == &p->pt
 *   (struct process *)p  (when p is a concrete process_XYZ_t *)
 *
 * The scheduler only ever holds struct process *.
 * User data lives in the concrete struct produced by PROCESS_DEFINE.
 */
struct process {
  struct pt        pt;          /**< MUST be first – protothread state    */
  struct process  *next;        /**< intrusive list, sorted by prio       */

#ifndef PROCESS_CONF_NO_PROCESS_NAMES
  const char      *name;        /**< PROGMEM string                       */
#endif

  process_prio_t   prio;        /**< lower numeric = higher priority      */

  psstate_t        state     : 3;
  pstype_t         type      : 2; /**< PROCESS_TYPE_PROCESS / _DEVICE     */
  uint8_t          needs_poll: 1;
  uint8_t          has_pipein: 1;
  uint8_t          has_pipeout:1;

  process_thread_t thread;      /**< the protothread entry-point          */

#if PROCESS_CONF_PIPELINES > 0
  ipc_pipe_t *pipein;
  ipc_pipe_t *pipeout;
  size_t      written;          /**< bytes written so far (batch helper)  */
#endif

#if PROCESS_CONF_EVENT_INBOX
  struct {
    process_event_t ev;
    process_data_t  data;
  } inbox[PROCESS_CONF_INBOX_SIZE];
  uint8_t inbox_head;
  uint8_t inbox_tail;
#endif
};

/* ------------------------------------------------------------------ */
/* §7  process_type_t  –  compile-time type descriptor (PROGMEM)       */
/* ------------------------------------------------------------------ */

/**
 * One descriptor per named process type, stored in PROGMEM.
 * process_new() reads it to zero-init and wire up a concrete instance.
 * The thread pointer doubles as a type tag for singleton enforcement.
 */
typedef struct {
  const char       *name;   /**< PROGMEM – human-readable type name      */
  process_thread_t  thread; /**< entry point; also used as type identity  */
  uint8_t           size;   /**< sizeof(concrete struct), max 255 bytes   */
  uint8_t           prio;   /**< default scheduling priority              */
  uint8_t           type;   /**< 0 = process, 1 = device (singleton)      */
} process_type_t;

/* ------------------------------------------------------------------ */
/* §8  PROCESS_DEFINE / DEVICE_DEFINE                                  */
/* ------------------------------------------------------------------ */

/*
 * PROCESS_DEFINE(name, caption, priority, /* field declarations * /)
 *
 * Emits:
 *   1. PROGMEM name string
 *   2. Forward declaration of the thread function
 *   3. Concrete struct  process_##name##_t  { struct process base; ...fields }
 *   4. PROGMEM type descriptor  process_type_##name
 *
 * __VA_ARGS__ is a verbatim C struct body fragment – one or more field
 * declarations, exactly as they would appear inside struct { ... }.
 *
 * Example:
 *   PROCESS_DEFINE(shell, "shell", 2,
 *     uint8_t input_buf[32];
 *     uint8_t input_idx;
 *     struct pt child_vt;
 *   );
 */
#ifndef PROCESS_CONF_NO_PROCESS_NAMES

#define PROCESS_DEFINE(name, caption, priority, ...)                           \
  static const char process_name_##name[] CC_PROGMEM = caption;               \
  static ptstate_t  process_thread_##name(struct process *pt_process,         \
                                           process_event_t ev,                 \
                                           process_data_t  data);              \
  typedef struct {                                                             \
    struct process base; /* MUST be first – §6.7.2.1 */                       \
    __VA_ARGS__                                                                \
  } process_##name##_t;                                                        \
  static const process_type_t process_type_##name CC_PROGMEM = {              \
    /*  */ process_name_##name,                                             \
     process_thread_##name,                                           \
     sizeof(process_##name##_t),                                      \
     (priority),                                                      \
     PROCESS_TYPE_PROCESS                                             \
  }

/** Singleton variant – only one instance may run at a time. */
#define DEVICE_DEFINE(name, caption, priority, ...)                            \
  static const char process_name_##name[] CC_PROGMEM = caption;               \
  static ptstate_t  process_thread_##name(struct process *pt_process,         \
                                           process_event_t ev,                 \
                                           process_data_t  data);              \
  typedef struct {                                                             \
    struct process base;                                                       \
    __VA_ARGS__                                                                \
  } process_##name##_t;                                                        \
  static const process_type_t process_type_##name CC_PROGMEM = {              \
    /*  */ process_name_##name,                                             \
     process_thread_##name,                                           \
     sizeof(process_##name##_t),                                      \
     (priority),                                                      \
     PROCESS_TYPE_DEVICE                                              \
  }

#else /* PROCESS_CONF_NO_PROCESS_NAMES */

#define PROCESS_DEFINE(name, caption, priority, ...)                           \
  static ptstate_t process_thread_##name(struct process *pt_process,          \
                                          process_event_t ev,                  \
                                          process_data_t  data);               \
  typedef struct {                                                             \
    struct process base;                                                       \
    __VA_ARGS__                                                                \
  } process_##name##_t;                                                        \
  static const process_type_t process_type_##name CC_PROGMEM = {              \
    /*  */ NULL,                                                            \
     process_thread_##name,                                           \
     sizeof(process_##name##_t),                                      \
     (priority),                                                      \
     PROCESS_TYPE_PROCESS                                             \
  }

#define DEVICE_DEFINE(name, caption, priority, ...)                            \
  static ptstate_t process_thread_##name(struct process *pt_process,          \
                                          process_event_t ev,                  \
                                          process_data_t  data);               \
  typedef struct {                                                             \
    struct process base;                                                       \
    __VA_ARGS__                                                                \
  } process_##name##_t;                                                        \
  static const process_type_t process_type_##name CC_PROGMEM = {              \
    /*  */ NULL,                                                            \
     process_thread_##name,                                           \
     sizeof(process_##name##_t),                                      \
     (priority),                                                      \
     PROCESS_TYPE_DEVICE                                              \
  }

#endif /* PROCESS_CONF_NO_PROCESS_NAMES */

/* ------------------------------------------------------------------ */
/* §9  PROCESS_INSTANCE / PROCESS_POOL                                 */
/* ------------------------------------------------------------------ */

/**
 * PROCESS_INSTANCE(type_name, var_name)
 *
 * Declares one static zero-initialised concrete instance.
 * Zero-init is valid full initialisation: lc, child pt members, user
 * fields all start at 0.  process_start() / process_new() set the
 * scheduler metadata (name, thread, prio, type) before inserting into
 * the list; user fields are the caller's responsibility.
 *
 * Example:
 *   PROCESS_INSTANCE(shell, shell_proc);
 *   // later: process_start(&shell_proc.base, NULL);
 */
#define PROCESS_INSTANCE(type_name, var_name) \
  static process_##type_name##_t var_name

/**
 * PROCESS_POOL(type_name, pool_name, N)
 *
 * Declares a static array of N instances plus an inline allocator
 * pool_name##_alloc() that returns the first slot in PROCESS_STATE_NONE.
 * Freeing is implicit: when a process exits the scheduler resets its
 * state to PROCESS_STATE_NONE, making the slot available again.
 *
 * Example:
 *   PROCESS_POOL(shell, shell_pool, 4);
 *   shell_t *s = shell_pool_alloc();
 *   if (s) process_new(&process_type_shell, s);
 */
#define PROCESS_POOL(type_name, pool_name, N)                                  \
  static process_##type_name##_t pool_name[(N)];                               \
  static process_##type_name##_t *pool_name##_alloc(void) {                    \
    for (uint8_t _i = 0; _i < (N); _i++) {                                    \
      if (pool_name[_i].base.state == PROCESS_STATE_NONE)                      \
        return &pool_name[_i];                                                  \
    }                                                                          \
    return NULL;                                                               \
  }

/* ------------------------------------------------------------------ */
/* §10  PROCESS / DEVICE  –  zero-field compatibility shorthands       */
/* ------------------------------------------------------------------ */

/**
 * PROCESS(name, caption, priority)
 *
 * One-liner for processes that carry no user fields and just need a
 * single static instance.  100% compatible with existing code that
 * writes  PROCESS(foo, "foo", 2)  and then  process_start(&foo, NULL).
 *
 * &foo  is process_foo_t*, which passes as struct process* because
 * base is the first member.  process_start() accepts it directly.
 */
#define PROCESS(name, caption, priority)                                       \
  PROCESS_DEFINE(name, caption, priority, /* no user fields */);               \
  PROCESS_INSTANCE(name, name)

/** Singleton zero-field shorthand, analogous to PROCESS. */
#define DEVICE(name, caption, priority)                                        \
  DEVICE_DEFINE(name, caption, priority, /* no user fields */);                \
  PROCESS_INSTANCE(name, name)

/** External reference to a process defined in another translation unit. */
#define PROCESS_EXTERN(name) CC_EXTERN struct process name

/* ------------------------------------------------------------------ */
/* §11  Thread definition macros                                        */
/* ------------------------------------------------------------------ */

/**
 * PROCESS_THREAD(name, ev, data)
 *
 * Opens the thread function body.  pt_process is struct process *;
 * cast it to your concrete type with PROCESS_SELF(name) at the top.
 *
 * Usage:
 *   PROCESS_THREAD(blink, ev, data) {
 *     blink_t *self = PROCESS_SELF(blink);
 *     PROCESS_BEGIN();
 *     …
 *     PROCESS_END();
 *   }
 */
#define PROCESS_THREAD(name, ev, data)                                         \
  static ptstate_t process_thread_##name(                                      \
      struct process *pt_process, process_event_t ev, process_data_t data)

/**
 * PROCESS_THREAD_EX(name, pttype, ev, data)
 *
 * Escape hatch: lets you override the first-argument type.  Use only
 * when you have a custom pt-derived struct as the process base.
 */
#define PROCESS_THREAD_EX(name, pttype, ev, data)                              \
  static ptstate_t process_thread_##name(                                      \
      pttype *pt_process, process_event_t ev, process_data_t data)

/**
 * PROCESS_SELF(name)
 *
 * Casts pt_process back to the concrete struct pointer.
 * Safe because base is always the first member (§6.7.2.1).
 *
 *   blink_t *self = PROCESS_SELF(blink);
 */
#define PROCESS_SELF(name) ((process_##name##_t *)pt_process)

/* ------------------------------------------------------------------ */
/* §12  Protothread control macros  (reference pt_process->pt)         */
/* ------------------------------------------------------------------ */

/*
 * All macros reference pt_process->pt explicitly.  This makes the
 * expansion correct whether pt_process is struct process * or a
 * concrete process_XYZ_t * (both have the same first-member chain).
 */

#define PROCESS_BEGIN()                  PT_BEGIN(&pt_process->pt)
#define PROCESS_END()                    PT_END(&pt_process->pt)
#define PROCESS_EXIT()                   PT_EXIT(&pt_process->pt)
#define PROCESS_WAIT_EVENT()             PT_YIELD(&pt_process->pt)
#define PROCESS_WAIT_EVENT_UNTIL(c)      PT_YIELD_UNTIL(&pt_process->pt, (c))
#define PROCESS_SPAWN(child, thread)     PT_SPAWN(&pt_process->pt, (child), (thread))

/* ------------------------------------------------------------------ */
/* §13  process_current  –  scheduler global (read-only in user code)  */
/* ------------------------------------------------------------------ */

/**
 * Set by the scheduler to the process being dispatched.
 * Cleared to NULL after every call_process() returns.
 * Public read is fine; do not write from user code.
 * Use PROCESS_CURRENT() for the safe accessor form.
 */
CC_EXTERN struct process *process_current;
#define PROCESS_CURRENT() (process_current)

/* ------------------------------------------------------------------ */
/* §14  Pipeline macros  (reference pt_process, not process_current)   */
/* ------------------------------------------------------------------ */

#if PROCESS_CONF_PIPELINES > 0
/*
 * These macros reference pt_process directly, eliminating the global
 * dereference that was in the previous version.  They are safe to use
 * at any point inside a PROCESS_THREAD body.
 */

#define PROCESS_SET_PIPEIN(pipe)  \
  (pt_process->pipein = (pipe), pt_process->has_pipein = 1)

#define PROCESS_SET_PIPEOUT(pipe) \
  (pt_process->pipeout = (pipe), pt_process->has_pipeout = 1)

#define PROCESS_PIPEIN()          (pt_process->pipein)
#define PROCESS_PIPEOUT()         (pt_process->pipeout)

#define PROCESS_PIPE_AVAILABLE() \
  ((pt_process->has_pipein)  && (ipc_pipe_available(PROCESS_PIPEIN())  > 0))

#define PROCESS_PIPE_SPACE() \
  ((pt_process->has_pipeout) && (ipc_pipe_space(PROCESS_PIPEOUT()) > 0))

#define PROCESS_PIPE_READ(buf, max_len, read_len_ptr) \
  PT_PIPE_READ_EX(&pt_process->pt, PROCESS_PIPEIN(), \
                  (buf), (max_len), (read_len_ptr))

#define PROCESS_PIPE_WRITE_ATOMIC(buf, len) \
  PT_PIPE_WRITE_ATOMIC_EX(&pt_process->pt, PROCESS_PIPEOUT(), (buf), (len))

#define PROCESS_PIPE_WRITE(buf, len) \
  PT_PIPE_WRITE_EX(&pt_process->pt, PROCESS_PIPEOUT(), \
                   (buf), (len), pt_process->written)

#define PROCESS_PIPE_WRITE_BATCH(buf, len) \
  PT_PIPE_WRITE_BATCH_EX(&pt_process->pt, PROCESS_PIPEOUT(), \
                          (buf), (len), pt_process->written)

/** Wake callback suitable for ipc_pipe_init() – polls the process. */
CC_EXTERN void process_ipc_wake(void *ctx);

#endif /* PROCESS_CONF_PIPELINES > 0 */

/* ------------------------------------------------------------------ */
/* §15  Scheduler public API                                           */
/* ------------------------------------------------------------------ */

/**
 * process_init(error_logger)
 *
 * Must be called once before any other process API.
 * error_logger may be NULL; if set, all process_warn/error/fatal calls
 * post to it via PROCESS_EVENT_ERROR with an error_info * payload.
 */
CC_EXTERN void process_init(struct process *error_logger);

/**
 * process_start(p)
 *
 * Register a pre-allocated static instance and send PROCESS_EVENT_INIT.
 * p must be PROCESS_STATE_NONE.  For PROCESS_TYPE_DEVICE, rejects if
 * another instance with the same thread pointer is already running.
 *
 * Caller is responsible for pre-initialising any user fields BEFORE
 * calling process_start; the function only resets the protothread lc.
 */
CC_EXTERN void process_start(struct process *p, struct process_args *args);

/**
 * process_new(type, storage)
 *
 * Zero-initialise storage (memset), copy scheduler metadata from the
 * PROGMEM type descriptor, then call process_start().
 *
 * Returns the struct process * on success, NULL if type or storage is
 * NULL or if process_start() rejected the instance (p->state will be
 * PROCESS_STATE_NONE in that case).
 *
 * Typical usage with a pool:
 *   shell_t *s = shell_pool_alloc();
 *   if (s) process_new(&process_type_shell, s);
 */
CC_EXTERN struct process *process_new(const process_type_t *type, void *storage);

/**
 * process_destroy(p)
 *
 * Request orderly shutdown: sets state to EXITING and arms PT_FINAL so
 * the next scheduler invocation runs any PT_FINALLY / cleanup blocks.
 *
 * PROCESS_TYPE_DEVICE instances cannot be stopped at runtime; the call
 * is silently ignored for them (devices run until system reset).
 *
 * Replaces the old process_exit().
 */
CC_EXTERN void process_destroy(struct process *p);

/**
 * process_run()
 *
 * One iteration of the cooperative scheduler.  Call from loop().
 *
 * Execution order per call:
 *   1. If poll_requested: service all needs_poll flags (do_poll).
 *   2. Otherwise: dequeue and dispatch exactly one event (do_event).
 *
 * Keeps the worst-case latency bounded at O(N_processes) per call.
 */
CC_EXTERN void process_run(void);

/**
 * process_poll(p)
 *
 * Request a PROCESS_EVENT_POLL delivery for p on the next process_run()
 * call.  Safe to call from ISR.
 */
CC_EXTERN void process_poll(struct process *p);

/**
 * process_post(p, ev, data)
 *
 * Enqueue an event atomically.  p == NULL broadcasts to all processes.
 * Returns 1 on success, 0 if the queue is full.
 * Safe to call from ISR (uses CC_ATOMIC_RESTORE).
 *
 * If PROCESS_CONF_EVENT_INBOX is enabled and p != NULL, the event is
 * placed in p's private inbox first; if that is full it falls through
 * to the shared global queue.
 */
CC_EXTERN int process_post(struct process *p, process_event_t ev,
                            process_data_t data);

/**
 * process_post_from_isr(p, ev, data)
 *
 * Explicit ISR alias for process_post.  Same behaviour; the distinct
 * name makes call-sites self-documenting.
 */
CC_EXTERN int process_post_from_isr(struct process *p, process_event_t ev,
                                     process_data_t data);

/**
 * process_lookup_n(name_segment, len)
 *
 * Find the first running process whose PROGMEM name matches the
 * RAM string [name_segment, name_segment+len).  Returns NULL if not found.
 */
CC_EXTERN struct process *process_lookup_n(const char *name_segment, size_t len);

/* ------------------------------------------------------------------ */
/* §16  Structured logging API                                         */
/* ------------------------------------------------------------------ */
/*
 * Each function posts an error_info record to the registered logger.
 * If no logger was set in process_init(), all calls are no-ops.
 *
 * severity mapping:
 *   debug=1  flow=2  verbose=3  info=4  warn=5  error=6  fatal=7
 */
CC_EXTERN void process_debug  (struct process *src, process_event_t ev, uint8_t code);
CC_EXTERN void process_flow   (struct process *src, process_event_t ev, uint8_t code);
CC_EXTERN void process_verbose(struct process *src, process_event_t ev, uint8_t code);
CC_EXTERN void process_info   (struct process *src, process_event_t ev, uint8_t code);
CC_EXTERN void process_warn   (struct process *src, process_event_t ev, uint8_t code);
CC_EXTERN void process_error  (struct process *src, process_event_t ev, uint8_t code);
CC_EXTERN void process_fatal  (struct process *src, process_event_t ev, uint8_t code);

/* ------------------------------------------------------------------ */
/* §17  Compatibility aliases  (do not use in new code)                */
/* ------------------------------------------------------------------ */

/**
 * process_exit() is DEPRECATED.  Use process_destroy() in all new code.
 * Kept only so existing sketches compile without changes.
 */
#define process_exit(p) process_destroy(p)

/* ------------------------------------------------------------------ */
/* End of process.h                                                    */
/* ------------------------------------------------------------------ */
#endif /* __PROCESS_H__ */
// file:./src/sys/process.c

/*
 * This file is part of the Protoduino operating framework.
 */

/**
 * \file
 * Implementation of the Protoduino process kernel.
 */

#include "process.h"
#include <string.h> /* For memcmp in find */

/* Static Pools (No dynamic alloc for core) */
static struct process process_pool[PROCESS_CONF_MAX_PROCESSES];
static uint8_t process_pool_used[PROCESS_CONF_MAX_PROCESSES / 8 + 1]; /* Bitmask for used slots */

/* Deferred-free (tombstone) list to avoid freeing a process slot while
 * the event queue may still contain pointers to it. Slots are freed when
 * the event queue is empty (safe point).
 */
static struct process *deferred_free_list[PROCESS_CONF_MAX_PROCESSES];
static uint8_t deferred_free_count = 0;

static struct process *prio_lists[PROCESS_CONF_PRIOS]; /* Per-prio lists */
static struct process *process_error_sink = NULL;      /* Optional global sink for uncaught process errors */

static uint8_t poll_requested = 0;

/**
 * Get a pointer to the currently running process.
 *
 * This macro get a pointer to the currently running
 * process. Typically, this macro is used to post an event to the
 * current process with process_post().
 *
 * \hideinitializer
 */
#define PROCESS_CURRENT() process_current
 struct process *process_current;

/* Events queue */
static struct event_data event_queue[PROCESS_CONF_NUMEVENTS];
static uint8_t event_head = 0;
static uint8_t event_tail = 0;
static uint8_t event_pending = 0;

#if PROCESS_CONF_STATS
static process_num_events_t process_maxevents;
#endif

static process_event_t next_event = PROCESS_EVENT_MAX;

static uint8_t next_id = 1; /* Start from 1 */
static uint16_t next_module_id = 1;

static struct process *error_logger_sink = NULL;


/* Forward Declarations */
static void call_process(struct process *p, process_event_t ev, process_data_t data);
static void do_poll(void);
static void do_event(void);
static void exit_process(struct process *p);
static int check_pipe_cycle(struct process *start, struct process *dst); /* Detect loops */

/* Helpers */
static struct process *alloc_process(void)
{
  for (uint8_t i = 0; i < PROCESS_CONF_MAX_PROCESSES; i++)
  {
    if (!(process_pool_used[i / 8] & (1 << (i % 8))))
    {
      process_pool_used[i / 8] |= (1 << (i % 8));
      struct process *p = &process_pool[i];
      /* Clear reused slot to avoid stale pointers/flags.
       * Caller will set thread/prio/id as necessary.
       */
      memset(p, 0, sizeof(*p));
      p->state = PROCESS_STATE_NONE;
      p->prio = PROCESS_CONF_PRIOS - 1; /* Default to lowest priority */
      p->next = NULL;
      p->pipe_to = NULL;
      p->thread = NULL;
      p->is_dynamic = 0;
      return p;
    }
  }
  return NULL;
}

static void free_process(struct process *p)
{
  if (p >= process_pool && p < &process_pool[PROCESS_CONF_MAX_PROCESSES])
  {
    uint8_t idx = p - process_pool;
    process_pool_used[idx / 8] &= ~(1 << (idx % 8));
  }
}

static process_prio_t normalize_prio(process_prio_t prio)
{
  return prio < PROCESS_CONF_PRIOS ? prio : PROCESS_CONF_PRIOS - 1;
}

/* Initialization */
void process_init(void)
{
  memset(process_pool_used, 0, sizeof(process_pool_used));
  memset(prio_lists, 0, sizeof(prio_lists));
  memset(event_queue, 0, sizeof(event_queue));
  event_head = event_tail = event_pending = 0;
  next_event = PROCESS_EVENT_MAX;
  next_id = 1;
  next_module_id = 1;
  deferred_free_count = 0;
  process_error_sink = NULL;
}

/* Alloc ID */
process_id_t process_alloc_id(void)
{
  return next_id++;
}

/* Alloc Module ID */
uint16_t process_alloc_module_id(void)
{
  return next_module_id++;
}

/* Internal: Exit process
 *
 * We defer actual freeing of the process slot to avoid dangling pointers
 * in the event queue. process_unload() may post events referencing 'p'
 * (e.g., PROCESS_EVENT_UNLOADED). Freeing immediately would make those
 * queued references invalid. The slot will be freed later at a safe point.
 */
static void exit_process(struct process *p)
{
  if (!p)
    return;

  /* Mark as exiting and defer actual freeing to avoid dangling pointers
   * in the event queue. process_unload() may post events referencing 'p'
   * (e.g., PROCESS_EVENT_UNLOADED). Freeing immediately would make those
   * queued references invalid. We therefore move the slot to a deferred-free
   * list and actually free it when the event queue is empty.
   */
  p->state = PROCESS_STATE_EXITING;
  if (p->is_dynamic)
    process_unload(p); /* If dynamic, unload (may post UNLOADED) */

  /* Add to deferred-free list */
  if (deferred_free_count < PROCESS_CONF_MAX_PROCESSES)
  {
    deferred_free_list[deferred_free_count++] = p;
  }
  else
  {
    /* Should not happen: no space in deferred list — fallback to immediate free
     * to avoid memory leak, but this risks dangling pointers. */
    free_process(p);
  }
}

/* Pause/Continue */
int process_pause(struct process *p)
{
  if (!p || p->state != PROCESS_STATE_RUNNING)
    return ERR_PT_INVALID_STATE;
  p->state = PROCESS_STATE_PAUSED;
  return ERR_SUCCESS;
}

int process_continue(struct process *p)
{
  if (!p || p->state == PROCESS_STATE_PAUSED)
    return ERR_PT_INVALID_STATE;
  p->state = PROCESS_STATE_RUNNING;
  process_post(p, PROCESS_EVENT_CONTINUE, NULL);
  return ERR_SUCCESS;
}

/* Running Check */
int process_is_running(struct process *p)
{
  return p && (p->state == PROCESS_STATE_RUNNING || p->state == PROCESS_STATE_CALLED);
}

/* Post Event (Async) */
int process_post(struct process *p, process_event_t ev, process_data_t data)
{
  uint8_t sreg = SREG;
  cli(); /* ISR safe */
  uint8_t next = (event_head + 1) & EVENT_MASK;
  if (next == event_tail)
  {
    SREG = sreg;
    return ERR_PROTO_QUEUE_FULL;
  }

  event_queue[event_head].p = p;
  event_queue[event_head].ev = ev;
  event_queue[event_head].data = data;
  event_head = next;
  event_pending++;
#if PROCESS_CONF_STATS
  if (event_pending > process_maxevents)
    process_maxevents = event_pending;
#endif
  SREG = sreg;
  return ERR_SUCCESS;
}

/* Synchronous call (preserve caller context) */
void process_call(struct process *p, process_event_t ev, process_data_t data)
{
  struct process *caller = process_current;
  call_process(p, ev, data);
  process_current = caller;
}

/* Poll */
void process_poll(struct process *p)
{
  if (!p)
    return;
  if (!(p->state == PROCESS_STATE_RUNNING || p->state == PROCESS_STATE_CALLED))
    return;
  if (!(p->needspoll))
  {
    p->needspoll = 1;
    poll_requested = 1;
  }
}

/* Alloc User Event
 * Returns a new user event ID, or PROCESS_EVENT_NONE (0) on exhaustion.
 * User event IDs start after PROCESS_EVENT_MAX and must fit in a uint8_t.
 */
process_event_t process_alloc_event(void)
{
  if (next_event == 0xFF)
  {
    /* Out of user event IDs */
    return PROCESS_EVENT_NONE;
  }
  return next_event++;
}

/* Run Scheduler */
static void drain_deferred_free(void)
{
  if (event_pending == 0 && !poll_requested && deferred_free_count > 0)
  {
    for (uint8_t i = 0; i < deferred_free_count; i++)
    {
      struct process *p = deferred_free_list[i];
      /* Final sanity: only free if slot is in exiting state */
      if (p && p->state == PROCESS_STATE_EXITING)
      {
        p->state = PROCESS_STATE_NONE;
        free_process(p);
      }
    }
    deferred_free_count = 0;
  }
}

/* Run Scheduler */
int process_run(void)
{
  if (poll_requested)
    do_poll();
  do_event();
  process_win32_poll(); /* Platform hook */
  /* Attempt to free any deferred process slots now that we've processed events */
  drain_deferred_free();
  if (!event_pending && !poll_requested)
  {
    /* Idle: hook for sleep/power-save */
  }
  return event_pending + poll_requested;
}

/* Pending Events */
int process_nevents(void)
{
  return event_pending + poll_requested;
}

/* Internal: Process Polls */
static void do_poll(void)
{
  for (uint8_t prio = 0; prio < PROCESS_CONF_PRIOS; prio++)
  {
    struct process *p = prio_lists[prio];
    while (p)
    {
      struct process *next = p->next; /* Save next in case p is removed during call */
      if (p->needspoll)
      {
        p->state = PROCESS_STATE_CALLED;
        p->needspoll = 0;
        call_process(p, PROCESS_EVENT_POLL, NULL);
      }
      p = next;
    }
  }
  poll_requested = 0;
}

/* Internal: Process Events */
static void do_event(void)
{
  /* Process events until the queue is empty. We must not rely on the
   * mutable event_pending counter inside the loop (it is decremented when
   * events are popped). Looping until empty avoids starvation and ensures
   * newly posted events are handled in future iterations.
   */
  for (;;)
  {
    uint8_t sreg = SREG;
    cli();
    if (event_tail == event_head)
    {
      SREG = sreg;
      break;
    }

    struct event_data ev = event_queue[event_tail];
    event_tail = (event_tail + 1) & EVENT_MASK;
    event_pending--;
    SREG = sreg;

    if (ev.p == NULL)
    { /* Broadcast */
      for (uint8_t prio = 0; prio < PROCESS_CONF_PRIOS; prio++)
      {
        struct process *p = prio_lists[prio];
        while (p)
        {
          struct process *next = p->next; /* Save next before call_process may remove p */
          call_process(p, ev.ev, ev.data);
          p = next;
        }
      }
    }
    else
    {
      call_process(ev.p, ev.ev, ev.data);
    }
  }
}

/* Internal: Call Process Thread */
static void call_process(struct process *p, process_event_t ev, process_data_t data)
{
  if (!p || p->state == PROCESS_STATE_NONE)
    return;

  process_current = p;
  ptstate_t state = p->thread(p, ev, data);

  /* Interpret protothread state precisely (PTv2 semantics):
   *
   * - PT_WAITING: blocked; keep alive but marked "called" (not runnable until woken).
   * - PT_YIELDED: produced a value — forward to any pipe target and mark runnable.
   * - PT_EXITED / PT_ENDED / PT_FINALIZED: normal termination -> exit_process()
   * - state >= PT_ERROR: uncaught exception — post PROCESS_EVENT_ERROR, forward to
   *   optional global error sink, and terminate the process.
   */
  if (state == PT_YIELDED)
  {
    /* The protothread yielded a value. This is semantically different from
     * PT_WAITING — yields often represent value production (iterators).
     * Forward yielded data to pipe target if present, and leave process runnable.
     */
    if (p->pipe_to)
    {
      /* Post PIPE_DATA to the pipe target. The posted event will be processed
       * normally by the scheduler. */
      process_post(p->pipe_to, PROCESS_EVENT_PIPE_DATA, data);
    }
    p->state = PROCESS_STATE_RUNNING;
  }
  else if (state == PT_WAITING)
  {
    /* Blocked: no value produced, process is waiting for a condition or event. */
    p->state = PROCESS_STATE_CALLED;
  }
  else if (state == PT_EXITED || state == PT_ENDED || state == PT_FINALIZED)
  {
    /* Normal termination */
    exit_process(p);
  }
  else if (state >= PT_ERROR)
  {
    /* Uncaught exception — inform the process (so it has a chance to catch
     * in an event handler) and let a global sink know (if provided).
     *
     * We post PROCESS_EVENT_ERROR to the failing process so that, if it has
     * an error handler, it can run cleanup logic before being removed.
     */
    process_post(p, PROCESS_EVENT_ERROR, (process_data_t)(uintptr_t)state);
    if (process_error_sink)
    {
      /* Forward to global sink for logging/recording. The error sink receives
       * the same error code as data. */
      process_post(process_error_sink, PROCESS_EVENT_ERROR, (process_data_t)(uintptr_t)state);
    }
    /* After signaling error, mark for exit (deferred-free) */
    exit_process(p);
  }
  else
  {
    /* Catch-all: treat unknown states as waiting/blocked */
    p->state = PROCESS_STATE_CALLED;
  }

  process_current = NULL;
}

/* Find by Name */
struct process *process_find(const char *name)
{
#if PROCESS_CONF_NO_PROCESS_NAMES
  return NULL;
#else
  for (uint8_t prio = 0; prio < PROCESS_CONF_PRIOS; prio++)
  {
    struct process *p = prio_lists[prio];
    while (p)
    {
      if (strcmp_P(name, p->name) == 0)
        return p;
      p = p->next;
    }
  }
  return NULL;
#endif
}

/* Find by Module ID */
struct process *process_find_by_id(uint16_t module_id)
{
  for (uint8_t prio = 0; prio < PROCESS_CONF_PRIOS; prio++)
  {
    struct process *p = prio_lists[prio];
    while (p)
    {
      if (p->module_id == module_id)
        return p;
      p = p->next;
    }
  }
  return NULL;
}

/* Load ELF - stub (header documents expected behavior) */
struct process *process_load_elf(const uint8_t *elf_data, uint16_t elf_size, const char *name)
{
  /* TODO: Implement as per header comment */
  return NULL;
}

/* ELF Unload */
int process_unload(struct process *p)
{
  if (!p || !p->is_dynamic)
    return ERR_PT_INVALID_STATE;
  /* TODO: Implement as per header comment */
  process_post(p, PROCESS_EVENT_UNLOADED, NULL);
  p->is_dynamic = 0;
  p->module_id = 0;
  p->data_seg = NULL; /* Assume fixed mem released */
  return ERR_SUCCESS;
}

/*
 * Provides implementation functions for initializing system processes at boot.
 */

/**
 * \brief Start all processes in procinit array
 */
void procinit_init(void) {
  for (int i = 0; procinit[i] != NULL; ++i) {
    process_start((struct process *)procinit[i], NULL);
  }
}

void process_set_error_sink(struct process *sink) {
  error_logger_sink = sink;
}

/*
 * Implementation of module for automatically starting and exiting a list of processes.
 */

void autostart_start(void)
{
  const struct process *const *p = autostart_processes;
  while (*p)
  {
    process_start((struct process *)*p, NULL);
    p++;
  }
}

void autostart_exit(void)
{
  const struct process *const *p = autostart_processes;
  while (*p)
  {
    process_exit((struct process *)*p);
    p++;
  }
}

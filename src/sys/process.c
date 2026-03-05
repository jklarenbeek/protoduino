// file: ./src/sys/process.c
#include "process.h"
#include "ipc.h"

/* Internal event entry */
struct process_event_entry
{
  struct process *dest; /* NULL => broadcast */
  process_event_t ev;
  process_data_t data;
};

/* Global ring event queue */
static struct process_event_entry events[PROCESS_CONF_EVENT_QUEUE_SIZE];
static process_num_events_t event_head = 0;
static process_num_events_t event_tail = 0;

/* Process list (sorted by priority; lower numeric => higher priority) */
static struct process *process_list = NULL;

/* Poll request flag */
static volatile uint8_t poll_requested = 0;

/* Optional logger for errors */
static struct process *process_error_logger = NULL;

/* Rotating pool for error_info used when posting errors to logger */
static struct error_info error_pool[PROCESS_CONF_ERROR_POOL_SIZE];
static uint8_t error_pool_idx = 0;

/* ---------------- internal helpers ---------------- */

/* Non-atomic enqueue (caller must ensure atomic) */
static int enqueue_event_nolock(struct process *p, process_event_t ev, process_data_t data)
{
  process_num_events_t next = (event_head + 1) % PROCESS_CONF_EVENT_QUEUE_SIZE;
  if (next == event_tail) return 0; /* full */
  events[next].dest = p;
  events[next].ev = ev;
  events[next].data = data;
  event_head = next;
  return 1;
}

/* Non-atomic dequeue one event into out */
static int dequeue_event_nolock(struct process_event_entry *out)
{
  if (event_head == event_tail)
    return 0;
  *out = events[event_tail];
  event_tail = (event_tail + 1) % PROCESS_CONF_EVENT_QUEUE_SIZE;
  return 1;
}

/* Per-process inbox utilities (if enabled) */
#if PROCESS_CONF_EVENT_INBOX
static int process_inbox_push(struct process *p, process_event_t ev, process_data_t data)
{
  if (!p || p->state == PROCESS_STATE_NONE || p->state == PROCESS_STATE_EXITING)
    return 0;

  uint8_t next = (uint8_t)((p->inbox_head + 1) % PROCESS_CONF_INBOX_SIZE);
  if (next == p->inbox_tail) return 0; /* full */
  p->inbox[p->inbox_head].ev = ev;
  p->inbox[p->inbox_head].data = data;
  p->inbox_head = next;
  return 1;
}

static int process_inbox_pop(struct process *p, struct process_event_entry *out)
{
  if (!p || p->state == PROCESS_STATE_NONE || p->state == PROCESS_STATE_EXITING)
    return 0;
  if (p->inbox_head == p->inbox_tail)
    return 0;

  out->ev = p->inbox[p->inbox_tail].ev;
  out->data = p->inbox[p->inbox_tail].data;
  out->dest = p;
  p->inbox_tail = (uint8_t)((p->inbox_tail + 1) % PROCESS_CONF_INBOX_SIZE);
  return 1;
}
#endif /* PROCESS_CONF_EVENT_INBOX */

/* Call a process's protothread and handle protothread v2 lifecycle correctly */
static void call_process(struct process *p, process_event_t ev, process_data_t data)
{
  if (!p || p->state == PROCESS_STATE_NONE)
    return;

  uint8_t exiting = p->state == PROCESS_STATE_EXITING;

  p->state = PROCESS_STATE_RUNNING;
  process_current = p; // <--- SET GLOBAL
  ptstate_t ret = p->thread(&p->pt, ev, data);
  process_current = NULL; // <--- CLEAR GLOBAL

  p->state = !exiting ? PROCESS_STATE_CALLED : PROCESS_STATE_EXITING;

  /* If still running (WAITING or YIELDED) return */
  if (PT_ISRUNNING(ret)) return;

  /* If the thread is finalized we have to exit the process and return */
  if (ret == PT_FINALIZED)
  {
    exit_process(p);
    return;
  }

  /* If thread returned EXITED/ENDED or an ERROR => scheduler must run FINAL blocks */
  if (PT_ISEXITING(ret))
  {
    p->state = PROCESS_STATE_EXITING;

    /* Post an error event to logger if it's an error */
    if (PT_ISERROR(ret) && process_error_logger)
    {
      process_error(p, ret);
    }

    /* Check for message leak *before* the process is removed */
    if (ev == PROCESS_EVENT_MSG && process_error_logger)
    {
      process_log(5, src, PROCESS_EVENT_MSG, ERR_CLEAN_LEAKED);
    }

    /* Arm the final state for the protothread */
    PT_FINAL(&p->pt);
  }
}

/* ---------------- Poll & Event dispatch ---------------- */

/* Run all polls (when poll_requested). Returns 1 if any poll handled. */
static int do_poll(void)
{
  if (!poll_requested) return 0;

  int any = 0;
  for (struct process *pp = process_list; pp != NULL; pp = pp->next)
  {
    if (pp->needs_poll)
    {
      pp->needs_poll = 0;
      call_process(pp, PROCESS_EVENT_POLL, NULL);
      any++;
    }
  }

  return any;
}

/* Handle exactly one event. Returns 1 if event processed; 0 if none. */
static int do_event(void)
{
  struct process_event_entry e;

  /* First service per-process inbox if enabled (low-latency directed msgs) */
#if PROCESS_CONF_EVENT_INBOX
  for (struct process *pp = process_list; pp != NULL; pp = pp->next)
  {
    if (pp->inbox_head != pp->inbox_tail)
    {
      if (process_inbox_pop(pp, &e))
      {
        call_process(pp, e.ev, e.data);
        return 1;
      }
    }
  }
#endif

  /* Otherwise pop one entry from global queue atomically */
  CC_ATOMIC_RESTORE()
  {
    if (!dequeue_event_nolock(&e))
    {
      /* nothing */
      e.dest = NULL;
      e.ev = PROCESS_EVENT_NONE;
      e.data = NULL;
    }
  }

  if (e.ev == PROCESS_EVENT_NONE)
    return 0;

  if (e.dest == NULL)
  {
    int any = 0;

    /* Broadcast: call every registered process (no extra polls between calls) */
    for (struct process *pp = process_list; pp != NULL; pp = pp->next)
    {
      if (pp->state != PROCESS_STATE_NONE)
      {
        call_process(pp, e.ev, e.data);
        any++
      }
    }
    return any;
  }
  else
  {
    /* Directed: deliver to specific process if active */
    if (e.dest->state != PROCESS_STATE_NONE)
    {
      call_process(e.dest, e.ev, e.data);
      return 1;
    }
    return 0;
  }
}

/* ---------------- Public API ---------------- */

void process_init(struct process *error_logger)
{
  event_head = event_tail = 0;
  process_list = NULL;
  poll_requested = 0;
  process_error_logger = error_logger;
}

void process_start(struct process *p)
{
  if (!p || p->state != PROCESS_STATE_NONE)
    return;

  PT_INIT(&p->pt);
  p->state = PROCESS_STATE_INIT;
  p->needs_poll = 0;

#if PROCESS_CONF_EVENT_INBOX
  p->inbox_head = 0;
  p->inbox_tail = 0;
#endif

  /* insert by priority (lower numeric => higher priority) */
  struct process **q = &process_list;
  while (*q != NULL && (*q)->prio <= p->prio)
    q = &((*q)->next);
  p->next = *q;
  *q = p;

  /* send INIT */
  process_post(p, PROCESS_EVENT_INIT, NULL);
}

static void exit_process(struct process *p)
{
  if (!p || p->state == PROCESS_STATE_NONE)
    return;

  /* unlink from process_list */
  struct process **q = &process_list;
  while (*q)
  {
    if (*q == p)
    {
      *q = p->next;
      break;
    }
    q = &((*q)->next);
  }

  p->state = PROCESS_STATE_NONE;
  p->next = NULL;
}

void process_exit(struct process *p) {
  if (!p || p->state == PROCESS_STATE_NONE || p->state == PROCESS_STATE_EXITING)
    return;

  /* Set the process state as finalizing */
  p->state = PROCESS_STATE_EXITING;
  /* Arm the final state for the protothread */
  PT_FINAL(&p->pt);
}

void process_run(void)
{
  do_poll();
  do_event();
}

void process_poll(struct process *p)
{
  if (!p || p->state == PROCESS_STATE_NONE || p->state == PROCESS_STATE_EXITING)
    return;
  p->needs_poll = 1;
  poll_requested = 1;
}

/* Post event (atomic). If per-process inbox enabled and destination != NULL,
 * try to place in inbox first; otherwise fall back to global queue.
 */
int process_post(struct process *p, process_event_t ev, process_data_t data)
{
  int ok = 0;

  /* Guarantee atomicity across all queue operations */
  CC_ATOMIC_RESTORE()
  {
#if PROCESS_CONF_EVENT_INBOX
    if (p != NULL)
    {
      ok = process_inbox_push(p, ev, data);
      if (!ok)
      {
        ok = enqueue_event_nolock(p, ev, data);
      }
    }
    else
    {
      ok = enqueue_event_nolock(NULL, ev, data);
    }
#else
    ok = enqueue_event_nolock(p, ev, data);
#endif
  }
  return ok;
}

/* process_post_from_isr: identical behavior for now */
int process_post_from_isr(struct process *p, process_event_t ev, process_data_t data)
{
  return process_post(p, ev, data);
}

// Helper: Match a name against a segment of a string.
// Handles PROGMEM if on AVR, matching the storage of process names.
static int process_match_n(const char *segment, size_t len, const char *process_name)
{
  size_t name_len = strlen_P(process_name);
  if (len != name_len)
    return 0;
  return strncmp_P(segment, process_name, len) == 0;
}

// Lookup active process by name with length limit.
// Returns a pointer to the process or NULL if not found.
struct process *process_lookup_n(const char *name_segment, size_t len)
{
  struct process *p = process_list;
  while (p)
  {
    /* Use the macro to safely get the name (handles NULL/PROGMEM) */
    const char *pname = PROCESS_NAME_STRING(p);

    if (process_match_n(name_segment, len, pname))
    {
      return p;
    }
    p = p->next;
  }
  return NULL;
}

static void process_log(struct process *src, uint8_t severity, process_event_t ev, uint8_t err)
{
  if (!process_error_logger) return;
  uint8_t idx = (uint8_t)(error_pool_idx++ % PROCESS_CONF_ERROR_POOL_SIZE);
  error_pool[idx].source = src;
  error_pool[idx].severity = severity;
  error_pool[idx].code = err;
  process_post(process_error_logger, ev, &error_pool[idx]);
}

void process_debug(struct process *src, uint8_t code) {
  process_log(src, 1, PROCESS_EVENT_ERROR, code);
}

void process_flow(struct process *src, uint8_t code) {
  process_log(src, 2, PROCESS_EVENT_ERROR, code);
}

void process_verbose(struct process *src, uint8_t code) {
  process_log(src, 3, PROCESS_EVENT_ERROR, code);
}

void process_info(struct process *src, uint8_t code) {
  process_log(src, 4, PROCESS_EVENT_ERROR, code);
}

void process_warn(struct process *src, uint8_t code) {
  process_log(src, 5, PROCESS_EVENT_ERROR, code);
}

void process_error(struct process *src, uint8_t code) {
  process_log(src, 6, PROCESS_EVENT_ERROR, code);
}

void process_fatal(struct process *src, uint8_t code) {
  process_log(src, 7, PROCESS_EVENT_ERROR, code);
}

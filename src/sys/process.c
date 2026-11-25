// file: ./src/sys/process.c
/*
 * process.c  –  protoduino cooperative process scheduler implementation
 *
 * Matches process.h design spec v2.0.  All storage is static; no malloc.
 * Targets AVR (atmega328p / mega class: 2 KB SRAM) but compiles clean on
 * any C99 toolchain.
 *
 * Internal organisation
 * ─────────────────────
 *   §A  Private types & static storage
 *   §B  Ring-buffer queue helpers  (no-lock; callers use CC_ATOMIC_RESTORE)
 *   §C  Per-process inbox helpers  (PROCESS_CONF_EVENT_INBOX)
 *   §D  process_unlink             (remove from scheduler list)
 *   §E  call_process               (single-process dispatch + lifecycle)
 *   §F  do_poll / do_event         (scheduler tick helpers)
 *   §G  Public API
 *   §H  Structured logging
 *   §I  IPC wake callback
 */

#include "process.h"
#include "ipc.h"

/* ================================================================== */
/* §A  Private types & static storage                                  */
/* ================================================================== */

/** Internal event entry stored in the ring buffer. */
struct process_event_entry {
  struct process  *dest; /**< NULL ⟹ broadcast to all active processes */
  process_event_t  ev;
  process_data_t   data;
};

/* Global ring event queue ----------------------------------------- */
/*
 * Standard "write-at-head, read-from-tail" ring buffer.
 *   empty : head == tail
 *   full  : (head + 1) % N == tail
 *   capacity : N - 1 usable slots
 *
 * event_head is the NEXT slot to write (not the last written).
 * event_tail is the NEXT slot to read.
 */
#if PROCESS_CONF_CORES > 1
/* One scheduler per core: what follows, for each core, picked by the core
 * that runs the code. The names below stand for the fields of this core. */
struct process_core {
  struct process_event_entry events_[PROCESS_CONF_EVENT_QUEUE_SIZE];
  process_num_events_t       event_head_;
  process_num_events_t       event_tail_;
  struct process            *process_list_;
  volatile uint8_t           poll_requested_;
  struct process            *process_error_logger_;
  struct error_info          error_pool_[PROCESS_CONF_ERROR_POOL_SIZE];
  uint8_t                    error_pool_idx_;
};
static struct process_core process_cores[PROCESS_CONF_CORES];
struct process *process_current_core[PROCESS_CONF_CORES];
#define events               (process_cores[PROCESS_CORE()].events_)
#define event_head           (process_cores[PROCESS_CORE()].event_head_)
#define event_tail           (process_cores[PROCESS_CORE()].event_tail_)
#define process_list         (process_cores[PROCESS_CORE()].process_list_)
#define poll_requested       (process_cores[PROCESS_CORE()].poll_requested_)
#define process_error_logger (process_cores[PROCESS_CORE()].process_error_logger_)
#define error_pool           (process_cores[PROCESS_CORE()].error_pool_)
#define error_pool_idx       (process_cores[PROCESS_CORE()].error_pool_idx_)

/* From one core to another: a ring with one writer, the sending core, and
 * one reader, the receiving one, so that neither needs a lock. The sender
 * keeps interrupts off while it writes (an interrupt handler may send too);
 * the receiver empties it in process_run(). An entry with no event is a
 * poll. A poll that finds the ring full is counted in `lost`; the receiver
 * then polls all of its processes, so that no wake is lost. */
struct process_mail {
  struct process  *dest;
  process_event_t  ev;
  process_data_t   data;
};
static struct {
  struct process_mail slot[PROCESS_CONF_MAILBOX_SIZE];
  volatile uint8_t    head;      /* written by the sending core   */
  volatile uint8_t    tail;      /* written by the receiving core */
  volatile uint8_t    lost;      /* written by the sending core   */
  uint8_t             lost_seen; /* written by the receiving core */
} process_mailbox[PROCESS_CONF_CORES][PROCESS_CONF_CORES]; /* [from][to] */
#else
static struct process_event_entry events[PROCESS_CONF_EVENT_QUEUE_SIZE];
static process_num_events_t event_head = 0; /* next write position */
static process_num_events_t event_tail = 0; /* next read  position */

/* Process list – sorted by prio; lower numeric = higher priority ---- */
static struct process *process_list = NULL;
/* The process whose thread is running; NULL outside call_process(). */
struct process *process_current = NULL;

/* Poll request flag – set by process_poll(), cleared by do_poll() -- */
static volatile uint8_t poll_requested = 0;

/* Optional structured-log sink ------------------------------------ */
static struct process *process_error_logger = NULL;

/* Rotating pool of error_info records ----------------------------- */
static struct error_info error_pool[PROCESS_CONF_ERROR_POOL_SIZE];
static uint8_t           error_pool_idx = 0;
#endif

/* ================================================================== */
/* §B  Ring-buffer helpers  (non-atomic; caller holds atomic context)  */
/* ================================================================== */

/**
 * Enqueue one event.  Returns 1 on success, 0 if the ring is full.
 * Caller MUST execute inside CC_ATOMIC_RESTORE() to be ISR-safe.
 *
 * Writes to events[event_head] then advances event_head.
 * Full condition: (event_head + 1) % N == event_tail.
 */
static int enqueue_event_nolock(struct process    *dest,
                                 process_event_t   ev,
                                 process_data_t    data)
{
  process_num_events_t next =
      (process_num_events_t)((event_head + 1u) % PROCESS_CONF_EVENT_QUEUE_SIZE);
  if (next == event_tail)
    return 0; /* full */

  events[event_head].dest = dest;
  events[event_head].ev   = ev;
  events[event_head].data = data;
  event_head = next;
  return 1;
}

/**
 * Dequeue one event into *out.  Returns 1 on success, 0 if empty.
 * Caller MUST execute inside CC_ATOMIC_RESTORE() to be ISR-safe.
 *
 * Reads from events[event_tail] then advances event_tail.
 * Empty condition: event_head == event_tail.
 */
static int dequeue_event_nolock(struct process_event_entry *out)
{
  if (event_head == event_tail)
    return 0; /* empty */

  *out       = events[event_tail];
  event_tail = (process_num_events_t)
               ((event_tail + 1u) % PROCESS_CONF_EVENT_QUEUE_SIZE);
  return 1;
}

#if PROCESS_CONF_CORES > 1
/* With no event, the data of a mail says what to do to its process. */
#define PROCESS_MAIL_POLL    ((process_data_t)0)
#define PROCESS_MAIL_DESTROY ((process_data_t)1)
#define PROCESS_MAIL_PAUSE   ((process_data_t)2)
#define PROCESS_MAIL_RESUME  ((process_data_t)3)

/* Sends an event, or with ev NONE an operation, to process p of core `to`. */
static int process_mail(uint8_t to, struct process *p, process_event_t ev, process_data_t data)
{
  uint8_t from = PROCESS_CORE();
  int ok = 0;

  CC_ATOMIC_RESTORE() {
    uint8_t head = process_mailbox[from][to].head;
    uint8_t next = (uint8_t)((head + 1u) % PROCESS_CONF_MAILBOX_SIZE);
    if (next != process_mailbox[from][to].tail) {
      process_mailbox[from][to].slot[head].dest = p;
      process_mailbox[from][to].slot[head].ev   = ev;
      process_mailbox[from][to].slot[head].data = data;
      CC_MEMORY_BARRIER(); /* the entry before the index that shows it */
      process_mailbox[from][to].head = next;
      ok = 1;
    } else if (ev == PROCESS_EVENT_NONE && data == PROCESS_MAIL_POLL) {
      process_mailbox[from][to].lost++;
      ok = 1;
    }
  }
  PROCESS_CONF_WAKE(); /* the other core may wait in WFE */
  return ok;
}

static int process_post_here(struct process *p, process_event_t ev, process_data_t data);
static void process_poll_here(struct process *p);

/* What the other cores sent to this one, delivered here. An event waits in
 * the mailbox while the queue of this core is full. */
static void process_mail_receive(void)
{
  uint8_t to = PROCESS_CORE();

  for (uint8_t from = 0; from < PROCESS_CONF_CORES; from++) {
    if (from == to)
      continue;
    while (process_mailbox[from][to].tail != process_mailbox[from][to].head) {
      uint8_t tail = process_mailbox[from][to].tail;
      CC_MEMORY_BARRIER(); /* the index before the entry it shows */
      struct process_mail m = process_mailbox[from][to].slot[tail];
      if (m.ev != PROCESS_EVENT_NONE) {
        if (!process_post_here(m.dest, m.ev, m.data))
          break;
      } else if (m.data == PROCESS_MAIL_DESTROY) {
        process_destroy(m.dest);
      } else if (m.data == PROCESS_MAIL_PAUSE) {
        process_pause(m.dest);
      } else if (m.data == PROCESS_MAIL_RESUME) {
        process_resume(m.dest);
      } else {
        process_poll_here(m.dest);
      }
      CC_MEMORY_BARRIER(); /* read before the slot is given back */
      process_mailbox[from][to].tail = (uint8_t)((tail + 1u) % PROCESS_CONF_MAILBOX_SIZE);
    }
    if (process_mailbox[from][to].lost != process_mailbox[from][to].lost_seen) {
      process_mailbox[from][to].lost_seen = process_mailbox[from][to].lost;
      for (struct process *pp = process_list; pp != NULL; pp = pp->next)
        process_poll_here(pp);
    }
  }
}

/* something in a mailbox for this core */
static int process_mail_waiting(void)
{
  uint8_t to = PROCESS_CORE();
  for (uint8_t from = 0; from < PROCESS_CONF_CORES; from++)
    if (from != to && (process_mailbox[from][to].tail != process_mailbox[from][to].head ||
                       process_mailbox[from][to].lost != process_mailbox[from][to].lost_seen))
      return 1;
  return 0;
}
#endif /* PROCESS_CONF_CORES > 1 */

/* ================================================================== */
/* §C  Per-process inbox helpers (PROCESS_CONF_EVENT_INBOX)            */
/* ================================================================== */

#if PROCESS_CONF_EVENT_INBOX

/**
 * Push one event into p's private inbox.
 * Returns 1 on success, 0 if full or process is inactive.
 * Caller MUST hold atomic context.
 */
static int process_inbox_push(struct process  *p,
                               process_event_t  ev,
                               process_data_t   data)
{
  if (!p || p->state == PROCESS_STATE_NONE || p->state == PROCESS_STATE_EXITING)
    return 0;

  uint8_t next = (uint8_t)((p->inbox_head + 1u) % PROCESS_CONF_INBOX_SIZE);
  if (next == p->inbox_tail)
    return 0; /* inbox full */

  p->inbox[p->inbox_head].ev   = ev;
  p->inbox[p->inbox_head].data = data;
  p->inbox_head = next;
  return 1;
}

/**
 * Pop one event from p's private inbox into *out.
 * Returns 1 on success, 0 if empty or process is inactive.
 */
static int process_inbox_pop(struct process             *p,
                              struct process_event_entry *out)
{
  if (!p || p->state == PROCESS_STATE_NONE || p->state == PROCESS_STATE_EXITING)
    return 0;
  if (p->inbox_head == p->inbox_tail)
    return 0;

  out->ev   = p->inbox[p->inbox_tail].ev;
  out->data = p->inbox[p->inbox_tail].data;
  out->dest = p;
  p->inbox_tail = (uint8_t)((p->inbox_tail + 1u) % PROCESS_CONF_INBOX_SIZE);
  return 1;
}

#endif /* PROCESS_CONF_EVENT_INBOX */

/* ================================================================== */
/* §D  process_unlink  –  remove a process from the scheduler list     */
/* ================================================================== */

/**
 * Unlinks p from process_list, resets state to NONE, clears next.
 * Called by call_process() when the protothread returns PT_FINALIZED,
 * and by process_destroy() indirectly via PT_FINAL + next call.
 */
static void process_unlink(struct process *p)
{
  if (!p)
    return;

  if (p->state == PROCESS_STATE_NONE) {
    p->next = NULL;
    return;
  }

  struct process **q = &process_list;
  while (*q) {
    if (*q == p) {
      *q = p->next;
      break;
    }
    q = &(*q)->next;
  }

  p->state = PROCESS_STATE_NONE;
  p->next  = NULL;
}

/* ================================================================== */
/* §E  call_process  –  dispatch one event to one process              */
/* ================================================================== */

/**
 * Invoke p's protothread with (ev, data) and handle the full v2
 * lifecycle:
 *
 *   PT_ISRUNNING  – thread yielded/waiting; nothing to do.
 *   PT_FINALIZED  – thread completed its PT_FINALLY block; unlink.
 *   PT_ISEXITING  – thread returned PT_EXITED/PT_ENDED or an error;
 *                   arm PT_FINAL so the next call runs cleanup.
 *
 * The scheduler global process_current is set for the duration of the
 * call so that legacy macros and user code can read it.  It is always
 * cleared before returning.
 */
static void call_process(struct process  *p,
                          process_event_t  ev,
                          process_data_t   data)
{
  if (!p || p->state == PROCESS_STATE_NONE || p->state == PROCESS_STATE_PAUSED)
    return;

  uint8_t was_exiting = (p->state == PROCESS_STATE_EXITING);

  p->state        = PROCESS_STATE_RUNNING;
  process_current = p;

  /*
   * Call the thread.  pt_process == p (struct process *).
   * Inside the thread, &pt_process->pt is the protothread state.
   * §6.7.2.1: casting p to the concrete type is valid because
   * struct process is always the first member of the concrete struct.
   */
  ptstate_t ret = p->thread(p, ev, data);

  process_current = NULL;

  /* Restore state: if we were already exiting, stay in EXITING so
   * that PT_FINALLY blocks keep running across multiple scheduler ticks. */
  p->state = was_exiting ? PROCESS_STATE_EXITING : PROCESS_STATE_CALLED;

  /* Thread yielded or is waiting for the next event – nothing more to do. */
  if (PT_ISRUNNING(ret))
    return;

  /* Thread completed its PT_FINALLY block – unlink and free the slot. */
  if (ret == PT_FINALIZED) {
    process_unlink(p);
    return;
  }

  /* Thread exited (normal or error) – enter the cleanup / finalise phase. */
  if (PT_ISEXITING(ret)) {
    p->state = PROCESS_STATE_EXITING;

    /* Log the error before the process slot might be reused. */
    if (PT_ISERROR(ret) && process_error_logger)
      process_error(p, ev, (uint8_t)ret);

    /* A leaked PROCESS_EVENT_MSG on exit is always suspicious. */
    if (ev == PROCESS_EVENT_MSG && process_error_logger)
      process_warn(p, ev, ERR_CLEAN_LEAKED);

    /* Arm the finalise state: next invocation will run PT_FINALLY. */
    PT_FINAL(&p->pt);
  }
}

/* ================================================================== */
/* §F  do_poll / do_event  –  scheduler tick helpers                   */
/* ================================================================== */

/**
 * Service all pending poll requests.
 * Returns the number of polls dispatched (0 if poll_requested was clear).
 *
 * Polls are serviced in priority order (process_list is sorted).
 * poll_requested is cleared after the sweep so that an ISR can set it
 * again during the sweep without being lost.
 */
static int do_poll(void)
{
  if (!poll_requested)
    return 0;

  /* Clear before the sweep so a new poll set during dispatch isn't lost. */
  poll_requested = 0;

  int any = 0;
  for (struct process *pp = process_list; pp != NULL; pp = pp->next) {
    /* a paused process keeps its poll until it is resumed */
    if (pp->needs_poll && pp->state != PROCESS_STATE_PAUSED) {
      pp->needs_poll = 0;
      call_process(pp, PROCESS_EVENT_POLL, NULL);
      any++;
    }
  }
  return any;
}

/**
 * Dequeue and dispatch exactly one event.
 * Returns 1 if an event was processed, 0 if the queue was empty.
 *
 * Dispatch order:
 *   1. Per-process inbox (low-latency directed path, if enabled).
 *   2. Global ring-buffer (broadcast or directed).
 */
static int do_event(void)
{
  struct process_event_entry e;

  /* ── 1. Per-process inbox (if enabled) ────────────────────────── */
#if PROCESS_CONF_EVENT_INBOX
  for (struct process *pp = process_list; pp != NULL; pp = pp->next) {
    if (pp->inbox_head != pp->inbox_tail) {
      if (process_inbox_pop(pp, &e)) {
        call_process(pp, e.ev, e.data);
        return 1;
      }
    }
  }
#endif

  /* ── 2. Global ring-buffer ──────────────────────────────────────── */
  /* Dequeue atomically to remain ISR-safe. */
  CC_ATOMIC_RESTORE() {
    if (!dequeue_event_nolock(&e)) {
      e.dest = NULL;
      e.ev   = PROCESS_EVENT_NONE;
      e.data = NULL;
    }
  }

  if (e.ev == PROCESS_EVENT_NONE)
    return 0;

  if (e.dest == NULL) {
    /* Broadcast: deliver to every active process. */
    int any = 0;
    for (struct process *pp = process_list; pp != NULL; pp = pp->next) {
      if (pp->state != PROCESS_STATE_NONE) {
        call_process(pp, e.ev, e.data);
        any++;
      }
    }
    return any > 0 ? 1 : 0;
  }

  /* Directed: deliver only to the named process (if still active). */
  if (e.dest->state != PROCESS_STATE_NONE) {
    call_process(e.dest, e.ev, e.data);
    return 1;
  }
  return 0;
}

/* ================================================================== */
/* §G  Public API                                                      */
/* ================================================================== */

/* ── process_init ─────────────────────────────────────────────────── */

void process_init(struct process *error_logger)
{
  event_head          = 0;
  event_tail          = 0;
  process_list        = NULL;
  poll_requested      = 0;
  process_error_logger = error_logger;
}

/* ── process_start ────────────────────────────────────────────────── */

void process_start(struct process *p, struct process_args *args)
{
  if (!p || p->state != PROCESS_STATE_NONE)
    return;

  /* Singleton (device) enforcement: reject if another instance with the
   * same thread pointer is already in the list. */
  if (p->type == PROCESS_TYPE_DEVICE) {
    for (struct process *q = process_list; q != NULL; q = q->next) {
      if (q->thread == p->thread)
        return; /* already running */
    }
  }

  /* Reset only the protothread lc – user fields are the caller's concern. */
  PT_INIT(&p->pt);
  p->state      = PROCESS_STATE_INIT;
  p->needs_poll = 0;
#if PROCESS_CONF_CORES > 1
  p->core       = PROCESS_CORE();
#endif

#if PROCESS_CONF_EVENT_INBOX
  p->inbox_head = 0;
  p->inbox_tail = 0;
#endif

  /* Insert into the priority-sorted list (stable: equal prio keeps FIFO). */
  struct process **q = &process_list;
  while (*q != NULL && (*q)->prio <= p->prio)
    q = &(*q)->next;
  p->next = *q;
  *q      = p;

  /* Kick the process with its first event. */
  process_post(p, PROCESS_EVENT_INIT, args);
}

/* ── process_new ──────────────────────────────────────────────────── */

struct process *process_new(const process_type_t *type, void *storage)
{
  if (!type || !storage)
    return NULL;

  /* Full zero-init of the concrete struct.  Valid for all field types
   * (lc, child struct pt members, user data). */
  uint8_t sz = pgm_read_byte(&type->size);
  memset(storage, 0, sz);

  /* Wire scheduler metadata from the PROGMEM descriptor. */
  struct process *p = (struct process *)storage;

#ifndef PROCESS_CONF_NO_PROCESS_NAMES
  p->name   = (const char *)pgm_read_ptr(&type->name);
#endif
  p->thread = (process_thread_t)pgm_read_ptr(&type->thread);
  p->prio   = pgm_read_byte(&type->prio);
  p->type   = (pstype_t)pgm_read_byte(&type->type);
  p->state  = PROCESS_STATE_NONE; /* process_start checks this */

  process_start(p, NULL);

  /* If process_start() rejected the instance, return NULL to signal failure. */
  return (p->state != PROCESS_STATE_NONE) ? p : NULL;
}

/* ── process_destroy ──────────────────────────────────────────────── */

void process_destroy(struct process *p)
{
  if (!p
      || p->state == PROCESS_STATE_NONE
      || p->state == PROCESS_STATE_EXITING)
    return;

  /* Device instances run until system reset; silently ignore. */
  if (p->type == PROCESS_TYPE_DEVICE)
    return;

#if PROCESS_CONF_CORES > 1
  /* the state of a process is written by its own core */
  if (p->core != PROCESS_CORE()) {
    process_mail(p->core, p, PROCESS_EVENT_NONE, PROCESS_MAIL_DESTROY);
    return;
  }
#endif

  p->state = PROCESS_STATE_EXITING;
  PT_FINAL(&p->pt); /* arm PT_FINALLY – runs on next scheduler tick */
}

/* ── process_run ──────────────────────────────────────────────────── */

void process_run(void)
{
#if PROCESS_CONF_CORES > 1
  process_mail_receive();
#endif
  /* Polls take priority over queued events to keep latency bounded. */
  if (do_poll())
    return;
  do_event();
}

/* ── process_poll ─────────────────────────────────────────────────── */

#if PROCESS_CONF_CORES > 1
void process_poll(struct process *p)
{
  if (!p
      || p->state == PROCESS_STATE_NONE
      || p->state == PROCESS_STATE_EXITING)
    return;
  /* the needs_poll of a process of another core is its own to write */
  if (p->core != PROCESS_CORE()) {
    process_mail(p->core, p, PROCESS_EVENT_NONE, PROCESS_MAIL_POLL);
    return;
  }
  process_poll_here(p);
}

static void process_poll_here(struct process *p)
#else
void process_poll(struct process *p)
#endif
{
  if (!p
      || p->state == PROCESS_STATE_NONE
      || p->state == PROCESS_STATE_EXITING)
    return;

  p->needs_poll  = 1;
  poll_requested = 1;
  PROCESS_CONF_WAKE();
}

/* ── process_post ─────────────────────────────────────────────────── */

#if PROCESS_CONF_CORES > 1
/* A process of another core gets its event through the mailbox. A
 * broadcast (p NULL) reaches the processes of this core: each core has its
 * own tick. */
int process_post(struct process *p, process_event_t ev, process_data_t data)
{
  if (p && p->core != PROCESS_CORE())
    return process_mail(p->core, p, ev, data);
  return process_post_here(p, ev, data);
}

static int process_post_here(struct process *p, process_event_t ev, process_data_t data)
#else
int process_post(struct process *p, process_event_t ev, process_data_t data)
#endif
{
  int ok = 0;

  CC_ATOMIC_RESTORE() {
#if PROCESS_CONF_EVENT_INBOX
    if (p != NULL) {
      /* Try the fast per-process inbox first; overflow to global queue. */
      ok = process_inbox_push(p, ev, data);
      if (!ok)
        ok = enqueue_event_nolock(p, ev, data);
    } else {
      ok = enqueue_event_nolock(NULL, ev, data);
    }
#else
    ok = enqueue_event_nolock(p, ev, data);
#endif
  }

  if (ok)
    PROCESS_CONF_WAKE();
  return ok;
}

/* ── process_post_from_isr ────────────────────────────────────────── */

int process_post_from_isr(struct process  *p,
                           process_event_t  ev,
                           process_data_t   data)
{
  /* Identical to process_post; distinct name documents ISR call-site intent. */
  return process_post(p, ev, data);
}

/* ── process_lookup_n ─────────────────────────────────────────────── */

/**
 * Compare a RAM segment [segment, segment+len) against a PROGMEM
 * process name string.  Returns 1 on match.
 */
static int process_match_n(const char *segment, size_t len,
                            const char *progmem_name)
{
  size_t name_len = strlen_P(progmem_name);
  if (len != name_len)
    return 0;
  return strncmp_P(segment, progmem_name, len) == 0;
}

struct process *process_lookup_n(const char *name_segment, size_t len)
{
#if PROCESS_CONF_CORES > 1
  /* the processes of all cores */
  for (struct process *p = process_next(NULL); p != NULL; p = process_next(p)) {
#else
  for (struct process *p = process_list; p != NULL; p = p->next) {
#endif
    const char *pname = PROCESS_NAME_STRING(p);
    if (process_match_n(name_segment, len, pname))
      return p;
  }
  return NULL;
}

/* ================================================================== */
/* §H  Structured logging                                              */
/* ================================================================== */

/**
 * Core log helper.  Allocates a slot from the rotating error pool,
 * fills it, and posts it to the logger process.
 *
 * The rotating pool avoids dynamic allocation.  Oldest entries are
 * silently overwritten when the pool wraps; PROCESS_CONF_ERROR_POOL_SIZE
 * should be tuned to the burst rate expected in the application.
 */
static void process_log(struct process  *src,
                         uint8_t          severity,
                         process_event_t  ev,
                         uint8_t          err)
{
  if (!process_error_logger)
    return;

  uint8_t idx = (uint8_t)(error_pool_idx++ % PROCESS_CONF_ERROR_POOL_SIZE);
  error_pool[idx].source   = src;
  error_pool[idx].severity = severity;
  error_pool[idx].event    = ev;
  error_pool[idx].error    = err;

  /* Post PROCESS_EVENT_ERROR so the logger can filter on event type. */
  process_post(process_error_logger, PROCESS_EVENT_ERROR, &error_pool[idx]);
}

void process_debug  (struct process *src, process_event_t ev, uint8_t code)
{ process_log(src, 1, ev, code); }

void process_flow   (struct process *src, process_event_t ev, uint8_t code)
{ process_log(src, 2, ev, code); }

void process_verbose(struct process *src, process_event_t ev, uint8_t code)
{ process_log(src, 3, ev, code); }

void process_info   (struct process *src, process_event_t ev, uint8_t code)
{ process_log(src, 4, ev, code); }

void process_warn   (struct process *src, process_event_t ev, uint8_t code)
{ process_log(src, 5, ev, code); }

void process_error  (struct process *src, process_event_t ev, uint8_t code)
{ process_log(src, 6, ev, code); }

void process_fatal  (struct process *src, process_event_t ev, uint8_t code)
{ process_log(src, 7, ev, code); }

/* ================================================================== */
/* §I  IPC wake callback                                               */
/* ================================================================== */

#if PROCESS_CONF_PIPELINES > 0
/**
 * process_ipc_wake(ctx)
 *
 * Pass as the wake_cb to ipc_pipe_init() with ctx = &my_proc.base.
 * The pipe will call this from its write path (possibly ISR context)
 * whenever data transitions the pipe from empty to non-empty.
 */
void process_ipc_wake(void *ctx)
{
  struct process *p = (struct process *)ctx;
  if (p)
    process_poll(p);
}
#endif

/* ================================================================== */
/* §J  Introspection and control: what a shell needs                  */
/* ================================================================== */

struct process *process_next(struct process *p)
{
#if PROCESS_CONF_CORES > 1
  /* the processes of all cores, core 0 first */
  uint8_t c = p ? p->core : 0;
  struct process *n = p ? p->next : process_cores[0].process_list_;
  while (!n && ++c < PROCESS_CONF_CORES)
    n = process_cores[c].process_list_;
  return n;
#else
  return p ? p->next : process_list;
#endif
}

int process_pause(struct process *p)
{
  if (!p || p->state != PROCESS_STATE_CALLED)
    return 0;
#if PROCESS_CONF_CORES > 1
  if (p->core != PROCESS_CORE())
    return process_mail(p->core, p, PROCESS_EVENT_NONE, PROCESS_MAIL_PAUSE);
#endif
  p->state = PROCESS_STATE_PAUSED;
  return 1;
}

int process_resume(struct process *p)
{
  if (!p || p->state != PROCESS_STATE_PAUSED)
    return 0;
#if PROCESS_CONF_CORES > 1
  if (p->core != PROCESS_CORE())
    return process_mail(p->core, p, PROCESS_EVENT_NONE, PROCESS_MAIL_RESUME);
#endif
  p->state = PROCESS_STATE_CALLED;
  process_poll(p); /* whatever it missed, it looks again */
  return 1;
}

#if PROCESS_CONF_PIPELINES > 0
int process_connect(struct process *writer, struct process *reader)
{
  ipc_pipe_t *pipe;

  if (!writer || !reader)
    return 0;

  if (writer->type == PROCESS_TYPE_DEVICE) {
    /* what the device receives becomes the input of the reader */
    if (!writer->has_pipein)
      return 0;
    pipe = writer->pipein;
    pipe->wake_cb  = process_ipc_wake;
    pipe->wake_ctx = (void *)reader;
    reader->pipein     = pipe;
    reader->has_pipein = 1;
    return 1;
  }

  /* a device transmits what is written to its pipeout */
  if (reader->type == PROCESS_TYPE_DEVICE)
    pipe = reader->has_pipeout ? reader->pipeout : NULL;
  else
    pipe = reader->has_pipein ? reader->pipein : NULL;
  if (!pipe)
    return 0;

  writer->pipeout     = pipe;
  writer->has_pipeout = 1;
  return 1;
}

void process_disconnect(struct process *writer)
{
  if (!writer || writer->type == PROCESS_TYPE_DEVICE)
    return;
  writer->has_pipeout = 0;
  writer->pipeout     = NULL;
}

struct process *process_reader(const ipc_pipe_t *pipe)
{
#if PROCESS_CONF_CORES > 1
  for (struct process *p = process_next(NULL); p != NULL; p = process_next(p)) {
#else
  for (struct process *p = process_list; p != NULL; p = p->next) {
#endif
    if (p->type == PROCESS_TYPE_DEVICE
          ? (p->has_pipeout && p->pipeout == pipe)
          : (p->has_pipein && p->pipein == pipe))
      return p;
  }
  return NULL;
}
int process_connect_pipe(struct process *writer, ipc_pipe_t *pipe)
{
  if (!writer || !pipe || writer->type == PROCESS_TYPE_DEVICE)
    return 0;
  writer->pipeout     = pipe;
  writer->has_pipeout = 1;
  return 1;
}
#endif /* PROCESS_CONF_PIPELINES > 0 */

int process_is_idle(void)
{
#if PROCESS_CONF_CORES > 1
  return !poll_requested && event_head == event_tail && !process_mail_waiting();
#else
  return !poll_requested && event_head == event_tail;
#endif
}

const struct error_info *process_log_entry(uint8_t age)
{
  const struct error_info *e;

  if (age >= PROCESS_CONF_ERROR_POOL_SIZE)
    return NULL;
  e = &error_pool[(uint8_t)(error_pool_idx - 1 - age) % PROCESS_CONF_ERROR_POOL_SIZE];
  return e->severity ? e : NULL;
}

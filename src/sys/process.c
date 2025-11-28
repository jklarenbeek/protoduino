// file: ./src/sys/process.c

#include "process.h"
#include <stddef.h>

// --------------------------------------------------------------------------
// Configuration
// --------------------------------------------------------------------------

#define PROCESS_CONF_EVENT_QUEUE_SIZE 8
static process_event_t events[PROCESS_CONF_EVENT_QUEUE_SIZE];
static process_num_events_t event_head, event_tail;

static struct process *process_list = NULL;
static volatile uint8_t poll_requested = 0;

static struct process *process_error_logger = NULL;


// --------------------------------------------------------------------------
// Implementation
// --------------------------------------------------------------------------

void process_init(struct process *error_logger)
{
  event_head = event_tail = 0;
  process_list = NULL;
  poll_requested = 0;
  process_error_logger;
}

void process_start(struct process *p)
{
  if (p->state != PROCESS_STATE_NONE) return;

  PT_INIT(&p->pt);
  p->state = PROCESS_STATE_CALLED;
  p->needspoll = 0;

  // Insert by priority (0 is highest)
  struct process **q = &process_list;
  while (*q != NULL && (*q)->prio <= p->prio) {
    q = &((*q)->next);
  }
  p->next = *q;
  *q = p;

  process_post(p, PROCESS_EVENT_INIT, NULL);
}

void process_exit(struct process *p)
{
  if (p->state == PROCESS_STATE_NONE) return;

  // Remove from list
  struct process **q = &process_list;
  while (*q != NULL) {
    if (*q == p) {
      *q = p->next;
      break;
    }
    q = &((*q)->next);
  }

  p->state = PROCESS_STATE_NONE;
  p->next = NULL;
}

/**
 * @brief Invokes the process thread and handles lifecycle transitions.
 * * This is where the Protothread v2 Finalize logic lives.
 */
static void call_process(struct process *p, process_event_t ev, process_data_t data)
{
  // 1. Mark as running
  p->state = PROCESS_STATE_RUNNING;

  // 2. Execute the Protothread
  ptstate_t ret = p->thread(&p->pt, ev, data);

  // 3. Handle Lifecycle Return Values

  // CASE A: Thread is still running (Waiting or Yielded)
  if (ret == PT_WAITING || ret == PT_YIELDED) {
    p->state = PROCESS_STATE_CALLED;
    return;
  }

  // CASE B: Thread Ended, Exited, or Errored
  // We must now perform the "Finalize Ritual" as per 03-pt-finally.ino

  if (PT_ISERROR(ret)) {
    // 3a. Handle Exceptions
    struct error_info info = { .source = p, .code = PT_ERROR_CODE(ret) };
    if (process_error_logger != NULL)
      process_post(&process_error_logger, PROCESS_EVENT_ERROR, &info);
  }

  // 3b. Trigger Finalization
  // This sets the LC to the specific 'case' label of the PT_FINALLY block.
  PT_FINAL(&p->pt);

  // 3c. Run until PT_FINALIZED (255)
  // We loop here to ensure the cleanup block (PT_FINALLY) executes completely.
  // The user's example 03-pt-finally.ino demonstrates this exact while loop.
  // Note: PT_ISRUNNING returns false only when ret == PT_FINALIZED (255).
  while (PT_ISRUNNING(ret = p->thread(&p->pt, ev, data))) {
      // In a complex system, we might yield here, but cleanup is usually atomic.
      // This loop effectively steps through the PT_FINALLY block.
  }

  // 4. Remove from kernel
  process_exit(p);
}

// --------------------------------------------------------------------------
// Scheduler
// --------------------------------------------------------------------------

static int do_poll(void)
{
  if (!poll_requested) return 0;

  poll_requested = 0;
  int any_polled = 0;

  for (struct process *p = process_list; p != NULL; p = p->next) {
    if (p->needspoll) {
      p->needspoll = 0;
      call_process(p, PROCESS_EVENT_POLL, NULL);
      any_polled = 1;
    }
  }
  return any_polled;
}

static int do_event(void)
{
  if (event_head == event_tail) return 0;

  process_event_t ev = events[event_tail];
  event_tail = (event_tail + 1) % PROCESS_CONF_EVENT_QUEUE_SIZE;

  // Iterate all processes (Broadcast style for simplicity in this kernel version,
  // or targeted if we added 'destination' to event queue)
  for (struct process *p = process_list; p != NULL; p = p->next) {
    if (p->state != PROCESS_STATE_NONE) {
        call_process(p, ev, NULL);
    }
  }
  return 1;
}

void process_run(void)
{
  if (do_poll()) return;
  do_event();
}

// --------------------------------------------------------------------------
// Messaging
// --------------------------------------------------------------------------

void process_post(struct process *p, process_event_t ev, process_data_t data)
{
  process_num_events_t next = (event_head + 1) % PROCESS_CONF_EVENT_QUEUE_SIZE;
  if (next != event_tail) {
    events[event_head] = ev;
    event_head = next;
    // Note: data passing simplified for this scheduler version
  }
}

void process_poll(struct process *p)
{
  if (p && p->state != PROCESS_STATE_NONE) {
    p->needspoll = 1;
    poll_requested = 1;
  }
}
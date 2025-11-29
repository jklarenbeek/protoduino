// file: ./src/sys/process.c
#define _POSIX_C_SOURCE 200809L
#include "process.h"
#include "ipc.h"    /* optional: for PROCESS_EVENT_MSG consumer expectation */
#include <string.h>

/* Internal event entry */
struct process_event_entry {
    struct process *dest;      /* NULL => broadcast */
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
#define ERROR_INFO_POOL_SIZE 4
static struct error_info error_pool[ERROR_INFO_POOL_SIZE];
static uint8_t error_pool_idx = 0;

/* ---------------- internal helpers ---------------- */

/* Non-atomic enqueue (caller must ensure atomic) */
static int enqueue_event_nolock(struct process *p, process_event_t ev, process_data_t data)
{
    process_num_events_t next = (event_head + 1) % PROCESS_CONF_EVENT_QUEUE_SIZE;
    if (next == event_tail) {
        return 0; /* full */
    }
    events[event_head].dest = p;
    events[event_head].ev = ev;
    events[event_head].data = data;
    event_head = next;
    return 1;
}

/* Non-atomic dequeue one event into out */
static int dequeue_event_nolock(struct process_event_entry *out)
{
    if (event_head == event_tail) return 0;
    *out = events[event_tail];
    event_tail = (event_tail + 1) % PROCESS_CONF_EVENT_QUEUE_SIZE;
    return 1;
}

/* Per-process inbox utilities (if enabled) */
#if PROCESS_CONF_PER_PROCESS_INBOX
static int process_inbox_push(struct process *p, process_event_t ev, process_data_t data)
{
    if (!p) return 0;
#if PROCESS_CONF_INBOX_POINTERS
    uint8_t next = (uint8_t)((p->inbox_head + 1) % PROCESS_CONF_INBOX_SIZE);
    if (next == p->inbox_tail) return 0; /* full */
    p->inbox_ev[p->inbox_head] = ev;
    p->inbox_data[p->inbox_head] = data;
    p->inbox_head = next;
    return 1;
#else
    uint8_t next = (uint8_t)((p->inbox_head + 1) % PROCESS_CONF_INBOX_SIZE);
    if (next == p->inbox_tail) return 0; /* full */
    p->inbox[p->inbox_head].ev = ev;
    p->inbox[p->inbox_head].data = data;
    p->inbox_head = next;
    return 1;
#endif
}

static int process_inbox_pop(struct process *p, struct process_event_entry *out)
{
    if (!p) return 0;
    if (p->inbox_head == p->inbox_tail) return 0; /* empty */
#if PROCESS_CONF_INBOX_POINTERS
    out->ev = p->inbox_ev[p->inbox_tail];
    out->data = p->inbox_data[p->inbox_tail];
#else
    out->ev = p->inbox[p->inbox_tail].ev;
    out->data = p->inbox[p->inbox_tail].data;
#endif
    out->dest = p;
    p->inbox_tail = (uint8_t)((p->inbox_tail + 1) % PROCESS_CONF_INBOX_SIZE);
    return 1;
}
#endif /* PROCESS_CONF_PER_PROCESS_INBOX */

/* Call a process's protothread and handle PT lifecycle correctly */
static void call_process(struct process *p, process_event_t ev, process_data_t data)
{
    if (!p) return;

    p->state = PROCESS_STATE_RUNNING;
    ptstate_t ret = p->thread(&p->pt, ev, data);

    /* If still running (WAITING or YIELDED), mark called and return */
    if (PT_ISRUNNING(ret)) {
        p->state = PROCESS_STATE_CALLED;
        return;
    }

    /* If thread returned EXITED/ENDED or an ERROR => scheduler must run FINAL blocks */
    if (PT_ISEXITING(ret)) {
        /* Post an error event to logger if it's an error */
        if (PT_ISERROR(ret) && process_error_logger) {
            uint8_t idx = (uint8_t)(error_pool_idx++ % ERROR_INFO_POOL_SIZE);
            error_pool[idx].source = p;
            error_pool[idx].code = (uint8_t)ret; /* raw ptstate_t forwarded as requested */
            /* best-effort: ignore return value - logger may be full */
            (void)process_post(process_error_logger, PROCESS_EVENT_ERROR, &error_pool[idx]);
        }

        /* Arm the final state for the protothread */
        PT_FINAL(&p->pt);

        /* Run final/finalizer blocks until PT_FINALIZED */
        ptstate_t fret;
        do {
            fret = p->thread(&p->pt, ev, data);
            /* if the finalizer itself returns PT_ISERROR, post it as well */
            if (PT_ISERROR(fret) && process_error_logger) {
                uint8_t idx2 = (uint8_t)(error_pool_idx++ % ERROR_INFO_POOL_SIZE);
                error_pool[idx2].source = p;
                error_pool[idx2].code = (uint8_t)fret;
                (void)process_post(process_error_logger, PROCESS_EVENT_ERROR, &error_pool[idx2]);
            }
            /* loop until FINALIZED */
        } while (fret != PT_FINALIZED);

        /* finished finalization: remove process */
        process_exit(p);
        return;
    }

    /* If thread already FINALIZED (rare), just remove it */
    if (ret == PT_FINALIZED) {
        process_exit(p);
        return;
    }

    /* Any other return value (defensive) -> set called */
    p->state = PROCESS_STATE_CALLED;
}

/* ---------------- Poll & Event dispatch ---------------- */

/* Run all polls (when poll_requested). Returns 1 if any poll handled. */
static int do_poll(void)
{
    if (!poll_requested) return 0;
    poll_requested = 0;
    int any = 0;
    for (struct process *pp = process_list; pp != NULL; pp = pp->next) {
        if (pp->needspoll) {
            pp->needspoll = 0;
            call_process(pp, PROCESS_EVENT_POLL, NULL);
            any = 1;
        }
    }
    return any;
}

/* Handle exactly one event. Returns 1 if event processed; 0 if none. */
static int do_event(void)
{
    struct process_event_entry e;
    /* First service per-process inbox if enabled (low-latency directed msgs) */
#if PROCESS_CONF_PER_PROCESS_INBOX
    for (struct process *pp = process_list; pp != NULL; pp = pp->next) {
        if (pp->inbox_head != pp->inbox_tail) {
            if (process_inbox_pop(pp, &e)) {
                call_process(pp, e.ev, e.data);
                return 1;
            }
        }
    }
#endif

    /* Otherwise pop one entry from global queue atomically */
    CC_ATOMIC_RESTORE() {
        if (!dequeue_event_nolock(&e)) {
            /* nothing */
            e.dest = NULL; e.ev = PROCESS_EVENT_NONE; e.data = NULL;
        }
    }

    if (e.ev == PROCESS_EVENT_NONE) return 0;

    if (e.dest == NULL) {
        /* Broadcast: call every registered process (no extra polls between calls) */
        for (struct process *pp = process_list; pp != NULL; pp = pp->next) {
            if (pp->state != PROCESS_STATE_NONE) {
                call_process(pp, e.ev, e.data);
            }
        }
    } else {
        /* Directed: deliver to specific process if active */
        if (e.dest->state != PROCESS_STATE_NONE) {
            call_process(e.dest, e.ev, e.data);
        }
    }
    return 1;
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
    if (!p) return;
    if (p->state != PROCESS_STATE_NONE) return;

    PT_INIT(&p->pt);
    p->state = PROCESS_STATE_CALLED;
    p->needspoll = 0;

#if PROCESS_CONF_PER_PROCESS_INBOX
    p->inbox_head = 0;
    p->inbox_tail = 0;
#endif

    /* insert by priority (lower numeric => higher priority) */
    struct process **q = &process_list;
    while (*q != NULL && (*q)->prio <= p->prio) q = &((*q)->next);
    p->next = *q;
    *q = p;

    /* send INIT */
    (void)process_post(p, PROCESS_EVENT_INIT, NULL);
}

void process_exit(struct process *p)
{
    if (!p) return;
    if (p->state == PROCESS_STATE_NONE) return;

    /* unlink from process_list */
    struct process **q = &process_list;
    while (*q) {
        if (*q == p) {
            *q = p->next;
            break;
        }
        q = &((*q)->next);
    }

    p->state = PROCESS_STATE_NONE;
    p->next = NULL;
}

void process_run(void)
{
    /* game-loop: run polls first (if requested), else one event */
    if (do_poll()) return;
    (void)do_event();
}

/* Post event (atomic). If per-process inbox enabled and destination != NULL,
 * try to place in inbox first; otherwise fall back to global queue.
 */
int process_post(struct process *p, process_event_t ev, process_data_t data)
{
    int ok = 0;
    CC_ATOMIC_RESTORE() {
#if PROCESS_CONF_PER_PROCESS_INBOX
        if (p != NULL) {
            ok = process_inbox_push(p, ev, data);
            if (!ok) {
                ok = enqueue_event_nolock(p, ev, data);
            }
        } else {
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

void process_poll(struct process *p)
{
    if (!p) return;
    if (p->state == PROCESS_STATE_NONE) return;
    p->needspoll = 1;
    poll_requested = 1;
}

void process_report_error(struct process *src, uint8_t code)
{
    if (!process_error_logger) return;
    uint8_t idx = (uint8_t)(error_pool_idx++ % ERROR_INFO_POOL_SIZE);
    error_pool[idx].source = src;
    error_pool[idx].code = code;
    /* best-effort post */
    (void)process_post(process_error_logger, PROCESS_EVENT_ERROR, &error_pool[idx]);
}

/* EOF */

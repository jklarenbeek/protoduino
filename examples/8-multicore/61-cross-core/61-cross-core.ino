// file: examples/8-multicore/61-cross-core/61-cross-core.ino
/*
 * 61-cross-core.ino  –  everything that crosses from one core to the other
 * ================================================================
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   A test that checks itself. Core 0 sends the numbers 1 to N to a process
 *   on core 1, three ways at once:
 *
 *     - as events (process_post across cores, through the mailbox)
 *     - as messages from a pool that core 0 takes from and core 1 gives back
 *     - and core 1 writes each number back, as a byte, into a pipe that
 *       core 0 reads
 *
 *   Core 1 wakes the source the other way: what it writes into the pipe
 *   polls it, and it posts an event for each message it gives back.
 *
 *   Each side keeps a sum. When the last number has come back, core 0 prints
 *   both sums and the counts, and PASS or FAIL. The pool is small on
 *   purpose, and so is the mailbox (PROCESS_CONF_MAILBOX_SIZE), so that both
 *   run full and the waiting is tested too.
 *
 * BUILD
 * -----
 *   tools/build_rp2.sh rp2040 examples/8-multicore/61-cross-core/61-cross-core.ino -DPROCESS_CONF_CORES=2
 *   protosim 61-cross-core-rp2040.elf --uart0-out out.txt --core-skew 3:1   (and other ratios)
 */

#include <protoduino.h>
#include <sys/process.h>
#include <sys/serial.h>
#include <sys/ipc.h>

#if PROCESS_CONF_CORES < 2
#error "61-cross-core: build with -DPROCESS_CONF_CORES=2"
#endif

#define PROCESS_CONF_SLEEP PROCESS_SLEEP_WAIT
#include <sys/tick.h>
#include <sys/idle.h>

#ifndef CROSS_N
#define CROSS_N 3000
#endif

#define EV_NUMBER  EVENT_DATA_RECV /* data: the number                 */
#define EV_MESSAGE EVENT_MSG_PUSH  /* data: an ipc_msg_t from the pool */

/* the pool of messages: 4 of them, taken on core 0, given back on core 1 */
static ipc_msg_t msg_store[4];
static struct ipc_pool msg_pool;

/* the pipe back: core 1 writes, core 0 reads */
static uint8_t back_buf[16];
static ipc_pipe_t back;

/* the sums: written by one core each, read at the end */
static volatile uint32_t sum_events, sum_messages, n_events, n_messages;

/* ---- the processes ----------------------------------------------------- */

PROCESS_DEFINE(source, "source", 2,
  uint32_t next;     /* the next number to send as an event   */
  uint32_t next_msg; /* and as a message                      */
  uint32_t back_n;   /* bytes come back                       */
  uint32_t back_sum; /* their sum                             */
  uint8_t  byte;
);
PROCESS_INSTANCE(source, source_proc);

/* ---- core 1: the sink -------------------------------------------------- */

PROCESS_DEFINE(sink, "sink", 2,
  uint32_t written; /* numbers written back into the pipe */
  uint8_t  byte;
);
PROCESS_INSTANCE(sink, sink_proc);

/* the work, in plain functions: a protothread keeps no locals across a wait */
static void sink_number(process_data_t data)
{
  uint32_t n = (uint32_t)(uintptr_t)data;
  sum_events += n;
  n_events++;
}

/* The numbers arrive in order, 1 to N: each one goes back as its low byte,
 * as far as the pipe has room. The sink never waits for room, which would
 * take the next event and lose it: it writes again on its next event (a
 * number, a message, or its tick). */
static void sink_write_back(process_sink_t *self)
{
  while (self->written < n_events)
  {
    self->byte = (uint8_t)(self->written + 1);
    if (ipc_pipe_write(&back, &self->byte, 1) != 1)
      return;
    self->written++;
  }
}

static void sink_message(process_data_t data)
{
  ipc_msg_t *m = (ipc_msg_t *)data;
  sum_messages += (uint32_t)(uintptr_t)m->argv[0];
  n_messages++;
  ipc_msg_free_to_pool(&msg_pool, m);
  /* there is room in the pool again: core 0 may send the next one */
  process_post(&source_proc.base, EVENT_MSG_POP, NULL);
}

PROCESS_THREAD(sink, ev, data)
{
  process_sink_t *self = PROCESS_SELF(sink);

  PROCESS_BEGIN();

  while (1)
  {
    PROCESS_WAIT_EVENT();
    if (ev == EV_NUMBER)
      sink_number(data);
    else if (ev == EV_MESSAGE)
      sink_message(data);
    sink_write_back(self);
  }

  PROCESS_END();
}

/* ---- core 0: the source ------------------------------------------------ */


static void print_u32(uint32_t v)
{
  char digits[11];
  uint8_t i = sizeof(digits);
  digits[--i] = 0;
  do
    digits[--i] = (char)('0' + v % 10);
  while ((v /= 10) && i);
  print(&digits[i]);
}

static uint32_t expected_byte_sum(void)
{
  uint32_t s = 0;
  for (uint32_t n = 1; n <= CROSS_N; n++)
    s += (uint8_t)n;
  return s;
}

static void report(process_source_t *self)
{
  uint32_t want = (uint32_t)CROSS_N * (CROSS_N + 1) / 2;
  bool ok = n_events == CROSS_N && n_messages == CROSS_N && sum_events == want && sum_messages == want &&
            self->back_n == CROSS_N && self->back_sum == expected_byte_sum() && ipc_pool_count_free(&msg_pool) == 4;
  print_P(PSTR("events "));   print_u32(n_events);     print_P(PSTR(" sum ")); print_u32(sum_events);     println();
  print_P(PSTR("messages ")); print_u32(n_messages);   print_P(PSTR(" sum ")); print_u32(sum_messages);   println();
  print_P(PSTR("pipe "));     print_u32(self->back_n); print_P(PSTR(" sum ")); print_u32(self->back_sum); println();
  print_P(ok ? PSTR("PASS") : PSTR("FAIL"));
  println();
  serial0_flush();
}

/* as many messages as the pool has */
static void source_messages(process_source_t *self)
{
  while (self->next_msg <= CROSS_N)
  {
    ipc_msg_t *m = ipc_msg_alloc_from_pool(&msg_pool);
    if (!m)
      return;
    m->argc = 1;
    m->argv[0] = (void *)(uintptr_t)self->next_msg;
    if (!process_post(&sink_proc.base, EV_MESSAGE, m))
    {
      ipc_msg_free_to_pool(&msg_pool, m);
      return;
    }
    self->next_msg++;
  }
}

/* what came back */
static void source_back(process_source_t *self)
{
  while (ipc_pipe_read(&back, &self->byte, 1) == 1)
  {
    self->back_n++;
    self->back_sum += self->byte;
  }
}

PROCESS_THREAD(source, ev, data)
{
  process_source_t *self = PROCESS_SELF(source);

  PROCESS_BEGIN();

  self->next = 1;
  self->next_msg = 1;
  while (self->back_n < CROSS_N || self->next_msg <= CROSS_N)
  {
    /* as many events as the mailbox takes */
    while (self->next <= CROSS_N && process_post(&sink_proc.base, EV_NUMBER, (process_data_t)(uintptr_t)self->next))
      self->next++;
    source_messages(self);
    source_back(self);
    PROCESS_WAIT_EVENT(); /* let go, and look again on the next event */
  }

  /* the last messages may still be on their way back to the pool */
  while (n_messages < CROSS_N)
    PROCESS_WAIT_EVENT(); /* let go, and look again on the next event */
  report(self);

  PROCESS_END();
}

static volatile bool core1_ready;
static bool started;

void setup(void)
{
  serial0_open(115200);
  ipc_pool_init(&msg_pool, msg_store, sizeof(msg_store[0]), 4);
  /* what comes back wakes the source (a poll of a process of core 0) */
  ipc_pipe_init(&back, back_buf, sizeof(back_buf), process_ipc_wake, &source_proc.base);
  process_init(NULL);
  print_P(PSTR("61-cross-core: "));
  print_u32(CROSS_N);
  print_P(PSTR(" numbers from core 0 to core 1 and back"));
  println();
}

void loop(void)
{
  if (!started && core1_ready)
  {
    process_new(&process_type_source, &source_proc);
    started = true;
  }
  process_loop();
}

void setup1(void)
{
  process_init(NULL);
  process_new(&process_type_sink, &sink_proc);
  core1_ready = true;
}

void loop1(void)
{
  process_loop();
}

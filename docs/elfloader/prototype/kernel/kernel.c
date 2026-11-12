// file: docs/elfloader/prototype/kernel/kernel.c
/*
 * Test kernel for the module scenarios: a sketch on the real protoduino
 * scheduler, built without the Arduino core (see core.c).
 *
 *   serial_process0  (DEVICE, prio 1)   UART0 at 115200 baud
 *   kernel           (PROCESS, prio 2)  reports, starts the loader, and is
 *                                       the error logger of the scheduler
 *   loader           (PROCESS, prio 2)  receives a module (MODULE_CONF_SUPPORT 2)
 *   heartbeat        (PROCESS, prio 3)  takes a turn as often as it can and
 *                                       counts: the witness that the system
 *                                       keeps running while a module arrives
 *
 * MODULE_CONF_SUPPORT   0 no modules, 1 start modules in flash, 2 load at run time
 * LOADER_CONF_THREAD    LOADER_THREAD_ROM (1) or LOADER_THREAD_OWN (2)
 *
 * Who programs a page is found out at run time (FLASH_DRIVER_AUTO, the
 * default of ./src/cpu/avr/flash.conf.h): the protoduino bootloader or
 * Optiboot.
 */
#include <protoduino.h>
#include <stdlib.h>
#include <sys/process.h>
#include <sys/process/serial.h>
#include <sys/timer.h>
#include <sys/tick.h>
#include <sys/module.h>
#if MODULE_CONF_SUPPORT >= 2
#include <mod/loader.h>
#endif

/* =========================================================================
 * heartbeat process
 * ========================================================================= */

PROCESS_DEFINE(heartbeat, "heartbeat", 3,
  uint16_t     beats;    /* turns taken                          */
  clock_time_t last;     /* time of the previous turn            */
  clock_time_t longest;  /* longest time between two turns, us   */
);

PROCESS_INSTANCE(heartbeat, heartbeat_proc);

PROCESS_THREAD(heartbeat, ev, data)
{
  process_heartbeat_t *self = PROCESS_SELF(heartbeat);
  (void)ev;
  (void)data;

  PROCESS_BEGIN();

  self->last = clock_time();

  while (1) {
    /* An event of our own: it queues behind everybody else's. Only that
     * event is a turn: the tick reaches every process, and a process that
     * answered each tick with another event would fill the queue. */
    process_post(pt_process, EVENT_HEARTBEAT, NULL);
    PROCESS_WAIT_EVENT_UNTIL(ev == EVENT_HEARTBEAT);

    clock_time_t now = clock_time();
    if (now - self->last > self->longest)
      self->longest = now - self->last;
    self->last = now;
    self->beats++;
  }

  PROCESS_END();
}

/* =========================================================================
 * kernel process
 * ========================================================================= */

PROCESS_DEFINE(kernel, "kernel", 2,
  const char *text;      /* text being written                   */
  uint8_t     ch;        /* byte on its way into the pipe        */
  uint8_t     slot;
  uint8_t     error;     /* why the load failed, 0: it did not   */
  uint16_t    beats;     /* the heartbeat during the load        */
  uint32_t    longest;
  char        num[11];
);

PROCESS_INSTANCE(kernel, kernel_proc);

/* Write one byte to the serial device; wait when its pipe is full. */
#define KERNEL_PUT(expr) \
  do { \
    while (ipc_pipe_space(PROCESS_PIPEOUT()) == 0) { process_post(pt_process, EVENT_INTERVAL, NULL); PROCESS_WAIT_EVENT(); } \
    self->ch = (expr); \
    ipc_pipe_write(PROCESS_PIPEOUT(), &self->ch, 1); \
    process_poll(&serial_process0_proc.base); \
  } while (0)

/* Write a string from flash, or the number in self->num. One per line. */
#define KERNEL_SAY(str) \
  for (self->text = PSTR(str); pgm_read_byte(self->text) != 0; self->text++) KERNEL_PUT(pgm_read_byte(self->text))
#define KERNEL_SAY_NUM() \
  for (self->text = self->num; *self->text != 0; self->text++) KERNEL_PUT(*self->text)

PROCESS_THREAD(kernel, ev, data)
{
  process_kernel_t *self = PROCESS_SELF(kernel);

  PROCESS_BEGIN();

  PROCESS_SET_PIPEOUT(&serial_process0_proc.tx_pipe);
  KERNEL_SAY("\nkernel up");

#if MODULE_CONF_SUPPORT >= 1
  module_init();
  utoa(module_kernel_id(), self->num, 16);
  KERNEL_SAY(" id=");
  KERNEL_SAY_NUM();

  utoa(module_autostart(), self->num, 10);
  KERNEL_SAY(" | modules started from flash: ");
  KERNEL_SAY_NUM();
#endif

#if MODULE_CONF_SUPPORT >= 2
  /* the first slot the module table does not list */
  for (self->slot = 0; self->slot < MODULE_CONF_SLOTS && module_is_installed(self->slot); self->slot++) { }

  if (!loader_is_available()) {
    KERNEL_SAY(" | the bootloader offers no way to load a module");
  } else if (self->slot == MODULE_CONF_SLOTS) {
    KERNEL_SAY(" | every slot holds a module");
  } else {
    self->num[0] = (char)('0' + self->slot);
    self->num[1] = 0;
    KERNEL_SAY(" | waiting for a module for slot ");
    KERNEL_SAY_NUM();
    KERNEL_SAY("\n");

    /* let the text leave before the heartbeat is watched */
    while (ipc_pipe_available(PROCESS_PIPEOUT()) > 0) { process_post(pt_process, EVENT_INTERVAL, NULL); PROCESS_WAIT_EVENT(); }

    heartbeat_proc.beats   = 0;
    heartbeat_proc.longest = 0;
    heartbeat_proc.last    = clock_time();
    /* the file arrives on the serial device; we are told when the module runs */
    if (loader_start(self->slot, &serial_process0_proc.base) != NULL)
      loader_proc.notify = pt_process;

    /* The loader tells us that the module runs; the scheduler tells its
     * error logger, which is us, that the loader ended with an error. */
    PROCESS_WAIT_EVENT_UNTIL(ev == EVENT_MODULE_INIT || ev == PROCESS_EVENT_ERROR);

    /* what the heartbeat saw, before this process prints anything */
    self->error   = (ev == EVENT_MODULE_INIT) ? 0 : ((struct error_info *)data)->error;
    self->beats   = heartbeat_proc.beats;
    self->longest = heartbeat_proc.longest;

    if (self->error == 0) {
      KERNEL_SAY("\nmodule started");
    } else {
      utoa(self->error, self->num, 16);
      KERNEL_SAY("\nload error ");
      KERNEL_SAY_NUM();
    }

    utoa(self->beats, self->num, 10);
    KERNEL_SAY(" | heartbeat turns during the load: ");
    KERNEL_SAY_NUM();
    ultoa(self->longest, self->num, 10);
    KERNEL_SAY(", longest wait ");
    KERNEL_SAY_NUM();
    KERNEL_SAY(" us");
  }
#endif

  KERNEL_SAY(" | end\n");

  while (1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}

/* =========================================================================
 * setup / loop
 * ========================================================================= */

void setup(void)
{
  /* The kernel process doubles as the error logger. */
  process_init(&kernel_proc.base);

  /* Start the UART0 device process first so its pipes are initialised
   * before the others connect. process_new() clears the struct, so the
   * baud rate is set afterwards: the INIT event has not been handled yet. */
  process_new(&process_type_serial_process0, &serial_process0_proc);
  serial_process0_proc.baud = UART_BAUD_115200;

  process_new(&process_type_kernel, &kernel_proc);
  process_new(&process_type_heartbeat, &heartbeat_proc);
}

void loop(void)
{
  process_run();
  tick_run();

  /* serial_process0 moves bytes from its tx_pipe to the UART buffer only
   * when it is polled, and stops when that buffer is full. Nothing polls
   * it again for the rest, so the sketch does. */
  if (ipc_pipe_available(&serial_process0_proc.tx_pipe) > 0)
    process_poll(&serial_process0_proc.base);
}

// file: ./src/mod/port.h

#ifndef __MOD_PORT_H__
#define __MOD_PORT_H__

/* port: the door through which a program on a PC works on a running system.
 *
 * A shell on the part spends its flash on reading commands and on writing
 * text, and a small part has none to spare. The PC has both, and it has the
 * .elf file of the sketch, which holds every name and every address. So the
 * part keeps only a door, and the shell runs on the PC (tools/port.mjs).
 *
 * How far the door opens is a setting, PORT_CONF_LEVEL. Each level adds to
 * the one below it:
 *
 *   1  look      read memory, flash and EEPROM: "ps", "mods", "ipc", "log",
 *                settings and views. The default.
 *   2  settings  also write the settings of the sketch (sys/settings.h), and
 *                nothing else, and run the actions the sketch names
 *                (PORT_ACTIONS). What a finished product needs.
 *   3  work      also write any memory and EEPROM and call any function:
 *                the whole shell. A debug door.
 *   4  load      also program pages of flash: modules into the slots of
 *                sys/module.h. Parts without a boot section only, the
 *                ATtiny25/45/85; on the others the bootloader loads.
 *
 * Levels 3 and 4 are behind a guard, PORT_CONF_GUARD:
 *
 *   PORT_GUARD_RESET   open for PORT_CONF_WINDOW seconds after a reset
 *                      through the reset pin, and as long as requests keep
 *                      coming with less than that between them. Somebody has
 *                      to be at the board: a power-on, a brown-out or the
 *                      watchdog keep it closed. The default.
 *   PORT_GUARD_SKETCH  open while port_may_open() says so, a function of the
 *                      sketch: a jumper, a button held, a setting.
 *   PORT_GUARD_NONE    always open. For a board on the desk.
 *
 * The guard is only as good as the lock bits: a part whose flash can be read
 * and written with a programmer has no secrets from whoever holds one.
 *
 * - The port is a process. It takes one byte per turn, and a function it
 *   calls runs in its turn like one the shell would call.
 * - The line is the device PORT_CONF_DEVICE: softserial_proc when the
 *   sketch included <sys/process/softserial.h> before this header,
 *   serial_process0_proc otherwise. The line belongs to the port: text that
 *   another process writes to the same device garbles its answers.
 * - PORT_MODULES_BEGIN ... PORT_MODULES_END list the modules the PC may
 *   start, each with the instance it runs in.
 * - Include <sys/tick.h> and, where they are used, <sys/settings.h> and
 *   <sys/module.h> before this header. The guard counts ticks.
 * - One port, port_proc, defined here: include this file in one source
 *   file only.
 *
 * A request is twelve bytes, and so is every request:
 *
 *   'p'  op  n  a.lo a.hi  d0 d1 d2 d3 d4 d5  check
 *
 *   'I'          answer: open (1/0), reset cause, extended fuse      level 1
 *   'R' 'F' 'E'  answer with n bytes of memory, flash or EEPROM at a level 1
 *   'e'          write the n bytes d0.. (at most six) to EEPROM at a  level 2*
 *   'A'          run action n; 0xFF runs settings_apply()             level 2
 *   'W'          write the n bytes d0.. to memory at a                level 3
 *   'C'          call the function at word address a with the three
 *                16-bit arguments in d; answer with its result        level 3
 *   'B'          start a page of flash at a: clear the page buffer    level 4
 *   'L'          put n words d0.. into the page buffer at offset a    level 4
 *   'G'          erase the page and program it from the buffer        level 4
 *   'T'          slot a: n = 0 take its module out and end it,
 *                1 check it and enter it into the table and start it,
 *                2 start it; answer with the result code              level 4
 *
 *   * at level 2 only within the settings
 *
 * The check byte makes the twelve bytes add up to zero under exclusive or.
 * The port looks at the last twelve bytes it received, whatever came
 * before, so a request that was damaged or cut short costs nothing but
 * itself, and nothing is written from a request that does not check out.
 *
 * The answer is a status byte, the bytes asked for when the status is 0,
 * and a check byte: their exclusive or with PORT_CHECK.
 */

#include <string.h>
#include <sys/process.h>
#include <sys/tick.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

/* ------------------------------------------------------------------ */
/* Configuration (define in the sketch, before this header)            */
/* ------------------------------------------------------------------ */

#define PORT_LEVEL_LOOK     1
#define PORT_LEVEL_SETTINGS 2
#define PORT_LEVEL_WORK     3
#define PORT_LEVEL_LOAD     4

#ifndef PORT_CONF_LEVEL
#define PORT_CONF_LEVEL PORT_LEVEL_LOOK
#endif

#define PORT_GUARD_NONE   0
#define PORT_GUARD_RESET  1
#define PORT_GUARD_SKETCH 2

#ifndef PORT_CONF_GUARD
#define PORT_CONF_GUARD PORT_GUARD_RESET
#endif

// seconds the door stays open after a reset through the pin, and after
// each request that used it; at most 255
#ifndef PORT_CONF_WINDOW
#define PORT_CONF_WINDOW 60
#endif

// where the reset cause is found. A bootloader may have cleared MCUSR: the
// protoduino bootloader leaves the cause in GPIOR0 (BOOT_RESET_CAUSE).
// The flag of the reset pin is cleared once the port has seen it: a flag
// stays until software clears it, and a watchdog reset later on must not
// find it and open the door again.
#ifndef PORT_CONF_RESET_CAUSE
#if defined(MCUSR)
#define PORT_CONF_RESET_CAUSE MCUSR
#else
#define PORT_CONF_RESET_CAUSE MCUCSR
#endif
#endif

#ifndef PORT_CONF_DEVICE
#if defined(__PROCESS_SOFTSERIAL_H__)
#define PORT_CONF_DEVICE (&softserial_proc.base)
#else
#include <sys/process/serial.h>
#define PORT_CONF_DEVICE (&serial_process0_proc.base)
#endif
#endif

// bytes of memory the PC may use as its own, for text it writes into a pipe
#ifndef PORT_CONF_SCRATCH
#if RAMEND < 0x200
#define PORT_CONF_SCRATCH 2
#else
#define PORT_CONF_SCRATCH 8
#endif
#endif

// the functions the PC can count on at level 3, each followed by a comma.
// A function nobody refers to is not part of the build.
#ifndef PORT_CONF_CALLS
#define PORT_CONF_CALLS \
  (port_call_t)process_pause, (port_call_t)process_resume, (port_call_t)process_destroy, \
  (port_call_t)process_new, (port_call_t)process_connect, (port_call_t)process_disconnect, \
  (port_call_t)ipc_pipe_write,
#endif

#if PORT_CONF_LEVEL >= PORT_LEVEL_LOAD
#if defined(RWWSRE)
#error "port: this part has a boot section and only code there can program flash; modules are loaded by the loader (mod/loader.h)"
#endif
#if !defined(__MODULE_H__) || !(MODULE_CONF_SUPPORT > 0)
#error "port: level 4 loads modules into slots: include <sys/module.h> with MODULE_CONF_SUPPORT 1 before this header"
#endif
#include <avr/boot.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PORT_SYNC  'p'
#define PORT_FRAME 12
#define PORT_CHECK 0xA5
#define PORT_ABI   2

/* the status byte of an answer */
#define PORT_OK      0  /* done                                      */
#define PORT_LEVEL   1  /* this port was built for a lower level      */
#define PORT_CLOSED  2  /* the guard keeps levels 3 and 4 closed      */
#define PORT_RANGE   3  /* not there: outside the settings or slots   */
#define PORT_BUSY    4  /* the slot holds a module, or it runs        */
#define PORT_UNKNOWN 5  /* no such request                            */

// ticks the door stays open
#define PORT_WINDOW_TICKS \
  ((uint16_t)((PORT_CONF_WINDOW) * 1000UL / TICK_MILLIS) ? (uint16_t)((PORT_CONF_WINDOW) * 1000UL / TICK_MILLIS) : 1)

/* ------------------------------------------------------------------ */
/* The modules the PC may start                                        */
/* ------------------------------------------------------------------ */

typedef struct port_module {
  const process_type_t *type;     /* what to run   */
  struct process       *instance; /* where it runs */
} port_module_t;

#define PORT_MODULES_BEGIN \
  extern "C" { extern const port_module_t port_modules[]; } \
  const port_module_t port_modules[] __attribute__((used)) CC_PROGMEM = {
#define PORT_MODULE(name, instance) { &process_type_##name, &(instance).base },
#define PORT_MODULES_END };

/* ------------------------------------------------------------------ */
/* Views and actions: what the PC offers of this sketch by name        */
/* ------------------------------------------------------------------ */

/* PORT_VIEW(name, what, help)
 *
 * `what` is one or more of these, with a semicolon between them:
 *
 *   byte e <address> <unit> <label>    one byte of EEPROM
 *   byte r <address> <unit> <label>    one byte of memory, or a register
 *   ring <address> <count> <unit>      a record of mod/record.h
 *
 * A unit is raw, tenths (of a volt), celsius (degrees above -40) or
 * percent (of 255). An address is a number or an expression of numbers.
 *
 * PORT_ACTIONS(X), defined before this header, lists functions of the
 * sketch that the PC may run at level 2, each X(name, function, help):
 *
 *   #define PORT_ACTIONS(X) X(restart, restart_board, "reset the board now")
 *
 * The texts are kept in the .elf file and cost the part nothing.
 */
#define PORT_STR_(x) #x
#define PORT_STR(x)  PORT_STR_(x)
#define PORT_NOTE(text) \
  __asm__(".pushsection .protoduino.notes,\"\",@progbits\n.asciz \"" text "\"\n.popsection\n")
#define PORT_VIEW(name, what, help) PORT_NOTE("view\\t" #name "\\t" what "\\t" help)

typedef void (*port_action_t)(void);

#ifdef PORT_ACTIONS
#define PORT_ACTION_FN(name, fn, help)   (port_action_t)fn,
#define PORT_ACTION_TEXT(name, fn, help) PORT_NOTE("action\\t" #name "\\t" help);
extern const port_action_t port_actions[];
const port_action_t port_actions[] __attribute__((used)) CC_PROGMEM = { PORT_ACTIONS(PORT_ACTION_FN) };
PORT_ACTIONS(PORT_ACTION_TEXT)
#define PORT_ACTION_COUNT (sizeof(port_actions) / sizeof(port_actions[0]))
#else
#define PORT_ACTION_COUNT 0
#endif

#if PORT_CONF_GUARD == PORT_GUARD_SKETCH
// supplied by the sketch: true while levels 3 and 4 may be used
bool port_may_open(void);
#endif

/* ------------------------------------------------------------------ */
/* The process                                                         */
/* ------------------------------------------------------------------ */

PROCESS_DEFINE(port, "port", 2,
  uint8_t  frame[PORT_FRAME];           /* the last twelve bytes received  */
  uint16_t at;                          /* where the answer has got to     */
  uint8_t  left;                        /* bytes of it still to come       */
  uint8_t  space;                       /* the memory they come from       */
  uint8_t  status;                      /* the first byte of the answer    */
  uint8_t  check;                       /* check byte of the answer so far */
  uint8_t  byte;
  uint8_t  cause;                       /* why the part was reset          */
#if PORT_CONF_LEVEL >= PORT_LEVEL_WORK && PORT_CONF_GUARD == PORT_GUARD_RESET
  uint16_t open;                        /* ticks the door stays open       */
#endif
#if PORT_CONF_LEVEL >= PORT_LEVEL_LOAD
  uint16_t page;                        /* the page being put together     */
#endif
  uint8_t  scratch[PORT_CONF_SCRATCH];  /* for the PC                      */
);

process_port_t port_proc;

typedef uint16_t (*port_call_t)(uint16_t, uint16_t, uint16_t);

/* What the PC needs to know about this build and cannot read from the .elf
 * file: how the structs are laid out with the settings in force. */
extern const uint8_t port_layout[];
const uint8_t port_layout[] __attribute__((used)) CC_PROGMEM = {
  PORT_ABI,
  PORT_CONF_LEVEL,
  sizeof(struct process),
  offsetof(struct process, next),
#ifndef PROCESS_CONF_NO_PROCESS_NAMES
  offsetof(struct process, name),
#else
  0xFF,
#endif
  offsetof(struct process, prio),   /* the state and the flags follow it */
  offsetof(struct process, thread),
  offsetof(struct process, pipein),
  offsetof(struct process, pipeout),
  sizeof(ipc_pipe_t),
  offsetof(ipc_pipe_t, wake_ctx),
  offsetof(process_port_t, scratch),
  PORT_CONF_SCRATCH,
  PORT_CONF_GUARD,
  PORT_CONF_WINDOW,
#if PORT_CONF_LEVEL >= PORT_LEVEL_LOAD
  SPM_PAGESIZE / 2,
#else
  0,
#endif
  PORT_ACTION_COUNT
};

#if PORT_CONF_LEVEL >= PORT_LEVEL_WORK
extern const port_call_t port_calls[];
const port_call_t port_calls[] __attribute__((used)) CC_PROGMEM = {
  PORT_CONF_CALLS
  (port_call_t)process_poll
};
#endif

// the list of the sketch, if it has one
extern const port_module_t port_modules[] __attribute__((weak));

// a 16-bit value of the request
static CC_ALWAYS_INLINE uint16_t port_word(const process_port_t *s, uint8_t i)
{
  return (uint16_t)s->frame[i] | ((uint16_t)s->frame[i + 1] << 8);
}

/* ---- the guard --------------------------------------------------- */

#if PORT_CONF_LEVEL >= PORT_LEVEL_WORK
static uint8_t port_is_open(const process_port_t *s)
{
#if PORT_CONF_GUARD == PORT_GUARD_NONE
  (void)s;
  return 1;
#elif PORT_CONF_GUARD == PORT_GUARD_RESET
  return s->open != 0;
#else
  (void)s;
  return port_may_open();
#endif
}

// a request of level 3 or 4 that may go ahead: the window starts again
static uint8_t port_unlocked(process_port_t *s)
{
  if (!port_is_open(s))
  {
    s->status = PORT_CLOSED;
    return 0;
  }
#if PORT_CONF_GUARD == PORT_GUARD_RESET
  s->open = PORT_WINDOW_TICKS;
#endif
  return 1;
}
#endif

/* ---- flash, for level 4 -------------------------------------------- */

#if PORT_CONF_LEVEL >= PORT_LEVEL_LOAD

// end of the kernel image in flash: nothing below it is ever written
extern const char __data_load_end[];

// the slot a page of the slots lies in, or 0xFF for a page outside them
static uint8_t port_slot_of(uint16_t page)
{
  if (page < (uint16_t)__data_load_end || page < MODULE_SLOT_ADDR(0) || page >= MODULE_TABLE_ADDR
      || (page & (SPM_PAGESIZE - 1)) != 0)
    return 0xFF;
  return (uint8_t)((page - MODULE_SLOT_ADDR(0)) / MODULE_CONF_SLOT_SIZE);
}

static uint8_t port_slot_busy(uint8_t slot)
{
  return module_is_installed(slot) || module_process(slot)->state != PROCESS_STATE_NONE;
}

// erase a page and program it from the page buffer. The processor stops
// while it does: about 9 ms.
static void port_page_write(uint16_t page)
{
  CC_ATOMIC_RESTORE()
  {
    boot_page_erase(page);
    boot_spm_busy_wait();
    boot_page_write(page);
    boot_spm_busy_wait();
  }
}

static void port_page_fill(uint16_t address, uint16_t word)
{
  CC_ATOMIC_RESTORE()
  {
    boot_page_fill(address, word);
  }
}

// a new page buffer, all ones
static void port_page_clear(void)
{
  SPMCSR = _BV(CTPB);
}

// enter a slot into the module table, or take it out, in place: the page
// buffer holds the new table, so no RAM is needed for it
static void port_table_write(uint8_t slot, uint16_t entry)
{
  uint16_t a = MODULE_TABLE_ADDR;
  uint8_t valid = pgm_read_word(a) == MODULE_MAGIC;
  uint16_t i, w;

  port_page_clear();
  for (i = 0; i < SPM_PAGESIZE; i += 2)
  {
    w = valid ? pgm_read_word(a + i) : 0xFFFF;
    if (i == 0)
      w = MODULE_MAGIC;
    else if (i == 2 + 2 * (uint16_t)slot)
      w = entry;
    port_page_fill(a + i, w);
  }
  port_page_write(a);
}

// 'T': the life of a slot. The answer is a result code: 0 when done.
static uint8_t port_slot(uint8_t slot, uint8_t what)
{
  struct process *p;
  uint8_t r;

  if (slot >= MODULE_CONF_SLOTS)
    return ERR_BOUNDS_UPPER;
  p = module_process(slot);

  if (what == 0)
  {
    // out of the table first: it does not start again, whatever happens
    if (module_is_installed(slot))
      port_table_write(slot, 0xFFFF);
    if (p->state != PROCESS_STATE_NONE)
    {
      process_resume(p);
      process_destroy(p); // it ends on its next turn
    }
    return ERR_SUCCESS;
  }
  if (what == 1)
  {
    if (port_slot_busy(slot))
      return ERR_STATE_LOCKED;
    r = module_check(slot);
    if (r != ERR_SUCCESS)
      return r;
    port_table_write(slot, module_kernel_id());
  }
  if (!module_is_installed(slot))
    return ERR_STATE_INVALID;
  if (p->state == PROCESS_STATE_NONE && module_start(slot) == NULL)
    return ERR_INIT_FAILED;
  return ERR_SUCCESS;
}

#endif /* PORT_CONF_LEVEL >= PORT_LEVEL_LOAD */

/* ---- requests ------------------------------------------------------- */

// the answer is s->left bytes of the memory s->space at s->at
static void port_answer(process_port_t *s, uint8_t space, uint16_t at, uint8_t n)
{
  s->space = space;
  s->at    = at;
  s->left  = n;
}

// carry out a request that checked out; the answer is in s->status and,
// when that is PORT_OK, at s->at
static void port_do(process_port_t *s)
{
  uint8_t op = s->frame[1];
  uint8_t n  = s->frame[2];
  uint16_t a = port_word(s, 3);

  s->status = PORT_OK;
  s->left   = 0;

  if (op == 'R' || op == 'F' || op == 'E')
  {
    port_answer(s, op, a, n);
    return;
  }
  if (op == 'I')
  {
    s->frame[5] = 0;
#if PORT_CONF_LEVEL >= PORT_LEVEL_WORK
    s->frame[5] = port_is_open(s);
#endif
    s->frame[6] = s->cause;
#if PORT_CONF_LEVEL >= PORT_LEVEL_LOAD
    s->frame[7] = boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS);
#else
    s->frame[7] = 0xFF;
#endif
    port_answer(s, 'R', (uint16_t)&s->frame[5], 3);
    return;
  }

#if PORT_CONF_LEVEL >= PORT_LEVEL_SETTINGS
  if (op == 'e' && n <= 6)
  {
#if defined(__SETTINGS_H__)
    if (a >= SETTINGS_CONF_AT && a + n <= SETTINGS_CONF_AT + SETTINGS_COUNT + 1)
    {
      eeprom_update_block(&s->frame[5], (void *)a, n);
      return;
    }
#endif
#if PORT_CONF_LEVEL >= PORT_LEVEL_WORK
    if (port_unlocked(s))
      eeprom_update_block(&s->frame[5], (void *)a, n);
#else
    s->status = PORT_RANGE;
#endif
    return;
  }
  if (op == 'A')
  {
#if defined(__SETTINGS_H__)
    if (n == 0xFF)
    {
      if (settings_apply)
        settings_apply();
      return;
    }
#endif
#ifdef PORT_ACTIONS
    if (n < PORT_ACTION_COUNT)
    {
      ((port_action_t)pgm_read_ptr(&port_actions[n]))();
      return;
    }
#endif
    s->status = PORT_RANGE;
    return;
  }
#endif

#if PORT_CONF_LEVEL >= PORT_LEVEL_WORK
  if ((op == 'W' && n <= 6) || op == 'C')
  {
    if (!port_unlocked(s))
      return;
    if (op == 'W')
    {
      memcpy((void *)a, &s->frame[5], n);
      return;
    }
    a = ((port_call_t)a)(port_word(s, 5), port_word(s, 7), port_word(s, 9));
    s->frame[5] = (uint8_t)a;
    s->frame[6] = (uint8_t)(a >> 8);
    port_answer(s, 'R', (uint16_t)&s->frame[5], 2);
    return;
  }
#endif

#if PORT_CONF_LEVEL >= PORT_LEVEL_LOAD
  if (op == 'B' || op == 'L' || op == 'G' || op == 'T')
  {
    uint8_t i;

    if (!port_unlocked(s))
      return;
    if (op == 'T')
    {
      s->frame[5] = port_slot((uint8_t)a, n);
      port_answer(s, 'R', (uint16_t)&s->frame[5], 1);
      return;
    }
    if (op == 'B')
    {
      i = port_slot_of(a);
      if (i == 0xFF)
        s->status = PORT_RANGE;
      else if (port_slot_busy(i))
        s->status = PORT_BUSY;
      else
      {
        s->page = a;
        port_page_clear();
      }
      return;
    }
    if (port_slot_of(s->page) == 0xFF)
    {
      s->status = PORT_RANGE; // no page was started
      return;
    }
    if (op == 'L')
    {
      if (n > 3 || a + 2 * n > SPM_PAGESIZE)
      {
        s->status = PORT_RANGE;
        return;
      }
      for (i = 0; i < n; i++)
        port_page_fill(s->page + a + 2 * i, port_word(s, 5 + 2 * i));
      return;
    }
    port_page_write(s->page);
    s->page = 0; // a page is programmed once
    return;
  }
#endif

  s->status = (op == 'R' || op == 'F' || op == 'E' || op == 'I' || op == 'e' || op == 'A' || op == 'W'
               || op == 'C' || op == 'B' || op == 'L' || op == 'G' || op == 'T')
                  ? PORT_LEVEL
                  : PORT_UNKNOWN;
}

// The byte in s->byte is the newest of the last twelve. True when they are
// a request: it has been carried out and an answer is due.
static uint8_t port_take(process_port_t *s)
{
  uint8_t i, x = 0;

  for (i = 0; i < PORT_FRAME - 1; i++)
  {
    s->frame[i] = s->frame[i + 1];
    x ^= s->frame[i];
  }
  s->frame[PORT_FRAME - 1] = s->byte;
  if (s->frame[0] != PORT_SYNC || (x ^ s->byte) != 0)
    return 0;

  s->frame[0] = 0; // a request is carried out once
#if PORT_CONF_LEVEL >= PORT_LEVEL_WORK && PORT_CONF_GUARD == PORT_GUARD_RESET
  if (s->open != 0)
    s->open = PORT_WINDOW_TICKS; // somebody is still at work
#endif
  port_do(s);
  if (s->status != PORT_OK)
    s->left = 0;
  s->check = PORT_CHECK ^ s->status;
  return 1;
}

// the next byte of the answer
static uint8_t port_give(process_port_t *s)
{
  uint8_t b;

  if (s->space == 'F')
    b = pgm_read_byte(s->at);
  else if (s->space == 'E')
    b = eeprom_read_byte((const uint8_t *)s->at);
  else
    b = *(const uint8_t *)s->at;
  s->at++;
  s->left--;
  s->check ^= b;
  return b;
}

PROCESS_THREAD(port, ev, data)
{
  process_port_t *self = PROCESS_SELF(port);

#if PORT_CONF_LEVEL >= PORT_LEVEL_WORK && PORT_CONF_GUARD == PORT_GUARD_RESET
  /* Above PROCESS_BEGIN: runs on every turn. The door closes with time. */
  if (ev == EVENT_TICK && data == NULL && self->open != 0)
    self->open--;
#else
  (void)ev;
  (void)data;
#endif

  PROCESS_BEGIN();

  /* Nobody on the part reads the tables above: they are for the PC, and
   * naming them here keeps them in the build. */
#if PORT_CONF_LEVEL >= PORT_LEVEL_WORK
  __asm__ __volatile__ ("" : : "r" (&port_calls[0]));
#endif
  __asm__ __volatile__ ("" : : "r" (&port_layout[0]), "r" (&port_modules[0]));

  /* Somebody at the board pulled its reset line: the door is open. */
  self->cause = PORT_CONF_RESET_CAUSE;
  PORT_CONF_RESET_CAUSE &= (uint8_t)~_BV(EXTRF);
#if PORT_CONF_LEVEL >= PORT_LEVEL_WORK && PORT_CONF_GUARD == PORT_GUARD_RESET
  if (self->cause & _BV(EXTRF))
    self->open = PORT_WINDOW_TICKS;
#endif

  process_connect(PORT_CONF_DEVICE, pt_process);
  process_connect(pt_process, PORT_CONF_DEVICE);

  while (1)
  {
    /* ---- a byte; the last twelve may be a request ---- */
    PROCESS_WAIT_EVENT_UNTIL(ipc_pipe_available(PROCESS_PIPEIN()) > 0);
    ipc_pipe_read(PROCESS_PIPEIN(), &self->byte, 1);
    if (!port_take(self))
      continue;

    /* ---- the answer: status, what was asked for, check byte ---- */
    self->byte = self->status;
    PROCESS_PIPE_WRITE(&self->byte, 1);
    while (self->left != 0)
    {
      self->byte = port_give(self);
      PROCESS_PIPE_WRITE(&self->byte, 1);
    }
    PROCESS_PIPE_WRITE(&self->check, 1);
  }

  PT_FINALLY(&pt_process->pt)

  PROCESS_END();
}

#ifdef __cplusplus
}
#endif

#endif /* __MOD_PORT_H__ */

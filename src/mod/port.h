// file: ./src/mod/port.h

#ifndef __MOD_PORT_H__
#define __MOD_PORT_H__

/* port: the door through which a program on a PC works on a running system.
 *
 * A shell on the part spends its flash on reading commands and on writing
 * text, and a small part has none to spare. The PC has both, and it has the
 * .elf file of the sketch, which holds every name and every address. So the
 * part keeps only a door with three things behind it,
 *
 *   read    memory, flash or EEPROM
 *   write   memory or EEPROM
 *   call    a function with up to three arguments
 *
 * and the shell runs on the PC (tools/port.mjs): "ps" walks the process
 * list by reading it, "stop" and "join" call the functions of the kernel,
 * and settings in EEPROM are read and written where they lie.
 *
 * How much of the door is open is a setting, PORT_CONF_LEVEL:
 *
 *   1  look     read only. A monitor: whoever is at the line can see
 *               everything and change nothing.
 *   2  change   also write memory and EEPROM: settings, and fields of a
 *               process.
 *   3  work     also call functions: the whole shell. The default.
 *
 * - The port is a process. It takes one byte per turn, like the shell, and
 *   a function it calls runs in its turn like one the shell would call.
 * - The line is the device PORT_CONF_DEVICE: softserial_proc when the
 *   sketch included <sys/process/softserial.h> before this header,
 *   serial_process0_proc otherwise.
 * - PORT_MODULES_BEGIN ... PORT_MODULES_END list the modules the PC may
 *   start, each with the instance it runs in, as SHELL_MODULES does for the
 *   shell on the part. Without the list the PC sees what is running.
 * - One port, port_proc, defined here: include this file in one source
 *   file only.
 * - A port at level 2 or 3 is a debug port. Whoever reaches the line can
 *   do anything to the part.
 *
 * A request is twelve bytes, and so is every request:
 *
 *   'p'  op  n  a.lo a.hi  d0 d1 d2 d3 d4 d5  check
 *
 *   'R' 'F' 'E'  answer with n bytes from memory, flash or EEPROM at a
 *   'W' 'e'      write the n bytes d0.. (at most six) to memory or EEPROM at a
 *   'C'          call the function at word address a with the three
 *                16-bit arguments in d; answer with its 16-bit result
 *
 * The check byte makes the twelve bytes add up to zero under exclusive or.
 * The port looks at the last twelve bytes it received, whatever came
 * before, so a request that was damaged or cut short costs nothing but
 * itself, and nothing is written from a request that does not check out.
 * The answer is the bytes asked for and one check byte: their exclusive or
 * with PORT_CHECK. An unknown request has no answer.
 */

#include <string.h>
#include <sys/process.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

/* ------------------------------------------------------------------ */
/* Configuration (define in the sketch, before this header)            */
/* ------------------------------------------------------------------ */

#define PORT_LEVEL_LOOK   1
#define PORT_LEVEL_CHANGE 2
#define PORT_LEVEL_WORK   3

#ifndef PORT_CONF_LEVEL
#define PORT_CONF_LEVEL PORT_LEVEL_WORK
#endif

#ifndef PORT_CONF_DEVICE
#if defined(__PROCESS_SOFTSERIAL_H__)
#define PORT_CONF_DEVICE (&softserial_proc.base)
#else
#include <sys/process/serial.h>
#define PORT_CONF_DEVICE (&serial_process0_proc.base)
#endif
#endif

// bytes of memory the PC may use as its own: for text it writes into a
// pipe, or for a page of flash.
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

#ifdef __cplusplus
extern "C" {
#endif

#define PORT_SYNC  'p'
#define PORT_FRAME 12
#define PORT_CHECK 0xA5
#define PORT_ABI   1

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
/* Views: what the PC shows of this sketch under a name of its own     */
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
 * The texts are kept in the .elf file and cost the part nothing.
 */
#define PORT_STR_(x) #x
#define PORT_STR(x)  PORT_STR_(x)
#define PORT_VIEW(name, what, help) \
  __asm__(".pushsection .protoduino.notes,\"\",@progbits\n.asciz \"view\\t" #name "\\t" what "\\t" help "\"\n.popsection\n")

/* ------------------------------------------------------------------ */
/* The process                                                         */
/* ------------------------------------------------------------------ */

PROCESS_DEFINE(port, "port", 2,
  uint8_t  frame[PORT_FRAME];           /* the last twelve bytes received  */
  uint16_t at;                          /* where the answer has got to     */
  uint8_t  left;                        /* bytes of it still to come       */
  uint8_t  space;                       /* the memory they come from       */
  uint8_t  check;                       /* check byte of the answer so far */
  uint8_t  byte;
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
  PORT_CONF_SCRATCH
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

// The byte in s->byte is the newest of the last twelve. When they are a
// request, carry it out as far as that takes no waiting, and return 1: an
// answer is due.
static uint8_t port_take(process_port_t *s)
{
  uint8_t i, x = 0, op, n;
  uint16_t a;

  for (i = 0; i < PORT_FRAME - 1; i++)
  {
    s->frame[i] = s->frame[i + 1];
    x ^= s->frame[i];
  }
  s->frame[PORT_FRAME - 1] = s->byte;
  if (s->frame[0] != PORT_SYNC || (x ^ s->byte) != 0)
    return 0;

  s->frame[0] = 0; // a request is carried out once
  op = s->frame[1];
  n  = s->frame[2];
  a  = port_word(s, 3);
  s->check = PORT_CHECK;
  s->left  = 0;

  if (op == 'R' || op == 'F' || op == 'E')
  {
    s->space = op;
    s->at    = a;
    s->left  = n;
    return 1;
  }
#if PORT_CONF_LEVEL >= PORT_LEVEL_CHANGE
  if (n <= 6 && op == 'W')
  {
    memcpy((void *)a, &s->frame[5], n);
    return 1;
  }
  if (n <= 6 && op == 'e')
  {
    eeprom_update_block(&s->frame[5], (void *)a, n);
    return 1;
  }
#endif
#if PORT_CONF_LEVEL >= PORT_LEVEL_WORK
  if (op == 'C')
  {
    a = ((port_call_t)a)(port_word(s, 5), port_word(s, 7), port_word(s, 9));
    s->frame[5] = (uint8_t)a;
    s->frame[6] = (uint8_t)(a >> 8);
    s->space = 'R';
    s->at    = (uint16_t)&s->frame[5];
    s->left  = 2;
    return 1;
  }
#endif
  return 0;
}

// the next byte of the answer; s->space is 0 after the last one
static uint8_t port_give(process_port_t *s)
{
  uint8_t b;

  if (s->left == 0)
  {
    s->space = 0;
    return s->check;
  }
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
  (void)ev;
  (void)data;

  PROCESS_BEGIN();

  /* Nobody on the part reads the tables above: they are for the PC, and
   * naming them here keeps them in the build. */
#if PORT_CONF_LEVEL >= PORT_LEVEL_WORK
  __asm__ __volatile__ ("" : : "r" (&port_calls[0]));
#endif
  __asm__ __volatile__ ("" : : "r" (&port_layout[0]), "r" (&port_modules[0]));

  process_connect(PORT_CONF_DEVICE, pt_process);
  process_connect(pt_process, PORT_CONF_DEVICE);

  while (1)
  {
    /* ---- a byte; the last twelve may be a request ---- */
    PROCESS_WAIT_EVENT_UNTIL(ipc_pipe_available(PROCESS_PIPEIN()) > 0);
    ipc_pipe_read(PROCESS_PIPEIN(), &self->byte, 1);
    if (!port_take(self))
      continue;

    /* ---- its answer, and the check byte behind it ---- */
    do
    {
      self->byte = port_give(self);
      PROCESS_PIPE_WRITE(&self->byte, 1);
    }
    while (self->space != 0);
  }

  PT_FINALLY(&pt_process->pt)

  PROCESS_END();
}

#ifdef __cplusplus
}
#endif

#endif /* __MOD_PORT_H__ */

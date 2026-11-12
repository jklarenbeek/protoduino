// file: ./src/mod/shell.h

#ifndef __MOD_SHELL_H__
#define __MOD_SHELL_H__

/* shell: a command line for the processes of a running system.
 *
 * The shell is a process like any other. It reads a line from its input,
 * carries it out and writes the answer to its output. Only the first letter
 * of a command counts, so "ps" and "p" are the same.
 *
 *   ps                 processes: name, state, priority
 *                      I started  R running  W waiting  P paused  X ending
 *   stop <name>        pause a process: it gets no events until "cont"
 *   cont <name>        let a paused process continue
 *   kill <name>        end a process
 *   mods               modules that can be run: number, name, + when running
 *   run <number>       start a module
 *   ipc                connections: "ticker > blink"
 *   join <a> <b>       connect the output of a to the input of b
 *   unjoin <a>         take the output of a away
 *   write <name> text  write text to the input of a process
 *   log                what the kernel logged: name, severity, event, code
 *
 * - The modules are the process types the sketch lists between
 *   SHELL_MODULES_BEGIN and SHELL_MODULES_END, each with the instance it
 *   runs in. Modules in flash slots follow them when SHELL_CONF_SLOTS is set
 *   (see ./src/sys/module.h).
 * - The line the shell talks on is the device SHELL_CONF_DEVICE, by default
 *   serial_process0_proc. The device has to be started before the shell.
 * - Give the shell to process_init() as the logger and "log" has something
 *   to show.
 * - Needs the tick (sys/tick.h): its answers wait for room in the pipe.
 * - No static variables.
 */

#include <sys/process.h>
#include <avr/pgmspace.h>

#ifndef SHELL_CONF_LINE
#define SHELL_CONF_LINE 24
#endif

#ifndef SHELL_CONF_ECHO
#define SHELL_CONF_ECHO 1
#endif

#ifndef SHELL_CONF_DEVICE
#include <sys/process/serial.h>
#define SHELL_CONF_DEVICE (&serial_process0_proc.base)
#endif

/* ------------------------------------------------------------------ */
/* The modules a shell can run                                         */
/* ------------------------------------------------------------------ */

typedef struct shell_module {
  const process_type_t *type;     /* what to run        */
  struct process       *instance; /* where it runs      */
} shell_module_t;

extern const shell_module_t shell_modules[];
extern const uint8_t        shell_module_count;

#define SHELL_MODULES_BEGIN const shell_module_t shell_modules[] CC_PROGMEM = {
#define SHELL_MODULE(name, instance) { &process_type_##name, &(instance).base },
#define SHELL_MODULES_END \
  }; \
  const uint8_t shell_module_count = sizeof(shell_modules) / sizeof(shell_modules[0]);

/* ------------------------------------------------------------------ */
/* The process                                                         */
/* ------------------------------------------------------------------ */

PROCESS_DEFINE(shell, "shell", 2,
  char            line[SHELL_CONF_LINE]; /* the command                    */
  char            out[SHELL_CONF_LINE];  /* one line of the answer         */
  uint8_t         len;                   /* characters in line             */
  uint8_t         n;                     /* characters in out              */
  uint8_t         i;                     /* where a list has got to        */
  uint8_t         byte;
  struct process *it;                    /* where a walk has got to        */
  const char     *text;                  /* where a text in flash has got to */
);

static const char shell_help[] CC_PROGMEM =
  "ps stop cont kill mods run ipc join unjoin write log\r\n";
static const char shell_states[] CC_PROGMEM = "?IRWXP??";

static void shell_put(process_shell_t *s, char c)
{
  if (s->n < sizeof(s->out))
    s->out[s->n++] = c;
}

static void shell_eol(process_shell_t *s)
{
  shell_put(s, '\r');
  shell_put(s, '\n');
}

static void shell_hex(process_shell_t *s, uint8_t v)
{
  uint8_t d = v >> 4;
  shell_put(s, (char)(d < 10 ? '0' + d : 'a' - 10 + d));
  d = v & 15;
  shell_put(s, (char)(d < 10 ? '0' + d : 'a' - 10 + d));
}

// a text in flash, such as the name of a process
static void shell_text(process_shell_t *s, const char *text)
{
  char c;

  if (text == NULL)
  {
    shell_put(s, '?');
    return;
  }
  while ((c = (char)pgm_read_byte(text++)) != 0)
    shell_put(s, c);
}

// the n-th word of the line, 0 being the command; its length, 0 if none
static uint8_t shell_word(const process_shell_t *s, uint8_t n, const char **word)
{
  const char *p = s->line;
  uint8_t len;

  for (;;)
  {
    while (*p == ' ')
      p++;
    for (len = 0; p[len] != 0 && p[len] != ' '; len++) { }
    if (n-- == 0 || len == 0)
      break;
    p += len;
  }
  *word = p;
  return len;
}

// the process the n-th word names
static struct process *shell_process(const process_shell_t *s, uint8_t n)
{
  const char *word;
  uint8_t len = shell_word(s, n, &word);

  return len ? process_lookup_n(word, len) : NULL;
}

// the pipe a process writes into, and the one that is written to reach it.
// A device publishes them the other way round (see process_connect()).
static ipc_pipe_t *shell_pipe(const struct process *p, uint8_t into)
{
  if ((p->type == PROCESS_TYPE_DEVICE) != (into != 0))
    return p->has_pipeout ? p->pipeout : NULL;
  return p->has_pipein ? p->pipein : NULL;
}

// a module of the list
static const process_type_t *shell_module(uint8_t i, struct process **instance)
{
  *instance = (struct process *)pgm_read_ptr(&shell_modules[i].instance);
  return (const process_type_t *)pgm_read_ptr(&shell_modules[i].type);
}

#define SHELL_WRITE_OUT() \
  do { \
    PROCESS_PIPE_WRITE(self->out, self->n); \
    self->n = 0; \
  } while (0)

PROCESS_THREAD(shell, ev, data)
{
  process_shell_t *self = PROCESS_SELF(shell);
  uint8_t cmd;
  (void)ev;
  (void)data;

  PROCESS_BEGIN();

  /* What the device receives is the input of the shell, and the shell
   * writes to what the device transmits. */
  process_connect(SHELL_CONF_DEVICE, pt_process);
  process_connect(pt_process, SHELL_CONF_DEVICE);

  while (1)
  {
    shell_put(self, '>');
    shell_put(self, ' ');
    SHELL_WRITE_OUT();

    /* ---- a line ---- */
    self->len = 0;
    while (1)
    {
      PROCESS_WAIT_EVENT_UNTIL(ipc_pipe_available(PROCESS_PIPEIN()) > 0);
      ipc_pipe_read(PROCESS_PIPEIN(), &self->byte, 1);

      if (self->byte == '\r' || self->byte == '\n')
        break;
      if (self->byte == 8 || self->byte == 127)
      {
        if (self->len > 0)
          self->len--;
      }
      else if (self->len < sizeof(self->line) - 1)
      {
        self->line[self->len++] = (char)self->byte;
      }
#if SHELL_CONF_ECHO
      PROCESS_PIPE_WRITE(&self->byte, 1);
#endif
    }
    self->line[self->len] = 0;
    shell_eol(self);
    SHELL_WRITE_OUT();

    if (self->len == 0)
      continue;

    /* ---- the command: lists write their own lines, the others end in
     * "ok" or "?". No switch: a protothread cannot wait inside one. ---- */
    {
      const char *word;
      shell_word(self, 0, &word);
      cmd = (uint8_t)word[0];
    }
    self->i = 0;

    if (cmd == 'p')
    {
      for (self->it = process_next(NULL); self->it != NULL; self->it = process_next(self->it))
      {
        shell_text(self, self->it->name);
        while (self->n < 9)
          shell_put(self, ' ');
        shell_put(self, (char)pgm_read_byte(&shell_states[self->it->state]));
        shell_put(self, ' ');
        shell_put(self, (char)('0' + self->it->prio));
        shell_eol(self);
        SHELL_WRITE_OUT();
      }
      continue;
    }

    if (cmd == 'm')
    {
      for (self->i = 0; self->i < shell_module_count; self->i++)
      {
        struct process *instance;
        const process_type_t *type = shell_module(self->i, &instance);

        shell_put(self, (char)('0' + self->i));
        shell_put(self, ' ');
        shell_text(self, (const char *)pgm_read_ptr(&type->name));
        shell_put(self, ' ');
        shell_put(self, instance->state != PROCESS_STATE_NONE ? '+' : '-');
        shell_eol(self);
        SHELL_WRITE_OUT();
      }
      continue;
    }

    if (cmd == 'i')
    {
      for (self->it = process_next(NULL); self->it != NULL; self->it = process_next(self->it))
      {
        ipc_pipe_t *pipe = shell_pipe(self->it, 1);
        struct process *reader = pipe ? process_reader(pipe) : NULL;

        if (reader == NULL || reader == self->it)
          continue;
        shell_text(self, self->it->name);
        shell_put(self, ' ');
        shell_put(self, '>');
        shell_put(self, ' ');
        shell_text(self, reader->name);
        shell_eol(self);
        SHELL_WRITE_OUT();
      }
      continue;
    }

    if (cmd == 'l')
    {
      for (self->i = 0; process_log_entry(self->i) != NULL; self->i++)
      {
        const struct error_info *e = process_log_entry(self->i);

        shell_text(self, e->source ? e->source->name : NULL);
        shell_put(self, ' ');
        shell_put(self, (char)('0' + e->severity));
        shell_put(self, ' ');
        shell_hex(self, e->event);
        shell_put(self, ' ');
        shell_hex(self, e->error);
        shell_eol(self);
        SHELL_WRITE_OUT();
      }
      continue;
    }

    if (cmd == 'h' || cmd == '?')
    {
      for (self->text = shell_help; (self->byte = pgm_read_byte(self->text)) != 0; self->text++)
        PROCESS_PIPE_WRITE(&self->byte, 1);
      continue;
    }

    /* ---- commands on one process ---- */
    self->it = shell_process(self, 1);

    if (cmd == 'r')
    {
      const char *word;
      uint8_t number = shell_word(self, 1, &word) ? (uint8_t)(word[0] - '0') : 255;

      if (number < shell_module_count)
      {
        struct process *instance;
        const process_type_t *type = shell_module(number, &instance);

        // process_new() clears the instance: never on one that runs
        if (instance->state == PROCESS_STATE_NONE)
          self->i = process_new(type, instance) != NULL;
      }
    }
    else if (self->it == NULL)
    {
      /* nobody by that name */
    }
    else if (cmd == 's')
    {
      self->i = (uint8_t)process_pause(self->it);
    }
    else if (cmd == 'c')
    {
      self->i = (uint8_t)process_resume(self->it);
    }
    else if (cmd == 'k')
    {
      if (self->it != pt_process && self->it->type != PROCESS_TYPE_DEVICE)
      {
        process_resume(self->it);
        process_destroy(self->it);
        self->i = 1;
      }
    }
    else if (cmd == 'j')
    {
      self->i = (uint8_t)process_connect(self->it, shell_process(self, 2));
    }
    else if (cmd == 'u')
    {
      process_disconnect(self->it);
      self->i = 1;
    }
    else if (cmd == 'w')
    {
      ipc_pipe_t *pipe = shell_pipe(self->it, 0);
      const char *word;
      uint8_t len = shell_word(self, 2, &word);

      if (pipe != NULL && len > 0)
      {
        ipc_pipe_write(pipe, (const uint8_t *)word, strlen(word));
        self->i = 1;
      }
    }

    if (self->i)
    {
      shell_put(self, 'o');
      shell_put(self, 'k');
    }
    else
    {
      shell_put(self, '?');
    }
    shell_eol(self);
    SHELL_WRITE_OUT();
  }

  PT_FINALLY(&pt_process->pt)

  PROCESS_END();
}

#endif /* __MOD_SHELL_H__ */

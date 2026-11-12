// file: ./src/mod/keeper.h

#ifndef __MOD_KEEPER_H__
#define __MOD_KEEPER_H__

/* keeper: looks after another board through its reset line.
 *
 *   pulse | keeper.beat      a sign of life from the board
 *   sense | limit | keeper   its supply: 1 while it is well, 0 when low
 *
 * The keeper holds the reset line of the board
 *
 *   - for a moment, when no sign of life came for `timeout` seconds,
 *   - for as long as the supply is low,
 *   - for a moment, when it reads an 'R' at its input: "write keeper R"
 *     from a shell or from the PC. A bootloader that waits for a sketch
 *     after a reset through the pin gets its reset this way, on a board
 *     that has no other means.
 *
 * After each release the board has `grace` seconds to start before signs
 * of life are expected; the first one ends the grace early. What the keeper
 * did is counted in three bytes of EEPROM at KEEPER_CONF_EEPROM: resets
 * after silence, times the supply was low, resets asked for.
 *
 * - It has two inputs. The supply and the commands share the one it
 *   publishes: a 0 and a 1 are the supply, an 'R' is a command. Signs of
 *   life go to `beat`, which is connected with process_connect_pipe().
 * - `keeper.timeout`, `keeper.grace` (seconds) and `keeper.hold` (ticks
 *   the line is held) are set in the instance.
 * - What the keeper does is in plain functions on a keeper_t: keeper_beat(),
 *   keeper_supply(), keeper_request() and keeper_tick().
 * - The reset line is KEEPER_CONF_RESET, bit 3 of port B, driven low or
 *   left alone, never driven high: the board has its own pull-up, and a
 *   programmer may pull the line too. KEEPER_CONF_LED, bit 4, is high
 *   while the board is alive and watched.
 * - A second is KEEPER_CONF_TICKS ticks: 64 with the watchdog at 16 ms.
 * - Needs the tick. No static variables.
 */

#include <sys/process.h>
#include <sys/tick.h>
#include <avr/io.h>
#include <avr/eeprom.h>

#ifndef KEEPER_CONF_RESET
#define KEEPER_CONF_RESET 3
#endif

#ifndef KEEPER_CONF_LED
#define KEEPER_CONF_LED 4
#endif

#ifndef KEEPER_CONF_TICKS
#define KEEPER_CONF_TICKS 64
#endif

#ifndef KEEPER_CONF_EEPROM
#define KEEPER_CONF_EEPROM 16
#endif

#define KEEPER_GRACE 0  /* released: the board is starting       */
#define KEEPER_WATCH 1  /* released: signs of life are expected  */
#define KEEPER_PULSE 2  /* held for a moment                     */
#define KEEPER_LOW   3  /* held while the supply is low          */

#define KEEPER_COUNT_SILENCE 0
#define KEEPER_COUNT_SUPPLY  1
#define KEEPER_COUNT_REQUEST 2

typedef struct keeper {
  uint8_t timeout;     /* seconds of silence before a reset          */
  uint8_t grace;       /* seconds a board may take to start          */
  uint8_t hold;        /* ticks the line is held for a reset         */
  uint8_t phase;
  uint8_t ticks;       /* ticks of this second, or of this pulse     */
  uint8_t seconds;     /* seconds of this phase, or of silence       */
} keeper_t;

static void keeper_count(uint8_t which)
{
  uint8_t *cell = (uint8_t *)(KEEPER_CONF_EEPROM + which);
  uint8_t n = eeprom_read_byte(cell);

  if (n == 0xFF)
    n = 0; // erased: nothing counted yet
  if (n < 0xFE)
    eeprom_update_byte(cell, (uint8_t)(n + 1));
}

static void keeper_enter(keeper_t *s, uint8_t phase)
{
  s->phase   = phase;
  s->ticks   = 0;
  s->seconds = 0;

  if (phase >= KEEPER_PULSE)
    DDRB |= _BV(KEEPER_CONF_RESET);            // low
  else
    DDRB &= (uint8_t)~_BV(KEEPER_CONF_RESET);  // left alone
  if (phase == KEEPER_WATCH)
    PORTB |= _BV(KEEPER_CONF_LED);
  else
    PORTB &= (uint8_t)~_BV(KEEPER_CONF_LED);
}

// the pins, and a board that is given time to start
static void keeper_start(keeper_t *s)
{
  PORTB &= (uint8_t)~_BV(KEEPER_CONF_RESET); // what the line is driven to, when it is driven
  DDRB  |= _BV(KEEPER_CONF_LED);
  keeper_enter(s, KEEPER_GRACE);
}

// a sign of life: the silence starts again
static void keeper_beat(keeper_t *s)
{
  if (s->phase <= KEEPER_WATCH)
    keeper_enter(s, KEEPER_WATCH);
}

// the supply: 1 well, 0 low
static void keeper_supply(keeper_t *s, uint8_t well)
{
  if (!well && s->phase != KEEPER_LOW)
  {
    keeper_count(KEEPER_COUNT_SUPPLY);
    keeper_enter(s, KEEPER_LOW);
  }
  else if (well && s->phase == KEEPER_LOW)
  {
    keeper_enter(s, KEEPER_GRACE);
  }
}

// somebody asks for a reset
static void keeper_request(keeper_t *s)
{
  if (s->phase == KEEPER_LOW)
    return;
  keeper_count(KEEPER_COUNT_REQUEST);
  keeper_enter(s, KEEPER_PULSE);
}

// a tick: what time has done
static void keeper_tick(keeper_t *s)
{
  if (s->phase == KEEPER_LOW)
    return;

  s->ticks++;
  if (s->phase == KEEPER_PULSE)
  {
    if (s->ticks >= s->hold)
      keeper_enter(s, KEEPER_GRACE);
    return;
  }
  if (s->ticks < KEEPER_CONF_TICKS)
    return;
  s->ticks = 0;
  s->seconds++;

  if (s->phase == KEEPER_GRACE)
  {
    if (s->seconds >= s->grace)
      keeper_enter(s, KEEPER_WATCH);
  }
  else if (s->seconds >= s->timeout)
  {
    keeper_count(KEEPER_COUNT_SILENCE);
    keeper_enter(s, KEEPER_PULSE);
  }
}

// what a byte at the input of a keeper means
static void keeper_input(keeper_t *s, uint8_t byte)
{
  if (byte <= 1)
    keeper_supply(s, byte);
  else if (byte == 'R')
    keeper_request(s);
}

PROCESS_DEFINE(keeper, "keeper", 2,
  ipc_pipe_t in;          /* the supply, and commands */
  ipc_pipe_t beat;        /* signs of life            */
  uint8_t    buf[2];
  uint8_t    beatbuf[2];
  keeper_t   keeper;
  uint8_t    byte;
);

PROCESS_THREAD(keeper, ev, data)
{
  process_keeper_t *self = PROCESS_SELF(keeper);

  PROCESS_BEGIN();

  ipc_pipe_init(&self->in, self->buf, sizeof(self->buf), process_ipc_wake, (void *)pt_process);
  PROCESS_SET_PIPEIN(&self->in);
  ipc_pipe_init(&self->beat, self->beatbuf, sizeof(self->beatbuf), process_ipc_wake, (void *)pt_process);
  keeper_start(&self->keeper);

  while (1)
  {
    PROCESS_WAIT_EVENT();

    while (ipc_pipe_read(&self->beat, &self->byte, 1) == 1)
      keeper_beat(&self->keeper);
    while (ipc_pipe_read(&self->in, &self->byte, 1) == 1)
      keeper_input(&self->keeper, self->byte);
    if (ev == EVENT_TICK && data == NULL)
      keeper_tick(&self->keeper);
  }

  PT_FINALLY(&pt_process->pt)
  {
    DDRB &= (uint8_t)~_BV(KEEPER_CONF_RESET);
  }

  PROCESS_END();
}

#endif /* __MOD_KEEPER_H__ */

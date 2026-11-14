// file: examples/6-attiny/51-keeper/51-keeper.ino

/*
 * 51-keeper.ino  –  a small chip that looks after a bigger board
 * ================================================================
 *
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   An ATtiny beside an Uno, a Mega or any other board, with one wire to
 *   its reset line and one to a pin the board toggles while it is well:
 *
 *     pulse | keeper.beat       the heartbeat of the board
 *     sense | limit | keeper    the supply, as the keeper's own ADC sees it
 *
 *   The keeper resets the board when the heartbeat stops, holds it in reset
 *   while the supply is low, and resets it when the PC says so:
 *
 *     node tools/port.mjs 51-keeper.elf --port /dev/ttyUSB0 restart
 *
 *   A bootloader that waits for a sketch after a reset through the pin,
 *   as the one in boot/ does, gets its reset this way on a board without
 *   an automatic one. What the keeper did, it remembers without power:
 *
 *     node tools/port.mjs 51-keeper.elf --port /dev/ttyUSB0 resets
 *     resets after a missed heartbeat: 2
 *     times the supply was low: 0
 *     resets asked for: 1
 *
 * THE SETTINGS
 * ------------
 *   The list below is what the keeper runs on. The values are built in,
 *   and each can be changed on the finished chip, where it is kept in
 *   EEPROM:
 *
 *     node tools/port.mjs 51-keeper.elf --port /dev/ttyUSB0 settings "set timeout 10"
 *
 *   What is fixed at build time has a #define with an #ifndef around it:
 *   set it on the command line of the build, or change it here.
 *
 * THE DOOR
 * --------
 *   Reading, the settings and "restart" are open to whoever is at the line.
 *   Everything else the port can do ("join", "write", any memory) opens
 *   for a minute after a reset of the keeper through its own reset pin.
 *
 * WIRING   (ATtiny85, 45 or 25 at 8 MHz, internal oscillator)
 *   PB0  serial in     to TX of a USB serial adapter, 9600 baud
 *   PB1  serial out    to RX of the adapter
 *   PB2  heartbeat     from a pin of the board that changes at least once per `timeout`
 *   PB3  reset         to the reset line of the board; pulled low, never driven high
 *   PB4  LED           on while the board is alive and watched
 *   The ATtiny is fed from the supply of the board: that is what it measures.
 */

#include <protoduino.h>
#include <sys/process.h>

/* ---- how much of it fits ------------------------------------------
 * A stage as a process of its own, with a pipe to the next, costs about 30
 * bytes of RAM. The same stage as a function costs what its state takes.
 * So the RAM of the part decides how this sketch is put together, and both
 * can be set here or on the command line of the build:
 *
 *   ATtiny85  512 bytes   every stage a process, and the port up to level 3
 *   ATtiny45  256 bytes   the stages as functions of one process, and the port
 *                         at level 2: settings and actions
 *   ATtiny25  128 bytes   the stages as functions of one process, no port:
 *                         it runs on the settings as built
 */
#ifndef KEEPER_CONF_FUSED
#if defined(__AVR_ATtiny85__)
#define KEEPER_CONF_FUSED 0
#else
#define KEEPER_CONF_FUSED 1
#endif
#endif

#ifndef KEEPER_CONF_PORT
#if defined(__AVR_ATtiny25__)
#define KEEPER_CONF_PORT 0
#else
#define KEEPER_CONF_PORT 1
#endif
#endif

#if KEEPER_CONF_PORT
#include <sys/process/softserial.h>
#endif

/* the watchdog ticks every 16 ms; between ticks the chip sleeps, with its
 * I/O clock on so that an edge of the heartbeat is noticed */
#define TICK_CONF_SOURCE TICK_SOURCE_WATCHDOG
#include <sys/tick.h>
#define PROCESS_CONF_SLEEP PROCESS_SLEEP_WAIT
#include <sys/idle.h>

#include <mod/pulse.h>
#include <mod/sense.h>
#include <mod/limit.h>
#include <mod/keeper.h>

#define SETTINGS(X) \
  X(timeout, 4,  "seconds without a heartbeat before the board is reset") \
  X(grace,   10, "seconds a board may take to start") \
  X(hold,    6,  "ticks of 16 ms the reset line is held") \
  X(low,     43, "tenths of a volt: at or below, the supply is low") \
  X(well,    45, "tenths of a volt: at or above, it is well again") \
  X(period,  16, "ticks between two looks at the supply")
#include <sys/settings.h>

#if KEEPER_CONF_PORT
/* 1 look, 2 also settings and actions, 3 also anything: behind the guard */
#ifndef PORT_CONF_LEVEL
#if defined(__AVR_ATtiny85__)
#define PORT_CONF_LEVEL PORT_LEVEL_WORK
#else
#define PORT_CONF_LEVEL PORT_LEVEL_SETTINGS /* the 4 KB of an ATtiny45 hold no more */
#endif
#endif
static void restart_board(void);
#define PORT_ACTIONS(X) X(restart, restart_board, "reset the board through its reset line now")
#include <mod/port.h>

PORT_VIEW(resets,
  "byte e 16 raw resets after a missed heartbeat;"
  "byte e 17 raw times the supply was low;"
  "byte e 18 raw resets asked for",
  "what the keeper did to the board");
#endif

#if KEEPER_CONF_FUSED

/* ---- one process: the stages are its functions --------------------- */

PROCESS_DEFINE(guard, "keeper", 2,
#if KEEPER_CONF_PORT
  ipc_pipe_t in;        /* commands from the PC: "write keeper R" */
  uint8_t    buf[2];
#endif
  keeper_t   keeper;
  sense_t    sense;
  limit_t    limit;
  uint8_t    period;    /* ticks between two looks at the supply  */
  uint8_t    ticks;
  uint8_t    byte;
);

PROCESS_INSTANCE(guard, guard_proc);

#define THE_KEEPER (guard_proc.keeper)
#define THE_LIMIT  (guard_proc.limit)
#define THE_SENSE  (guard_proc.sense)
#define THE_PERIOD (guard_proc.period)

PROCESS_THREAD(guard, ev, data)
{
  process_guard_t *self = PROCESS_SELF(guard);

  PROCESS_BEGIN();

#if KEEPER_CONF_PORT
  ipc_pipe_init(&self->in, self->buf, sizeof(self->buf), process_ipc_wake, (void *)pt_process);
  PROCESS_SET_PIPEIN(&self->in);
#endif
  pulse_start();
  keeper_start(&self->keeper);

  while (1)
  {
    PROCESS_WAIT_EVENT();

#if KEEPER_CONF_PORT
    while (ipc_pipe_read(&self->in, &self->byte, 1) == 1)
      keeper_input(&self->keeper, self->byte);
#endif
    if (!(ev == EVENT_TICK && data == NULL))
      continue;

    /* pulse | keeper.beat */
    if (pulse_seen())
      keeper_beat(&self->keeper);

    /* sense | limit | keeper */
    if (++self->ticks >= self->period)
    {
      self->ticks = 0;
      if (limit_step(&self->limit, sense_read(&self->sense)))
        keeper_supply(&self->keeper, self->limit.is);
    }

    keeper_tick(&self->keeper);
  }

  PT_FINALLY(&pt_process->pt)

  PROCESS_END();
}

#else

/* ---- a process per stage, and pipes between them -------------------- */

PROCESS_INSTANCE(pulse, pulse_proc);
PROCESS_INSTANCE(sense, sense_proc);
PROCESS_INSTANCE(limit, limit_proc);
PROCESS_INSTANCE(keeper, keeper_proc);

#define THE_KEEPER (keeper_proc.keeper)
#define THE_LIMIT  (limit_proc.limit)
#define THE_SENSE  (sense_proc.sense)
#define THE_PERIOD (sense_proc.period)

static bool connected;

#endif

/* the action the PC may run at level 2 */
static void restart_board(void)
{
  keeper_request(&THE_KEEPER);
}

/* the settings, to where they are used; the PC calls this after a change */
void settings_apply(void)
{
  THE_KEEPER.timeout = setting(SETTING_timeout);
  THE_KEEPER.grace   = setting(SETTING_grace);
  THE_KEEPER.hold    = setting(SETTING_hold);
  THE_LIMIT.off      = setting(SETTING_low);
  THE_LIMIT.on       = setting(SETTING_well);
  THE_PERIOD         = setting(SETTING_period);
}

void setup(void)
{
  process_init(NULL);

#if KEEPER_CONF_PORT
  process_new(&process_type_softserial, &softserial_proc);
  process_new(&process_type_port, &port_proc);
#endif
#if KEEPER_CONF_FUSED
  process_new(&process_type_guard, &guard_proc);
#else
  process_new(&process_type_keeper, &keeper_proc);
  process_new(&process_type_pulse, &pulse_proc);
  process_new(&process_type_sense, &sense_proc);
  process_new(&process_type_limit, &limit_proc);
#endif

  sense_supply(&THE_SENSE);
  settings_apply();
  /* time comes from the watchdog here: the clock of the core can stop */
  idle_stop_core_clock();
  idle_start();
  tick_start();
}

void loop(void)
{
  process_loop(); /* the scheduler, the tick, and sleep until there is work */

#if !KEEPER_CONF_FUSED
  /* the wiring, once the readers have their pipes */
  if (!connected)
    connected = process_connect(&sense_proc.base, &limit_proc.base)
             && process_connect(&limit_proc.base, &keeper_proc.base)
             && process_connect_pipe(&pulse_proc.base, &keeper_proc.beat);
#endif
}

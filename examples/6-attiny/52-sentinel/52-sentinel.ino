// file: examples/6-attiny/52-sentinel/52-sentinel.ino

/*
 * 52-sentinel.ino  –  asleep nearly always, and it remembers
 * ================================================================
 *
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   A chip on a coin cell that you leave somewhere: in a freezer, a
 *   greenhouse, a parcel. It wakes when its watchdog says so, measures the
 *   temperature of its own chip and its supply, writes both into EEPROM,
 *   and sleeps again with everything but the watchdog switched off.
 *
 *     sense | record        the temperature
 *     sense | record        the supply
 *
 *   When you come back with a serial adapter, it answers. The first bit of
 *   a request wakes it:
 *
 *     node tools/port.mjs 52-sentinel.elf --port /dev/ttyUSB0 temperature supply
 *     lowest -18 C, highest 6 C, 24 samples, oldest first:
 *     -17 C  -18 C  -18 C  ...
 *     lowest 2.9 V, highest 3.0 V, 24 samples, oldest first:
 *     3.0 V  3.0 V  2.9 V  ...
 *
 *   Nothing here needs a part or a pin beside the serial line: both sensors
 *   are inside the chip.
 *
 * THE SETTINGS
 * ------------
 *   `period` is the number of watchdog ticks between two measurements and
 *   can be changed on the finished chip ("set period 75"). With the tick at
 *   8 s, the default of 8 is about a minute and 75 is ten minutes.
 *
 *   Fixed at build time: the tick (TICK_CONF_WATCHDOG) and the number of
 *   samples kept of each (SENTINEL_CONF_SAMPLES). Two records of 24 fit the
 *   128 bytes of EEPROM of an ATtiny25.
 *
 * WIRING   (ATtiny85, 45 or 25 at 8 MHz, internal oscillator)
 *   PB0  serial in     to TX of a USB serial adapter, 9600 baud
 *   PB1  serial out    to RX of the adapter
 *   Without the port, as on an ATtiny25, the records are read from EEPROM
 *   with a programmer: lowest, highest, then the samples.
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
#ifndef SENTINEL_CONF_FUSED
#if defined(__AVR_ATtiny85__)
#define SENTINEL_CONF_FUSED 0
#else
#define SENTINEL_CONF_FUSED 1
#endif
#endif

#ifndef SENTINEL_CONF_PORT
#if defined(__AVR_ATtiny25__)
#define SENTINEL_CONF_PORT 0
#else
#define SENTINEL_CONF_PORT 1
#endif
#endif

#if SENTINEL_CONF_PORT
#include <sys/process/softserial.h>
#endif

/* the watchdog is the only clock: a tick every 8 s, and between ticks the
 * deepest sleep the chip has */
#define TICK_CONF_SOURCE TICK_SOURCE_WATCHDOG
#ifndef TICK_CONF_WATCHDOG
#define TICK_CONF_WATCHDOG WDTO_8S
#endif
#define PROCESS_CONF_SLEEP PROCESS_SLEEP_DEEP
#include <sys/tick.h>
#include <sys/idle.h>

#include <mod/sense.h>
#include <mod/record.h>

#define SETTINGS(X) \
  X(period, 8, "ticks of the watchdog between two measurements")
#include <sys/settings.h>

/* samples kept of each, and where: the settings take the first bytes */
#ifndef SENTINEL_CONF_SAMPLES
#define SENTINEL_CONF_SAMPLES 24
#endif
#define SENTINEL_TEMPERATURE 8
#define SENTINEL_SUPPLY      (SENTINEL_TEMPERATURE + 2 + SENTINEL_CONF_SAMPLES)

#if SENTINEL_CONF_PORT
/* 1 look, 2 also settings, 3 also anything: behind the guard, open for a
 * minute after a reset through the reset pin */
#ifndef PORT_CONF_LEVEL
#if defined(__AVR_ATtiny85__)
#define PORT_CONF_LEVEL PORT_LEVEL_WORK
#else
#define PORT_CONF_LEVEL PORT_LEVEL_SETTINGS /* the 4 KB of an ATtiny45 hold no more */
#endif
#endif
#include <mod/port.h>

PORT_VIEW(temperature, "ring " PORT_STR(SENTINEL_TEMPERATURE) " " PORT_STR(SENTINEL_CONF_SAMPLES) " celsius",
          "the temperature of the chip: lowest, highest, the last samples");
PORT_VIEW(supply, "ring " PORT_STR(SENTINEL_SUPPLY) " " PORT_STR(SENTINEL_CONF_SAMPLES) " tenths",
          "the supply voltage: lowest, highest, the last samples");
#endif

#if SENTINEL_CONF_FUSED

/* ---- one process: the stages are its functions --------------------- */

PROCESS_DEFINE(sentinel, "sentinel", 3,
  sense_t  temperature;
  record_t temperature_record;
  sense_t  supply;
  record_t supply_record;
  uint8_t  period;
  uint8_t  ticks;
);

PROCESS_INSTANCE(sentinel, sentinel_proc);

#define THE_TEMPERATURE        (sentinel_proc.temperature)
#define THE_TEMPERATURE_RECORD (sentinel_proc.temperature_record)
#define THE_SUPPLY             (sentinel_proc.supply)
#define THE_SUPPLY_RECORD      (sentinel_proc.supply_record)

PROCESS_THREAD(sentinel, ev, data)
{
  process_sentinel_t *self = PROCESS_SELF(sentinel);

  PROCESS_BEGIN();

  record_open(&self->temperature_record);
  record_open(&self->supply_record);

  while (1)
  {
    PROCESS_WAIT_TICKS(self->ticks, self->period);

    /* sense | record, twice */
    record_put(&self->temperature_record, sense_read(&self->temperature));
    record_put(&self->supply_record, sense_read(&self->supply));
  }

  PT_FINALLY(&pt_process->pt)

  PROCESS_END();
}

#else

/* ---- a process per stage, and pipes between them -------------------- */

PROCESS_INSTANCE(sense, temperature_proc);
PROCESS_INSTANCE(record, temperature_record_proc);
PROCESS_INSTANCE(sense, supply_proc);
PROCESS_INSTANCE(record, supply_record_proc);

#define THE_TEMPERATURE        (temperature_proc.sense)
#define THE_TEMPERATURE_RECORD (temperature_record_proc.record)
#define THE_SUPPLY             (supply_proc.sense)
#define THE_SUPPLY_RECORD      (supply_record_proc.record)

static bool connected;

#endif

void settings_apply(void)
{
#if SENTINEL_CONF_FUSED
  sentinel_proc.period = setting(SETTING_period);
#else
  temperature_proc.period = setting(SETTING_period);
  supply_proc.period      = setting(SETTING_period);
#endif
}

void setup(void)
{
  process_init(NULL);

#if SENTINEL_CONF_PORT
  process_new(&process_type_softserial, &softserial_proc);
  process_new(&process_type_port, &port_proc);
#endif
#if SENTINEL_CONF_FUSED
  process_new(&process_type_sentinel, &sentinel_proc);
#else
  process_new(&process_type_sense, &temperature_proc);
  process_new(&process_type_record, &temperature_record_proc);
  process_new(&process_type_sense, &supply_proc);
  process_new(&process_type_record, &supply_record_proc);
#endif

  sense_chip(&THE_TEMPERATURE);
  THE_TEMPERATURE_RECORD.at   = SENTINEL_TEMPERATURE;
  THE_TEMPERATURE_RECORD.size = SENTINEL_CONF_SAMPLES;
  sense_supply(&THE_SUPPLY);
  THE_SUPPLY_RECORD.at   = SENTINEL_SUPPLY;
  THE_SUPPLY_RECORD.size = SENTINEL_CONF_SAMPLES;

  settings_apply();
  /* time comes from the watchdog here: the clock of the core can stop */
  idle_stop_core_clock();
  idle_start();
  tick_start();
}

void loop(void)
{
  process_loop(); /* the scheduler, the tick, and sleep until there is work */

#if !SENTINEL_CONF_FUSED
  if (!connected)
    connected = process_connect(&temperature_proc.base, &temperature_record_proc.base)
             && process_connect(&supply_proc.base, &supply_record_proc.base);
#endif
}

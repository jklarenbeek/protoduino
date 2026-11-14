// file: examples/6-attiny/53-regulator/53-regulator.ino

/*
 * 53-regulator.ino  –  a signal in, a smooth answer out
 * ================================================================
 *
 * WHAT THIS DEMONSTRATES
 * ----------------------
 *   The chain a small chip is made for: measure, calm the measurement
 *   down, decide, act.
 *
 *     sense | smooth | curve | pwm
 *
 *   As built it is a fan that follows a temperature: a sensor on PB3 (an
 *   NTC and a resistor as a divider, or a potentiometer to try it), a fan
 *   or a lamp on PB4 through a transistor. Below `lo` the fan stands
 *   still, at `lo` it starts with `min`, at `hi` it runs with `max`, and
 *   `shift` says how calmly it follows.
 *
 *   Each stage is a module of its own. Take `curve` out and the output
 *   follows the input directly; put `limit` in its place and it is a
 *   thermostat; change what `sense` measures and it regulates on the
 *   supply or on the temperature of the chip.
 *
 * THE SETTINGS
 * ------------
 *   All of them can be changed while the fan runs, and are kept in EEPROM:
 *
 *     node tools/port.mjs 53-regulator.elf --port /dev/ttyUSB0 settings "set lo 90" duty
 *
 * WIRING   (ATtiny85, 45 or 25 at 8 MHz, internal oscillator)
 *   PB0  serial in     to TX of a USB serial adapter, 9600 baud
 *   PB1  serial out    to RX of the adapter
 *   PB3  sensor        0 V to the supply; ADC channel 3
 *   PB4  output        PWM at 31 kHz, high is on
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
#ifndef REGULATOR_CONF_FUSED
#if defined(__AVR_ATtiny85__)
#define REGULATOR_CONF_FUSED 0
#else
#define REGULATOR_CONF_FUSED 1
#endif
#endif

#ifndef REGULATOR_CONF_PORT
#if defined(__AVR_ATtiny25__)
#define REGULATOR_CONF_PORT 0
#else
#define REGULATOR_CONF_PORT 1
#endif
#endif

#if REGULATOR_CONF_PORT
#include <sys/process/softserial.h>
#endif

/* the watchdog ticks every 16 ms; between ticks the processor sleeps and
 * the timer that makes the PWM runs on */
#define TICK_CONF_SOURCE TICK_SOURCE_WATCHDOG
#include <sys/tick.h>
#define PROCESS_CONF_SLEEP PROCESS_SLEEP_WAIT
#include <sys/idle.h>

#include <mod/sense.h>
#include <mod/smooth.h>
#include <mod/curve.h>
#include <mod/pwm.h>

/* the ADC channel of the sensor: 3 is PB3 */
#ifndef REGULATOR_CONF_CHANNEL
#define REGULATOR_CONF_CHANNEL 3
#endif

#define SETTINGS(X) \
  X(lo,     80,  "input at which the output starts, 0 to 255") \
  X(hi,     180, "input at which it is at its highest") \
  X(min,    64,  "output at the start, 0 to 255") \
  X(max,    255, "output at the highest") \
  X(shift,  3,   "calm: each sample moves the average by 1/2 (1) to 1/256 (8)") \
  X(period, 4,   "ticks of 16 ms between two samples")
#include <sys/settings.h>

#if REGULATOR_CONF_PORT
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

/* 0x4B is OCR1B, the duty cycle the timer works with */
PORT_VIEW(duty, "byte r 0x4B percent duty cycle", "the output as it is now");
#endif

#if REGULATOR_CONF_FUSED

/* ---- one process: the stages are its functions --------------------- */

PROCESS_DEFINE(regulator, "regulator", 3,
  sense_t  sense;
  smooth_t smooth;
  curve_t  curve;
  uint8_t  period;
  uint8_t  ticks;
);

PROCESS_INSTANCE(regulator, regulator_proc);

#define THE_SENSE  (regulator_proc.sense)
#define THE_SMOOTH (regulator_proc.smooth)
#define THE_CURVE  (regulator_proc.curve)
#define THE_PERIOD (regulator_proc.period)

PROCESS_THREAD(regulator, ev, data)
{
  process_regulator_t *self = PROCESS_SELF(regulator);

  PROCESS_BEGIN();

  pwm_start();

  while (1)
  {
    PROCESS_WAIT_TICKS(self->ticks, self->period);

    /* sense | smooth | curve | pwm, as one line */
    pwm_set(curve_map(&self->curve, smooth_step(&self->smooth, sense_read(&self->sense))));
  }

  PT_FINALLY(&pt_process->pt)
  {
    pwm_stop();
  }

  PROCESS_END();
}

#else

/* ---- a process per stage, and pipes between them -------------------- */

PROCESS_INSTANCE(sense, sense_proc);
PROCESS_INSTANCE(smooth, smooth_proc);
PROCESS_INSTANCE(curve, curve_proc);
PROCESS_INSTANCE(pwm, pwm_proc);

#define THE_SENSE  (sense_proc.sense)
#define THE_SMOOTH (smooth_proc.smooth)
#define THE_CURVE  (curve_proc.curve)
#define THE_PERIOD (sense_proc.period)

static bool connected;

#endif

void settings_apply(void)
{
  THE_CURVE.lo     = setting(SETTING_lo);
  THE_CURVE.hi     = setting(SETTING_hi);
  THE_CURVE.min    = setting(SETTING_min);
  THE_CURVE.max    = setting(SETTING_max);
  THE_SMOOTH.shift = setting(SETTING_shift);
  THE_PERIOD       = setting(SETTING_period);
}

void setup(void)
{
  process_init(NULL);

#if REGULATOR_CONF_PORT
  process_new(&process_type_softserial, &softserial_proc);
  process_new(&process_type_port, &port_proc);
#endif
#if REGULATOR_CONF_FUSED
  process_new(&process_type_regulator, &regulator_proc);
#else
  process_new(&process_type_sense, &sense_proc);
  process_new(&process_type_smooth, &smooth_proc);
  process_new(&process_type_curve, &curve_proc);
  process_new(&process_type_pwm, &pwm_proc);
#endif

  sense_pin(&THE_SENSE, REGULATOR_CONF_CHANNEL);
  settings_apply();
  /* time comes from the watchdog here: the clock of the core can stop */
  idle_stop_core_clock();
  idle_start();
  tick_start();
}

void loop(void)
{
  process_loop(); /* the scheduler, the tick, and sleep until there is work */

#if !REGULATOR_CONF_FUSED
  if (!connected)
    connected = process_connect(&sense_proc.base, &smooth_proc.base)
             && process_connect(&smooth_proc.base, &curve_proc.base)
             && process_connect(&curve_proc.base, &pwm_proc.base);
#endif
}

// file: ./src/sys/idle.h

#ifndef __IDLE_H__
#define __IDLE_H__

/* Idle: save power while no process has anything to do.
 *
 * A cooperative system that waits is a loop that spins. With sleep switched
 * on, the loop ends each pass with a wait for the next thing that can give a
 * process work: an interrupt, or the next tick. How deep that wait goes is a
 * setting, PROCESS_CONF_SLEEP, in the same words on every platform:
 *
 *   PROCESS_SLEEP_NONE  never wait: the loop spins. The default.
 *   PROCESS_SLEEP_WAIT  the processor stops, everything else runs on: timers,
 *                       PWM, the UART and the clock of the core. Any interrupt
 *                       ends the wait. Nothing changes for the sketch.
 *   PROCESS_SLEEP_DEEP  clocks stop too, memory is kept and the program goes
 *                       on where it stopped. Only the tick and pins end the
 *                       wait, so it needs a tick that runs in that state
 *                       (TICK_SOURCE_WATCHDOG on an AVR), and gives up what
 *                       stops: the clock of the core, the UART receiving.
 *
 * Define it before this header, and run the loop with process_loop():
 *
 *   #define TICK_CONF_SOURCE   TICK_SOURCE_WATCHDOG   // for DEEP
 *   #define PROCESS_CONF_SLEEP PROCESS_SLEEP_WAIT
 *   #include <sys/idle.h>
 *
 *   void setup(void) { ...  idle_start(); }
 *   void loop(void)  { process_loop(); }
 *
 * What each level is on each platform lives in cpu/<platform>/idle.h:
 *
 *              WAIT                        DEEP
 *   AVR        idle mode                   power-down, the watchdog ticks
 *   ESP32      block the loop task:        the same, with automatic light
 *              FreeRTOS idles the CPU      sleep                  (to come)
 *   RP2040     wait for interrupt          dormant                (to come)
 *
 * A level that loses memory, so that waking up is a restart (deep sleep of
 * an ESP32, the core powered off on an RP2350), is not an idle level: it ends
 * every protothread, and a sketch asks for it itself.
 *
 * - The question whether there is work and the wait are one step: an
 *   interrupt that arrives in between ends the wait at once.
 * - A process that keeps itself busy with events of its own keeps the
 *   processor awake. So does polling a UART instead of using its interrupt.
 */

#include <protoduino.h>
#include <sys/process.h>
#include <sys/tick.h>

#define PROCESS_SLEEP_NONE 0
#define PROCESS_SLEEP_WAIT 1
#define PROCESS_SLEEP_DEEP 2

#ifndef PROCESS_CONF_SLEEP
#define PROCESS_CONF_SLEEP PROCESS_SLEEP_NONE
#endif

#if defined(__AVR__)
#include "../cpu/avr/idle.h"
#else
#error "idle: Unsupported Platform!"
#endif

/**
 * Wait for work, as deep as PROCESS_CONF_SLEEP allows. Returns at once when
 * there is work, and always with PROCESS_SLEEP_NONE.
 */
static CC_ALWAYS_INLINE void idle_run(void)
{
#if PROCESS_CONF_SLEEP != PROCESS_SLEEP_NONE
  cpu_idle_wait(PROCESS_CONF_SLEEP, tick_millis_left());
#endif
}

/**
 * Prepare the platform for sleeping: peripherals the build says are not
 * used are switched off (CPU_CONF_POWER_OFF on an AVR). Call once, in
 * setup(), after the processes have started.
 */
static inline void idle_start(void)
{
  cpu_idle_start();
}

/**
 * One pass of the loop: the scheduler, the tick, and the wait.
 *
 *   void loop(void) { process_loop(); }
 */
static CC_ALWAYS_INLINE void process_loop(void)
{
  process_run();
  tick_run();
  idle_run();
}

#endif /* __IDLE_H__ */

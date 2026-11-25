#ifndef __PROCESS_CONF_H__
#define __PROCESS_CONF_H__

/* ------------------------------------------------------------------ */
/* Configuration (override in protoduino_config.h)                    */
/* ------------------------------------------------------------------ */

#ifndef PROCESS_CONF_MAX_ARGS
#define PROCESS_CONF_MAX_ARGS 4
#endif

// A part with little RAM gets a shorter log and a shorter event queue by
// default: with 256 bytes or less, every byte the kernel keeps is one a
// process cannot have.
#if defined(__AVR__)
#include <avr/io.h>
#endif
#if defined(RAMEND) && (RAMEND < 0x200)
#define PROCESS_CONF_SMALL 1
#else
#define PROCESS_CONF_SMALL 0
#endif

#ifndef PROCESS_CONF_ERROR_POOL_SIZE
#if PROCESS_CONF_SMALL
#define PROCESS_CONF_ERROR_POOL_SIZE 1
#else
#define PROCESS_CONF_ERROR_POOL_SIZE 4
#endif
#endif

#ifndef PROCESS_CONF_EVENT_QUEUE_SIZE
#if PROCESS_CONF_SMALL
#define PROCESS_CONF_EVENT_QUEUE_SIZE 3
#else
#define PROCESS_CONF_EVENT_QUEUE_SIZE 4
#endif
#endif

#ifndef PROCESS_CONF_EVENT_INBOX
#define PROCESS_CONF_EVENT_INBOX 0
#endif

#ifndef PROCESS_CONF_INBOX_SIZE
#define PROCESS_CONF_INBOX_SIZE 4
#endif

#ifndef PROCESS_CONF_PIPELINES
#define PROCESS_CONF_PIPELINES 1
#endif

#ifndef PROCESS_CONF_PIPELINES_MAX_STAGES
#define PROCESS_CONF_PIPELINES_MAX_STAGES 3
#endif

#ifndef PROCESS_CONF_PIPE_BUFFER_SIZE
#define PROCESS_CONF_PIPE_BUFFER_SIZE PT_CONF_PIPE_BUFFER_SIZE
#endif

/* Called after an interrupt (or anybody) asked the scheduler for a turn:
 * process_poll() and a successful process_post(). On a part where the loop
 * sleeps by itself when an interrupt arrives (AVR, RP2040) nothing has to
 * happen. Where the loop runs as a task of an RTOS and blocks to save power
 * (ESP32 under FreeRTOS), the port defines this to wake that task. */
// One scheduler per core. With 2, a process belongs to the core that starts
// it; process_post() and process_poll() for a process of the other core go
// through a mailbox. Set it where every file of the build sees it: in
// protoduino-config.h, or as a build flag (-DPROCESS_CONF_CORES=2). RP2040
// and RP2350 only.
#ifndef PROCESS_CONF_CORES
#define PROCESS_CONF_CORES 1
#endif

#if PROCESS_CONF_CORES > 1
#if !((defined(PICO_RP2040) && PICO_RP2040) || (defined(PICO_RP2350) && PICO_RP2350) || defined(ARDUINO_ARCH_RP2040))
#error "process: PROCESS_CONF_CORES > 1 needs a chip with more than one core (RP2040, RP2350)"
#endif
// the core this runs on: CPUID of the SIO
#define PROCESS_CORE() ((uint8_t)(*(volatile const uint32_t *)0xd0000000u))
// events and polls from one core to the other that can wait at a time
#ifndef PROCESS_CONF_MAILBOX_SIZE
#define PROCESS_CONF_MAILBOX_SIZE 16
#endif
#else
#define PROCESS_CORE() 0
#endif

#ifndef PROCESS_CONF_WAKE
#if (defined(PICO_RP2040) && PICO_RP2040) || (defined(PICO_RP2350) && PICO_RP2350) || defined(ARDUINO_ARCH_RP2040)
// RP2040, RP2350: the loop waits with WFE (cpu/rp2/idle.h). An event sent
// here is latched: work that arrives between the look and the wait ends the
// wait at once, from an interrupt or from the other core.
#define PROCESS_CONF_WAKE() __asm volatile("sev" ::: "memory")
#else
#define PROCESS_CONF_WAKE() do { } while (0)
#endif
#endif

#endif /* __PROCESS_CONF_H__ */
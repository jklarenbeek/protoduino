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
#ifndef PROCESS_CONF_WAKE
#define PROCESS_CONF_WAKE() do { } while (0)
#endif

#endif /* __PROCESS_CONF_H__ */
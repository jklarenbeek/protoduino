// file: ./src/sys/process/serial.h
//
// Public API for serial device processes.
//
// Each serial_processN wraps hardware UART N as a singleton DEVICE-flavour
// protoduino process that exposes bidirectional byte streams via ipc_pipe_t.
//
// USAGE
// -----
// 1. Set the desired baud rate then start the device:
//
//      serial_process0_proc.baud = UART_BAUD_9600;
//      process_new(&process_type_serial_process0, &serial_process0_proc);
//
// 2. Connect a user process's pipein/pipeout to the device:
//
//      SERIAL_PROCESS0_CONNECT(pt_process);
//
//    Equivalent to:
//      pt_process->pipein      = &serial_process0_proc.rx_pipe;
//      pt_process->pipeout     = &serial_process0_proc.tx_pipe;
//      pt_process->has_pipein  = 1;
//      pt_process->has_pipeout = 1;
//
// 3. Use standard pipeline API in the user process:
//      PROCESS_PIPE_READ, PROCESS_PIPE_WRITE, ipc_pipe_available, etc.
//
// BUFFER SIZES (override in protoduino-config.h or before including):
//   #define PROCESS_SERIAL_RX_BUF_SIZE  64
//   #define PROCESS_SERIAL_TX_BUF_SIZE  96
//
// DEFAULT BAUD (override before process_new):
//   #define PROCESS_SERIAL_DEFAULT_BAUD UART_BAUD_9600

#ifndef __PROCESS_SERIAL_H__
#define __PROCESS_SERIAL_H__

#include <protoduino.h>
#include <sys/process.h>
#include <sys/serial.h>
#include <sys/ipc.h>

// ---------------------------------------------------------------------------
// Buffer / baud defaults
// ---------------------------------------------------------------------------

#ifndef PROCESS_SERIAL_RX_BUF_SIZE
#  define PROCESS_SERIAL_RX_BUF_SIZE  64
#endif

#ifndef PROCESS_SERIAL_TX_BUF_SIZE
#  define PROCESS_SERIAL_TX_BUF_SIZE  96
#endif

#ifndef PROCESS_SERIAL_DEFAULT_BAUD
#  define PROCESS_SERIAL_DEFAULT_BAUD  UART_BAUD_9600
#endif

// ---------------------------------------------------------------------------
// Per-instance struct type (same layout for all UARTs).
// Declared here so user code can read/write the `baud` field before calling
// process_new(). The actual definition is in the .c files.
// ---------------------------------------------------------------------------

typedef struct {
  struct process base;        /* MUST be first – §6.7.2.1 */
  uint32_t       baud;        /* Set before process_new() – 0 → default */
  uint8_t        rx_buf[PROCESS_SERIAL_RX_BUF_SIZE];
  uint8_t        tx_buf[PROCESS_SERIAL_TX_BUF_SIZE];
  ipc_pipe_t     rx_pipe;
  ipc_pipe_t     tx_pipe;
} serial_process_t;

// ---------------------------------------------------------------------------
// Per-UART declarations (one set per HAVE_HW_UARTx)
// ---------------------------------------------------------------------------

#ifdef HAVE_HW_UART0

/* Thread function – defined in serial0.c */
ptstate_t process_thread_serial_process0(struct process *pt_process,
                                          process_event_t ev,
                                          process_data_t  data);

/* PROGMEM type descriptor – defined in serial0.c */
extern const process_type_t process_type_serial_process0;

/* Singleton instance – defined in serial0.c */
extern serial_process_t serial_process0_proc;

/* Wire a user process's pipein/pipeout to UART0. */
#define SERIAL_PROCESS0_CONNECT(proc_ptr) \
  do { \
    (proc_ptr)->pipein      = &serial_process0_proc.rx_pipe; \
    (proc_ptr)->pipeout     = &serial_process0_proc.tx_pipe; \
    (proc_ptr)->has_pipein  = 1; \
    (proc_ptr)->has_pipeout = 1; \
    /* Wire rx_pipe wake callback → poll this consumer on RX ISR write. */ \
    serial_process0_proc.rx_pipe.wake_cb  = process_ipc_wake; \
    serial_process0_proc.rx_pipe.wake_ctx = (void *)(proc_ptr); \
  } while (0)

#endif /* HAVE_HW_UART0 */

// ---------------------------------------------------------------------------

#ifdef HAVE_HW_UART1

ptstate_t process_thread_serial_process1(struct process *pt_process,
                                          process_event_t ev,
                                          process_data_t  data);
extern const process_type_t process_type_serial_process1;
extern serial_process_t serial_process1_proc;

#define SERIAL_PROCESS1_CONNECT(proc_ptr) \
  do { \
    (proc_ptr)->pipein      = &serial_process1_proc.rx_pipe; \
    (proc_ptr)->pipeout     = &serial_process1_proc.tx_pipe; \
    (proc_ptr)->has_pipein  = 1; \
    (proc_ptr)->has_pipeout = 1; \
    serial_process1_proc.rx_pipe.wake_cb  = process_ipc_wake; \
    serial_process1_proc.rx_pipe.wake_ctx = (void *)(proc_ptr); \
  } while (0)

#endif /* HAVE_HW_UART1 */

// ---------------------------------------------------------------------------

#ifdef HAVE_HW_UART2

ptstate_t process_thread_serial_process2(struct process *pt_process,
                                          process_event_t ev,
                                          process_data_t  data);
extern const process_type_t process_type_serial_process2;
extern serial_process_t serial_process2_proc;

#define SERIAL_PROCESS2_CONNECT(proc_ptr) \
  do { \
    (proc_ptr)->pipein      = &serial_process2_proc.rx_pipe; \
    (proc_ptr)->pipeout     = &serial_process2_proc.tx_pipe; \
    (proc_ptr)->has_pipein  = 1; \
    (proc_ptr)->has_pipeout = 1; \
    serial_process2_proc.rx_pipe.wake_cb  = process_ipc_wake; \
    serial_process2_proc.rx_pipe.wake_ctx = (void *)(proc_ptr); \
  } while (0)

#endif /* HAVE_HW_UART2 */

// ---------------------------------------------------------------------------

#ifdef HAVE_HW_UART3

ptstate_t process_thread_serial_process3(struct process *pt_process,
                                          process_event_t ev,
                                          process_data_t  data);
extern const process_type_t process_type_serial_process3;
extern serial_process_t serial_process3_proc;

#define SERIAL_PROCESS3_CONNECT(proc_ptr) \
  do { \
    (proc_ptr)->pipein      = &serial_process3_proc.rx_pipe; \
    (proc_ptr)->pipeout     = &serial_process3_proc.tx_pipe; \
    (proc_ptr)->has_pipein  = 1; \
    (proc_ptr)->has_pipeout = 1; \
    serial_process3_proc.rx_pipe.wake_cb  = process_ipc_wake; \
    serial_process3_proc.rx_pipe.wake_ctx = (void *)(proc_ptr); \
  } while (0)

#endif /* HAVE_HW_UART3 */

#endif /* __PROCESS_SERIAL_H__ */

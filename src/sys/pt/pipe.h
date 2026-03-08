// file: ./src/sys/pt/pipe.h

#ifndef __PT_PIPE_H__
#define __PT_PIPE_H__

#include <protoduino.h>
#include "../ipc.h"

struct ipc_pt
{
  lc_t lc;
  struct ipc_pipe pipein;
  struct ipc_pipe pipeout;
  size_t written;
};

#define PT_STDIN(pt) ((pt)->pipein)
#define PT_STDOUT(pt) ((pt)->pipeout)

#define PT_PIPE_AVAILABLE(pt) (ipc_pipe_available(PT_STDIN(pt)) > 0)
#define PT_PIPE_SPACE(pt) (ipc_pipe_space(PT_STDOUT(pt)) > 0)

/**
 * Efficient Blocking Read
 * Waits until data is available, then reads it.
 * Uses PT_WAIT_UNTIL to generate the minimal "wait" state machine.
 */
#define PT_PIPE_READ_EX(pt, pipe, buf, max_len, read_len_ptr)             \
  do                                                                      \
  {                                                                       \
    PT_WAIT_UNTIL((pt), ipc_pipe_available(pipe) > 0);                    \
    *(read_len_ptr) = ipc_pipe_read((pipe), (uint8_t *)(buf), (max_len)); \
  } while (0)

#define PT_PIPE_READ(pt, buf, max_len, read_len_ptr) \
  PT_PIPE_READ_EX(pt, PT_STDIN(pt), buf, max_len, read_len_ptr)

/* * WARNING: potential deadlock if len > pipe->size.
 * Best for small command messages defined in ipc.md
 */
#define PT_PIPE_WRITE_ATOMIC_EX(pt, pipe, buf, len)         \
  do                                                        \
  {                                                         \
    /* Block until the pipe can accept the WHOLE message */ \
    PT_WAIT_UNTIL((pt), ipc_pipe_space(pipe) >= (len));     \
    ipc_pipe_write((pipe), (buf), (len));                   \
  } while (0)

#define PT_PIPE_WRITE_ATOMIC(pt, buf, len) \
  PT_PIPE_WRITE_ATOMIC_EX(pt, PT_STDOUT(pt), buf, len)

/* * WARNING: Assumes 'pt' is of type 'struct ipc_pt*' which contains 'size_t written'.
 * Best for general command messages
 */
#define PT_PIPE_WRITE_EX(pt, pipe, buf, len, written) \
  do                                                  \
  {                                                   \
    written = 0;                                      \
    while ((written) < (len))                           \
    {                                                 \
      PT_WAIT_UNTIL((pt), ipc_pipe_space(pipe) > 0);  \
      written += ipc_pipe_write((pipe),               \
        (const uint8_t *)(buf) + (written),           \
        (len) - (written));                           \
    }                                                 \
  } while (0)

#define PT_PIPE_WRITE(pt, buf, len) \
  PT_PIPE_WRITE_EX(pt, PT_STDOUT(pt), buf, len, (pt)->written)

/**
 * @brief Reduces CPU usage by reducing the frequency of wake-ups.
 *
 * Waking up a process to write 1 byte is expensive. This approach
 * waits until the pipe has a significant chunk of space (e.g., half
 * the total length needed or a fixed block size) before waking up
 * the writer. This keeps the writer sleeping longer and processing
 * more data per wake cycle.
 */
#define PT_PIPE_WRITE_BATCH_EX(pt, pipe, buf, len, written)        \
  do {                                                             \
    written = 0;                                                   \
    while ((written) < (len)) {                                    \
      PT_WAIT_UNTIL((pt), ipc_pipe_space(pipe) >= (((len) - (written)) > PT_CONF_PIPE_THRESHOLD_SIZE ? PT_CONF_PIPE_THRESHOLD_SIZE : 1)); \
      written += ipc_pipe_write((pipe), (const uint8_t *)(buf) + (written), ((len) - (written))); \
    }                                                              \
  } while (0)

#define PT_PIPE_WRITE_BATCH(pt, buf, len) \
  PT_PIPE_WRITE_BATCH_EX(pt, PT_STDOUT(pt), buf, len, (pt)->written)

#endif /* __PT_PIPE_H__ */

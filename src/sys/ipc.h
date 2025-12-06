// file: ./src/sys/ipc.h
#ifndef __IPC_H__
#define __IPC_H__ 1

/* Unified IPC layer (messages + pipes) for protoduino-style kernels.
 *
 * - Independent from process.c/process.h
 * - Uses CC_ATOMIC_RESTORE() / CC_ATOMIC_FORCEON() from ./src/cc.h
 * - Uses error codes from ./src/sys/errors.h
 *
 * Design:
 *  - ipc_msg_t: small packet with type, argc, argv[] (fixed small size)
 *  - pool: fixed-block allocator for messages or arbitrary small buffers
 *  - ipc_pipe: byte ring buffer with wake callback to notify reader
 *
 * NOTE: The layer does not call process_post() directly. Instead you supply
 *       a wake callback (void (*wake_cb)(void *ctx)) that integrates with
 *       your scheduler (e.g., process_poll() or process_post()).
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "../cc.h"              /* contains CC_ATOMIC_RESTORE() macros */
#include "errors.h"             /* error taxonomy (ERR_* defines) */

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------
 * Message (packet) API
 * -------------------------------------------------------------------------*/

/* Small, fixed-argument IPC message. Use ipc_msg_alloc() to obtain from the ipc pool. */
typedef struct ipc_msg {
    uint8_t type;                         /* application / subsystem message type */
    uint8_t argc;                         /* number of valid argv[] entries (0..IPC_MSG_MAX_ARGS) */
    void *argv[IPC_MSG_MAX_ARGS];         /* argument payload pointers (caller-allocated or pool) */
} ipc_msg_t;

/* Message pool is the same fixed-block pool used by other users (opaque view) */
struct ipc_pool {
    void *free_list;      /* pointer to first free block (stored inside blocks) */
    uint8_t *buffer;      /* base buffer (optional, used for bounds check) */
    size_t block_size;    /* size of each block (>= sizeof(void*)) */
    uint16_t n_blocks;    /* total blocks */
    uint16_t free_count;  /* number free */
};

/* Initialize an ipc pool. buffer must be BLOCK_SIZE * N large.
 * Returns: ERR_SUCCESS or ERR_RES_EXHAUSTED/ERR_SYS_INVAL on bad args.
 */
int ipc_pool_init(struct ipc_pool *p, void *buffer, size_t block_size, uint16_t n_blocks);

/* Allocate one block from pool (returns pointer or NULL on failure) */
void *ipc_pool_alloc(struct ipc_pool *p);

/* Free a previously allocated block back to pool (no-op for NULL) */
void ipc_pool_free(struct ipc_pool *p, void *blk);

/* Number of free blocks currently available */
uint16_t ipc_pool_count_free(struct ipc_pool *p);

/* Message helpers using the ipc pool:
 * - ipc_msg_alloc_from_pool(pool) returns ipc_msg_t* or NULL
 * - ipc_msg_free_to_pool(pool, msg)
 */
ipc_msg_t *ipc_msg_alloc_from_pool(struct ipc_pool *pool);
void ipc_msg_free_to_pool(struct ipc_pool *pool, ipc_msg_t *m);

/* Initialize message contents: sets type/argc and copies argv pointers (no deep copy) */
int ipc_msg_init(ipc_msg_t *m, uint8_t type, uint8_t argc, void *argv[]);

/* Convenience: set single arg */
int ipc_msg_set_arg(ipc_msg_t *m, uint8_t idx, void *arg);

/* ---------------------------------------------------------------------------
 * Pipe (stream) API
 * -------------------------------------------------------------------------*/

/* wake callback type used to notify scheduler/reader that data arrived.
 * Example: for protoduino, your callback may call process_poll(reader_proc) or
 * process_post(reader_proc, PROCESS_EVENT_POLL, NULL).
 */
typedef void (*ipc_wake_cb_t)(void *ctx);

typedef struct ipc_pipe {
    uint8_t *buf;             /* pointer to buffer storage */
    size_t size;              /* total buffer size in bytes (must be >= 2) */
    size_t head;              /* write index (next write position) */
    size_t tail;              /* read index (next read position) */
    ipc_wake_cb_t wake_cb;    /* callback to notify reader (may be NULL) */
    void *wake_ctx;           /* context passed to callback */
} ipc_pipe_t;

/* Initialize a pipe. buffer must be `size` bytes. wake_cb may be NULL.
 * Returns ERR_SUCCESS or ERR_SYS_INVAL if args invalid.
 */
int ipc_pipe_init(ipc_pipe_t *p, void *buffer, size_t size, ipc_wake_cb_t wake_cb, void *wake_ctx);

/* Query available bytes in pipe (reader-visible) */
size_t ipc_pipe_available(const ipc_pipe_t *p);

/* Query free space in pipe (for writer) */
size_t ipc_pipe_space(const ipc_pipe_t *p);

/* Write up to len bytes from src into pipe. Returns number of bytes written (may be 0).
 * If data transitions pipe from empty->non-empty, wake_cb (if set) is invoked
 * after the write and *outside* the atomic block (to minimize ISR disable time).
 * The wake callback (e.g., process_poll) must be safe to call from ISR.
 */
size_t ipc_pipe_write(ipc_pipe_t *p, const uint8_t *src, size_t len);

/* Read up to len bytes from pipe into dst. Returns number of bytes read (may be 0). */
size_t ipc_pipe_read(ipc_pipe_t *p, uint8_t *dst, size_t len);

/* Clear pipe (drop contents) */
void ipc_pipe_clear(ipc_pipe_t *p);

/* ---------------------------------------------------------------------------
 * Convenience helpers (errors & debug)
 * -------------------------------------------------------------------------*/

/* Convert a ipc_msg_t to a textual description into user buffer.
 * Not intended for production on AVR but useful when debugging on host.
 * Returns ERR_SUCCESS or ERR_SYS_INVAL.
 */
int ipc_msg_to_string(const ipc_msg_t *m, char *buf, size_t buflen);

#ifdef __cplusplus
}
#endif

#endif /* __IPC_H__ */

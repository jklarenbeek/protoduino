// file: ./src/sys/ipc.c

#include "ipc.h"
#include <string.h>
#include <stdio.h>   /* snprintf used only in debug helper */

/* -------------------------------------------------------------------------
 * IPC pool implementation (fixed-block free-list). Deterministic & tiny.
 * All modifications to pool's free list are wrapped in CC_ATOMIC_RESTORE()
 * so they're safe across ISR/foreground boundaries on AVR when using the
 * CC_ATOMIC_RESTORE() macro that maps to ATOMIC_BLOCK(ATOMIC_RESTORESTATE).
 * ----------------------------------------------------------------------*/

int ipc_pool_init(struct ipc_pool *p, void *buffer, size_t block_size, uint16_t n_blocks)
{
    if (!p || !buffer || n_blocks == 0) return ERR_SYS_INVAL;
    if (block_size < sizeof(void*)) block_size = sizeof(void*);
    p->buffer = (uint8_t*)buffer;
    p->block_size = block_size;
    p->n_blocks = n_blocks;
    p->free_count = n_blocks;
    p->free_list = NULL;
    /* Build free list: place pointer to next in first sizeof(void*) bytes of each block */
    for (int i = (int)n_blocks - 1; i >= 0; --i) {
        void *blk = p->buffer + (size_t)i * block_size;
        /* push blk onto free_list */
        *((void**)blk) = p->free_list;
        p->free_list = blk;
    }
    return ERR_SUCCESS;
}

void *ipc_pool_alloc(struct ipc_pool *p)
{
    if (!p) return NULL;
    void *blk = NULL;
    CC_ATOMIC_RESTORE() {
        blk = p->free_list;
        if (blk) {
            p->free_list = *((void**)blk);
            p->free_count--;
        }
    }
    return blk;
}

void ipc_pool_free(struct ipc_pool *p, void *blk)
{
    if (!p || !blk) return;
    CC_ATOMIC_RESTORE() {
        *((void**)blk) = p->free_list;
        p->free_list = blk;
        p->free_count++;
    }
}

uint16_t ipc_pool_count_free(struct ipc_pool *p)
{
    return (p ? p->free_count : 0);
}

/* -------------------------------------------------------------------------
 * Message helpers (msg struct uses argv pointers only - no deep-copy)
 * ----------------------------------------------------------------------*/

ipc_msg_t *ipc_msg_alloc_from_pool(struct ipc_pool *pool)
{
    if (!pool) return NULL;
    return (ipc_msg_t*) ipc_pool_alloc(pool);
}

void ipc_msg_free_to_pool(struct ipc_pool *pool, ipc_msg_t *m)
{
    if (!pool || !m) return;
    ipc_pool_free(pool, (void*)m);
}

int ipc_msg_init(ipc_msg_t *m, uint8_t type, uint8_t argc, void *argv[])
{
    if (!m) return ERR_SYS_INVAL;
    if (argc > IPC_MSG_MAX_ARGS) return ERR_SYS_INVAL;
    m->type = type;
    m->argc = argc;
    /* copy pointers (no deep copy) */
    for (uint8_t i = 0; i < IPC_MSG_MAX_ARGS; ++i) {
        m->argv[i] = (i < argc && argv) ? argv[i] : NULL;
    }
    return ERR_SUCCESS;
}

int ipc_msg_set_arg(ipc_msg_t *m, uint8_t idx, void *arg)
{
    if (!m) return ERR_SYS_INVAL;
    if (idx >= IPC_MSG_MAX_ARGS) return ERR_SYS_INVAL;
    m->argv[idx] = arg;
    if (idx >= m->argc) m->argc = idx + 1;
    return ERR_SUCCESS;
}

/* Convert a message into human-readable text (best-effort).
 * Only for debug; safe on platforms with snprintf.
 */
int ipc_msg_to_string(const ipc_msg_t *m, char *buf, size_t buflen)
{
    if (!m || !buf || buflen == 0) return ERR_SYS_INVAL;
    int pos = 0;
    int n = snprintf(buf + pos, (pos < (int)buflen) ? buflen - pos : 0,
                     "MSG(type=%u argc=%u)", (unsigned)m->type, (unsigned)m->argc);
    if (n < 0) return ERR_FAILURE;
    pos += n;
    for (uint8_t i = 0; i < m->argc && pos < (int)buflen - 1; ++i) {
        n = snprintf(buf + pos, buflen - pos, " [%u]=%p", (unsigned)i, m->argv[i]);
        if (n < 0) break;
        pos += n;
    }
    buf[(pos < (int)buflen) ? pos : (int)buflen - 1] = '\0';
    return ERR_SUCCESS;
}

/* -------------------------------------------------------------------------
 * Pipe implementation: ring buffer for bytes. Minimal, deterministic, and
 * racing protected by CC_ATOMIC_RESTORE() for atomic head/tail updates.
 *
 * Notifications: writer calls wake_cb(wake_ctx) if configured and if the
 * pipe changed from empty->non-empty. The wake callback must be safe to call
 * from the context you're in (ISR or thread), or you must use your ISR
 * variant that posts to the scheduler.
 * ----------------------------------------------------------------------*/

int ipc_pipe_init(ipc_pipe_t *p, void *buffer, size_t size, ipc_wake_cb_t wake_cb, void *wake_ctx)
{
    if (!p || !buffer || size < 2) return ERR_SYS_INVAL;
    p->buf = (uint8_t*)buffer;
    p->size = size;
    p->head = 0;
    p->tail = 0;
    p->wake_cb = wake_cb;
    p->wake_ctx = wake_ctx;
    /* zero buffer optional */
    return ERR_SUCCESS;
}

size_t ipc_pipe_available(const ipc_pipe_t *p)
{
    if (!p) return 0;
    size_t h = p->head;
    size_t t = p->tail;
    return (h >= t) ? (h - t) : (p->size + h - t);
}

size_t ipc_pipe_space(const ipc_pipe_t *p)
{
    if (!p) return 0;
    /* leave one slot empty to differentiate full vs empty */
    return p->size ? (p->size - ipc_pipe_available(p) - 1) : 0;
}

size_t ipc_pipe_write(ipc_pipe_t *p, const uint8_t *src, size_t len)
{
    if (!p || !src || len == 0) return 0;
    size_t written = 0;
    bool was_empty = false;

    CC_ATOMIC_RESTORE() {
        size_t space = ipc_pipe_space(p);
        if (space == 0) { written = 0; }
        else {
            size_t towrite = (len <= space) ? len : space;
            size_t h = p->head;
            /* first chunk until end of buffer */
            size_t first = p->size - h;
            if (first > towrite) first = towrite;
            memcpy(&p->buf[h], src, first);
            h = (h + first) % p->size;
            if (first < towrite) {
                size_t second = towrite - first;
                memcpy(&p->buf[h], src + first, second);
                h = (h + second) % p->size;
            }
            p->head = h;
            written = towrite;
            was_empty = (towrite > 0 && ipc_pipe_available(p) == towrite);
        }
    } /* atomic end */

    /* Notify reader if we transitioned empty->non-empty (call outside atomic block
     * is also okay; in our case we call after the atomic block; that's safe).
     */
    if (was_empty && p->wake_cb) {
        p->wake_cb(p->wake_ctx);
    }
    return written;
}

size_t ipc_pipe_read(ipc_pipe_t *p, uint8_t *dst, size_t len)
{
    if (!p || !dst || len == 0) return 0;
    size_t read = 0;

    CC_ATOMIC_RESTORE() {
        size_t avail = ipc_pipe_available(p);
        if (avail == 0) { read = 0; }
        else {
            size_t toread = (len <= avail) ? len : avail;
            size_t t = p->tail;
            size_t first = p->size - t;
            if (first > toread) first = toread;
            memcpy(dst, &p->buf[t], first);
            t = (t + first) % p->size;
            if (first < toread) {
                size_t second = toread - first;
                memcpy(dst + first, &p->buf[t], second);
                t = (t + second) % p->size;
            }
            p->tail = t;
            read = toread;
        }
    } /* atomic end */

    return read;
}

void ipc_pipe_clear(ipc_pipe_t *p)
{
    if (!p) return;
    CC_ATOMIC_RESTORE() {
        p->head = p->tail = 0;
    }
}

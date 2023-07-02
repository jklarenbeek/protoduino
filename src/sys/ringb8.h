#include "cc.h"
#include <stdbool.h>

struct ringb8
{
    uint8_t mask;
    uint8_t tail;
    uint8_t head;
    uint8_t *buffer;
};

#define VAR_RINGB8(name) CONCAT2(name,_ringb8);
#define VAR_RINGB8_DATA(name) CONCAT2(name,_ringb8_data)

#define RINGB8(name, size) \
static uint8_t VAR_RINGB8_DATA(name)[size]; \
static struct ringb8 VAR_RINGB8(name) = { \
    (size - 1), 0, 0, \
    &VAR_RINGB8_DATA(name), \
    }

CC_EXTERN void ringb8_init(struct ringb8 *self, uint_fast8_t *buffer, uint_fast8_t size);
CC_EXTERN void ringb8_init(struct ringb8 *self, uint_fast8_t *buffer, uint_fast8_t size);
CC_EXTERN uint_fast8_t ringb8_count(struct ringb8 *self)
CC_EXTERN uint_fast8_t ringb8_available(struct ringb8 *self);
CC_EXTERN uint_fast8_t ringb8_peek(struct ringb8 *self);
CC_EXTERN uint_fast8_t ringb8_get(struct ringb8 *self);
CC_EXTERN uint_fast8_t ringb8_ahead(struct ringb8 *self);
CC_EXTERN uint_fast8_t ringb8_put(struct ringb8 *self, uint_fast8_t value);

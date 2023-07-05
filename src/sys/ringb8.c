#include "ringb8.h"

void ringb8_init(struct ringb8_t *self, uint_fast8_t *buffer, uint_fast16_t size)
{
    self->mask = (uint8_t)(size - 1);
    self->tail = 0;
    self->head = 0;
    self->data = buffer;
}

// the total size of the buffer
uint_fast16_t ringb8_size(struct ringb8_t *self)
{
    return self->mask + 1;
}

// number of bytes in the buffer
uint_fast8_t ringb8_count(struct ringb8_t *self)
{
    return (self->head - self->tail) & self->mask;
}

CC_FLATTEN uint_fast8_t ringb8_available(struct ringb8_t *self)
{
    return (uint_fast8_t)(ringb8_size(self) - ringb8_count(self));
}

// read next byte in buffer without increasing tail ptr
uint_fast8_t ringb8_peek(struct ringb8_t *self)
{
    return self->data[self->tail];
}

// read next byte in buffer and increase tail ptr
uint_fast8_t ringb8_get(struct ringb8_t *self)
{
    uint_fast8_t value = ringb8_peek(self);
    self->tail = (self->tail + 1) & self->mask;
    return value;
}

uint_fast8_t ringb8_last(struct ringb8_t *self)
{
    return self->data[(self->head - 1) & self->mask];
}

void ringb8_put(struct ringb8_t *self, uint_fast8_t value)
{
    self->data[self->head] = (uint8_t)value;
    self->head = (self->head + 1) & self->mask;
}

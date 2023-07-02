#include "ringb8.h"

void ringb8_init(struct ringb8 *self, uint_fast8_t *buffer, uint_fast8_t size)
{
    self->mask = size - 1;
    self->tail = 0;
    self->head = 0;
    self->data = buffer
}

// the total size of the buffer
uint_fast8_t ringb8_size(struct ringb8 *self)
{
    return self->mask + 1;
}

// number of bytes in the buffer
uint_fast8_t ringb8_count(struct ringb8 *self)
{
    return (self->head - self->tail) & self->mask;
}

uint8_fast8_t ringb8_available(struct ringb8 *self)
{
    return (ringb8_size(self) - ringb8_count(self));
}

// read next byte in buffer without increasing tail ptr
uint_fast8_t ringb8_peek(struct ringb8 *self)
{
    return self->data[self->tail];
}

// read next byte in buffer and increase tail ptr
uint_fast8_t ringb8_get(struct ringb8 *self)
{
    uint_fast8_t value = ringb8_peek(self);
    self->tail = (self->tail + 1) & self->mask;
    return value;
}

uint_fast8_t ringb8_ahead(struct ringb8 *self)
{
    return self->data[(self->head - 1) & self->mask];
}

uint_fast8_t ringb8_put(struct ringb8 *self, uint_fast8_t value)
{
    self->data[self->head] = value;
    self->head = (self->head + 1) & self->mask;
}

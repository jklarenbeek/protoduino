// file: ./src/lib/ringb8.h

/**
 * @brief Unchecked Ring buffer implementation for 8-bit data.
 *
 * @warning this module does not check on buffer overflow. this is up te the user.
 */

#ifndef RINGB8_H
#define RINGB8_H


#include <cc.h>
#include <stdint.h>

/**
 * @struct ringb8_t
 * @brief Structure representing the ring buffer.
 */
struct ringb8_t
{
    uint8_t mask;
    uint8_t tail;
    uint8_t head;
    uint8_t *data;
};

/**
 * @def VAR_RINGB8(name)
 * @brief Macro for generating a variable name for the ring buffer.
 *
 * The macro expands to the concatenation of the given name with "_ringb8".
 *
 * @param name The base name for the ring buffer variable.
 */
#define VAR_RINGB8(name) CC_CONCAT2EXT(name,_ringb8)
/**
 * @def VAR_RINGB8_DATA(name)
 * @brief Macro for generating a variable name for the ring buffer data array.
 *
 * The macro expands to the concatenation of the given name with "_ringb8_data".
 *
 * @param name The base name for the ring buffer data array variable.
 */
#define VAR_RINGB8_DATA(name) CC_CONCAT2EXT(name,_ringb8_data)

/**
 * @def RINGB8(name, size)
 * @brief Macro for defining a ring buffer with the specified size.
 *
 * The macro defines a static array for the ring buffer data and a static
 * structure for the ring buffer itself, using the specified size.
 *
 * @param name The base name for the ring buffer variables.
 * @param size The size of the ring buffer.
 */
#define RINGB8(name, size) \
static uint8_t VAR_RINGB8_DATA(name)[size]; \
static struct ringb8_t VAR_RINGB8(name) = { \
    (size - 1), 0, 0, \
    VAR_RINGB8_DATA(name) }

/**
 * @brief Initialize the ring buffer.
 *
 * @param self Pointer to the ring buffer structure.
 * @param buffer Pointer to the data buffer.
 * @param size Size of the buffer
 *
 * @warning The size parameter of the buffer can not exceed 256 and MUST be an exponent of 2.
 */
CC_EXTERN void ringb8_init(struct ringb8_t *self, uint_fast8_t *buffer, uint_fast16_t size);
#define RINGB8_INIT(name, buffer, size) ringb8_init(&VAR_RINGB8(name), buffer, size)

/**
 * @brief Get the total size of the buffer.
 *
 * @param self Pointer to the ring buffer structure.
 * @return The total size of the buffer.
 */
CC_EXTERN uint_fast16_t ringb8_size(struct ringb8_t *self);
#define RINGB8_SIZE(name) ringb8_size(&VAR_RINGB8(name))

/**
 * @brief Get the number of bytes in the buffer.
 *
 * @param self Pointer to the ring buffer structure.
 * @return The number of bytes in the buffer.
 */
CC_EXTERN uint_fast8_t ringb8_count(struct ringb8_t *self);
#define RINGB8_COUNT(name) ringb8_count(&VAR_RINGB8(name))

/**
 * @brief Get the number of available bytes in the buffer.
 *
 * @param self Pointer to the ring buffer structure.
 * @return The number of available bytes in the buffer.
 */
CC_EXTERN uint_fast8_t ringb8_available(struct ringb8_t *self);
#define RINGB8_AVAILABLE(name) ringb8_available(&VAR_RINGB8(name))

/**
 * @brief Read the next byte in the buffer without increasing the tail pointer.
 *
 * @param self Pointer to the ring buffer structure.
 * @return The next byte in the buffer.
 */
CC_EXTERN uint_fast8_t ringb8_peek(struct ringb8_t *self);
#define RINGB8_PEEK(name) ringb8_peek(&VAR_RINGB8(name))

/**
 * @brief Read the next byte in the buffer and increase the tail pointer.
 *
 * @param self Pointer to the ring buffer structure.
 * @return The next byte in the buffer.
 */
CC_EXTERN uint_fast8_t ringb8_get(struct ringb8_t *self);
#define RINGB8_GET(name) ringb8_get(&VAR_RINGB8(name))

/**
 * @brief Get the last byte written to the buffer.
 *
 * @param self Pointer to the ring buffer structure.
 * @return The last byte written to the buffer.
 */
CC_EXTERN uint_fast8_t ringb8_last(struct ringb8_t *self);
#define RINGB8_LAST(name) ringb8_last(&VAR_RINGB8(name))

/**
 * @brief Write a byte to the buffer.
 *
 * @param self Pointer to the ring buffer structure.
 * @param value The byte to write.
 */
CC_EXTERN void ringb8_put(struct ringb8_t *self, uint_fast8_t value);
#define RINGB8_PUT(name, value) ringb8_put(&VAR_RINGB8(name), value)

#endif /* RINGB8_H */
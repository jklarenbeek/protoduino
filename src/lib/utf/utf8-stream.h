#ifndef __UTF8_STREAM_H__
#define __UTF8_STREAM_H__

#include <protoduino.h>
#include <Stream.h>
#include "utf8.h"

/**
 * @brief Reads a UTF-8 encoded character from the stream and decodes it into a Unicode rune.
 *
 * This function reads a UTF-8 encoded character from the specified stream and decodes it into a Unicode rune.
 * It examines the byte sequence of the UTF-8 character to determine its length and performs the decoding
 * to obtain the corresponding Unicode rune. The decoded rune is stored in the provided pointer.
 *
 * @param st   Pointer to the stream from which to read the UTF-8 character.
 * @param rune Pointer to a variable where the decoded Unicode rune will be stored.
 *
 * @return Returns the number of bytes read from the stream if successful. If the stream has no available bytes,
 *         it returns 0. If the UTF-8 sequence is corrupted or incomplete, it returns a specific error code
 *         (negative value) defined in the implementation.
 *
 *         - `UTF8_RET_CORRUPT`: Returned when the UTF-8 sequence is corrupted.
 *         - `UTF8_RET_OVERFLOW`: Returned when the UTF-8 sequence represents a four-character sequence,
 *           which cannot be implemented due to the rune being 16 bits wide.
 *         - `UTF8_RET_INCOMPLETE`: Returned when the UTF-8 sequence is incomplete and more bytes are required.
 *
 * @see utf8_putr
 * @see utf8_puts
 * @see utf8_puts_P
 * @see utf8_puti
 */
int8_t utf8_getr(Stream *st, rune16_t *rune);

/**
 * @brief Encodes and writes a Unicode rune as a UTF-8 character to the stream.
 *
 * This function encodes the specified Unicode rune as a UTF-8 character and writes it to the provided stream.
 * It examines the value of the rune to determine the appropriate UTF-8 encoding and writes the corresponding
 * byte sequence to the stream.
 *
 * @param st   Pointer to the stream where the UTF-8 character will be written.
 * @param rune The Unicode rune to encode and write.
 *
 * @return Returns the number of bytes written to the stream if successful. If the stream does not have enough
 *         available space to write the UTF-8 character, it returns 0.
 *
 * @see utf8_getr
 * @see utf8_puts
 * @see utf8_puts_P
 * @see utf8_puti
 */
int8_t utf8_putr(Stream *st, const rune16_t rune);

/**
 * @brief Writes a null-terminated string of Unicode runes as UTF-8 characters to the stream.
 *
 * This function encodes and writes a null-terminated string of Unicode runes as UTF-8 characters to the provided stream.
 * It iterates over each rune in the string, encoding and writing it using the utf8_putr function. If there is not enough
 * space in the stream to write a rune, the function stops and returns the number of runes successfully written.
 *
 * @param st   Pointer to the stream where the UTF-8 characters will be written.
 * @param str  Pointer to the null-terminated string of Unicode runes to write.
 *
 * @return Returns the number of Unicode runes (UTF-8 characters) successfully written to the stream. If the stream does
 *         not have enough available space to write a UTF-8 character, the function stops and returns the number of runes
 *         successfully written before encountering the space limitation.
 *
 * @see utf8_getr
 * @see utf8_putr
 * @see utf8_puts_P
 * @see utf8_puti
 */
int utf8_puts(Stream *st, const rune16_t * rune);

/**
 * @brief Writes a null-terminated string of Unicode runes stored in program memory as UTF-8 characters to the stream.
 *
 * This function encodes and writes a null-terminated string of Unicode runes stored in program memory (CC_PROGMEM) as UTF-8
 * characters to the provided stream. It iterates over each rune in the string, retrieving it from program memory using
 * `pgm_read_word` function, and then encoding and writing it using the utf8_putr function. If there is not enough space
 * in the stream to write a rune, the function stops and returns the number of runes successfully written.
 *
 * @param st   Pointer to the stream where the UTF-8 characters will be written.
 * @param str  Pointer to the null-terminated string of Unicode runes stored in program memory to write.
 *
 * @return Returns the number of Unicode runes (UTF-8 characters) successfully written to the stream. If the stream does
 *         not have enough available space to write a UTF-8 character, the function stops and returns the number of runes
 *         successfully written before encountering the space limitation.
 *
 * @see utf8_getr
 * @see utf8_putr
 * @see utf8_puts
 * @see utf8_puti
 * @see pgm_read_word
 */
int utf8_puts_P(Stream *st, const rune16_t * rune);

/**
 * @brief Writes an unsigned 8-bit integer value as a decimal UTF-8 character sequence to the stream.
 *
 * This function writes the provided unsigned 8-bit integer value as a decimal UTF-8 character stream to the specified stream. It uses the
 * `print` function of the stream object to write the value. The `print` function converts the integer value to its textual
 * representation and sends it to the stream. The actual behavior may vary depending on the implementation of the `print`
 * function for the specific stream type.
 *
 * @param st     Pointer to the stream where the UTF-8 character will be written.
 * @param value  The unsigned 8-bit integer value to be written as a UTF-8 character.
 *
 * @return Returns the number of bytes written to the stream. The return value may be used to check if the write operation
 *         was successful or to determine the number of bytes written.
 *
 * @see utf8_getr
 * @see utf8_putr
 * @see utf8_puts
 * @see utf8_puts_P
 * @see Stream::print
 */
int utf8_puti(Stream *st, uint8_t value);

#endif

/**
 * This module provides utf8 support
 * Some functions are taken from https://github.com/9fans/plan9port
 *
 */

/**
 * \file utf8.h
 * \brief UTF-8 support header.
 *
 * This header file provides UTF-8 support functions and type definitions.
 * It includes the "protoduino.h" header file for necessary declarations.
 *
 * \author Joham https://github.com/jklarenbeek
 * \date 2023
 */

#ifndef __UTF8_H__
#define __UTF8_H__

#include <cc.h>
#include <stdbool.h>
#include "rune16.h"

// https://www.ssec.wisc.edu/~tomw/java/unicode.html
// https://gist.github.com/jpassaro/bf400b0410810a071a7fb3509ef6c2c3
// https://www.asciitable.com/


/**
 * \typedef rune32_t
 * \brief 32-bit Unicode character representation.
 *
 * This typedef defines the type "rune32_t" as a 32-bit Unicode character representation.
 * It is used for storing Unicode characters with a maximum value of 0xFFFFFFFF (4294967295).
 *
 */
typedef uint32_t rune32_t;

/**
 * \def UTF8_DECODE_ERROR
 * \brief Decode error value for UTF-8.
 *
 * This macro defines the decode error value for UTF-8 as 0xFFFD.
 * It is used when a decoding error occurs while processing UTF-8 encoded strings.
 */
#define UTF8_DECODE_ERROR 0xFFFD

/**
 * \def UTF8_RET_SUCCESS
 * \brief Return value for successful UTF-8 operation.
 *
 * This macro defines the return value for a successful UTF-8 operation as 0.
 * It is used to indicate that a UTF-8 operation has completed successfully.
 */
#define UTF8_RET_SUCCESS 0

/**
 * \def UTF8_RET_CORRUPT
 * \brief Return value for corrupt UTF-8 data.
 *
 * This macro defines the return value for corrupt UTF-8 data as -1.
 * It is used to indicate that the input data is not a valid UTF-8 encoded string.
 */
#define UTF8_RET_CORRUPT -1

/**
 * \def UTF8_RET_OVERFLOW
 * \brief Return value for buffer overflow in UTF-8 operations.
 *
 * This macro defines the return value for buffer overflow in UTF-8 operations as -2.
 * It is used to indicate that the target buffer is not large enough to hold the result of the operation.
 */
#define UTF8_RET_OVERFLOW -2

/**
 * \def UTF8_RET_INCOMPLETE
 * \brief Return value for incomplete UTF-8 sequence.
 *
 * This macro defines the return value for an incomplete UTF-8 sequence as -3.
 * It is used to indicate that the input data contains an incomplete UTF-8 encoded sequence.
 */
#define UTF8_RET_INCOMPLETE -3

/**
 * \fn char* utf8_ecpy(char *to, char *e, char *from)
 * \brief Safely copy a null-terminated UTF-8 string with size checking.
 *
 * This function safely copies a null-terminated UTF-8 string from `from` to `to`
 * with size checking. It ensures that the copy operation does not exceed the
 * specified size limit `e`. If the copy operation reaches the size limit, it
 * truncates the string and appends a null-terminating character to ensure
 * proper termination.
 *
 * \param to   Pointer to the destination buffer where the UTF-8 string will be copied.
 * \param e    Pointer to the end of the destination buffer.
 * \param from Pointer to the null-terminated UTF-8 string to be copied.
 * \return Pointer to the end of the copied string in the destination buffer.
 */
CC_EXTERN char * utf8_ecpy(char *to, char *e, char *from);

/**
 * \fn uint8_t utf8_torune16(rune16_t *rune, const char *str)
 * \brief Convert a UTF-8 encoded string to a 16-bit Unicode rune.
 *
 * This function converts a UTF-8 encoded string to a 16-bit Unicode rune.
 * It takes the input UTF-8 string `str` and decodes it to determine the Unicode rune value.
 * The decoded rune value is stored in the provided `rune` pointer. The function returns
 * the number of bytes consumed from the UTF-8 string during the decoding process.
 *
 * \param rune Pointer to store the decoded 16-bit Unicode rune.
 * \param str  The UTF-8 encoded string to convert.
 * \return The number of bytes consumed from the UTF-8 string.
 */
CC_EXTERN uint8_t utf8_torune16(rune16_t *rune, const char *str);

/**
 * \fn uint8_t utf8_fromrune16(char *str, const rune16_t rune)
 * \brief Convert a 16-bit Unicode rune to a UTF-8 encoded string.
 *
 * This function converts a 16-bit Unicode rune to a UTF-8 encoded string representation.
 * It takes the input rune and determines the appropriate number of bytes needed to represent
 * the rune in UTF-8 encoding. The UTF-8 encoded bytes are stored in the provided `str` buffer.
 * The function returns the number of bytes written to the `str` buffer.
 *
 * \param str  The buffer to store the UTF-8 encoded string.
 * \param rune The 16-bit Unicode rune to convert to UTF-8 encoding.
 * \return The number of bytes written to the `str` buffer.
 */
CC_EXTERN uint8_t utf8_fromrune16(char *str, const rune16_t rune);

/**
 * \fn int utf8_len(char *s)
 * \brief Calculate the length of a UTF-8 encoded string.
 *
 * This function calculates the length of a UTF-8 encoded string by counting the number
 * of Unicode characters present in the string. It iterates over each byte in the string
 * and determines if it represents a single ASCII character or a multi-byte UTF-8 character,
 * incrementing the count accordingly. The function returns the total number of Unicode characters
 * present in the string.
 *
 * \param s The UTF-8 encoded string to calculate the length for.
 * \return The length of the string in number of Unicode characters.
 */
CC_EXTERN int utf8_len(char *s);

/**
 * \fn uint8_t utf8_rune16len(const rune16_t rune)
 * \brief Get the UTF-8 encoded size of a single 16-bit Unicode character.
 *
 * This function calculates the encoded length of a single 16-bit Unicode character
 * when represented in UTF-8 encoding. The function determines the number of bytes required
 * to encode the given Unicode character based on its value range.
 *
 * \param rune The 16-bit Unicode character to calculate the encoded length for.
 * \return The encoded length of the Unicode character in number of bytes.
 */
CC_EXTERN uint8_t utf8_rune16len(const rune16_t rune);

/**
 * \fn int utf8_rune16nlen(rune16_t *rune, int nlen)
 * \brief Get the UTF-8 encoded size of a sequence of 16-bit Unicode characters.
 *
 * This function calculates the encoded length of a sequence of 16-bit Unicode characters
 * when represented in UTF-8 encoding. It iterates through the array of Unicode characters
 * until it encounters a null character or until the specified number of characters (`nlen`) is reached.
 *
 * \param rune Pointer to the array of 16-bit Unicode characters.
 * \param nlen Number of characters in the sequence.
 * \return The encoded length of the sequence in number of bytes.
 */
CC_EXTERN int utf8_rune16nlen(rune16_t * rune, int nlen);

#endif
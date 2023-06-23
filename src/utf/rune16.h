/**
 * This module provides unicode support
 * most functions are taken from https://github.com/9fans/plan9port
 * 
 */

#ifndef __RUNE16_H__
#define __RUNE16_H__

#include <protoduino.h>
#include "utf8.h"

/**
 * @brief Copies a rune16_t string up to a specified end pointer, from a source string.
 *
 * This function copies characters from the source string `s2` to the destination string `s1`,
 * up to the specified end pointer `es1`. The copying stops when either the null-terminating character
 * of `s2` is encountered or the end pointer `es1` is reached. The resulting string `s1` is always null-terminated,
 * even if the end pointer `es1` is reached before the entire source string is copied.
 *
 * @param s1   The destination string where the characters will be copied.
 * @param es1  The end pointer specifying the maximum limit of the destination string.
 * @param s2   The source string from which the characters will be copied.
 * @return A pointer to the resulting string `s1`.
 *         - If `s1` is greater than or equal to `es1`, `s1` is returned without any modifications.
 *         - Otherwise, `s1` is returned after the copying is complete, with a null-terminating character added if necessary.
 */
CC_EXTERN rune16_t *rune16_strecpy(rune16_t *s1, rune16_t *es1, rune16_t *s2);

/**
 * @brief Unsigned 16-bit integer type representing a Unicode character (rune).
 */
typedef uint16_t rune16_t;

/**
 * @brief Concatenates two rune16_t strings.
 *
 * @param s1 The destination string to which s2 will be appended.
 * @param s2 The source string that will be appended to s1.
 * @return The resulting string after concatenation.
 */
CC_EXTERN rune16_t *rune16_strcat(rune16_t *s1, rune16_t *s2);

/**
 * @brief Locates the first occurrence of a rune16_t character in a string.
 *
 * @param s The string to be searched.
 * @param c The character to be located.
 * @return A pointer to the first occurrence of the character in the string, or NULL if the character is not found.
 */
CC_EXTERN rune16_t *rune16_strchr(rune16_t *s, rune16_t c);

/**
 * @brief Compares two rune16_t strings.
 *
 * @param s1 The first string to compare.
 * @param s2 The second string to compare.
 * @return An integer value indicating the relationship between the strings:
 *         - 0 if the strings are equal.
 *         - A positive value if s1 is greater than s2.
 *         - A negative value if s1 is less than s2.
 */
CC_EXTERN int rune16_strcmp(rune16_t *s1, rune16_t *s2);

/**
 * @brief Copies a rune16_t string.
 *
 * @param s1 The destination string where s2 will be copied.
 * @param s2 The source string to be copied.
 * @return A pointer to the destination string.
 */
CC_EXTERN rune16_t *rune16_strcpy(rune16_t *s1, rune16_t *s2);

/**
 * @brief Locates the last occurrence of a rune16_t character in a string.
 *
 * @param s The string to be searched.
 * @param c The character to be located.
 * @return A pointer to the last occurrence of the character in the string, or NULL if the character is not found.
 */
CC_EXTERN rune16_t *rune16_strrchr(rune16_t *s, rune16_t c);

/**
 * @brief Locates the first occurrence of a rune16_t substring in a string.
 *
 * @param s1 The string to be searched.
 * @param s2 The substring to be located.
 * @return A pointer to the first occurrence of the substring in the string, or NULL if the substring is not found.
 */
CC_EXTERN rune16_t *rune16_strstr(rune16_t *s1, rune16_t *s2);

/**
 * @brief Returns the length of a rune16_t string.
 *
 * @param s The string to be measured.
 * @return The number of characters in the string, excluding the terminating null character.
 */
CC_EXTERN long rune16_strlen(rune16_t *s);

/**
 * @brief Appends a specified number of characters from one rune16_t string to another.
 *
 * @param s1 The destination string to which the characters will be appended.
 * @param s2 The source string from which the characters will be copied.
 * @param n  The maximum number of characters to be copied.
 * @return The resulting string after concatenation.
 */
CC_EXTERN rune16_t *rune16_strncat(rune16_t *s1, rune16_t *s2, long n);

/**
 * @brief Compares a specified number of characters from two rune16_t strings.
 *
 * @param s1 The first string to compare.
 * @param s2 The second string to compare.
 * @param n  The maximum number of characters to compare.
 * @return An integer value indicating the relationship between the strings:
 *         - 0 if the specified number of characters are equal.
 *         - A positive value if s1 is greater than s2.
 *         - A negative value if s1 is less than s2.
 */
CC_EXTERN int rune16_strncmp(rune16_t *s1, rune16_t *s2, long n);

/**
 * @brief Copies a specified number of characters from one rune16_t string to another.
 *
 * @param s1 The destination string where the characters will be copied.
 * @param s2 The source string from which the characters will be copied.
 * @param n  The maximum number of characters to be copied.
 * @return A pointer to the destination string.
 */
CC_EXTERN rune16_t *rune16_strncpy(rune16_t *s1, rune16_t *s2, long n);

/**
 * @brief Returns the lowercase equivalent of a rune16_t character.
 *
 * @param c The character to be converted.
 * @return The lowercase equivalent of the character if it exists, otherwise the character itself.
 */
CC_EXTERN rune16_t rune16_tolower(const rune16_t c);

/**
 * @brief Returns the uppercase equivalent of a rune16_t character.
 *
 * @param c The character to be converted.
 * @return The uppercase equivalent of the character if it exists, otherwise the character itself.
 */
CC_EXTERN rune16_t rune16_toupper(const rune16_t c);

/**
 * @brief Returns the titlecase equivalent of a rune16_t character.
 *
 * @param c The character to be converted.
 * @return The titlecase equivalent of the character if it exists, otherwise the character itself.
 */
CC_EXTERN rune16_t rune16_totitle(const rune16_t c);

/**
 * @brief Checks if a rune16_t character is lowercase.
 *
 * @param c The character to be checked.
 * @return true if the character is lowercase, false otherwise.
 */
CC_EXTERN bool rune16_islower(const rune16_t c);

/**
 * @brief Checks if a rune16_t character is uppercase.
 *
 * @param c The character to be checked.
 * @return true if the character is uppercase, false otherwise.
 */
CC_EXTERN bool rune16_isupper(const rune16_t c);

/**
 * @brief Checks if a rune16_t character is alphabetic.
 *
 * @param c The character to be checked.
 * @return true if the character is alphabetic, false otherwise.
 */
CC_EXTERN bool rune16_isalpha(const rune16_t c);

/**
 * @brief Checks if a rune16_t character is a titlecase character.
 *
 * @param c The character to be checked.
 * @return true if the character is a titlecase character, false otherwise.
 */
CC_EXTERN bool rune16_istitle(const rune16_t c);

/**
 * @brief Checks if a rune16_t character is a whitespace character.
 *
 * @param c The character to be checked.
 * @return true if the character is a whitespace character, false otherwise.
 */
CC_EXTERN bool rune16_isspace(const rune16_t c);

#endif /* __RUNE16_H__ */

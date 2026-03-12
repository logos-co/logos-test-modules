#ifndef LIBSTRUTIL_H
#define LIBSTRUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reverse a string in-place into the provided buffer.
 * @param input  Source string (null-terminated).
 * @param output Buffer for the reversed string (must be at least strlen(input)+1).
 */
void strutil_reverse(const char* input, char* output);

/**
 * Convert a string to uppercase into the provided buffer.
 * @param input  Source string (null-terminated).
 * @param output Buffer for the uppercased string (must be at least strlen(input)+1).
 */
void strutil_uppercase(const char* input, char* output);

/**
 * Convert a string to lowercase into the provided buffer.
 * @param input  Source string (null-terminated).
 * @param output Buffer for the lowercased string (must be at least strlen(input)+1).
 */
void strutil_lowercase(const char* input, char* output);

/**
 * Count the number of characters in a string.
 * @param input Source string (null-terminated).
 * @return Number of characters (excluding null terminator).
 */
int strutil_count_chars(const char* input);

/**
 * Count occurrences of a character in a string.
 * @param input Source string (null-terminated).
 * @param ch    Character to count.
 * @return Number of occurrences.
 */
int strutil_count_char(const char* input, char ch);

/**
 * Return the library version string. Caller must NOT free.
 */
const char* strutil_version(void);

#ifdef __cplusplus
}
#endif

#endif /* LIBSTRUTIL_H */

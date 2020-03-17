// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_span.h
 *
 * @brief An az_span represents a contiguous byte buffer and is used for string manipulations,
 * HTTP requests/responses, building/parsing JSON payloads, and more.
 *
 * NOTE: You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_SPAN_H
#define _az_SPAN_H

#include <az_result.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <_az_cfg_prefix.h>

/**
 * az_span is a "view" over a byte buffer. It contains a pointer to the
 * start of the byte buffer, the buffer's capacity; and the length (number of
 * bytes) in use from the start of the buffer.
 */
typedef struct
{
  struct
  {
    uint8_t* ptr;
    int32_t length;
    int32_t capacity;
  } _internal;
} az_span;

/********************************  SPAN GETTERS */

/**
 * @brief az_span_ptr returns the span byte buffer's starting memory address
 *
 */
AZ_NODISCARD AZ_INLINE uint8_t* az_span_ptr(az_span span) { return span._internal.ptr; }

/**
 * @brief az_span_length returns the span byte buffer's length (number of bytes in use from the
 * buffer's start)
 *
 */
AZ_NODISCARD AZ_INLINE int32_t az_span_length(az_span span) { return span._internal.length; }

/**
 * @brief az_span_capacity returns the span byte buffer's capacity (in bytes)
 *
 */
AZ_NODISCARD AZ_INLINE int32_t az_span_capacity(az_span span) { return span._internal.capacity; }

/********************************  CONSTRUCTORS */

/**
 * @brief Creates an empty span literal
 */
#define AZ_SPAN_LITERAL_NULL \
  { \
    ._internal = {.ptr = NULL, .length = 0, .capacity = 0 } \
  }

/**
 * @brief The AZ_SPAN_NULL macro returns a NULL (or empty) az_span
 *
 */
#define AZ_SPAN_NULL (az_span) AZ_SPAN_LITERAL_NULL

// Returns the size (in bytes) of a literal string
// Note: Concatenating "" to S produces a compiler error if S is not a literal string
#define _az_STRING_LITERAL_LEN(S) (sizeof(S "") - 1)

/**
 * @brief The AZ_SPAN_LITERAL_FROM_STR macro returns a literal az_span over a literal string. For
 * example:
 *
 * `static const az_span hw = AZ_SPAN_LITERAL_FROM_STR("Hello world");`
 */
#define AZ_SPAN_LITERAL_FROM_STR(STRING_LITERAL) \
  { \
    ._internal = { \
      .ptr = (uint8_t*)STRING_LITERAL, \
      .length = _az_STRING_LITERAL_LEN(STRING_LITERAL), \
      .capacity = _az_STRING_LITERAL_LEN(STRING_LITERAL), \
    }, \
  }

/**
 * @brief The AZ_SPAN_FROM_STR macro returns an az_span expression over a literal string. For
 * example: `some_function(AZ_SPAN_FROM_STR("Hello world"));` where `void some_function(const
 * az_span span);`
 */
#define AZ_SPAN_FROM_STR(STRING_LITERAL) (az_span) AZ_SPAN_LITERAL_FROM_STR(STRING_LITERAL)

/**
 * @brief az_span_init returns a span over a byte buffer.
 *
 * @param[in] ptr The memory address of the 1st byte in the byte buffer
 * @param[in] length The number of bytes initialized in the byte buffer
 * @param[in] capacity The number of total bytes in the byte buffer
 * @return az_span The "view" over the byte buffer.
 */
AZ_NODISCARD az_span az_span_init(uint8_t* ptr, int32_t length, int32_t capacity);

/**
 * @brief az_span_from_str returns an az_span from a 0-terminated array of bytes (chars)
 *
 * @param[in] str The pointer to the 0-terminated array of bytes (chars)
 * @return az_span An az_span over the byte buffer; length & capacity are set to the string's
 * length.
 */
AZ_NODISCARD az_span az_span_from_str(char* str);

/**
 * @brief AZ_SPAN_LITERAL_FROM_BUFFER returns a literal az_span over an uninitialized byte buffer.
 * For example:
 *
 * uint8_t buffer[1024];
 * const az_span buf = AZ_SPAN_LITERAL_FROM_BUFFER(buffer);  // Len=0, Cap=1024
 */
#define AZ_SPAN_LITERAL_FROM_BUFFER(BYTE_BUFFER) \
  { \
    ._internal = { \
      .ptr = BYTE_BUFFER, \
      .length = 0, \
      .capacity = (sizeof(BYTE_BUFFER) / sizeof(BYTE_BUFFER[0])), \
    }, \
  }

/**
 * @brief AZ_SPAN_FROM_BUFFER returns an az_span expression over an uninitialized byte buffer. For
 * example:
 *
 * uint8_t buffer[1024];
 * some_function(AZ_SPAN_FROM_BUFFER(buffer));  // Len=0, Cap=1024
 */
#define AZ_SPAN_FROM_BUFFER(BYTE_BUFFER) (az_span) AZ_SPAN_LITERAL_FROM_BUFFER(BYTE_BUFFER)

/**
 * @brief AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER returns a literal az_span over an initialized byte
 * buffer. For example:
 *
 * uint8_t buffer[] = { 1, 2, 3 };
 * const az_span buf = AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(buffer); // Len=3, Cap=3
 */
#define AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(BYTE_BUFFER) \
  { \
    ._internal = { \
      .ptr = BYTE_BUFFER, \
      .length = (sizeof(BYTE_BUFFER) / sizeof(BYTE_BUFFER[0])), \
      .capacity = (sizeof(BYTE_BUFFER) / sizeof(BYTE_BUFFER[0])), \
    }, \
  }

/**
 * @brief AZ_SPAN_FROM_INITIALIZED_BUFFER returns an az_span expression over an initialized byte
 * buffer. For example
 *
 * uint8_t buffer[] = { 1, 2, 3 };
 * const az_span buf = AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(buffer); // Len=3, Cap=3
 */
#define AZ_SPAN_FROM_INITIALIZED_BUFFER(BYTE_BUFFER) \
  (az_span) AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(BYTE_BUFFER)

/******************************  SPAN MANIPULATION */

/**
 * @brief az_span_slice returns a new az_span which is a sub-span of the specified span.
 *
 * @param[in] span The original az_span
 * @param[in] low_index An index into the original az_span indicating where the returned az_span
 * will start
 * @param[in] high_index An index into the original az_span indicating where the returned az_span
 * should stop. The byte at the high_index is NOT included in the returned az_span.
 * @return An az_span (view) into a portion (from low_index to high_index - 1) of the original
 * az_span.
 */
AZ_NODISCARD az_span az_span_slice(az_span span, int32_t low_index, int32_t high_index);

/**
 * @brief az_span_is_content_equal returns `true` if the lengths and bytes referred by \p span1 and
 * \p span2 are identical.
 *
 * @return Returns true if the lengths of both spans are identical and the bytes in both spans are
 * also identical.
 */
AZ_NODISCARD AZ_INLINE bool az_span_is_content_equal(az_span span1, az_span span2)
{
  return az_span_length(span1) == az_span_length(span2)
      && memcmp(az_span_ptr(span1), az_span_ptr(span2), az_span_length(span1)) == 0;
}

/**
 * @brief az_span_is_content_equal_ignoring_case returns `true` if the lengths and characters
 * referred to by \p span1 and \p span2 are identical except for case. This function assumes the
 * bytes in both spans are ASCII characters.
 *
 * @return Returns true if the lengths of both spans are identical and the ASCII characters in both
 * spans are also identical except for case.
 */
AZ_NODISCARD bool az_span_is_content_equal_ignoring_case(az_span span1, az_span span2);

/**
 * @brief az_span_to_str copies a source span containing a string (not 0-terminated) to a
 destination char buffer and appends the 0-terminating byte.
 *
 * The buffer referred to by destination must have a size that is at least 1 byte bigger
 * than the \p source az_span. The string \p destination is converted to a zero-terminated str.
 Content
 * is copied to \p source buffer and then \0 is added at the end. Then out_result will be created
 out
 * of buffer
 *
 * @param[in] destination A pointer to a buffer where the string should be copied
 * @param[in] destination_max_size The maximum available space within the buffer referred to by
 destination.
 * @param[in] source The az_span containing the not-0-terminated string
 * @return An #az_result value indicating the result of the operation.
 *          #AZ_OK If \p source span's content is successfully copied to the destination.
 *          #AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY if the \p destination buffer is too small to
 copy the string and 0-terminate it
 */
AZ_NODISCARD az_result
az_span_to_str(char* destination, int32_t destination_max_size, az_span source);

/******************************  SPAN PARSING */

/**
 * @brief az_span_to_uint64 parses an az_span containing ASCII digits into a uint64 number
 *
 * @param[in] span The az_span containing the ASCII digits to be parsed.
 * @param[in] out_number The pointer to the variable that is to receive the number
 * @return An #az_result value indicating the result of the operation.
 *          #AZ_OK if successful
 *          #AZ_ERROR_PARSER_UNEXPECTED_CHAR if a non-ASCII digit is found within the span.
 */

AZ_NODISCARD az_result az_span_to_uint64(az_span span, uint64_t* out_number);

/**
 * @brief az_span_to_uint32 parses an az_span containing ASCII digits into a uint32 number
 *
 * @param span The az_span containing the ASCII digits to be parsed.
 * @param out_number The pointer to the variable that is to receive the number
 * @return An #az_result value indicating the result of the operation.
 *          #AZ_OK if successful
 *          #AZ_ERROR_PARSER_UNEXPECTED_CHAR if a non-ASCII digit
 * is found within the span.
 */
AZ_NODISCARD az_result az_span_to_uint32(az_span span, uint32_t* out_number);

/******************************  SPAN APPENDING */

/**
 * @brief az_span_append_uint8 appends the uint8 \p byte to the \p destination starting at the
 * destination span's length.
 *
 * @param[in] destination The az_span where the byte should be appended to.
 * @param[in] byte The uint8 to append to the destination span
 * @param[out] out_span A pointer to an az_span that receives the span referring to the
 * destination span with its length increased by 1
 * @return An #az_result value indicating the result of the operation.
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY if the destination is not big enough to
 * contain the appended byte
 */
AZ_NODISCARD az_result az_span_append_uint8(az_span destination, uint8_t byte, az_span* out_span);

/**
 * @brief az_span_append appends the bytes referred to by the source span into the
 * destination span starting at the destination span's length.
 *
 * @param[in] destination The az_span where the bytes should be appended to.
 * @param[in] source Refers to the bytes to be appended to the destination
 * @param[out] out_span A pointer to an az_span that receives the span referring to the
 * destination span with its length updated.
 * @return An #az_result value indicating the result of the operation.
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY if the \p destination is not big enough to
 *  contain the appended bytes.
 */
AZ_NODISCARD az_result az_span_append(az_span destination, az_span source, az_span* out_span);

/**
 * @brief az_span_append_i32toa appends an int32 as digit characters to the destination
 * starting at the destination span's length.
 *
 * @param[in] destination The az_span where the bytes should be appended to
 * @param[in] source The int32 whose number is appended to the destination span as ASCII
 * digits
 * @param[out] out_span A pointer to an az_span that receives the span referring to the
 * destination span with its length updated
 * @return An #az_result value indicating the result of the operation.
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY if the \p destination is not big enough to
 * contain the appended bytes
 */
AZ_NODISCARD az_result
az_span_append_i32toa(az_span destination, int32_t source, az_span* out_span);

/**
 * @brief az_span_append_u32toa appends a uint32 as digit characters to the destination
 * starting at the destination span's length.
 *
 * @param[in] destination The az_span where the bytes should be appended to
 * @param[in] source The uint32 whose number is appended to the destination span as ASCII
 * digits
 * @param[out] out_span A pointer to an az_span that receives the span referring to the
 * destination span with its length updated
 * @return An #az_result value indicating the result of the operation:
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY if the destination is not big enough to
 * contain the appended bytes
 */
AZ_NODISCARD az_result
az_span_append_u32toa(az_span destination, uint32_t source, az_span* out_span);

/**
 * @brief az_span_append_i64toa appends an int64 as digit characters to the destination
 * starting at the destination span's length.
 *
 * @param[in] destination The az_span where the bytes should be appended to
 * @param[in] source The int64 whose number is appended to the destination span as ASCII
 * digits
 * @param[out] out_span A pointer to an az_span that receives the span referring to the
 * destination span with its length updated
 * @return An #az_result value indicating the result of the operation:
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY if the destination is not big enough to
 * contain the appended bytes
 */
AZ_NODISCARD az_result
az_span_append_i64toa(az_span destination, int64_t source, az_span* out_span);

/**
 * @brief az_span_append_u64toa appends a uint64 as digit characters to the destination
 * starting at the destination span's length.
 *
 * @param[in] destination The az_span where the bytes should be appended to
 * @param[in] source The uint64 whose number is appended to the destination span as ASCII
 * digits
 * @param[out] out_span A pointer to an az_span that receives the span referring to the
 * destination span with its length updated
 * @return An #az_result value indicating the result of the operation:
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY if the destination is not big enough to
 * contain the appended bytes
 */
AZ_NODISCARD az_result
az_span_append_u64toa(az_span destination, uint64_t source, az_span* out_span);

/**
 * @brief az_span_append_dtoa appends a double as digit characters to the destination
 * starting at the destination span's length
 *
 * @param[in] destination The az_span where the bytes should be appended to
 * @param[in] source The double whose number is appended to the destination span as ASCII
 * digits
 * @param[out] out_span A pointer to an az_span that receives the span referring to the
 * destination span with its length updated
 * @return An #az_result value indicating the result of the operation:
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY if the destination is not big enough to
 * contain the appended bytes
 */
AZ_NODISCARD az_result az_span_append_dtoa(az_span destination, double source, az_span* out_span);

/******************************  SPAN COPYING */

/**
 * @brief az_span_set sets all the bytes of the destination span (up to its capacity) to \p
 * fill.
 *
 * @param[in] destination The span whose bytes will be set to \p fill
 * @param[in] fill The byte to be replicated within the destination span
 */
AZ_INLINE void az_span_set(az_span destination, uint8_t fill)
{
  memset(az_span_ptr(destination), fill, az_span_capacity(destination));
}

/**
 * @brief az_span_copy copies the content of the source span to the destination span
 *
 * @param[in] destination The span whose bytes will be replaced by the source's bytes
 * @param[in] source The span containing the bytes to copy to the destination
 * @param[out] out_span A pointer to an az_span that receives the span referring to the destination
 * span with its length updated
 * @return An #az_result value indicating the result of the operation.
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY if the \p destination is not big enough to hold the
 * source's length
 */
AZ_NODISCARD az_result az_span_copy(az_span destination, az_span source, az_span* out_span);

/**
 * @brief az_span_copy_url_encode copies a URL in the source span to the destination span by
 * url-encoding the source span's characters
 *
 * @param[in] destination The span whose bytes will receive the url-encoded source
 * @param[in] source The span containing the non-url-encoded bytes
 * @param[out] out_span A pointer to an az_span that receives the span referring to the
 * destination span with its length updated
 * @return An #az_result value indicating the result of the operation.
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY if the \p destination is not big enough to
 * hold the source's length
 */
AZ_NODISCARD az_result
az_span_copy_url_encode(az_span destination, az_span source, az_span* out_span);

/******************************  SPAN PAIR  */

/**
 * An az_pair represents a key/value pair of az_span instances.
 * This is typically used for HTTP query parameters and headers.
 */
typedef struct
{
  az_span key;
  az_span value;
} az_pair;

/**
 * @brief The AZ_PAIR_NULL macro returns an az_pair instance whose key and value fields are
 * initialized to AZ_SPAN_NULL
 *
 */
#define AZ_PAIR_NULL \
  (az_pair) { .key = AZ_SPAN_NULL, .value = AZ_SPAN_NULL }

/**
 * @brief az_pair_init returns an az_pair with its key and value fields initialized to the specified
 * key and value parameters.
 *
 * @param[in] key A span whose bytes represent the key
 * @param[in] value A span whose bytes represent the key's value
 * @return  An az_pair with the field initialized to the parameters' values
 */
AZ_NODISCARD AZ_INLINE az_pair az_pair_init(az_span key, az_span value)
{
  return (az_pair){ .key = key, .value = value };
}

/**
 * @brief az_pair_from_str returns an az_pair with its key and value fields initialized to span's
 * over the specified key and value 0-terminated string parameters.
 *
 * @param[in] key A string representing the key
 * @param[in] value A string representing the key's value
 * @return  An az_pair with the field initialized to the az_span instances over the passed-in
 * strings
 */
AZ_NODISCARD AZ_INLINE az_pair az_pair_from_str(char* key, char* value)
{
  return az_pair_init(az_span_from_str(key), az_span_from_str(value));
}

#include <_az_cfg_suffix.h>

#endif // _az_SPAN_H

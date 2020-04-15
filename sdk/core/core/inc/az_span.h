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
 * start of the byte buffer and the buffer's available size.
 */
typedef struct
{
  struct
  {
    uint8_t* ptr;
    int32_t size; ///< size must be >= 0
  } _internal;
} az_span;

/********************************  SPAN GETTERS */

/**
 * @brief az_span_ptr returns the span byte buffer's starting memory address
 *
 */
AZ_NODISCARD AZ_INLINE uint8_t* az_span_ptr(az_span span) { return span._internal.ptr; }

/**
 * @brief az_span_size Returns the number of bytes within the span.
 *
 */
AZ_NODISCARD AZ_INLINE int32_t az_span_size(az_span span) { return span._internal.size; }

/********************************  CONSTRUCTORS */

/**
 * @brief The AZ_SPAN_NULL macro returns an empty az_span.
 *
 */
#define AZ_SPAN_NULL \
  (az_span) \
  { \
    ._internal = {.ptr = NULL, .size = 0 } \
  }

// Returns the size (in bytes) of a literal string
// Note: Concatenating "" to S produces a compiler error if S is not a literal string
//       The stored string's length does not include the \0 terminator.
#define _az_STRING_LITERAL_LEN(S) (sizeof(S "") - 1)

/**
 * @brief The AZ_SPAN_LITERAL_FROM_STR macro returns a literal az_span over a literal string.
 * An empty ("") literal string results in a span with size set to 0.
 * The size of the span is equal to the length of the string.
 * For example:
 *
 * `static const az_span hw = AZ_SPAN_LITERAL_FROM_STR("Hello world");`
 */
#define AZ_SPAN_LITERAL_FROM_STR(STRING_LITERAL) \
  { \
    ._internal = { \
      .ptr = (uint8_t*)STRING_LITERAL, \
      .size = _az_STRING_LITERAL_LEN(STRING_LITERAL), \
    }, \
  }

/**
 * @brief The AZ_SPAN_FROM_STR macro returns an az_span expression over a literal string. For
 * example: `some_function(AZ_SPAN_FROM_STR("Hello world"));` where `void some_function(const
 * az_span span);`
 */
#define AZ_SPAN_FROM_STR(STRING_LITERAL) (az_span) AZ_SPAN_LITERAL_FROM_STR(STRING_LITERAL)

// Returns 1 if the address of the array is equal to the address of its 1st element
// https://stackoverflow.com/questions/16794900/validate-an-argument-is-array-type-in-c-c-pre-processing-macro-on-compile-time
#define _az_IS_ARRAY(array) ((sizeof(array[0]) == 1) && (((void*)&(array)) == ((void*)(&array[0]))))

/**
 * @brief AZ_SPAN_FROM_BUFFER returns an az_span expression over an uninitialized byte buffer. For
 * example:
 *
 * uint8_t buffer[1024];
 * some_function(AZ_SPAN_FROM_BUFFER(buffer));  // Size = 1024
 *
 * BYTE_BUFFER MUST be an array defined like 'uint8_t buffer[10]'; and not 'uint8_t* buffer'
 */
#define AZ_SPAN_FROM_BUFFER(BYTE_BUFFER) \
  (az_span) \
  { \
    ._internal = { \
      .ptr = (uint8_t*)BYTE_BUFFER, \
      .size = (sizeof(BYTE_BUFFER) / _az_IS_ARRAY(BYTE_BUFFER)), \
    }, \
  }

/**
 * @brief az_span_init returns a span over a byte buffer.
 *
 * @param[in] ptr The memory address of the 1st byte in the byte buffer
 * @param[in] size The number of total bytes in the byte buffer
 * @return az_span The "view" over the byte buffer.
 */
AZ_NODISCARD az_span az_span_init(uint8_t* ptr, int32_t size);

/**
 * @brief az_span_from_str returns an az_span from a 0-terminated array of bytes (chars)
 *
 * @param[in] str The pointer to the 0-terminated array of bytes (chars)
 * @return az_span An az_span over the byte buffer; size is set to the string's length not including
 * the \0 terminator.
 */
AZ_NODISCARD az_span az_span_from_str(char* str);

/******************************  SPAN MANIPULATION */

/**
 * @brief az_span_slice returns a new az_span which is a sub-span of the specified span.
 *
 * @param[in] span The original az_span.
 * @param[in] start_index An index into the original az_span indicating where the returned az_span
 * will start.
 * @param[in] end_index An index into the original az_span indicating where the returned az_span
 * should stop. The byte at the high_index is NOT included in the returned az_span.
 * @return An az_span into a portion (from \p start_index to \p end_index - 1) of the original
 * az_span.
 */
AZ_NODISCARD az_span az_span_slice(az_span span, int32_t start_index, int32_t end_index);

/**
 * @brief az_span_is_content_equal returns `true` if the sizes and bytes referred by \p span1 and
 * \p span2 are identical.
 *
 * @return Returns true if the sizes of both spans are identical and the bytes in both spans are
 * also identical.
 */
AZ_NODISCARD AZ_INLINE bool az_span_is_content_equal(az_span span1, az_span span2)
{
  return az_span_size(span1) == az_span_size(span2)
      && memcmp(az_span_ptr(span1), az_span_ptr(span2), (size_t)az_span_size(span1)) == 0;
}

/**
 * @brief az_span_is_content_equal_ignoring_case returns `true` if the sizes and characters
 * referred to by \p span1 and \p span2 are identical except for case. This function assumes the
 * bytes in both spans are ASCII characters.
 *
 * @return Returns true if the sizes of both spans are identical and the ASCII characters in both
 * spans are also identical except for case.
 */
AZ_NODISCARD bool az_span_is_content_equal_ignoring_case(az_span span1, az_span span2);

/**
 * @brief az_span_to_str copies a source span containing a string (not 0-terminated) to a
 destination char buffer and appends the 0-terminating byte.
 *
 * The buffer referred to by destination must have a size that is at least 1 byte bigger
 * than the \p source az_span. The string \p destination is converted to a zero-terminated str.
 * Content is copied to \p source buffer and then \0 is added at the end.
 *
 * @param[in] destination A pointer to a buffer where the string should be copied
 * @param[in] destination_max_size The maximum available space within the buffer referred to by
 * destination.
 * @param[in] source The az_span containing the not-0-terminated string
 * @return An #az_result value indicating the result of the operation.
 *          #AZ_OK If \p source span content is successfully copied to the destination.
 *          #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the \p destination buffer is too small to
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
 *          #AZ_ERROR_PARSER_UNEXPECTED_CHAR if a non-ASCII digit is found within the span.
 */
AZ_NODISCARD az_result az_span_to_uint32(az_span span, uint32_t* out_number);

/**
 * @brief az_span_find searches for `target` in `source`, returning an #az_span within `source` if
 * it finds it.
 *
 * @param[in] source The #az_span with the content to be searched on.
 * @param[in] target The #az_span containing the token to be searched in `source`.
 * @return The position of `target` in `source` if `source` contains `target`,
 *         0 if `target` is empty (if its size is equal zero),
 *         -1 if `source` is empty (if its size is equal zero) and `target` is non-empty,
 *         -1 if `target` is not found in `source`.
 */
AZ_NODISCARD int32_t az_span_find(az_span source, az_span target);

/******************************  SPAN COPYING */

/**
 * @brief az_span_copy copies the content of the source span to the destination span.
 *
 * @param[in] destination The span whose bytes will be replaced by the source's bytes.
 * @param[in] source The span containing the bytes to copy to the destination.
 * @return An #az_span that is a slice of the \p destination span (i.e. the remainder) after the
 * source bytes has been copied.
 *
 * @remarks The method assumes that the \p destination has a large enough size to hold the \p
 * source.
 * @remarks This method copies all of source to destination even if source and destination overlap.
 */
az_span az_span_copy(az_span destination, az_span source);

/**
 * @brief az_span_copy_uint8 copies the uint8 \p byte to the \p destination at its 0-th index.
 *
 * @param[in] destination The az_span where the byte should be copied to.
 * @param[in] byte The uint8 to copy into the destination span.
 * @return An #az_span that is a slice of the \p destination span (i.e. the remainder) after the
 * byte has been copied. The method assumes that the \p destination has a large enough size to
 * hold one more byte.
 */
az_span az_span_copy_uint8(az_span destination, uint8_t byte);

/**
 * @brief az_span_copy_url_encode Copies a URL in the source span to the destination span by
 * url-encoding the source span characters.
 *
 * @param[in] destination The span whose bytes will receive the url-encoded source
 * @param[in] source The span containing the non-url-encoded bytes
 * @param[out] out_span A pointer to an az_span that receives the remainder of the destination span
 * after the url-encoded source has been copied.
 * @return An #az_result value indicating the result of the operation.
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the \p destination is not big enough to contain
 * the encoded bytes
 */
AZ_NODISCARD az_result
az_span_copy_url_encode(az_span destination, az_span source, az_span* out_span);

/**
 * @brief az_span_copy_i32toa copies an int32 as digit characters to the destination starting at its
 * 0-th index.
 *
 * @param[in] destination The az_span where the bytes should be copied to.
 * @param[in] source The int32 whose number is copied to the destination span as ASCII digits.
 * @param[out] out_span A pointer to an az_span that receives the remainder of the destination span
 * after the int32 has been copied.
 * @return An #az_result value indicating the result of the operation.
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the \p destination is not big enough to contain
 * the copied bytes
 */
AZ_NODISCARD az_result az_span_copy_i32toa(az_span destination, int32_t source, az_span* out_span);

/**
 * @brief az_span_copy_u32toa copies a uint32 as digit characters to the destination starting at its
 * 0-th index.
 *
 * @param[in] destination The az_span where the bytes should be copied to.
 * @param[in] source The uint32 whose number is copied to the destination span as ASCII digits.
 * @param[out] out_span A pointer to an az_span that receives the remainder of the destination span
 * after the uint32 has been copied.
 * @return An #az_result value indicating the result of the operation:
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the destination is not big enough to contain the
 * copied bytes
 */
AZ_NODISCARD az_result az_span_copy_u32toa(az_span destination, uint32_t source, az_span* out_span);

/**
 * @brief az_span_copy_i64toa copies an int64 as digit characters to the destination starting at its
 * 0-th index.
 *
 * @param[in] destination The az_span where the bytes should be copied to.
 * @param[in] source The int64 whose number is copied to the destination span as ASCII digits.
 * @param[out] out_span A pointer to an az_span that receives the remainder of the destination span
 * after the int64 has been copied.
 * @return An #az_result value indicating the result of the operation:
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the destination is not big enough to contain the
 * copied bytes
 */
AZ_NODISCARD az_result az_span_copy_i64toa(az_span destination, int64_t source, az_span* out_span);

/**
 * @brief az_span_copy_u64toa copies a uint64 as digit characters to the destination starting at its
 * 0-th index.
 *
 * @param[in] destination The az_span where the bytes should be copied to.
 * @param[in] source The uint64 whose number is copied to the destination span as ASCII digits.
 * @param[out] out_span A pointer to an az_span that receives the remainder of the destination span
 * after the uint64 has been copied.
 * @return An #az_result value indicating the result of the operation:
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the destination is not big enough to contain the
 * copied bytes
 */
AZ_NODISCARD az_result az_span_copy_u64toa(az_span destination, uint64_t source, az_span* out_span);

/**
 * @brief az_span_copy_dtoa copies a double as digit characters to the destination starting at its
 * 0-th index.
 *
 * @param[in] destination The az_span where the bytes should be copied to.
 * @param[in] source The double whose number is copied to the destination span as ASCII digits.
 * @param[out] out_span A pointer to an az_span that receives the remainder of the destination span
 * after the double has been copied.
 * @return An #az_result value indicating the result of the operation:
 *          #AZ_OK if successful
 *          #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the destination is not big enough to contain the
 * copied bytes
 */
AZ_NODISCARD az_result az_span_copy_dtoa(az_span destination, double source, az_span* out_span);

/**
 * @brief az_span_fill Fills all the bytes of the destination span with the specified value.
 *
 * @param[in] destination The span whose bytes will be set to \p value.
 * @param[in] value The byte to be replicated within the destination span.
 */
AZ_INLINE void az_span_fill(az_span destination, uint8_t value)
{
  memset(az_span_ptr(destination), value, (size_t)az_span_size(destination));
}

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
 * initialized to AZ_SPAN_NULL.
 *
 */
#define AZ_PAIR_NULL \
  (az_pair) { .key = AZ_SPAN_NULL, .value = AZ_SPAN_NULL }

/**
 * @brief az_pair_init returns an az_pair with its key and value fields initialized to the specified
 * key and value parameters.
 *
 * @param[in] key A span whose bytes represent the key.
 * @param[in] value A span whose bytes represent the key's value.
 * @return  An az_pair with the field initialized to the parameters' values.
 */
AZ_NODISCARD AZ_INLINE az_pair az_pair_init(az_span key, az_span value)
{
  return (az_pair){ .key = key, .value = value };
}

/**
 * @brief az_pair_from_str returns an az_pair with its key and value fields initialized to span's
 * over the specified key and value 0-terminated string parameters.
 *
 * @param[in] key A string representing the key.
 * @param[in] value A string representing the key's value.
 * @return  An az_pair with the fields initialized to the az_span instances over the passed-in
 * strings.
 */
AZ_NODISCARD AZ_INLINE az_pair az_pair_from_str(char* key, char* value)
{
  return az_pair_init(az_span_from_str(key), az_span_from_str(value));
}

#include <_az_cfg_suffix.h>

#endif // _az_SPAN_H

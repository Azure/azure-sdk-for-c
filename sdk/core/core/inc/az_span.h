// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_H
#define _az_SPAN_H

#include <az_result.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <_az_cfg_prefix.h>

/**
 * An immutable span of bytes (octets).
 */
typedef struct {
  struct {
    uint8_t * ptr;
    int32_t length;
    int32_t capacity;
  } _internal;
} az_span;

/********************************  SPAN GETTERS */

/**
 * @brief convenient way to get the pointer from one span
 *
 */
AZ_NODISCARD AZ_INLINE uint8_t * az_span_ptr(az_span span) { return span._internal.ptr; }

/**
 * @brief convenient way to get the length from one span
 *
 */
AZ_NODISCARD AZ_INLINE int32_t az_span_length(az_span span) { return span._internal.length; }

/**
 * @brief convenient way to get the capacity from one span
 *
 */
AZ_NODISCARD AZ_INLINE int32_t az_span_capacity(az_span span) { return span._internal.capacity; }

/********************************  CONSTRUCTORS */

/**
 * @brief Creates an empty span
 *
 */
AZ_NODISCARD AZ_INLINE az_span az_span_null() { return (az_span){ 0 }; }

// A size of the string literal.
// Details: to make sure that `S` is a `string literal`, we are appending `""`
// to `S`.
#define AZ_STRING_LITERAL_LEN(S) (sizeof(S "") - 1)

/**
 * Creates a az_span literal which can be used to initialize a constant. For example
 *
 * `static const az_span foo = AZ_SPAN_LITERAL_FROM_STR("Hello world");`
 */
#define AZ_SPAN_LITERAL_FROM_STR(STRING_LITERAL) \
  { \
    ._internal \
        = {.ptr = (uint8_t *)STRING_LITERAL, \
           .length = AZ_STRING_LITERAL_LEN(STRING_LITERAL), \
           .capacity = AZ_STRING_LITERAL_LEN(STRING_LITERAL), \
          } \
  }

/**
 * @brief Creates a az_span expression. For example,
 * `some_function(AZ_SPAN_FROM_STR("Hello world"));`
 * where
 * `void some_function(const az_span span);`
 */
#define AZ_SPAN_FROM_STR(STRING_LITERAL) (az_span) AZ_SPAN_LITERAL_FROM_STR(STRING_LITERAL)

/**
 * @brief Init span
 *
 */
AZ_NODISCARD AZ_INLINE az_span az_span_init(uint8_t * ptr, int32_t length, int32_t capacity) {
  return (az_span){ ._internal = { .ptr = ptr, .length = length, .capacity = capacity } };
}

/**
 * @brief Creates a span from zero terminated string
 *
 */
AZ_NODISCARD AZ_INLINE az_span az_span_from_str(char * str) {
  int32_t length = strlen(str);
  return az_span_init((uint8_t *)str, length, length);
}

/**
 * Creates a az_span literal which can be used to initialize a constant. For example
 *
 * uint8_t buffer[2 * 1024];
 * const az_span foo = AZ_SPAN_LITERAL_FROM_BUFFER(buffer);
 */
#define AZ_SPAN_LITERAL_FROM_BUFFER(BYTE_BUFFER) \
  { \
    ._internal \
        = {.ptr = BYTE_BUFFER, \
           .length = 0, \
           .capacity = (sizeof(BYTE_BUFFER) / sizeof(BYTE_BUFFER[0])) } \
  }

#define AZ_SPAN_FROM_BUFFER(BYTE_BUFFER) (az_span) AZ_SPAN_LITERAL_FROM_BUFFER(BYTE_BUFFER)

/**
 * Creates a az_span literal which can be used to initialize a constant. For example
 *
 * uint8_t buffer[] = {"Hello World"};
 * const az_span foo = AZ_SPAN_LITERAL_FROM_BUFFER(buffer);
 */
#define AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(BYTE_BUFFER) \
  { \
    ._internal \
        = {.ptr = BYTE_BUFFER, \
           .length = (sizeof(BYTE_BUFFER) / sizeof(BYTE_BUFFER[0])), \
           .capacity = (sizeof(BYTE_BUFFER) / sizeof(BYTE_BUFFER[0])) } \
  }

#define AZ_SPAN_FROM_INITIALIZED_BUFFER(BYTE_BUFFER) \
  (az_span) AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(BYTE_BUFFER)

/******************************  SPAN MANIPULATION */

/**
 * @brief Returns a sub span of the given span.
 *
 */
AZ_NODISCARD az_result
az_span_slice(az_span span, int32_t low_index, int32_t high_index, az_span * out_sub_span);

/**
 * @brief Returns `true` if a content of the @a span is equal to a content of the @b
 * span.
 */
AZ_NODISCARD AZ_INLINE bool az_span_is_equal(az_span a, az_span b) {
  return az_span_length(a) == az_span_length(b)
      && memcmp(az_span_ptr(a), az_span_ptr(b), az_span_length(a)) == 0;
}

/**
 * @brief Returns `true` if a content of the @a span is equal to a content of the @b
 * span using case-insensitive compare.
 *
 * @param a comparing span
 * @param b comparing span
 * @return AZ_NODISCARD az_span_is_equal_content_ignoring_case
 */
AZ_NODISCARD bool az_span_is_content_equal_ignoring_case(az_span a, az_span b);

/**
 * @brief convert all the span content to uint64
 *
 * @param self source span
 * @param out result span
 * @return AZ_NODISCARD az_span_to_uint64
 */
AZ_NODISCARD az_result az_span_to_uint64(az_span self, uint64_t * out);

/**
 * @brief move the content of span @b src to @b dst
 *
 * @param dst buffer where to copy span
 * @param src span to be copied
 * @param out span resulting after copy
 * @return AZ_NODISCARD az_span_move
 */
AZ_NODISCARD az_result az_span_copy(az_span dst, az_span src, az_span * out);

// TODO: this will become az_span_append once we remove actions ....
/**
 * @brief append az_span if there is enough capacity for it
 *
 * @param self src span where to append
 * @param span content to be appended
 * @return AZ_NODISCARD az_span_append_
 */
AZ_NODISCARD az_result az_span_append(az_span self, az_span span, az_span * out);

// TODO: remove this signature/function once actions are GONE
/**
 * @brief append az_span if there is enough capacity for it
 *
 * @param self src span where to append
 * @param span content to be appended
 * @return AZ_NODISCARD az_span_append_
 */
// AZ_NODISCARD az_result az_span_append_(az_span * self, az_span span);

/**
 * @brief converts @b src span to zero-terminated srt. Content is copied to @b buffer and then \0 is
 * addeed at the end. Then out_result will be created out of buffer
 *
 * @param s buffer where to write str
 * @param max_size span to str to create
 * @param span span with zero terminated str
 * @return AZ_NODISCARD az_span_to_str
 */
AZ_NODISCARD az_result az_span_to_str(char * s, int32_t max_size, az_span span);

#include <_az_cfg_suffix.h>

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_span.h
 *
 * @brief az_span and utilities definition
 */

#ifndef _az_SPAN_H
#define _az_SPAN_H

#include <az_result.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <_az_cfg_prefix.h>

/**
 * An immutable span of bytes (octets).
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
 * @brief convenient way to get the pointer from one span
 *
 */
AZ_NODISCARD AZ_INLINE uint8_t* az_span_ptr(az_span span) { return span._internal.ptr; }

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
#define AZ_SPAN_NULL (az_span){ ._internal.ptr = NULL, ._internal.length = 0, ._internal.capacity = 0 }

// A size of the string literal.
// Details: to make sure that `S` is a `string literal`, we are appending `""`
// to `S`.
#define _az_STRING_LITERAL_LEN(S) (sizeof(S "") - 1)

/**
 * Creates an az_span literal which can be used to initialize a constant. For example
 *
 * `static const az_span foo = AZ_SPAN_LITERAL_FROM_STR("Hello world");`
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
AZ_NODISCARD AZ_INLINE az_span az_span_init(uint8_t* ptr, int32_t length, int32_t capacity)
{
  return (az_span){ ._internal = { .ptr = ptr, .length = length, .capacity = capacity, }, };
}

/**
 * @brief Creates a span from zero terminated string
 *
 */
AZ_NODISCARD AZ_INLINE az_span az_span_from_str(char* str)
{
  int32_t length = (int32_t)strlen(str);
  return az_span_init((uint8_t*)str, length, length);
}

/**
 * Creates an az_span literal which can be used to initialize a constant. For example
 *
 * uint8_t buffer[2 * 1024];
 * const az_span foo = AZ_SPAN_LITERAL_FROM_BUFFER(buffer);
 */
#define AZ_SPAN_LITERAL_FROM_BUFFER(BYTE_BUFFER) \
  { \
    ._internal = { \
      .ptr = BYTE_BUFFER, \
      .length = 0, \
      .capacity = (sizeof(BYTE_BUFFER) / sizeof(BYTE_BUFFER[0])), \
    }, \
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
    ._internal = { \
      .ptr = BYTE_BUFFER, \
      .length = (sizeof(BYTE_BUFFER) / sizeof(BYTE_BUFFER[0])), \
      .capacity = (sizeof(BYTE_BUFFER) / sizeof(BYTE_BUFFER[0])), \
    }, \
  }

#define AZ_SPAN_FROM_INITIALIZED_BUFFER(BYTE_BUFFER) \
  (az_span) AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(BYTE_BUFFER)

/******************************  SPAN MANIPULATION */

/**
 * @brief Returns a sub span of the given span.
 *
 */
AZ_NODISCARD az_result
az_span_slice(az_span span, int32_t low_index, int32_t high_index, az_span* out_sub_span);

/**
 * @brief Returns `true` if a content of the @a span is equal to a content of the @b
 * span.
 */
AZ_NODISCARD AZ_INLINE bool az_span_is_equal(az_span a, az_span b)
{
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
AZ_NODISCARD az_result az_span_to_uint64(az_span self, uint64_t* out);

/**
 * @brief move the content of span @b src to @b dst
 *
 * @param dst buffer where to copy span
 * @param src span to be copied
 * @param out span resulting after copy
 * @return AZ_NODISCARD az_span_move
 */
AZ_NODISCARD az_result az_span_copy(az_span dst, az_span src, az_span* out);

AZ_NODISCARD az_result az_span_copy_url_encode(az_span dst, az_span src, az_span* out);

/**
 * @brief append az_span if there is enough capacity for it
 *
 * @param self src span where to append
 * @param span content to be appended
 * @return AZ_NODISCARD az_span_append_
 */
AZ_NODISCARD az_result az_span_append(az_span self, az_span span, az_span* out);

/**
 * @brief converts @b src span to zero-terminated str. Content is copied to @b buffer and then \0 is
 * addeed at the end. Then out_result will be created out of buffer
 *
 * @param s buffer where to write str
 * @param max_size span to str to create
 * @param span span with zero terminated str
 * @return AZ_NODISCARD az_span_to_str
 */
AZ_NODISCARD az_result az_span_to_str(char* s, int32_t max_size, az_span span);

AZ_NODISCARD az_result az_span_append_double(az_span span, double value, az_span* out);

AZ_NODISCARD az_result az_span_append_uint64(az_span* self, uint64_t n);

AZ_NODISCARD az_result az_span_append_int64(az_span* self, int64_t n);

/**
 * @brief a pair of az_span of bytes as a key and value
 *
 */
typedef struct
{
  az_span key;
  az_span value;
} az_pair;

AZ_NODISCARD AZ_INLINE az_pair az_pair_init(az_span key, az_span value)
{
  return (az_pair){ .key = key, .value = value };
}

AZ_NODISCARD AZ_INLINE az_pair az_pair_null() {
  return az_pair_init(AZ_SPAN_NULL, AZ_SPAN_NULL);
}

AZ_NODISCARD AZ_INLINE az_pair az_pair_from_str(char* key, char* value)
{
  return az_pair_init(az_span_from_str(key), az_span_from_str(value));
}

/**
 * @brief Set all the content of the span to @b fill without updating the length of the span.
 * This is util to set memory to zero before using span or make sure span is clean before use
 *
 * @param span source span
 * @param fill byte to use for filling span
 */
AZ_INLINE void az_span_set(az_span span, uint8_t fill)
{
  memset(az_span_ptr(span), fill, az_span_capacity(span));
}

/**
 * @brief                  Appends one uint8_t value to the #az_span if there is enough capacity for it.
 * @param[in]  span        #az_span where to append the char.
 * @param[in]  c           Value to be appended.
 * @param[out] out_span    #az_span returned as result of the concatenation.
 * @return                 An #az_result value indicating the result of the operation.
 *                         #AZ_OK                      If no errors occurs.
 *                         #AZ_ERROR_BUFFER_OVERFLOW   If there is not enough capacity left to append the `c`.
 */
AZ_NODISCARD az_result az_span_append_uint8(az_span span, uint8_t c, az_span* out_span);

/**
 * @brief                 Appends one uint32_t to the #az_span if there is enough capacity for it.
 * @param[in]  span       #az_span where to append the value.
 * @param[in]  n          Value to be appended.
 * @param[out] out_span   Pointer where to store the resulting #az_span.
 * @return                An #az_result value indicating the result of the operation.
 *                        #AZ_OK                      If no errors occurs.
 *                        #AZ_ERROR_BUFFER_OVERFLOW   If there is not enough capacity left to append the `n`.
 */
AZ_NODISCARD az_result az_span_append_u32toa(az_span span, uint32_t n, az_span* out_span);

/**
 * @brief                 Appends one int32_t to the #az_span if there is enough capacity for it.
 * @param[in]  span       #az_span where to append the value.
 * @param[in]  n          Value to be appended.
 * @param[out] out_span   Pointer where to store the resulting #az_span.
 * @return                An #az_result value indicating the result of the operation.
 *                        #AZ_OK                      If no errors occurs.
 *                        #AZ_ERROR_BUFFER_OVERFLOW   If there is not enough capacity left to append the `n`.
 */
AZ_NODISCARD az_result az_span_append_i32toa(az_span span, int32_t n, az_span* out_span);

#include <_az_cfg_suffix.h>

#endif // _az_SPAN_H

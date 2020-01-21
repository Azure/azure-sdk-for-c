// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_H
#define _az_SPAN_H

#include <az_result.h>
#include <az_str.h>

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
    uint8_t * begin;
    int32_t length;
    int32_t capacity;
  } _internal;
} az_span;

/********************************  SPAN GETTERS */

/**
 * @brief convenient way to get the capacity from one span
 *
 */
AZ_NODISCARD AZ_INLINE int32_t az_span_capacity_get(az_span const span) {
  return span._internal.capacity;
}

/**
 * @brief convenient way to get the length from one span
 *
 */
AZ_NODISCARD AZ_INLINE int32_t az_span_length_get(az_span const span) {
  return span._internal.length;
}

/**
 * @brief convenient way to get the pointer from one span
 *
 */
AZ_NODISCARD AZ_INLINE uint8_t * az_span_ptr_get(az_span const span) {
  return span._internal.begin;
}

/********************************  CONSTRUCTORS */

/**
 * @brief Creates an empty span
 *
 */
AZ_NODISCARD AZ_INLINE az_span az_span_empty() { return (az_span){ 0 }; }

/**
 * Creates a span which can be used inside constant initializers. For example
 *
 * `static az_const_span foo[] = { AZ_CONST_STR("Hello"), AZ_CONST_STR("world!") };`
 */
#define AZ_SPAN_FROM_CONST_STR(STRING_LITERAL) \
  { .begin = (uint8_t const *)STRING_LITERAL, .size = AZ_STRING_LITERAL_LEN(STRING_LITERAL), }

/**
 * @brief Creates a span which can be passed as a paramater. For example,
 * `some_function(AZ_STR("Hello world"));`
 * where
 * `void some_function(az_span const span);`
 */
#define AZ_SPAN_FROM_STR(STRING_LITERAL) (az_span) AZ_SPAN_FROM_CONST_STR(STRING_LITERAL)

/**
 * @brief Creates a span from zero terminated string
 *
 */
AZ_NODISCARD AZ_INLINE az_span az_span_from_str(char const * str) {
  return (az_span){
    ._internal = { .begin = (uint8_t const *)str, .capacity = strlen(str), .length = strlen(str) }
  };
}

/**
 * @brief creates one span from array. Capacity and Length would be the same.
 *
 */
#define AZ_SPAN_FROM_ARRAY(ARRAY) \
  { \
    ._internal \
        = {.begin = ARRAY, \
           .capacity = (sizeof(ARRAY) / sizeof(*ARRAY)), \
           .length = (sizeof(ARRAY) / sizeof(*ARRAY)) } \
  }

/**
 * @brief Use this function when size for one span is given in runtime
 * Don't use this function for arrays. Use @var AZ_SPAN_FROM_ARRAY instead.
 *
 */
AZ_NODISCARD AZ_INLINE az_span az_span_from_array(uint8_t const * ptr, int32_t size) {
  return (az_span){ ._internal = { .begin = ptr, .capacity = size, .length = size } };
}

/******************************  SPAN MANIPULATION */

/**
 * @brief Returns a sub span of the given span.
 *
 */
AZ_NODISCARD az_result az_span_sub(
    az_span const span,
    int32_t const low_index,
    int32_t high_index,
    az_span * out_sub_span);

/**
 * @brief Returns `true` if a content of the @a span is equal to a content of the @b
 * span.
 */
AZ_NODISCARD AZ_INLINE bool az_span_is_content_equal(az_span const a, az_span const b) {
  return az_span_length_get(a) == az_span_length_get(b)
      && memcmp(az_span_ptr_get(a), az_span_ptr_get(b), az_span_length_get(a)) == 0;
}

/**
 * @brief Returns `true` if a capacity of the @a span is equal to a capacity of the @b
 * span.
 */
AZ_NODISCARD AZ_INLINE bool az_span_is_capacity_equal(az_span const a, az_span const b) {
  return az_span_capacity_get(a) == az_span_capacity_get(b);
}

/**
 * @brief Returns `true` if span @b a is equal span @b b on content and capacity
 *
 */
AZ_NODISCARD AZ_INLINE bool az_span_is_equal(az_span const a, az_span const b) {
  return az_span_is_capacity_equal(a, b) && az_span_is_content_equal(a, b);
}

/**
 * @brief Returns `true` if a content of the @a span is equal to a content of the @b
 * span using case-insensetive compare.
 *
 * @param a comparing span
 * @param b comparing span
 * @return AZ_NODISCARD az_span_is_equal_content_ignoring_case
 */
AZ_NODISCARD bool az_span_is_content_equal_ignoring_case(az_span const a, az_span const b);

/**
 * @brief convert all the span content to uint64
 *
 * @param self
 * @param out
 * @return AZ_NODISCARD az_span_to_uint64
 */
AZ_NODISCARD az_result az_span_to_uint64(az_span const self, uint64_t * const out);

/**
 * @brief move the content of span @b src to @buffer and make @b out_result point to it
 *
 * @param buffer
 * @param src
 * @param out_result
 * @return AZ_NODISCARD az_mut_span_move
 */
AZ_NODISCARD az_result az_span_copy(az_span const dst, az_span const src, int32_t * out_result);

/**
 * @brief append az_span if there is enough capacity for it
 *
 * @param self src span where to append
 * @param span content to be appended
 * @return AZ_NODISCARD az_span_append
 */
AZ_NODISCARD az_result az_span_append(az_span * const self, az_span const span);

/**
 * @brief Append only only byte if there is available capacity for it
 *
 * @param self src span where to append
 * @param c byte to be appended
 * @return AZ_NODISCARD az_span_append_byte
 */
AZ_NODISCARD az_result az_span_append_byte(az_span * const self, uint8_t const c);

/**
 * @brief converts @b src span to zero-terminated srt. Content is copied to @b buffer and then \0 is
 * addeed at the end. Then out_result will be created out of buffer
 *
 * @param buffer
 * @param src
 * @param out_result
 * @return AZ_NODISCARD az_mut_span_to_str
 */
AZ_NODISCARD az_result
az_span_to_str(az_span const buffer, az_span const src, az_span * const out_result);

#include <_az_cfg_suffix.h>

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_H
#define _az_SPAN_H

#include <az_action.h>
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
    uint8_t * begin;
    size_t capacity;
    size_t length;
  } _internal;
} az_span;

AZ_NODISCARD AZ_INLINE az_span az_span_empty() { return (az_span){ 0 }; }

AZ_NODISCARD AZ_INLINE bool az_span_is_empty(az_span const span) {
  return span._internal.length == 0;
}

/**
 * @brief returns a span with the left @var n bytes of the given @var span.
 *
 * If the @var n is greater than the span capacity then the whole @var span is returned.
 */
AZ_NODISCARD AZ_INLINE az_span az_span_take(az_span const span, size_t const n) {
  if (span._internal.capacity <= n) {
    return span;
  }
  return (az_span){ ._internal = { .begin = span._internal.begin, .capacity = n, .length = n } };
}

/**
 * @brief returns a span with @var n positions are dropped.
 *
 * If the @var n is greater than span capacity then an empty span is returned
 */
AZ_NODISCARD AZ_INLINE az_span az_span_drop(az_span const span, size_t const n) {
  if (span._internal.capacity <= n) {
    return az_span_empty();
  }
  size_t const length_and_capacity = span._internal.length - n;
  return (az_span){ ._internal = { .begin = span._internal.begin + n,
                                   .capacity = length_and_capacity,
                                   .length = length_and_capacity } };
}

/**
 * Returns a sub span of the given span.
 */
AZ_NODISCARD AZ_INLINE az_span
az_span_sub(az_span const span, size_t const begin, size_t const end) {
  az_span const t = az_span_take(span, end);
  return az_span_drop(t, begin);
}

/**
 * Returns `true` if a content of the @a span is equal to a content of the @b
 * span.
 */
AZ_NODISCARD AZ_INLINE bool az_span_is_equal_content(az_span const a, az_span const b) {
  return a._internal.length == b._internal.length
      && memcmp(a._internal.begin, b._internal.begin, a._internal.length) == 0;
}

/**
 * Returns `true` if a capacity of the @a span is equal to a capacity of the @b
 * span.
 */
AZ_NODISCARD AZ_INLINE bool az_span_is_equal_capacity(az_span const a, az_span const b) {
  return a._internal.capacity == b._internal.capacity;
}

/**
 * Returns `true` if span @b a is equal span @b b on content and capacity
 */
AZ_NODISCARD AZ_INLINE bool az_span_is_equal(az_span const a, az_span const b) {
  return az_span_is_equal_capacity(a, b) && az_span_is_equal_content(a, b);
}

/**
 * Returns `true` if a content of the @a span is equal to a content of the @b
 * span using case-insensetive compare.
 */
AZ_NODISCARD bool az_span_is_equal_content_ignoring_case(az_span const a, az_span const b);

AZ_NODISCARD AZ_INLINE bool az_span_is_overlap(az_span const a, az_span const b) {
  return (!az_span_is_empty(a) && !az_span_is_empty(b))
      && ((a._internal.begin < b._internal.begin
           && (a._internal.begin + a._internal.length - 1) >= b._internal.begin)
          || (b._internal.begin < a._internal.begin
              && (b._internal.begin + b._internal.length - 1) >= a._internal.begin)
          || (a._internal.begin == b._internal.begin));
}

AZ_NODISCARD az_result az_span_to_uint64(az_span const self, uint64_t * const out);

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
AZ_NODISCARD AZ_INLINE az_span az_span_from_runtime_array(uint8_t const * ptr, size_t size) {
  return (az_span){ ._internal = { .begin = ptr, .capacity = size, .length = size } };
}

/**
 * @brief Use this only to create a span from uint8_t object.
 * The size of the returned span is always one.
 * Don't use this function for arrays. Use @var AZ_SPAN_FROM_ARRAY instead.
 * Don't us
 */
AZ_NODISCARD AZ_INLINE az_span az_span_from_single_item(uint8_t const * ptr) {
  return az_span_from_runtime_array(ptr, 1);
}

AZ_NODISCARD AZ_INLINE az_span az_str_to_span(char const * str) {
  return (az_span){
    ._internal = { .begin = (uint8_t const *)str, .capacity = strlen(str), .length = strlen(str) }
  };
}

/****************** Mutating az_span (used to be az_mut_span in the origins)     ******/

/**
 * @brief move the content of span @b src to @buffer and make @b out_result point to it
 *
 * @param buffer
 * @param src
 * @param out_result
 * @return AZ_NODISCARD az_mut_span_move
 */
AZ_NODISCARD az_result
az_span_move(az_span const buffer, az_span const src, az_span * const out_result);

/**
 * @brief move the content from span @b a to span @b b and viceverse.
 * The smallest span (less content) is swapped only. Example
 * a = 111
 * b = 22
 * after swaping
 * a = 221
 * b = 11
 *
 * @param a source/destination span
 * @param b destination/source span
 */
void az_span_swap(az_span const a, az_span const b);

/**
 * @brief Set all the content of the span to @b fill without updating the length of the span.
 * This is util to set memory to zero before using span or make sure span is clean before use
 *
 * @param span source span
 * @param fill byte to use for filling span
 * @return AZ_INLINE az_span_fill
 */
AZ_INLINE void az_span_fill(az_span const span, uint8_t const fill) {
  if (!az_span_is_empty(span)) {
    memset(span._internal.begin, fill, span._internal.capacity);
  }
}

/**
 * @brief Set one specific index of az_span from span content only
 *
 */
AZ_NODISCARD AZ_INLINE az_result
az_span_set(az_span const self, size_t const i, uint8_t const value) {
  if (self._internal.length <= i) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  self._internal.begin[i] = value;
  return AZ_OK;
}

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

/****************** Building az_span (used to be az_span_builder in the origins) *****/

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
 * @brief Append @b size number of zeros to @b self if there is enough capacity for it
 *
 * @param self src span where to append
 * @param size number of zeros to be appended
 * @return AZ_NODISCARD az_span_append_zeros
 */
AZ_NODISCARD az_result az_span_append_zeros(az_span * const self, size_t const size);

/**
 * @brief Replace all contents from a starting position to an end position with the content of a
 * provided span
 *
 * @param self src span where to replace content
 * @param start starting position where to replace
 * @param end end position where to replace
 * @param span content to use for replacement
 * @return AZ_NODISCARD az_span_replace
 */
AZ_NODISCARD az_result
az_span_replace(az_span * const self, size_t start, size_t end, az_span const span);

#include <_az_cfg_suffix.h>

#endif

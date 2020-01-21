// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_PRIVATE_H
#define _az_SPAN_PRIVATE_H

#include <az_action.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

typedef int32_t az_result_byte;

AZ_NODISCARD AZ_INLINE az_span_init(az_span * self, az_span ref) {
  self->_internal.begin = ref._internal.begin;
  self->_internal.length = ref._internal.length;
  self->_internal.capacity = ref._internal.capacity;
}

/**
 * @brief returns a span with the left @var n bytes of the given @var span.
 *
 * If the @var n is greater than the span capacity then the whole @var span is returned.
 */
AZ_NODISCARD AZ_INLINE az_span az_span_take(az_span const span, int32_t const n) {
  if (az_span_capacity_get(span) <= n) {
    return span;
  }
  return (az_span){ ._internal = { .begin = span._internal.begin, .capacity = n, .length = n } };
}

/**
 * @brief returns a span with @var n positions are dropped.
 * example
 * span = { length = 3, capacity = 5}
 * drop(span, 2)
 * span = { length = 1, capacity = 3}
 *
 * If the @b n is greater than span capacity then an empty span is returned
 */
AZ_NODISCARD AZ_INLINE az_span az_span_drop(az_span const span, int32_t const n) {
  if (az_span_capacity_get(span) <= n) {
    return az_span_empty();
  }
  return (az_span){ ._internal = { .begin = span._internal.begin + n,
                                   .capacity = span._internal.capacity - n,
                                   .length = span._internal.length - n } };
}

AZ_NODISCARD AZ_INLINE bool az_span_is_empty(az_span const span) {
  return span._internal.length == 0;
}

/**
 * Returns a byte in `index` position.
 * Returns `AZ_ERROR_EOF` if the `index` is out of the span range.
 */
AZ_NODISCARD AZ_INLINE az_result_byte az_span_get(az_span const span, int32_t const index) {
  if (az_span_length_get(span) <= index) {
    return AZ_ERROR_EOF;
  }
  return az_span_ptr_get(span)[index];
}

// Parsing utilities
AZ_NODISCARD AZ_INLINE az_result az_error_unexpected_char(az_result_byte const c) {
  return az_failed(c) ? c : AZ_ERROR_PARSER_UNEXPECTED_CHAR;
}

AZ_NODISCARD AZ_INLINE bool az_span_is_overlap(az_span const a, az_span const b) {
  return (!az_span_is_empty(a) && !az_span_is_empty(b))
      && ((a._internal.begin < b._internal.begin
           && (a._internal.begin + a._internal.length - 1) >= b._internal.begin)
          || (b._internal.begin < a._internal.begin
              && (b._internal.begin + b._internal.length - 1) >= a._internal.begin)
          || (a._internal.begin == b._internal.begin));
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
az_span_set(az_span const self, int32_t const i, uint8_t const value) {
  if (self._internal.length <= i) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  self._internal.begin[i] = value;
  return AZ_OK;
}

/**
 * @brief Append @b size number of zeros to @b self if there is enough capacity for it
 *
 * @param self src span where to append
 * @param size number of zeros to be appended
 * @return AZ_NODISCARD az_span_append_zeros
 */
AZ_NODISCARD az_result az_span_append_zeros(az_span * const self, int32_t const size);

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
az_span_replace(az_span * const self, int32_t start, int32_t end, az_span const span);

#include <_az_cfg_suffix.h>

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_PRIVATE_H
#define _az_SPAN_PRIVATE_H

#include <az_result.h>
#include <az_span.h>
#include <az_span_internal.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

typedef int32_t az_result_byte;

/**
 * @brief returns a span with the left @var n bytes of the given @var span.
 *
 * If the @var n is greater than the span capacity then the whole @var span is returned.
 */
AZ_NODISCARD AZ_INLINE az_span az_span_take(az_span span, int32_t n) {
  if (az_span_capacity(span) <= n) {
    return span;
  }
  return az_span_init(az_span_ptr(span), n, n);
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
AZ_NODISCARD AZ_INLINE az_span az_span_drop(az_span span, int32_t n) {
  if (az_span_capacity(span) <= n) {
    return az_span_null();
  }
  int32_t current_length = az_span_length(span);
  int32_t current_capacity = az_span_capacity(span);

  int32_t new_length = current_length < n ? 0 : current_length - n;

  return az_span_init(az_span_ptr(span) + n, new_length, current_capacity - n);
}

/**
 * Returns a byte in `index` position.
 * Returns `AZ_ERROR_EOF` if the `index` is out of the span range.
 */
AZ_NODISCARD AZ_INLINE az_result_byte az_span_get(az_span const span, int32_t const index) {
  if (az_span_length(span) <= index) {
    return AZ_ERROR_EOF;
  }
  return az_span_ptr(span)[index];
}

// Parsing utilities
AZ_NODISCARD AZ_INLINE az_result az_error_unexpected_char(az_result_byte const c) {
  return az_failed(c) ? c : AZ_ERROR_PARSER_UNEXPECTED_CHAR;
}

AZ_NODISCARD AZ_INLINE bool az_span_is_overlap(az_span const a, az_span const b) {
  uint8_t * a_ptr = az_span_ptr(a);
  uint8_t * b_ptr = az_span_ptr(b);
  int32_t a_length = az_span_length(a);
  int32_t b_length = az_span_length(b);
  return (!az_span_is_empty(a) && !az_span_is_empty(b))
      && ((a_ptr < b_ptr && (a_ptr + a_length - 1) >= b_ptr)
          || (b_ptr < a_ptr && (b_ptr + b_length - 1) >= a_ptr) || (a_ptr == b_ptr));
}

/**
 * @brief Use this only to create a span from uint8_t object.
 * The size of the returned span is always one.
 * Don't use this function for arrays. Use @var AZ_SPAN_FROM_ARRAY instead.
 * Don't us
 */
AZ_NODISCARD AZ_INLINE az_span az_span_from_single_item(uint8_t * ptr) {
  return az_span_init(ptr, 1, 1);
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
 * @brief Set one specific index of az_span from span content only
 *
 */
AZ_NODISCARD AZ_INLINE az_result
az_span_set(az_span const self, int32_t const i, uint8_t const value) {
  if (self._internal.length <= i) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  az_span_ptr(self)[i] = value;
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

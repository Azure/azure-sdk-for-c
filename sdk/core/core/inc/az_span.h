// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_H
#define AZ_SPAN_H

#include <az_action.h>
#include <az_contract.h>
#include <az_result.h>
#include <az_static_assert.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <_az_cfg_prefix.h>

/**
 * An immutable span of bytes (octets).
 */
typedef struct {
  uint8_t const * begin;
  size_t size;
} az_span;

typedef az_result az_result_byte;

AZ_NODISCARD AZ_INLINE bool az_span_is_empty(az_span const span) { return span.size <= 0; }

AZ_NODISCARD AZ_INLINE bool az_span_is_valid(az_span const span) {
  return span.size == 0 || (span.begin != NULL && span.begin <= span.begin + span.size - 1);
}

/**
 * Returns a byte in `index` position.
 * Returns `AZ_ERROR_EOF` if the `index` is out of the span range.
 */
AZ_NODISCARD AZ_INLINE az_result_byte az_span_get(az_span const span, size_t const index) {
  if (span.size <= index) {
    return AZ_ERROR_EOF;
  }
  return span.begin[index];
}

/**
 * @brief returns a span with the left @n bytes of the given @span.
 *
 * If the @n is greater than the @span.size than the whole @span is returned.
 */
AZ_NODISCARD AZ_INLINE az_span az_span_take(az_span const span, size_t const n) {
  if (span.size <= n) {
    return span;
  }
  return (az_span){ .begin = span.begin, .size = n };
}

/**
 * @brief returns a span with @n positions are dropped.
 *
 * If the @n is greater than @span.size than an empty span is returned
 */
AZ_NODISCARD AZ_INLINE az_span az_span_drop(az_span const span, size_t const n) {
  if (span.size <= n) {
    return (az_span){ .begin = NULL, .size = 0 };
  }
  return (az_span){ .begin = span.begin + n, .size = span.size - n };
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
AZ_NODISCARD AZ_INLINE bool az_span_eq(az_span const a, az_span const b) {
  return a.size == b.size && memcmp(a.begin, b.begin, a.size) == 0;
}

/**
 * Returns `true` if a content of the @a span is equal to a content of the @b
 * span using case-insensetive compare.
 */
AZ_NODISCARD AZ_INLINE bool az_span_eq_ignore_case(az_span const a, az_span const b);

AZ_NODISCARD AZ_INLINE bool az_span_is_overlap(az_span const a, az_span const b) {
  return (!az_span_is_empty(a) && !az_span_is_empty(b))
      && ((a.begin < b.begin && (a.begin + a.size - 1) >= b.begin)
          || (b.begin < a.begin && (b.begin + b.size - 1) >= a.begin) || (a.begin == b.begin));
}

#define AZ_ARRAY_SIZE(ARRAY) (sizeof(ARRAY) / sizeof(*ARRAY))

#define AZ_SPAN_FROM_ARRAY(ARRAY) \
  { .begin = ARRAY, .size = AZ_ARRAY_SIZE(ARRAY) }

/**
 * ```c
 * typedef struct {
 *   az_result (* func)(void *, az_const_span);
 *   void * self;
 * } az_span_action;
 * ```
 *
 * Example of usage
 *
 * ```c
 * az_span_action const action = ...;
 * az_span_action_do(append, AZ_STR("Something"));
 * ```
 */
AZ_ACTION_TYPE(az_span_action, az_span)

#include <_az_cfg_suffix.h>

#endif

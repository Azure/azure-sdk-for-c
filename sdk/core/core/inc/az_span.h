// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_H
#define AZ_SPAN_H

#include <az_callback.h>
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
  /**
   * Points to the first byte.
   */
  uint8_t const * begin;
  /**
   * A number of bytes in the span.
   */
  size_t size;
} az_const_span;

/**
 * A mutable span of bytes.
 */
typedef struct {
  uint8_t * begin;
  size_t size;
} az_span;

typedef az_result az_result_byte;

AZ_INLINE bool az_span_is_empty(az_span const span) { return !(span.size > 0); }

AZ_INLINE bool az_const_span_is_empty(az_const_span const span) { return !(span.size > 0); }

AZ_INLINE bool az_span_is_valid(az_span const span) {
  return span.begin == NULL ? span.size == 0
                            : ((span.size == 0 || span.size > 0)
                               && (span.size == 0 || span.begin <= (span.begin + span.size - 1)));
}

AZ_INLINE bool az_const_span_is_valid(az_const_span const span) {
  return span.begin == NULL ? span.size == 0
                            : ((span.size == 0 || span.size > 0)
                               && (span.size == 0 || span.begin <= (span.begin + span.size - 1)));
}

/**
 * Returns a byte in `index` position.
 * Returns `AZ_ERROR_EOF` if the `index` is out of the span range.
 */
AZ_INLINE az_result_byte az_const_span_get(az_const_span const span, size_t const index) {
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
AZ_INLINE az_const_span az_const_span_take(az_const_span const span, size_t const n) {
  if (span.size <= n) {
    return span;
  }
  return (az_const_span){ .begin = span.begin, .size = n };
}

/**
 * @brief returns a span with @n positions are dropped.
 *
 * If the @n is greater than @span.size than an empty span is returned
 */
AZ_INLINE az_const_span az_const_span_drop(az_const_span const span, size_t const n) {
  if (span.size <= n) {
    return (az_const_span){ .begin = NULL, .size = 0 };
  }
  return (az_const_span){ .begin = span.begin + n, .size = span.size - n };
}

/**
 * Returns a sub span of the given span.
 */
AZ_INLINE az_const_span
az_const_span_sub(az_const_span const span, size_t const begin, size_t const end) {
  az_const_span const t = az_const_span_take(span, end);
  return az_const_span_drop(t, begin);
}

/**
 * Cast the given mutable span to an immutable span.
 */
AZ_INLINE az_const_span az_span_to_const_span(az_span const span) {
  return (az_const_span){ .begin = span.begin, .size = span.size };
}

/**
 * Returns `true` if a content of the @a span is equal to a content of the @b
 * span.
 */
AZ_INLINE bool az_const_span_eq(az_const_span const a, az_const_span const b) {
  return a.size == b.size && memcmp(a.begin, b.begin, a.size) == 0;
}

AZ_INLINE az_result
az_span_move(az_span const buffer, az_const_span const src, az_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  if (!az_span_is_valid(buffer) || !az_const_span_is_valid(src)) {
    return AZ_ERROR_ARG;
  }

  if (buffer.size < src.size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  if (!az_const_span_is_empty(src)) {
    memmove((void *)buffer.begin, (void const *)src.begin, src.size * sizeof(*src.begin));
  }

  out_result->begin = buffer.begin;
  out_result->size = src.size;

  return AZ_OK;
}

AZ_INLINE bool az_const_span_is_overlap(az_const_span const a, az_const_span const b) {
  return (!az_const_span_is_empty(a) && !az_const_span_is_empty(b))
      && ((a.begin < b.begin && (a.begin + a.size - 1) >= b.begin)
          || (b.begin < a.begin && (b.begin + b.size - 1) >= a.begin) || (a.begin == b.begin));
}

AZ_INLINE bool az_span_is_overlap(az_span const a, az_span const b) {
  return az_const_span_is_overlap(az_span_to_const_span(a), az_span_to_const_span(b));
}

AZ_INLINE az_result
az_span_copy(az_span const buffer, az_const_span const src, az_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  if (!az_span_is_valid(buffer) || !az_const_span_is_valid(src)) {
    return AZ_ERROR_ARG;
  }

  if (az_const_span_is_overlap(az_span_to_const_span(buffer), src)) {
    return AZ_ERROR_ARG;
  }

  if (buffer.size < src.size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  if (!az_const_span_is_empty(src)) {
    memcpy((void *)buffer.begin, (void const *)src.begin, src.size);
  }

  out_result->begin = buffer.begin;
  out_result->size = src.size;

  return AZ_OK;
}

AZ_INLINE az_span az_span_drop(az_span const span, size_t const n) {
  if (span.size <= n) {
    return (az_span){ .begin = NULL, .size = 0 };
  }
  return (az_span){ .begin = span.begin + n, .size = span.size - n };
}

AZ_INLINE az_span az_span_take(az_span const span, size_t n) {
  if (span.size <= n) {
    return span;
  }
  return (az_span){ .begin = span.begin, .size = n };
}

AZ_INLINE az_result az_span_set(az_span const span, uint8_t const fill) {
  if (!az_span_is_valid(span)) {
    return AZ_ERROR_ARG;
  }

  if (!az_span_is_empty(span)) {
    memset(span.begin, fill, span.size);
  }

  return AZ_OK;
}

AZ_INLINE az_result az_span_swap(az_span const a, az_span const b) {
  if (!az_span_is_valid(a) || !az_span_is_valid(b)) {
    return AZ_ERROR_ARG;
  }

  if (a.size != b.size || az_span_is_overlap(a, b)) {
    return AZ_ERROR_ARG;
  }

  if (!az_span_is_empty(a)) {
    uint8_t * a_ptr = a.begin;
    uint8_t * b_ptr = b.begin;
    uint8_t const * const a_end = a.begin + a.size;
    do {
      uint8_t const old_a = *a_ptr;
      *a_ptr = *b_ptr;
      *b_ptr = old_a;

      ++a_ptr;
      ++b_ptr;
    } while (a_ptr != a_end);
  }

  return AZ_OK;
}

AZ_INLINE az_result
az_span_to_c_str(az_span const buffer, az_const_span const src, az_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);

  if (!az_const_span_is_valid(src)) {
    return AZ_ERROR_ARG;
  }

  AZ_CONTRACT_ARG_NOT_NULL(buffer.begin);
  if (buffer.size < src.size + 1) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  if (!az_const_span_is_empty(src)) {
    memmove((void *)buffer.begin, (void const *)src.begin, src.size);
  }

  buffer.begin[src.size] = '\0';

  out_result->begin = buffer.begin;
  out_result->size = src.size + 1;

  return AZ_OK;
}

az_result az_span_replace(
    az_span const buffer,
    az_const_span const src,
    uint8_t (*const func)(uint8_t const),
    az_span * const out_result);

#define AZ_ARRAY_SIZE(ARRAY) (sizeof(ARRAY) / sizeof(*ARRAY))

#define AZ_SPAN(ARRAY) \
  { .begin = ARRAY, .size = AZ_ARRAY_SIZE(ARRAY) }

/**
 * ```c
 * typedef struct {
 *   az_result (*func)(ptrdiff_t, az_const_span);
 *   ptrdiff_t data;
 * } az_span_visitor;
 * ```
 *
 * Example of usage
 *
 * ```c
 * az_span_visitor const visitor = ...;
 * visitor.func(visitor.data, AZ_CONST_SPAN("Something"));
 * ```
 */
AZ_CALLBACK_DECL(az_span_visitor, az_const_span)

#include <_az_cfg_suffix.h>

#endif

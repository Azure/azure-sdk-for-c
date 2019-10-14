// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_H
#define AZ_SPAN_H

#include <az_contract.h>
#include <az_result.h>
#include <az_static_assert.h>

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <_az_cfg_prefix.h>

#define AZ_CONTRACT_ARG_VALID_SPAN(arg) \
  do { \
    if ((arg).begin == NULL && (arg).size > 0) { \
      return AZ_ERROR_ARG; \
    } \
  } while (0)

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

AZ_STATIC_ASSERT(CHAR_BIT == 8)

typedef az_result az_result_byte;

AZ_INLINE bool az_span_is_empty(az_span const span) { return !(span.size > 0); }

AZ_INLINE bool az_const_span_is_empty(az_const_span const span) { return span.size == 0; }

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
  return a.size == b.size && memcmp(a.begin, b.begin, a.size * sizeof(*b.begin)) == 0;
}

AZ_INLINE az_result az_span_move(az_span * const p_dest, az_const_span const src) {
  AZ_CONTRACT_ARG_NOT_NULL(p_dest);
  AZ_CONTRACT_ARG_VALID_SPAN(*p_dest);
  AZ_CONTRACT_ARG_VALID_SPAN(src);

  if (p_dest->size < src.size) {
    return AZ_ERROR_BUFFER_SPACE;
  }

  if (!az_const_span_is_empty(src)) {
    memmove((void *)p_dest->begin, (void const *)src.begin, p_dest->size * sizeof(*p_dest->begin));
  }

  p_dest->size = src.size;

  return AZ_OK;
}

AZ_INLINE uint8_t * az_span_end(az_span const span) { return &span.begin[span.size]; }

AZ_INLINE uint8_t const * az_const_span_end(az_const_span const span) {
  return &span.begin[span.size];
}

AZ_INLINE bool az_const_span_is_overlap(az_const_span const a, az_const_span const b) {
  uint8_t const * const a_begin = a.begin;
  uint8_t const * const a_end = az_const_span_end(a);
  uint8_t const * const b_begin = b.begin;
  uint8_t const * const b_end = az_const_span_end(b);

  return (!az_const_span_is_empty(a) && !az_const_span_is_empty(b))
      && ((a_begin < b_begin && a_end > b_begin) || (b_begin < a_begin && b_end > a_begin));
}

AZ_INLINE bool az_span_is_overlap(az_span const a, az_span const b) {
  return az_const_span_is_overlap(az_span_to_const_span(a), az_to_const_span(b));
}

AZ_INLINE az_result az_span_copy(az_span * const p_dest, az_const_span const src) {
  AZ_CONTRACT_ARG_NOT_NULL(p_dest);
  AZ_CONTRACT_ARG_VALID_SPAN(*p_dest);
  AZ_CONTRACT_ARG_VALID_SPAN(src);

  if (az_const_span_is_overlap(az_span_to_const_span(*p_dest), src)) {
    return AZ_ERROR_ARG;
  }

  if (p_dest->size < src.size) {
    return AZ_ERROR_BUFFER_SPACE;
  }

  if (!az_const_span_is_empty(src)) {
    memcpy((void *)p_dest->begin, (void const *)src.begin, p_dest->size * sizeof(*p_dest->begin));
  }

  p_dest->size = src.size;

  return AZ_OK;
}

AZ_INLINE az_span az_span_drop(az_span const span, size_t n) {
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

AZ_INLINE az_result az_span_set(az_span const span, uint8_t fill) {
  AZ_CONTRACT_ARG_VALID_SPAN(span);

  if (!az_span_is_empty(span)) {
    memset(span.begin, fill, span.size);
  }

  return AZ_OK;
}

AZ_INLINE az_result az_span_swap(az_span const a, az_span const b) {
  AZ_CONTRACT_ARG_VALID_SPAN(a);
  AZ_CONTRACT_ARG_VALID_SPAN(b);

  if (a.size != b.size || az_span_is_overlap(a, b)) {
    return AZ_ERROR_ARG;
  }

  if (!az_span_is_empty(a)) {
    uint8_t * a_ptr = a.begin;
    uint8_t * b_ptr = b.begin;
    uint8_t const * const a_end = az_span_end(a);
    do {
      uint8_t const old_a = *a_ptr;
      *a_ptr = *b_ptr;
      *b_ptr = old_a;

      ++a_ptr;
      ++b_ptr;
    } while (a_ptr <= a_end);
  }

  return AZ_OK;
}

AZ_INLINE az_result az_span_to_c_str(az_span * const p_dest, az_const_span const src) {
  AZ_CONTRACT_ARG_NOT_NULL(p_dest);
  AZ_CONTRACT_ARG_NOT_NULL(p_dest->begin);
  if (p_dest->size < 1) {
    return AZ_ERROR_ARG;
  }

  az_span tmp = (az_span){ .begin = p_dest->begin, .size = p_dest->size - 1 };
  AZ_RETURN_IF_FAILED(az_span_move(&tmp, src));

  tmp.begin[tmp.size] = '\0';
  ++tmp.size;
  *p_dest = tmp;

  return AZ_OK;
}

#define AZ_ARRAY_SIZE(ARRAY) (sizeof(ARRAY) / sizeof(*ARRAY))

#define AZ_SPAN(ARRAY) \
  { .begin = ARRAY, .size = AZ_ARRAY_SIZE(ARRAY) }

#include <_az_cfg_suffix.h>

#endif

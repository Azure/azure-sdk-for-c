// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_H
#define AZ_SPAN_H

#include <az_result.h>
#include <az_static_assert.h>

#include <limits.h>
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

AZ_STATIC_ASSERT(CHAR_BIT == 8)

typedef az_result az_result_byte;

static inline bool az_const_span_is_empty(az_const_span const span) { return span.size == 0; }

/**
 * Returns a byte in `index` position.
 * Returns `AZ_ERROR_EOF` if the `index` is out of the span range.
 */
static inline az_result_byte az_const_span_get(az_const_span const span, size_t const index) {
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
static inline az_const_span az_const_span_take(az_const_span const span, size_t const n) {
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
static inline az_const_span az_const_span_drop(az_const_span const span, size_t const n) {
  if (span.size <= n) {
    return (az_const_span){ .begin = NULL, .size = 0 };
  }
  return (az_const_span){ .begin = span.begin + n, .size = span.size - n };
}

/**
 * Returns a sub span of the given span.
 */
static inline az_const_span az_const_span_sub(
    az_const_span const span,
    size_t const begin,
    size_t const end) {
  az_const_span const t = az_const_span_take(span, end);
  return az_const_span_drop(t, begin);
}

/**
 * Cast the given mutable span to an immutable span.
 */
static inline az_const_span az_to_const_span(az_span const span) {
  return (az_const_span){ .begin = span.begin, .size = span.size };
}

/**
 * Returns `true` if a content of the @a span is equal to a content of the @b
 * span.
 */
static inline bool az_const_span_eq(az_const_span const a, az_const_span const b) {
  return a.size == b.size && memcmp(a.begin, b.begin, a.size) == 0;
}

static inline az_result az_span_copy(az_const_span const src, az_span const dst) { 
  if (dst.size < src.size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  memcpy(dst.begin, src.begin, src.size);
  return AZ_OK;
}

static inline az_span az_span_drop(az_span const span, size_t n) {
  if (span.size <= n) {
    return (az_span){ .begin = NULL, .size = 0 };
  }
  return (az_span){ .begin = span.begin + n, .size = span.size - n };
}

static inline az_span az_span_take(az_span const span, size_t n) {
  if (span.size <= n) {
    return span;
  }
  return (az_span){ .begin = span.begin, .size = n };
}

#define AZ_SPAN(ARRAY) \
  { .begin = ARRAY, .size = sizeof(ARRAY) / sizeof(*ARRAY) }

#include <_az_cfg_suffix.h>

#endif

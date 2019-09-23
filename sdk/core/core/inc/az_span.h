// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_H
#define AZ_SPAN_H

#include <az_assert.h>

#include <az_static_assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

// An immutable span of bytes (octets).
typedef struct {
  // Points to the first byte.
  uint8_t const *begin;
  // A number of bytes in the span.
  size_t size;
} az_const_span;

// A mutable span of bytes.
typedef struct {
  uint8_t *begin;
  size_t size;
} az_span;

AZ_STATIC_ASSERT(CHAR_BIT == 8);

inline uint8_t const *az_const_span_ptr(az_const_span const span, size_t const index) {
  AZ_ASSERT(index <= span.size);
  return span.begin + index;
}

inline uint8_t az_const_span_get(az_const_span const span, size_t const index) {
  AZ_ASSERT(index < span.size);
  return span.begin[index];
}

inline uint8_t const *az_const_span_end(az_const_span const span) {
  return az_const_span_ptr(span, span.size);
}

inline size_t az_const_span_index(az_const_span const span, uint8_t const *const p) {
  size_t const result = p - span.begin;
  AZ_ASSERT(result <= span.size);
  return result;
}

inline az_const_span az_const_sub_span(az_const_span const span, size_t const from, size_t const to) {
  AZ_ASSERT(from <= to);
  AZ_ASSERT(to <= span.size);
  return (az_const_span){ .begin = az_const_span_ptr(span, from), .size = to - from };
}

inline az_const_span az_to_const_span(az_span const span) {
  return (az_const_span) { .begin = span.begin, .size = span.size };
}

inline bool az_const_span_eq(az_const_span const a, az_const_span const b) {
  return a.size == b.size && memcmp(a.begin, b.begin, a.size) == 0;
}

#ifdef __cplusplus
}
#endif

#endif

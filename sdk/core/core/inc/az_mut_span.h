// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_MUT_SPAN_H
#define AZ_MUT_SPAN_H

#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <_az_cfg_prefix.h>

/**
 * A mutable span of bytes.
 */
typedef struct {
  uint8_t * begin;
  size_t size;
} az_mut_span;

AZ_NODISCARD AZ_INLINE az_mut_span az_mut_span_create_empty() { return (az_mut_span){ 0 }; }

AZ_NODISCARD AZ_INLINE bool az_mut_span_is_empty(az_mut_span const span) { return span.size == 0; }

/**
 * Cast the given mutable span to an immutable span.
 */
AZ_NODISCARD AZ_INLINE az_span az_mut_span_to_span(az_mut_span const span) {
  return (az_span){ .begin = span.begin, .size = span.size };
}

AZ_NODISCARD AZ_INLINE bool az_mut_spans_overlap(az_mut_span const a, az_mut_span const b) {
  return az_span_is_overlap(az_mut_span_to_span(a), az_mut_span_to_span(b));
}

AZ_NODISCARD az_result
az_mut_span_move(az_mut_span const buffer, az_span const src, az_mut_span * const out_result);

AZ_NODISCARD AZ_INLINE az_mut_span az_mut_span_drop(az_mut_span const span, size_t const n) {
  if (span.size <= n) {
    return az_mut_span_create_empty();
  }
  return (az_mut_span){ .begin = span.begin + n, .size = span.size - n };
}

AZ_NODISCARD AZ_INLINE az_mut_span az_mut_span_take(az_mut_span const span, size_t n) {
  if (span.size <= n) {
    return span;
  }
  return (az_mut_span){ .begin = span.begin, .size = n };
}

AZ_INLINE void az_mut_span_fill(az_mut_span const span, uint8_t const fill) {
  if (!az_mut_span_is_empty(span)) {
    memset(span.begin, fill, span.size);
  }
}

AZ_NODISCARD AZ_INLINE az_result
az_mut_span_set(az_mut_span const self, size_t const i, uint8_t const value) {
  if (self.size <= i) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  self.begin[i] = value;
  return AZ_OK;
}

AZ_NODISCARD az_result
az_mut_span_to_str(az_mut_span const buffer, az_span const src, az_mut_span * const out_result);

#include <_az_cfg_suffix.h>

#endif

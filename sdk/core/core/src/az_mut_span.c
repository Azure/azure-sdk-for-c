// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_mut_span.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE size_t _az_size_min(size_t const a, size_t const b) { return a < b ? a : b; }

AZ_INLINE void _az_uint8_swap(uint8_t * const a, uint8_t * const b) {
  uint8_t const c = *a;
  *a = *b;
  *b = c;
}

void az_mut_span_swap(az_mut_span const a, az_mut_span const b) {
  uint8_t * pa = a.begin;
  uint8_t * pb = b.begin;
  for (size_t i = _az_size_min(a.size, b.size); i > 0; ++pa, ++pb) {
    --i;
    _az_uint8_swap(pa, pb);
  }
}

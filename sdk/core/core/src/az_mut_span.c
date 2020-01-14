// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_mut_span.h>
#include <az_mut_span_internal.h>
#include <az_span_internal.h>

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

  AZ_NODISCARD az_result az_mut_span_move(
      az_mut_span const buffer, az_span const src, az_mut_span * const out_result) {
    AZ_CONTRACT_ARG_NOT_NULL(out_result);

    AZ_CONTRACT_ARG_VALID_MUT_SPAN(buffer);
    AZ_CONTRACT_ARG_VALID_SPAN(src);

    AZ_CONTRACT(buffer.size >= src.size, AZ_ERROR_BUFFER_OVERFLOW);

    if (!az_span_is_empty(src)) {
      memmove((void *)buffer.begin, (void const *)src.begin, src.size);
    }

    out_result->begin = buffer.begin;
    out_result->size = src.size;

    return AZ_OK;
  }

  AZ_NODISCARD az_result az_mut_span_to_str(
      az_mut_span const buffer, az_span const src, az_mut_span * const out_result) {
    AZ_CONTRACT_ARG_NOT_NULL(out_result);
    AZ_CONTRACT_ARG_VALID_MUT_SPAN(buffer);
    AZ_CONTRACT_ARG_VALID_SPAN(src);

    if (buffer.size < src.size + 1) {
      return AZ_ERROR_BUFFER_OVERFLOW;
    }

    if (!az_span_is_empty(src)) {
      az_mut_span result = { 0 };
      AZ_RETURN_IF_FAILED(az_mut_span_move(buffer, src, &result));
    }

    buffer.begin[src.size] = '\0';

    out_result->begin = buffer.begin;
    out_result->size = src.size + 1;

    return AZ_OK;
  }

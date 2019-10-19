// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>

#include <_az_cfg.h>

az_result az_span_replace(
    az_span const buffer,
    az_const_span const src,
    uint8_t (*const func)(uint8_t const),
    az_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(func);
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  if (!az_span_is_valid(buffer) || !az_const_span_is_valid(src)) {
    return AZ_ERROR_ARG;
  }

  if (buffer.size < src.size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  if (az_const_span_is_overlap(az_span_to_const_span(buffer), src) && buffer.begin > src.begin) {
    for (size_t ri = src.size; ri > 0; --ri) {
      buffer.begin[ri - 1] = func(src.begin[ri - 1]);
    }
  } else {
    for (size_t i = 0; i < src.size; ++i) {
      buffer.begin[i] = func(src.begin[i]);
    }
  }

  *out_result = (az_span){ .begin = buffer.begin, .size = src.size };
  return AZ_OK;
}

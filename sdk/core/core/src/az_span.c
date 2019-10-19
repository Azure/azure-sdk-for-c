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

  size_t first_index = 0;
  size_t last_index = src.size - 1;
  signed int increment = +1;
  if (az_const_span_is_overlap(az_span_to_const_span(buffer), src) && buffer.begin > src.begin) {
    first_index = last_index;
    last_index = 0;
    increment = -1;
  }

  for (size_t i = first_index; i <= last_index; i += increment) {
    buffer.begin[i] = func(src.begin[i]);
  }

  *out_result = (az_span){ .begin = buffer.begin, .size = src.size };
  return AZ_OK;
}

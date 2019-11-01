// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_mut_span.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_mut_span_replace(
    az_mut_span const buffer,
    az_span const src,
    uint8_t (*const func)(uint8_t const),
    az_mut_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(func);
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  AZ_CONTRACT_ARG_VALID_MUT_SPAN(buffer);
  AZ_CONTRACT_ARG_VALID_SPAN(src);

  if (buffer.size < src.size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  if (az_span_is_overlap(az_mut_span_to_span(buffer), src) && buffer.begin > src.begin) {
    for (size_t ri = src.size; ri > 0; --ri) {
      buffer.begin[ri - 1] = func(src.begin[ri - 1]);
    }
  } else {
    for (size_t i = 0; i < src.size; ++i) {
      buffer.begin[i] = func(src.begin[i]);
    }
  }

  *out_result = (az_mut_span){ .begin = buffer.begin, .size = src.size };
  return AZ_OK;
}

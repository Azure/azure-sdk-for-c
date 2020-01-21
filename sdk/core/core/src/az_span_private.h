// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_PRIVATE_H
#define _az_SPAN_PRIVATE_H

#include <az_action.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

typedef int32_t az_result_byte;

/**
 * Returns a byte in `index` position.
 * Returns `AZ_ERROR_EOF` if the `index` is out of the span range.
 */
AZ_NODISCARD AZ_INLINE az_result_byte az_span_get(az_span const span, size_t const index) {
  if (span._internal.length <= index) {
    return AZ_ERROR_EOF;
  }
  return span._internal.begin[index];
}

// Parsing utilities
AZ_NODISCARD AZ_INLINE az_result az_error_unexpected_char(az_result_byte const c) {
  return az_failed(c) ? c : AZ_ERROR_PARSER_UNEXPECTED_CHAR;
}

#include <_az_cfg_suffix.h>

#endif

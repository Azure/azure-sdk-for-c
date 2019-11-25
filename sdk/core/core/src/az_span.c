// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>

#include <ctype.h>

#include <_az_cfg.h>

enum {
  AZ_ASCII_LOWER_DIF = 'a' - 'A',
};

az_span const AZ_SPAN_NULL = { .begin = NULL, .size = 0 };

/**
 * ASCII lower case.
 */
AZ_NODISCARD AZ_INLINE az_result_byte az_ascii_lower(az_result_byte const value) {
  return 'A' <= value && value <= 'Z' ? value + AZ_ASCII_LOWER_DIF : value;
}

AZ_NODISCARD bool az_span_eq_ascii_ignore_case(az_span const a, az_span const b) {
  size_t const size = a.size;
  if (size != b.size) {
    return false;
  }
  for (size_t i = 0; i < size; ++i) {
    if (az_ascii_lower(az_span_get(a, i)) != az_ascii_lower(az_span_get(b, i))) {
      return false;
    }
  }
  return true;
}

AZ_NODISCARD az_result az_span_get_uint64(az_span const self, uint64_t * const out) {
  if (self.size <= 0) {
    return AZ_ERROR_EOF;
  }
  uint64_t value = 0;
  size_t i = 0;
  while (true) {
    az_result_byte const result = az_span_get(self, i);
    if (result == AZ_ERROR_EOF) {
      *out = value;
      return AZ_OK;
    }
    if (!isdigit(result)) {
      return az_error_unexpected_char(result);
    }
    uint64_t const d = (uint64_t)result - '0';
    if ((UINT64_MAX - d) / 10 < value) {
      return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
    }
    value = value * 10 + d;
    i += 1;
  }
}

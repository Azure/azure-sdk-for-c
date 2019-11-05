// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>

#include <_az_cfg.h>

enum {
  AZ_ASCII_LOWER_DIF = 'a' - 'A',
};

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

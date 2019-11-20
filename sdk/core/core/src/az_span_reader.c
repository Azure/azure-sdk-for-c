// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_span_reader_private.h"
#include <az_span_reader.h>

#include <ctype.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_span_reader_expect_char(az_span_reader * const p_reader, uint8_t const expected) {
  az_result_byte const c = az_span_reader_current(p_reader);
  if (c != expected) {
    return az_error_unexpected_char(c);
  }
  az_span_reader_next(p_reader);
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_reader_expect_span(az_span_reader * const self, az_span const span) {
  az_span_reader k = az_span_reader_create(span);
  while (true) {
    az_result_byte const ko = az_span_reader_current(&k);
    if (ko == AZ_ERROR_EOF) {
      return AZ_OK;
    }
    az_span_reader_next(&k);
    AZ_RETURN_IF_FAILED(az_span_reader_expect_char(self, (uint8_t)ko));
  }
}

AZ_NODISCARD az_result
az_span_reader_find_next_char(az_span_reader * const self, uint8_t const needle) {
  while (true) {
    AZ_RETURN_IF_FAILED(az_span_reader_current(self));
    if (az_span_reader_expect_char(self, needle) == AZ_OK) {
      return AZ_OK;
    } else {
      az_span_reader_next(self);
    }
  }
}

AZ_NODISCARD az_result az_span_reader_find_next_chars(
    az_span_reader * const self,
    uint8_t const needle1,
    uint8_t const needle2) {
  while (true) {
    AZ_RETURN_IF_FAILED(az_span_reader_current(self));
    if ((az_span_reader_expect_char(self, needle1) == AZ_OK)
        || (az_span_reader_expect_char(self, needle2) == AZ_OK)) {
      return AZ_OK;
    } else {
      az_span_reader_next(self);
    }
  }
}

AZ_NODISCARD az_result
az_span_reader_expect_digit(az_span_reader * const self, uint8_t * const digit) {
  az_result_byte const c = az_span_reader_current(self);
  if (!isdigit(c)) {
    return az_error_unexpected_char(c);
  }
  *digit = (uint8_t)(c - '0');
  az_span_reader_next(self);
  return AZ_OK;
}

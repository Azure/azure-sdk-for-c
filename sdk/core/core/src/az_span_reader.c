// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span_reader.h>

#include <_az_cfg.h>

AZ_NODISCARD static az_result az_span_reader_expect_span(
    az_span_reader * const self,
    az_span const span) {
  az_span_reader k = az_span_reader_create(span);
  while (true) {
    az_result_byte const ko = az_span_reader_current(&k);
    if (ko == AZ_ERROR_EOF) {
      return AZ_OK;
    }
    az_result_byte const o = az_span_reader_current(self);
    if (o != ko) {
      return az_error_unexpected_char(o);
    }
    az_span_reader_next(self);
    az_span_reader_next(&k);
  }
}

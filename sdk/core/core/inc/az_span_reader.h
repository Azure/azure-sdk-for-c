// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_READER_H
#define AZ_SPAN_READER_H

#include <az_result.h>
#include <az_span.h>

#include <ctype.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span span;
  size_t i;
} az_span_reader;

AZ_NODISCARD AZ_INLINE az_span_reader az_span_reader_create(az_span const span) {
  return (az_span_reader){ .span = span, .i = 0 };
}

AZ_NODISCARD AZ_INLINE bool az_span_reader_is_empty(az_span_reader const * const p_reader) {
  return p_reader->span.size <= p_reader->i;
}

AZ_NODISCARD AZ_INLINE az_result_byte
az_span_reader_current(az_span_reader const * const p_reader) {
  return az_span_get(p_reader->span, p_reader->i);
}

AZ_INLINE void az_span_reader_next(az_span_reader * const p_reader) {
  if (az_span_reader_is_empty(p_reader)) {
    return;
  }
  p_reader->i += 1;
}

AZ_NODISCARD AZ_INLINE az_result
az_span_reader_set_pos(az_span_reader * const p_reader, size_t const i) {
  AZ_CONTRACT(i <= p_reader->span.size, AZ_ERROR_ARG);

  p_reader->i = i;
  return AZ_OK;
}

/**
 * Read a span form a reader and compare it with the given @span
 *
 * If it doesn't match the given @span, the function returns AZ_ERROR_UNEXPECTED_CHAR.
 */
AZ_NODISCARD az_result az_span_reader_expect_span(az_span_reader * const self, az_span const span);

AZ_NODISCARD az_result
az_span_reader_expect_digit(az_span_reader * const self, uint8_t * const digit);

AZ_NODISCARD az_result
az_span_reader_expect_char(az_span_reader * const p_reader, uint8_t const expected);

#include <_az_cfg_suffix.h>

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_READER_H
#define _az_SPAN_READER_H

#include <az_contract_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span span;
  int32_t i;
} az_span_reader;

AZ_NODISCARD AZ_INLINE az_span_reader az_span_reader_create(az_span const span) {
  return (az_span_reader){ .span = span, .i = 0 };
}

AZ_NODISCARD AZ_INLINE bool az_span_reader_is_empty(az_span_reader const * const p_reader) {
  return az_span_length(p_reader->span) <= p_reader->i;
}

AZ_INLINE void az_span_reader_next(az_span_reader * const p_reader) {
  if (az_span_reader_is_empty(p_reader)) {
    return;
  }
  p_reader->i += 1;
}

AZ_NODISCARD AZ_INLINE az_result
az_span_reader_set_pos(az_span_reader * const p_reader, int32_t i) {
  AZ_CONTRACT(i <= az_span_length(p_reader->span), AZ_ERROR_ARG);

  p_reader->i = i;
  return AZ_OK;
}

/**
 * Read a span form a reader and compare it with the given @var span
 *
 * If it doesn't match the given @var span, the function returns AZ_ERROR_UNEXPECTED_CHAR.
 */
AZ_NODISCARD az_result az_span_reader_expect_span(az_span_reader * const self, az_span const span);

AZ_NODISCARD az_result
az_span_reader_expect_digit(az_span_reader * const self, uint8_t * const digit);

AZ_NODISCARD az_result
az_span_reader_expect_char(az_span_reader * const p_reader, uint8_t const expected);

#include <_az_cfg_suffix.h>

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_READER_H
#define AZ_SPAN_READER_H

#include <az_option_byte.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_const_span span;
  size_t i;
} az_span_reader;

static inline az_span_reader az_span_reader_create(az_const_span const span) {
  return (az_span_reader){ .span = span, .i = 0 };
}

static inline bool az_span_reader_is_empty(az_span_reader const * const p_reader) {
  return p_reader->span.size <= p_reader->i;
}

static inline az_option_byte az_span_reader_current(az_span_reader const * const p_reader) {
  return az_const_span_get(p_reader->span, p_reader->i);
}

static inline void az_span_reader_next(az_span_reader * const p_reader) {
  if (az_span_reader_is_empty(p_reader)) {
    return;
  }
  p_reader->i += 1;
}

#include <_az_cfg_suffix.h>

#endif

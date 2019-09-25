// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_READER_H
#define AZ_SPAN_READER_H

#include <az_str.h>
#include <az_result.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  AZ_STREAM_ERROR_END = AZ_MAKE_ERROR(AZ_STREAM_FACILITY, 1)
};

typedef struct {
  az_const_span span;
  size_t i;
} az_span_reader;

inline bool az_span_reader_is_empty(az_span_reader const *const p_reader) {
  return p_reader->span.size == p_reader->i;
}

inline az_result az_span_reader_current(az_span_reader const *const p_reader) {
  if (az_span_reader_is_empty(p_reader)) {
    return AZ_STREAM_ERROR_END;
  }
  return az_const_span_get(p_reader->span, p_reader->i);
}

inline az_result az_span_reader_next(az_span_reader *const p_reader) {
  if (az_span_reader_is_empty(p_reader)) {
    return AZ_STREAM_ERROR_END;
  }
  p_reader->i += 1;
  return AZ_OK;
}

#ifdef __cplusplus
}
#endif

#endif

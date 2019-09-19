// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_STR_STREAM_H
#define AZ_STR_STREAM_H

#include <az_str.h>
#include <az_result.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  AZ_STREAM_ERROR_END = AZ_MAKE_ERROR(AZ_STREAM_RESULT, 1)
};

typedef struct {
  az_const_str buffer;
  size_t i;
} az_str_reader;

inline bool az_str_reader_is_empty(az_str_reader const *const p_reader) {
  return p_reader->buffer.size == p_reader->i;
}

inline char az_str_reader_current(az_str_reader const *const p_reader) {
  return az_const_str_item(p_reader->buffer, p_reader->i);
}

inline az_result az_str_reader_next(az_str_reader* const p_reader) {
  if (p_reader->i == p_reader->buffer.size) {
    return AZ_STREAM_ERROR_END;
  }
  p_reader->i += 1;
  return AZ_OK;
}

#ifdef __cplusplus
}
#endif

#endif

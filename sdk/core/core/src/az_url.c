// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_hex_private.h"
#include <az_precondition_internal.h>
#include <az_url_internal.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE bool _az_url_should_encode(uint8_t c)
{
  switch (c)
  {
    case '-':
    case '_':
    case '.':
    case '~':
      return false;
    default:
      return !(('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'));
  }
}

AZ_NODISCARD az_result _az_url_encode(az_span destination, az_span source, int32_t* out_length)
{
  AZ_PRECONDITION_NOT_NULL(out_length);
  AZ_PRECONDITION_VALID_SPAN(source, 0, true);

  int32_t const input_size = az_span_size(source);
  AZ_PRECONDITION_VALID_SPAN(destination, input_size, false);

  int32_t result_size = 0;
  for (int32_t i = 0; i < input_size; ++i)
  {
    result_size += _az_url_should_encode(az_span_ptr(source)[i]) ? 3 : 1;
  }

  if (az_span_size(destination) < result_size)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }

  uint8_t* src_ptr = az_span_ptr(source);
  uint8_t* dest_ptr = az_span_ptr(destination);

  for (int32_t src_idx = 0; src_idx < input_size; ++src_idx)
  {
    uint8_t c = src_ptr[src_idx];
    if (!_az_url_should_encode(c))
    {
      *dest_ptr = c;
      dest_ptr += 1;
    }
    else
    {
      dest_ptr[0] = '%';
      dest_ptr[1] = _az_number_to_upper_hex(c >> 4);
      dest_ptr[2] = _az_number_to_upper_hex(c & 0x0F);
      dest_ptr += 3;
    }
  }

  *out_length = result_size;

  return AZ_OK;
}

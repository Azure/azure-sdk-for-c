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
  AZ_PRECONDITION_VALID_SPAN(destination, input_size, true);

  int32_t result_size = 0;
  for (int32_t i = 0; i < input_size; ++i)
  {
    result_size += _az_url_should_encode(az_span_ptr(source)[i]) ? 3 : 1;
  }

  if (az_span_size(destination) < result_size)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }

  uint8_t* p_s = az_span_ptr(source);
  uint8_t* p_d = az_span_ptr(destination);
  int32_t s = 0;
  for (int32_t i = 0; i < input_size; ++i)
  {
    uint8_t c = p_s[i];
    if (!_az_url_should_encode(c))
    {
      *p_d = c;
      p_d += 1;
      s += 1;
    }
    else
    {
      p_d[0] = '%';
      p_d[1] = _az_number_to_upper_hex(c >> 4);
      p_d[2] = _az_number_to_upper_hex(c & 0x0F);
      p_d += 3;
      s += 3;
    }
  }

  *out_length = result_size;

  return AZ_OK;
}

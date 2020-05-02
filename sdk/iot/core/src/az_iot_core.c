// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_core.h"
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <az_retry_internal.h>

#include <_az_cfg.h>

_az_NODISCARD int32_t az_iot_retry_calc_delay(
    int32_t operation_msec,
    int16_t attempt,
    int32_t min_retry_delay_msec,
    int32_t max_retry_delay_msec,
    int32_t random_msec)
{
  _az_PRECONDITION_RANGE(0, operation_msec, INT32_MAX - 1);
  _az_PRECONDITION_RANGE(0, attempt, INT16_MAX - 1);
  _az_PRECONDITION_RANGE(0, min_retry_delay_msec, INT32_MAX - 1);
  _az_PRECONDITION_RANGE(0, max_retry_delay_msec, INT32_MAX - 1);
  _az_PRECONDITION_RANGE(0, random_msec, INT32_MAX - 1);

  int32_t delay = _az_retry_calc_delay(attempt, min_retry_delay_msec, max_retry_delay_msec);
  
  if (delay < 0) 
  {
    delay = max_retry_delay_msec;
  }

  if (max_retry_delay_msec - delay > random_msec)
  {
    delay += random_msec;
  }

  delay -= operation_msec;

  return delay > 0 ? delay : 0;
}

_az_NODISCARD az_span _az_span_token(az_span source, az_span delimiter, az_span* out_remainder)
{
  _az_PRECONDITION_VALID_SPAN(delimiter, 1, false);
  _az_PRECONDITION_NOT_NULL(out_remainder);

  if (az_span_size(source) == 0)
  {
    return AZ_SPAN_NULL;
  }
  else
  {
    int32_t index = az_span_find(source, delimiter);

    if (index != -1)
    {
      *out_remainder = az_span_slice(source, index + az_span_size(delimiter), az_span_size(source));

      return az_span_slice(source, 0, index);
    }
    else
    {
      *out_remainder = AZ_SPAN_NULL;

      return source;
    }
  }
}

_az_NODISCARD int32_t _az_iot_u32toa_size(uint32_t number)
{
  if (number == 0)
  {
    return 1;
  }
  else
  {
    uint32_t div = 1000000000;
    int32_t digit_count = 10;
    while (number / div == 0)
    {
      div /= 10;
      digit_count--;
    }

    return digit_count; 
  }
}

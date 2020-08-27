// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <azure/iot/az_iot_common.h>
#include <azure/iot/internal/az_iot_common_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_span_internal.h>

#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_retry_internal.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD int32_t az_iot_retry_calc_delay(
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

  _az_LOG_WRITE(AZ_LOG_IOT_RETRY, AZ_SPAN_NULL);

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

AZ_NODISCARD int32_t _az_iot_u32toa_size(uint32_t number)
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

AZ_NODISCARD int32_t _az_iot_u64toa_size(uint64_t number)
{
  if (number == 0)
  {
    return 1;
  }
  else
  {
    uint64_t div = 10000000000000000000ul;
    int32_t digit_count = 20;
    while (number / div == 0)
    {
      div /= 10;
      digit_count--;
    }

    return digit_count;
  }
}

AZ_NODISCARD az_result _az_span_copy_url_encode(az_span destination, az_span source, az_span* out_remainder)
{
  int32_t length;
  AZ_RETURN_IF_FAILED(_az_span_url_encode(destination, source, &length));
  *out_remainder = az_span_slice(destination, length, az_span_size(destination));
  return AZ_OK;
}

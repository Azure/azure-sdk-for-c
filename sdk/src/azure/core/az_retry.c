// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_precondition.h>
#include <azure/core/az_result.h>
#include <azure/core/az_retry.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_retry_internal.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD int32_t az_retry_calculate_delay(
    int32_t operation_msec,
    int16_t attempt,
    int32_t min_retry_delay_msec,
    int32_t max_retry_delay_msec,
    int32_t random_jitter_msec)
{
  _az_PRECONDITION_RANGE(0, operation_msec, INT32_MAX - 1);
  _az_PRECONDITION_RANGE(0, attempt, INT16_MAX - 1);
  _az_PRECONDITION_RANGE(0, min_retry_delay_msec, INT32_MAX - 1);
  _az_PRECONDITION_RANGE(0, max_retry_delay_msec, INT32_MAX - 1);
  _az_PRECONDITION_RANGE(0, random_jitter_msec, INT32_MAX - 1);

  if (_az_LOG_SHOULD_WRITE(AZ_LOG_RETRY))
  {
    _az_LOG_WRITE(AZ_LOG_RETRY, AZ_SPAN_EMPTY);
  }

  int32_t delay = _az_retry_calc_delay(attempt, min_retry_delay_msec, max_retry_delay_msec);

  if (delay < 0)
  {
    delay = max_retry_delay_msec;
  }

  if (max_retry_delay_msec - delay > random_jitter_msec)
  {
    delay += random_jitter_msec;
  }

  delay -= operation_msec;

  return delay > 0 ? delay : 0;
}

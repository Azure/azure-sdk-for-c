// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_platform.h>
#include <azure/core/internal/az_config_internal.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <time.h>

#include <unistd.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_platform_clock_msec(int64_t* out_clock_msec)
{
  _az_PRECONDITION_NOT_NULL(out_clock_msec);

  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
  {
    return AZ_ERROR_CLOCK;
  }

  *out_clock_msec = (int64_t)((ts.tv_sec * _az_TIME_MILLISECONDS_PER_SECOND)
                              + (ts.tv_nsec / _az_TIME_NANOSECONDS_PER_MILLISECOND));

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_sleep_msec(int32_t milliseconds)
{
  (void)usleep((useconds_t)milliseconds * _az_TIME_MICROSECONDS_PER_MILLISECOND);
  return AZ_OK;
}

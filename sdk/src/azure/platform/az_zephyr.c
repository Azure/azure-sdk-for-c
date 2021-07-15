// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_platform.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <zephyr.h>

AZ_NODISCARD az_result az_platform_clock_msec(int64_t* out_clock_msec)
{
  _az_PRECONDITION_NOT_NULL(out_clock_msec);

  *out_clock_msec = k_uptime_get();

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_sleep_msec(int32_t milliseconds)
{
  k_sleep(K_MSEC(milliseconds));

  return AZ_OK;
}

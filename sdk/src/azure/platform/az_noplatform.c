// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_platform.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_platform_clock_msec(int64_t* out_clock_msec)
{
  _az_PRECONDITION_NOT_NULL(out_clock_msec);
  *out_clock_msec = 0;
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result az_platform_sleep_msec(int32_t milliseconds)
{
  (void)milliseconds;
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

void az_platform_critical_error()
{
  while (1)
  {
  }
}

AZ_NODISCARD az_result az_platform_get_random(int32_t* out_random)
{
  _az_PRECONDITION_NOT_NULL(out_random);
  *out_random = 0;
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result az_platform_timer_create(
    az_platform_timer* timer,
    az_platform_timer_callback callback,
    void* sdk_data)
{
  (void)timer;
  (void)callback;
  (void)sdk_data;

  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result az_platform_timer_start(az_platform_timer* timer, int32_t milliseconds)
{
  (void)timer;
  (void)milliseconds;

  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result  az_platform_timer_destroy(az_platform_timer* timer)
{
  (void)timer;

  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result az_platform_mutex_init(az_platform_mutex* mutex_handle)
{
  (void)mutex_handle;

  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result az_platform_mutex_acquire(az_platform_mutex* mutex_handle)
{
  (void)mutex_handle;

  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result az_platform_mutex_release(az_platform_mutex* mutex_handle)
{
  (void)mutex_handle;

  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result az_platform_mutex_destroy(az_platform_mutex* mutex_handle)
{
  (void)mutex_handle;

  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

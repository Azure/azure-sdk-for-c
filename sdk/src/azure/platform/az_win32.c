// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_platform.h>
#include <azure/core/internal/az_precondition_internal.h>

// Two macros below are not used in the code below, it is windows.h that consumes them.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_platform_clock_msec(int64_t* out_clock_msec)
{
  _az_PRECONDITION_NOT_NULL(out_clock_msec);
  *out_clock_msec = GetTickCount64();
  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_sleep_msec(int32_t milliseconds)
{
  Sleep(milliseconds);
  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_get_random(int32_t* out_random)
{
  _az_PRECONDITION_NOT_NULL(out_random);
  *out_random = 0;
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result az_platform_timer_create(
    _az_platform_timer* out_timer,
    _az_platform_timer_callback callback,
    void* callback_context)
{
  (void)out_timer;
  (void)callback;
  (void)callback_context;

  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result az_platform_timer_start(_az_platform_timer* out_timer, int32_t milliseconds)
{
  (void)out_timer;
  (void)milliseconds;

  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result az_platform_timer_destroy(_az_platform_timer* out_timer)
{
  (void)out_timer;

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

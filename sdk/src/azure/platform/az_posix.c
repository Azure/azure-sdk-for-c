// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_platform.h>
#include <azure/core/internal/az_config_internal.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <azure/core/_az_cfg.h>

void _timer_callback_handler(union sigval sv);

AZ_NODISCARD az_result az_platform_clock_msec(int64_t* out_clock_msec)
{
  _az_PRECONDITION_NOT_NULL(out_clock_msec);
  struct timespec curr_time;

  if (clock_getres(CLOCK_MONOTONIC, &curr_time) == 0) // Check if high-res timer is available
  {
    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    *out_clock_msec = ((int64_t)curr_time.tv_sec * _az_TIME_MILLISECONDS_PER_SECOND) +
        ((int64_t)curr_time.tv_nsec / _az_TIME_NANOSECONDS_PER_MILLISECOND);
  }
  else
  {
    // NOLINTNEXTLINE(bugprone-misplaced-widening-cast)
    *out_clock_msec = (int64_t)((time(NULL)) * _az_TIME_MILLISECONDS_PER_SECOND);
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_sleep_msec(int32_t milliseconds)
{
  (void)usleep((useconds_t)milliseconds * _az_TIME_MICROSECONDS_PER_MILLISECOND);
  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_get_random(int32_t* out_random)
{
  _az_PRECONDITION_NOT_NULL(out_random);
  
  *out_random = (int32_t)random();
  return AZ_OK;
}

void _timer_callback_handler(union sigval sv)
{
  az_platform_timer *timer_handle = sv.sival_ptr;

  _az_PRECONDITION_NOT_NULL(timer_handle);
  _az_PRECONDITION_NOT_NULL(timer_handle->platform_timer._internal.callback);

  timer_handle->platform_timer._internal.callback(timer_handle->platform_timer._internal.sdk_data);
}

AZ_NODISCARD az_result az_platform_timer_create(
    az_platform_timer* timer_handle,
    az_platform_timer_callback callback,
    void* sdk_data)
{
  _az_PRECONDITION_NOT_NULL(timer_handle);
  _az_PRECONDITION_NOT_NULL(callback);
  memset(timer_handle, 0, sizeof(az_platform_timer));

  timer_handle->platform_timer._internal.callback = callback;
  timer_handle->platform_timer._internal.sdk_data = sdk_data;

  timer_handle->_internal.sev.sigev_notify = SIGEV_THREAD;
  timer_handle->_internal.sev.sigev_notify_function = &_timer_callback_handler;
  timer_handle->_internal.sev.sigev_value.sival_ptr = timer_handle;

  if (0 != timer_create(
          CLOCK_MONOTONIC, &timer_handle->_internal.sev, &timer_handle->_internal.timerid))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result
az_platform_timer_start(az_platform_timer* timer_handle, int32_t milliseconds)
{
  _az_PRECONDITION_NOT_NULL(timer_handle);

  timer_handle->_internal.trigger.it_value.tv_sec = milliseconds / _az_TIME_MILLISECONDS_PER_SECOND;
  timer_handle->_internal.trigger.it_value.tv_nsec
      = (milliseconds % _az_TIME_MILLISECONDS_PER_SECOND) * _az_TIME_NANOSECONDS_PER_MILLISECOND;

  if (0
      != timer_settime(timer_handle->_internal.timerid, 0, &timer_handle->_internal.trigger, NULL))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_timer_destroy(az_platform_timer* timer_handle)
{
  _az_PRECONDITION_NOT_NULL(timer_handle);

  if (0 != timer_delete(timer_handle->_internal.timerid))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mutex_init(az_platform_mutex* mutex_handle)
{
  _az_PRECONDITION_NOT_NULL(mutex_handle);

  if (0 != pthread_mutex_init(mutex_handle, NULL))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mutex_acquire(az_platform_mutex* mutex_handle)
{
  _az_PRECONDITION_NOT_NULL(mutex_handle);

  if (0 != pthread_mutex_lock(mutex_handle))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mutex_release(az_platform_mutex* mutex_handle)
{
  _az_PRECONDITION_NOT_NULL(mutex_handle);

  if (0 != pthread_mutex_unlock(mutex_handle))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mutex_destroy(az_platform_mutex* mutex_handle)
{
  _az_PRECONDITION_NOT_NULL(mutex_handle);

  if (0 != pthread_mutex_destroy(mutex_handle))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_platform.h>
#include <azure/core/internal/az_config_internal.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <azure/core/_az_cfg.h>

#ifdef __linux__
#define _az_PLATFORM_POSIX_CLOCK_ID CLOCK_BOOTTIME
#else
#define _az_PLATFORM_POSIX_CLOCK_ID CLOCK_MONOTONIC
#endif

#ifndef __APPLE__

static void _timer_callback_handler(union sigval sv)
{
  _az_platform_timer* out_timer = sv.sival_ptr;

  _az_PRECONDITION_NOT_NULL(out_timer);
  _az_PRECONDITION_NOT_NULL(out_timer->_internal.platform_timer._internal.callback);

  out_timer->_internal.platform_timer._internal.callback(
      out_timer->_internal.platform_timer._internal.callback_context);
}

#endif // __APPLE__

AZ_NODISCARD az_result az_platform_clock_msec(int64_t* out_clock_msec)
{
  _az_PRECONDITION_NOT_NULL(out_clock_msec);
  struct timespec curr_time;

  if (clock_getres(_az_PLATFORM_POSIX_CLOCK_ID, &curr_time)
      == 0) // Check if high-res timer is available
  {
    clock_gettime(_az_PLATFORM_POSIX_CLOCK_ID, &curr_time);
    *out_clock_msec = ((int64_t)curr_time.tv_sec * _az_TIME_MILLISECONDS_PER_SECOND)
        + ((int64_t)curr_time.tv_nsec / _az_TIME_NANOSECONDS_PER_MILLISECOND);
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

#ifndef __APPLE__

AZ_NODISCARD az_result az_platform_timer_create(
    _az_platform_timer* out_timer,
    _az_platform_timer_callback callback,
    void* callback_context)
{
  _az_PRECONDITION_NOT_NULL(out_timer);
  _az_PRECONDITION_NOT_NULL(callback);

  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memset(out_timer, 0, sizeof(_az_platform_timer));

  out_timer->_internal.platform_timer._internal.callback = callback;
  out_timer->_internal.platform_timer._internal.callback_context = callback_context;

  out_timer->_internal.sev.sigev_notify = SIGEV_THREAD;
  out_timer->_internal.sev.sigev_notify_function = &_timer_callback_handler;
  out_timer->_internal.sev.sigev_value.sival_ptr = out_timer;

  if (0
      != timer_create(
          _az_PLATFORM_POSIX_CLOCK_ID, &out_timer->_internal.sev, &out_timer->_internal.timerid))
  {
    if (ENOMEM == errno)
    {
      return AZ_ERROR_OUT_OF_MEMORY;
    }
    else
    {
      return AZ_ERROR_ARG;
    }
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_timer_start(_az_platform_timer* out_timer, int32_t milliseconds)
{
  _az_PRECONDITION_NOT_NULL(out_timer);
  _az_PRECONDITION_RANGE(0, milliseconds, INT32_MAX - 1);

  out_timer->_internal.trigger.it_value.tv_sec = milliseconds / _az_TIME_MILLISECONDS_PER_SECOND;
  out_timer->_internal.trigger.it_value.tv_nsec
      = (milliseconds % _az_TIME_MILLISECONDS_PER_SECOND) * _az_TIME_NANOSECONDS_PER_MILLISECOND;

  if (0 != timer_settime(out_timer->_internal.timerid, 0, &out_timer->_internal.trigger, NULL))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_timer_destroy(_az_platform_timer* out_timer)
{
  _az_PRECONDITION_NOT_NULL(out_timer);

  if (0 != timer_delete(out_timer->_internal.timerid))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mutex_init(az_platform_mutex* mutex_handle)
{
  _az_PRECONDITION_NOT_NULL(mutex_handle);

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

  int mutex_init_result = pthread_mutex_init(mutex_handle, &attr);

  if (ENOMEM == mutex_init_result)
  {
    return AZ_ERROR_OUT_OF_MEMORY;
  }
  else if (0 != mutex_init_result)
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

#endif // __APPLE__

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_config_internal.h>
#include <az_platform_internal.h>

#include <stddef.h>
#include <time.h>

#include <unistd.h>

#include <_az_cfg.h>

AZ_NODISCARD int64_t az_platform_clock_msec()
{
  // Convert clock_t to milliseconds
  // Floating point arithmetic is used to cover CLOCKS_PER_SEC all values
  // including 12000, 2000, 1500, 300, 500, 100, 2
  return (int64_t)(clock() / (CLOCKS_PER_SEC / (double)_az_TIME_MILLISECONDS_PER_SECOND));
}

void az_platform_sleep_msec(int32_t milliseconds)
{
  (void)usleep(milliseconds * _az_TIME_MICROSECONDS_PER_MILLISECOND);
}

void az_platform_mtx_destroy(az_platform_mtx* mtx)
{
  if (pthread_mutex_destroy(&mtx->_internal.mutex) == 0)
  {
    *mtx = (az_platform_mtx){ 0 };
  }
}

AZ_NODISCARD az_result az_platform_mtx_init(az_platform_mtx* mtx)
{
  return pthread_mutex_init(&mtx->_internal.mutex, NULL) == 0 ? AZ_OK : AZ_ERROR_MUTEX;
}

AZ_NODISCARD az_result az_platform_mtx_lock(az_platform_mtx* mtx)
{
  return pthread_mutex_lock(&mtx->_internal.mutex) == 0 ? AZ_OK : AZ_ERROR_MUTEX;
}

AZ_NODISCARD az_result az_platform_mtx_unlock(az_platform_mtx* mtx)
{
  return pthread_mutex_unlock(&mtx->_internal.mutex) == 0 ? AZ_OK : AZ_ERROR_MUTEX;
}

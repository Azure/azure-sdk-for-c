// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_platform_internal.h>

#include <_az_cfg.h>

_az_NODISCARD int64_t az_platform_clock_msec() { return GetTickCount64(); }

void az_platform_sleep_msec(int32_t milliseconds) { Sleep(milliseconds); }

void az_platform_mtx_destroy(az_platform_mtx* mtx)
{
  DeleteCriticalSection(&mtx->_internal.cs);
  *mtx = (az_platform_mtx){ 0 };
}

_az_NODISCARD az_result az_platform_mtx_init(az_platform_mtx* mtx)
{
  InitializeCriticalSection(&mtx->_internal.cs);
  return AZ_OK;
}

_az_NODISCARD az_result az_platform_mtx_lock(az_platform_mtx* mtx)
{
  EnterCriticalSection(&mtx->_internal.cs);
  return AZ_OK;
}

_az_NODISCARD az_result az_platform_mtx_unlock(az_platform_mtx* mtx)
{
  LeaveCriticalSection(&mtx->_internal.cs);
  return AZ_OK;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_platform_impl.h>
#include <az_platform_internal.h>

#include <_az_cfg.h>

AZ_NODISCARD int64_t az_platform_clock_msec() { return GetTickCount64(); }

void az_platform_sleep_msec(int32_t milliseconds) { Sleep(milliseconds); }

void az_platform_mtx_destroy(az_platform_mtx * mtx) {
  DeleteCriticalSection(&mtx->cs);
  mtx->cs = (CRITICAL_SECTION){ 0 };
}

AZ_NODISCARD az_result az_platform_mtx_init(az_platform_mtx * mtx) {
  InitializeCriticalSection(&mtx->cs);
  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mtx_lock(az_platform_mtx * mtx) {
  EnterCriticalSection(&mtx->cs);
  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mtx_unlock(az_platform_mtx * mtx) {
  LeaveCriticalSection(&mtx->cs);
  return AZ_OK;
}

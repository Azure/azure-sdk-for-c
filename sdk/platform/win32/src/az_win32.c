// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_platform.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <_az_cfg.h>

AZ_NODISCARD int64_t az_platform_clock_msec() { return GetTickCount64(); }

void az_platform_sleep_msec(int32_t milliseconds) { Sleep(milliseconds); }

AZ_NODISCARD bool az_platform_atomic_compare_exchange(
    void* volatile* obj,
    void* expected,
    void* desired)
{
  return InterlockedCompareExchangePointer(obj, expected, desired) == expected;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

// Two macros below are not used in the code below, it is windows.h that consumes them.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <azure/core/_az_cfg.h>

inline AZ_NODISCARD int64_t az_platform_clock_msec() { return GetTickCount64(); }

inline void az_platform_sleep_msec(int32_t milliseconds) { Sleep(milliseconds); }

inline AZ_NODISCARD bool az_platform_atomic_compare_exchange(
    uintptr_t volatile* obj,
    uintptr_t expected,
    uintptr_t desired)
{
  return InterlockedCompareExchangePointer((void* volatile*)obj, (void*)desired, (void*)expected)
      == (void*)expected;
}

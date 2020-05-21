// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_platform.h>

#include <_az_cfg.h>

AZ_NODISCARD int64_t az_platform_clock_msec() { return 0; }

void az_platform_sleep_msec(int32_t milliseconds) { (void)milliseconds; }

AZ_NODISCARD bool az_platform_atomic_compare_exchange(
    uintptr_t volatile* obj,
    uintptr_t expected,
    uintptr_t desired)
{
  if (*obj == expected)
  {
    *obj = desired;
    return true;
  }

  return false;
}

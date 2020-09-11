// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_platform.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD int64_t az_platform_clock_msec()
{
  // This function does not have meaningful implementation to be called and its return value being
  // relied upon.
  _az_PRECONDITION(false);

  return 0;
}

void az_platform_sleep_msec(int32_t milliseconds)
{
  (void)milliseconds;

  // This function does not have meaningful implementation to be called and its behavior being
  // relied upon.
  _az_PRECONDITION(false);
}

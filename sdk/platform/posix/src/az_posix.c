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
  return (int64_t)((clock() / CLOCKS_PER_SEC) * _az_TIME_MILLISECONDS_PER_SECOND);
}

void az_platform_sleep_msec(int32_t milliseconds)
{
  (void)usleep((useconds_t)milliseconds * _az_TIME_MICROSECONDS_PER_MILLISECOND);
}

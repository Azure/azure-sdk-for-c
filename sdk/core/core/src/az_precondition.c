// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_platform_internal.h>
#include <az_precondition.h>

#include <stdint.h>

#include <_az_cfg.h>

void az_precondition_failed_default()
{
  /* By default, when a precondition fails the calling thread is suspended forever */
  while (true)
  {
    az_platform_sleep_msec(INT32_MAX);
  }
}

az_precondition_failed az_precondition_failed_callback = az_precondition_failed_default;

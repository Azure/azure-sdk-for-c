// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_platform_internal.h>
#include <az_precondition_internal.h>

#include <stdint.h>

#include <_az_cfg.h>

static void az_precondition_failed_default()
{
  /* By default, when a precondition fails the calling thread is suspended forever */
  while (true)
  {
    az_platform_sleep_msec(INT32_MAX);
  }
}

az_precondition_failed_fn _az_precondition_failed_callback = az_precondition_failed_default;

void az_precondition_failed_set_callback(az_precondition_failed_fn az_precondition_failed_callback)
{
  _az_precondition_failed_callback = az_precondition_failed_callback;
}

az_precondition_failed_fn az_precondition_failed_get_callback()
{
  return _az_precondition_failed_callback;
}

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

#ifndef AZ_NO_PRECONDITION_CHECKING
bool az_span_is_valid(az_span span, int32_t min_size, bool null_is_valid)
{
  if (min_size < 0)
  {
    return false;
  }

  uint8_t* ptr = az_span_ptr(span);
  int32_t const span_size = az_span_size(span);

  bool result = false;

  /* Span is valid if:
     The size is greater than or equal to a user defined minimum value AND one of the
     following:
        - If null_is_valid is true and the pointer in the span is null, the size must also be 0.
        - In the case of the pointer not being NULL, the size is greater than or equal to zero.
  */
  if (null_is_valid)
  {
    result = ptr == NULL ? span_size == 0 : span_size >= 0;
  }
  else
  {
    result = ptr != NULL && span_size >= 0;
  }

  return result && min_size <= span_size;
}
#endif // AZ_NO_PRECONDITION_CHECKING

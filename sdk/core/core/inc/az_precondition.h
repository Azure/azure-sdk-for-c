// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PRECONDITION_H
#define _az_PRECONDITION_H

#include <stdbool.h>
#include <stddef.h>

#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef void (*az_precondition_failed)();
extern az_precondition_failed az_precondition_failed_callback;

#ifdef NO_PRECONDITION_CHECKING
#define AZ_PRECONDITION(condition) void()
#else
#define AZ_PRECONDITION(condition) \
  do \
  { \
    if (!(condition)) \
    { \
      az_precondition_failed_callback(); \
    } \
  } while (0)
#endif

#define AZ_PRECONDITION_RANGE(low, arg, max) AZ_PRECONDITION((low <= arg && arg <= max))

#define AZ_PRECONDITION_NOT_NULL(arg) AZ_PRECONDITION((arg != NULL))

AZ_NODISCARD AZ_INLINE bool az_span_is_valid(az_span span, int32_t min_length, bool null_is_valid)
{
  int32_t span_length = az_span_length(span);
  int32_t span_capacity = az_span_capacity(span);
  /* Valid Span is:
      If the length is greater than or equal to a user defined minimum value AND one of the
     following:
        - If null_is_valid is true and the pointer in the span is null, the length and capacity must
     also be 0. In the case of the pointer not being NULL, two conditions must be met:
        - The length is greater than or equal to zero and the capacity is greater than or equal to
          the length.
  */
  return (
      (((null_is_valid && (az_span_ptr(span) == NULL) && (span_length == 0) && (span_capacity == 0))
        || (az_span_ptr(span) != NULL && (span_length >= 0) && (span_capacity >= span_length)))
       && min_length <= span_length));
}

#define AZ_PRECONDITION_VALID_SPAN(span, min, null_is_valid) \
  AZ_PRECONDITION(az_span_is_valid(span, min, null_is_valid))

#include <_az_cfg_suffix.h>

#endif // _az_PRECONDITION_H

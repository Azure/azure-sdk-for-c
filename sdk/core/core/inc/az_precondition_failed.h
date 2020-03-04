// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PRECONDITION_INTERNAL_H
#define _az_PRECONDITION_INTERNAL_H

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

AZ_NODISCARD AZ_INLINE bool az_span_is_valid(az_span span, int32_t min_length)
{
  int32_t span_length = az_span_length(span);
  /* Valid Span is:
     Span length equals to 0 (Pointer can be NULL) and greater or equal to min_length
     or span length is greater than 0 and pointer is not NULL, and greater or equal to min_length
  */
  return (
      (span_length == 0
       || (az_span_ptr(span) != NULL && az_span_ptr(span) <= az_span_ptr(span) + span_length - 1))
      && min_length <= span_length);
}

#define AZ_PRECONDITION_VALID_SPAN(span, min) AZ_PRECONDITION(az_span_is_valid(span, min))

#include <_az_cfg_suffix.h>

#endif // _az_PRECONDITION_INTERNAL_H

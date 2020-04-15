// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_precondition_internal.h
 *
 * @brief This header defines the types and functions your application uses
 *        to override the default precondition failure behavior.
 *
 *        Public SDK functions validate the arguments passed to them in an effort
 *        to ensure that calling code is passing valid values. The valid value is
 *        called a contract precondition. If an SDK function detects a precondition
 *        failure (invalid argument value), then by default, it calls a function that
 *        places the calling thread into an infinite sleep state; other threads
 *        continue to run.
 *
 *        To override the default behavior, implement a function matching the
 *        az_precondition_failed_fn function signature and then, in your application's
 *        initialization (before calling any Azure SDK function), call
 *        az_precondition_failed_set_callback passing it the address of your function.
 *        Now, when any Azure SDK function detects a precondition failure, it will invoke
 *        your callback instead. You might override the callback to attach a debugger or
 *        perhaps to reboot the device rather than allowing it to continue running with
 *        unpredictable behavior.
 *
 *        Also, if you define the NO_PRECONDITION_CHECKING symbol when compiling the SDK
 *        code (or adding option -DBUILD_PRECONDITIONS=OFF with cmake), all of the Azure SDK
 *        precondition checking will be excluding making the binary code smaller and faster. We
 *        recommend doing this before you ship your code.
 */

#ifndef _az_PRECONDITION_INTERNAL_H
#define _az_PRECONDITION_INTERNAL_H

#include <az_precondition.h>

#include <stdbool.h>
#include <stddef.h>

#include <az_span.h>

#include <_az_cfg_prefix.h>

az_precondition_failed_fn az_precondition_failed_get_callback();

#ifdef NO_PRECONDITION_CHECKING
#define AZ_PRECONDITION(condition)
#else
#define AZ_PRECONDITION(condition) \
  do \
  { \
    if (!(condition)) \
    { \
      az_precondition_failed_get_callback()(); \
    } \
  } while (0)
#endif

#define AZ_PRECONDITION_RANGE(low, arg, max) AZ_PRECONDITION((low <= arg && arg <= max))

#define AZ_PRECONDITION_NOT_NULL(arg) AZ_PRECONDITION((arg != NULL))
#define AZ_PRECONDITION_IS_NULL(arg) AZ_PRECONDITION((arg == NULL))

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

#endif // _az_PRECONDITION_INTERNAL_H

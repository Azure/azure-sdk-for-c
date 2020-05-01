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
 *        Also, if you define the AZ_NO_PRECONDITION_CHECKING symbol when compiling the SDK
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

#ifdef AZ_NO_PRECONDITION_CHECKING
#define _az_PRECONDITION(condition)
#else
#define _az_PRECONDITION(condition) \
  do \
  { \
    if (!(condition)) \
    { \
      az_precondition_failed_get_callback()(); \
    } \
  } while (0)
#endif // AZ_NO_PRECONDITION_CHECKING

#define _az_PRECONDITION_RANGE(low, arg, max) _az_PRECONDITION((low <= arg && arg <= max))

#define _az_PRECONDITION_NOT_NULL(arg) _az_PRECONDITION((arg != NULL))
#define _az_PRECONDITION_IS_NULL(arg) _az_PRECONDITION((arg == NULL))

#ifdef AZ_NO_PRECONDITION_CHECKING
#define _az_PRECONDITION_VALID_SPAN(span, min_size, null_is_valid)
#else
bool az_span_is_valid(az_span span, int32_t min_size, bool null_is_valid);
#define _az_PRECONDITION_VALID_SPAN(span, min_size, null_is_valid) \
  _az_PRECONDITION(az_span_is_valid(span, min_size, null_is_valid))
#endif // AZ_NO_PRECONDITION_CHECKING

#include <_az_cfg_suffix.h>

#endif // _az_PRECONDITION_INTERNAL_H

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_log.h
 *
 * @brief Log listener functionality.
 */

#ifndef _az_LOG_H
#define _az_LOG_H

#include <az_result.h>
#include <az_span.h>

#include <stddef.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

#define _az_LOG_MAKE_CLASSIFICATION(facility, code) \
  ((int32_t)((uint32_t)(facility) << 16) | (uint32_t)(code))

/// Log message classification.
typedef enum
{
  AZ_LOG_HTTP_REQUEST
  = _az_LOG_MAKE_CLASSIFICATION(_az_FACILITY_HTTP, 1), ///< HTTP request is about to be sent.

  AZ_LOG_HTTP_RESPONSE
  = _az_LOG_MAKE_CLASSIFICATION(_az_FACILITY_HTTP, 2), ///< HTTP response was received.

  AZ_LOG_HTTP_RETRY = _az_LOG_MAKE_CLASSIFICATION(
      _az_FACILITY_HTTP,
      3), ///< First HTTP request did not succeed and will be retried.
} az_log_classification;

/// Type of pointer to a function that will be invoked when a log message needs to be written.
typedef void (*az_log_fn)(az_log_classification classification, az_span message);

/// Make log listener being invoked only for the messages with specified classification types.
/// If classifications were not set, log listener will be invoked for all types of classifications.
///
/// @param classifications An array of classifications.
/// @param classifications_length Lenght of the \a classifications array.
void az_log_set_classifications(
    az_log_classification const* classifications,
    size_t classifications_length);

/// Set a function that will be invoked when a log message needs to be written.
///
/// @param listener Pointer to a function that will be invoked when a log message needs to be
/// written.
void az_log_set_listener(az_log_fn listener);

#include <_az_cfg_suffix.h>

#endif // _az_LOG_H

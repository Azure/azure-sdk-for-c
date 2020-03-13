// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_log.h
 *
 * @brief This header defines the types and functions your application uses
 *        to be notified of Azure SDK client library log messages.
 */

#ifndef _az_LOG_H
#define _az_LOG_H

#include <az_result.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

#define _az_LOG_MAKE_CLASSIFICATION(facility, code) \
  ((int32_t)((uint32_t)(facility) << 16) | (uint32_t)(code))

/**
 * @brief az_log_classification identify the classifications of log messages
 *        that the Azure SDK client libraries produce.
 */
typedef enum
{
  AZ_LOG_END_OF_LIST
  = -1, ///< Terminates the classification array passed to az_log_set_classifications.

  AZ_LOG_HTTP_REQUEST
  = _az_LOG_MAKE_CLASSIFICATION(_az_FACILITY_HTTP, 1), ///< HTTP request is about to be sent.

  AZ_LOG_HTTP_RESPONSE
  = _az_LOG_MAKE_CLASSIFICATION(_az_FACILITY_HTTP, 2), ///< HTTP response was received.

  AZ_LOG_HTTP_RETRY = _az_LOG_MAKE_CLASSIFICATION(
      _az_FACILITY_HTTP,
      3), ///< First HTTP request did not succeed and will be retried.
} az_log_classification;

/**
 * @brief az_log_message_fn defines the signature of the callback function that application
 developers
 * must write in order to receive Azure SDK log messages.
 *
 * @param classification The log message's az_log_classification.
 * @param message The 0-terminated log message.
 * @param message_length The length of the 0-terminated log message
 *                       (so you don't have to call strlen).
*/
typedef void (*az_log_message_fn)(
    az_log_classification classification,
    char const* message,
    int32_t message_length);

/**
 * @brief az_log_set_classifications allows the application to specify which
 * az_log_classification types it is interested in receiving. If no classifications are set
 * (NULL), the application will receive log messages for all az_log_classification values.
 *
 * @param classifications An array of az_log_classification values.
 *                        The last element of the array must be AZ_LOG_END_OF_LIST.
 */
void az_log_set_classifications(az_log_classification const classifications[]);

/**
 * @brief az_log_set_callback sets the function that will be invoked to report an Azure SDK
 * client library log message.
 *
 * @param az_log_message_callback A pointer to the function that will be invoked when an Azure
 * SDK client library report a log message matching one of the az_log_classifications passed to
 * az_log_set_classifications.
 */
void az_log_set_callback(az_log_message_fn az_log_message_callback);

#include <_az_cfg_suffix.h>

#endif // _az_LOG_H

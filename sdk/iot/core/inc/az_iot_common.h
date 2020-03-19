// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_common.h
 *
 * @brief Azure IoT common definitions.
 */

#ifndef _az_IOT_COMMON_H
#define _az_IOT_COMMON_H

#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * @brief Azure IoT service status codes.
 *
 */
typedef enum
{
  // Service success codes
  AZ_IOT_STATUS_OK = 200,
  AZ_IOT_STATUS_ACCEPTED = 202,
  AZ_IOT_STATUS_NO_CONTENT = 204,

  // Service error codes
  AZ_IOT_STATUS_BAD_REQUEST = 400,
  AZ_IOT_STATUS_UNAUTHORIZED = 401,
  AZ_IOT_STATUS_FORBIDDEN = 403,
  AZ_IOT_STATUS_NOT_FOUND = 404,
  AZ_IOT_STATUS_NOT_ALLOWED = 405,
  AZ_IOT_STATUS_NOT_CONFLICT = 409,
  AZ_IOT_STATUS_PRECONDITION_FAILED = 412,
  AZ_IOT_STATUS_REQUEST_TOO_LARGE = 413,
  AZ_IOT_STATUS_UNSUPPORTED_TYPE = 415,
  AZ_IOT_STATUS_THROTTLED = 429,
  AZ_IOT_STATUS_CLIENT_CLOSED = 499,
  AZ_IOT_STATUS_SERVER_ERROR = 500,
  AZ_IOT_STATUS_BAD_GATEWAY = 502,
  AZ_IOT_STATUS_SERVICE_UNAVAILABLE = 503,
  AZ_IOT_STATUS_TIMEOUT = 504,
} az_iot_status;

/**
 * @brief Checks if the status indicates a successful operation.
 *
 * @param[in] status The #az_iot_status to verify.
 * @return True if the status indicates success. False otherwise.
 */
AZ_NODISCARD bool az_iot_is_success_status(az_iot_status status);

/**
 * @brief Checks if the status indicates a retriable error occurred during the
 *        operation.
 *
 * @param[in] status The #az_iot_status to verify.
 * @return True if the operation should be retried. False otherwise.
 */
AZ_NODISCARD bool az_iot_is_retriable_status(az_iot_status status);

/**
 * @brief Calculates the recommended delay before retrying an operation that failed.
 *
 * @param[in] operation_msec The time it took, in milliseconds, to perform the operation that
 *                           failed.
 * @param[in] attempt The number of failed retry attempts.
 * @param[in] retry_delay_msec The minimum time, in milliseconds, to wait before a retry.
 * @param[in] max_retry_delay_msec The maximum time, in milliseconds, to wait before a retry.
 * @param[in] random_msec A random value between 0 and the maximum allowed jitter, in milliseconds.
 * @return The recommended delay in milliseconds.
 */
AZ_NODISCARD int32_t az_iot_retry_calc_delay(
    int32_t operation_msec,
    int16_t attempt,
    int32_t retry_delay_msec,
    int32_t max_retry_delay_msec,
    int32_t random_msec);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_COMMON_H

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_common.h
 *
 * @brief Azure IoT common definitions.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_IOT_CORE_H
#define _az_IOT_CORE_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

enum
{
  AZ_IOT_DEFAULT_MQTT_CONNECT_PORT = 8883,
  AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS = 240
};

/**
 * @brief Azure IoT service status codes.
 *
 */
typedef enum
{
  // Default, unset value
  AZ_IOT_STATUS_UNKNOWN = 0,

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
 *
 * Properties APIs
 *
 *   IoT message properties are used for Device-to-Cloud (D2C) as well as Cloud-to-Device (C2D).
 *   Properties are always appended to the MQTT topic of the published or received message and
 *   must contain Uri-encoded keys and values.
 */
/**
 * @brief Supported IoT message properties
 */
#define AZ_IOT_MESSAGE_PROPERTIES_MESSAGE_ID \
  "%24.mid" /**< Add unique identification to a message */
#define AZ_IOT_MESSAGE_PROPERTIES_CORRELATION_ID \
  "%24.cid" /**< Used in distributed tracing. More information here: \
https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-distributed-tracing */
#define AZ_IOT_MESSAGE_PROPERTIES_CONTENT_TYPE \
  "%24.ct" /**< URL encoded and of the form text%2Fplain or application%2Fjson, etc */
#define AZ_IOT_MESSAGE_PROPERTIES_CONTENT_ENCODING "%24.ce" /**< UTF-8, UTF-16, etc */

/**
 * @brief Telemetry or C2D properties.
 *
 */
typedef struct
{
  struct
  {
    az_span properties_buffer;
    int32_t properties_written;
    uint32_t current_property_index;
  } _internal;
} az_iot_message_properties;

/**
 * @brief Initializes the Telemetry or C2D properties.
 *
 * @note The properties init API will not encode properties. In order to support
 *       the following characters, they must be percent-encoded (RFC3986) as follows:
 *          `/` : `%2F`
 *          `%` : `%25`
 *          `#` : `%23`
 *          `&` : `%26`
 *       Only these characters would have to be encoded. If you would like to avoid the need to
 *       encode the names/values, avoid using these characters in names and values.
 *
 * @param[in] properties The #az_iot_message_properties to initialize
 * @param[in] buffer Can either be an empty #az_span or an #az_span containing properly formatted
 *                   (with above mentioned characters encoded if applicable) properties with the
 *                   following format: {key}={value}&{key}={value}.
 * @param[in] written_length The length of the properly formatted properties already initialized
 * within the buffer. If the \p buffer is empty (uninitialized), this should be 0.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_message_properties_init(
    az_iot_message_properties* properties,
    az_span buffer,
    int32_t written_length);

/**
 * @brief Appends a key-value property to the list of properties.
 *
 * @note The properties append API will not encode properties. In order to support
 *       the following characters, they must be percent-encoded (RFC3986) as follows:
 *          `/` : `%2F`
 *          `%` : `%25`
 *          `#` : `%23`
 *          `&` : `%26`
 *       Only these characters would have to be encoded. If you would like to avoid the need to
 *       encode the names/values, avoid using these characters in names and values.
 *
 * @param[in] properties The #az_iot_message_properties to use for this call
 * @param[in] name The name of the property.
 * @param[in] value The value of the property.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_message_properties_append(
    az_iot_message_properties* properties,
    az_span name,
    az_span value);

/**
 * @brief Finds the value of a property.
 * @remark This will return the first value of the property with the given name if multiple
 * properties with the same key exist.
 *
 * @param[in] properties The #az_iot_message_properties to use for this call
 * @param[in] name The name of the property.
 * @param[out] out_value An #az_span containing the value of the property.
 * @return #az_result.
 */
AZ_NODISCARD az_result az_iot_message_properties_find(
    az_iot_message_properties* properties,
    az_span name,
    az_span* out_value);

/**
 * @brief Iterates over the list of properties.
 *
 * @param[in] properties The #az_iot_message_properties to use for this call
 * @param[out] out_name An #az_span* containing the name of the next property.
 * @param[out] out_value An #az_span* containing the value of the next property.
 * @return #az_result
 */
AZ_NODISCARD az_result
az_iot_message_properties_next(az_iot_message_properties* properties, az_span* out_name, az_span* out_value);

/**
 * @brief Checks if the status indicates a successful operation.
 *
 * @param[in] status The #az_iot_status to verify.
 * @return `true` if the status indicates success. `false` otherwise.
 */
AZ_NODISCARD AZ_INLINE bool az_iot_is_success_status(az_iot_status status)
{
  return status < AZ_IOT_STATUS_BAD_REQUEST;
}

/**
 * @brief Checks if the status indicates a retriable error occurred during the
 *        operation.
 *
 * @param[in] status The #az_iot_status to verify.
 * @return `true` if the operation should be retried. `false` otherwise.
 */
AZ_NODISCARD AZ_INLINE bool az_iot_is_retriable_status(az_iot_status status)
{
  return ((status == AZ_IOT_STATUS_THROTTLED) || (status == AZ_IOT_STATUS_SERVER_ERROR));
}

/**
 * @brief Calculates the recommended delay before retrying an operation that failed.
 *
 * @param[in] operation_msec The time it took, in milliseconds, to perform the operation that
 *                           failed.
 * @param[in] attempt The number of failed retry attempts.
 * @param[in] min_retry_delay_msec The minimum time, in milliseconds, to wait before a retry.
 * @param[in] max_retry_delay_msec The maximum time, in milliseconds, to wait before a retry.
 * @param[in] random_msec A random value between 0 and the maximum allowed jitter, in milliseconds.
 * @return The recommended delay in milliseconds.
 */
AZ_NODISCARD int32_t az_iot_retry_calc_delay(
    int32_t operation_msec,
    int16_t attempt,
    int32_t min_retry_delay_msec,
    int32_t max_retry_delay_msec,
    int32_t random_msec);

#include <azure/core/_az_cfg_suffix.h>

#endif //!_az_IOT_CORE_H

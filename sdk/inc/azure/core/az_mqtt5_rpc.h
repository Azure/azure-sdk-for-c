// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Common include for az_mqtt5_rpc server and client.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_RPC_H
#define _az_MQTT5_RPC_H

#include <azure/core/az_span.h>
#include <stdio.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief The default timeout in seconds for subscribing/publishing.
 */
#define AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS 10
/**
 * @brief The default QOS to use for subscribing/publishing.
 */
#ifndef AZ_MQTT5_DEFAULT_RPC_QOS
#define AZ_MQTT5_DEFAULT_RPC_QOS AZ_MQTT5_QOS_AT_LEAST_ONCE
#endif

/**
 * @brief The MQTT5 RPC status property name.
 */
#define AZ_MQTT5_RPC_STATUS_PROPERTY_NAME "status"
/**
 * @brief The MQTT5 RPC status message property name.
 */
#define AZ_MQTT5_RPC_STATUS_MESSAGE_PROPERTY_NAME "statusMessage"
/**
 * @brief The MQTT5 RPC correlation id length.
 */
#define AZ_MQTT5_RPC_CORRELATION_ID_LENGTH 16

/**
 * @brief The MQTT5 RPC status codes for the response.
 */
typedef enum
{
  // Default, unset value.
  AZ_MQTT5_RPC_STATUS_UNKNOWN = 0,

  // Service success codes.
  AZ_MQTT5_RPC_STATUS_OK = 200,
  AZ_MQTT5_RPC_STATUS_ACCEPTED = 202,
  AZ_MQTT5_RPC_STATUS_NO_CONTENT = 204,

  // Service error codes.
  AZ_MQTT5_RPC_STATUS_BAD_REQUEST = 400,
  AZ_MQTT5_RPC_STATUS_UNAUTHORIZED = 401,
  AZ_MQTT5_RPC_STATUS_FORBIDDEN = 403,
  AZ_MQTT5_RPC_STATUS_NOT_FOUND = 404,
  AZ_MQTT5_RPC_STATUS_NOT_ALLOWED = 405,
  AZ_MQTT5_RPC_STATUS_NOT_CONFLICT = 409,
  AZ_MQTT5_RPC_STATUS_PRECONDITION_FAILED = 412,
  AZ_MQTT5_RPC_STATUS_REQUEST_TOO_LARGE = 413,
  AZ_MQTT5_RPC_STATUS_UNSUPPORTED_TYPE = 415,
  AZ_MQTT5_RPC_STATUS_THROTTLED = 429,
  AZ_MQTT5_RPC_STATUS_CLIENT_CLOSED = 499,
  AZ_MQTT5_RPC_STATUS_SERVER_ERROR = 500,
  AZ_MQTT5_RPC_STATUS_BAD_GATEWAY = 502,
  AZ_MQTT5_RPC_STATUS_SERVICE_UNAVAILABLE = 503,
  AZ_MQTT5_RPC_STATUS_TIMEOUT = 504,
} az_mqtt5_rpc_status;

/**
 * @brief Helper function to check if an az_span topic matches an #az_span subscription, even if the
 * subscription topic has wildcards.
 *
 * @param[in] sub the subscription topic to check against.
 * @param[in] topic the topic to check.
 *
 * @return true if the topic is valid within the subscription, false otherwise.
 */
AZ_NODISCARD bool az_span_topic_matches_sub(az_span sub, az_span topic);

/**
 * @brief Helper function to check if an #az_mqtt5_rpc_status indicates failure.
 *
 * @param[in] status the status to check.
 *
 * @return true if the status indicates failure, false otherwise.
 */
AZ_NODISCARD bool az_mqtt5_rpc_status_failed(az_mqtt5_rpc_status status);

/**
 * @brief Helper function to generate an MQTT topic given a format and parameters.
 *
 * @note For subscription topics, you may pass in '+' for any parameter to use the wildcard.
 * @note At this time, only one instance of each parameter (ex. {serviceId}) is supported in the
 * format.
 *
 * @param[in] format the format string to use to generate the topic. Can include {name} for command
 * name, {serviceId} for model id, {executorId} for the server's client_id, and/or {invokerId} for
 * the client's client_id.
 * @param[in] model_id the model id to use in the topic, or AZ_SPAN_EMPTY if not required for the
 * format.
 * @param[in] executor_client_id the executor client id to use in the topic, or AZ_SPAN_EMPTY if not
 * required for the format.
 * @param[in] invoker_client_id the invoker client id to use in the topic, or AZ_SPAN_EMPTY if not
 * required for the format.
 * @param[in] command_name the command name to use in the topic, or AZ_SPAN_EMPTY if not required
 * for the format.
 * @param[out] out_topic the buffer to write the topic to.
 * @param[out] out_topic_length the length of the topic written to the buffer. Can pass in NULL if
 * you don't need this value.
 *
 * @return #az_result indicating the result of the operation.
 */
AZ_NODISCARD az_result az_rpc_get_topic_from_format(
    az_span format,
    az_span model_id,
    az_span executor_client_id,
    az_span invoker_client_id,
    az_span command_name,
    az_span out_topic,
    int32_t* out_topic_length);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_RPC_H

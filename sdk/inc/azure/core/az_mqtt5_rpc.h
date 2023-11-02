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
 * @brief The default topic format for making RPC requests.
 */
#define AZ_MQTT5_RPC_SERVER_DEFAULT_SUBSCRIPTION_TOPIC_FORMAT \
  "services/{serviceId}/{executorId}/command/{name}/request"
/**
 * @brief The default topic format where RPC responses are published.
 */
#define AZ_MQTT5_RPC_CLIENT_DEFAULT_SUBSCRIPTION_TOPIC_FORMAT \
  "clients/{invokerClientId}/services/{serviceId}/{executorId}/command/{name}/response"
/**
 * @brief Key appended to the topic format to indicate a shared subscription.
 */
#define AZ_MQTT5_RPC_SERVICE_GROUP_ID_KEY "$share/"
/**
 * @brief Key used to indicate the service id in the topic format.
 */
#define AZ_MQTT5_RPC_SERVICE_ID_KEY "{serviceId}"
/**
 * @brief Key used to indicate the executor id in the topic format.
 */
#define AZ_MQTT5_RPC_EXECUTOR_ID_KEY "{executorId}"
/**
 * @brief Key used to replace the executor id in the topic format with any executor id.
 */
#define AZ_MQTT5_RPC_ANY_EXECUTOR_ID "_any_"
/**
 * @brief Key used to indicate the command id in the topic format.
 */
#define AZ_MQTT5_RPC_COMMAND_ID_KEY "{name}"
/**
 * @brief Key used to indicate the invoker client id in the topic format.
 */
#define AZ_MQTT5_RPC_CLIENT_ID_KEY "{invokerClientId}"

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
#define AZ_MQTT5_RPC_STATUS_PROPERTY_NAME "Status"
/**
 * @brief The MQTT5 RPC status message property name.
 */
#define AZ_MQTT5_RPC_STATUS_MESSAGE_PROPERTY_NAME "StatusMessage"
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
 * @brief Helper function to check if an #az_mqtt5_rpc_status indicates failure.
 *
 * @param[in] status the status to check.
 *
 * @return true if the status indicates failure, false otherwise.
 */
AZ_NODISCARD bool az_mqtt5_rpc_status_failed(az_mqtt5_rpc_status status);

/**
 * @brief Helper function to check if a topic format is valid.
 *
 * @param topic_format An #az_span containing the topic format to check.
 * @return true if the topic format is valid, false otherwise.
 */
AZ_NODISCARD bool _az_mqtt5_rpc_valid_topic_format(az_span topic_format);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_RPC_H

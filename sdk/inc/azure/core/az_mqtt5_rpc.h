// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Common for az_mqtt5_rpc server and client.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_RPC_H
#define _az_MQTT5_RPC_H

#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief The default timeout in seconds for subscribing.
 */
#define AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS 10
/**
 * @brief The default QOS to use for subscribing/publishing.
 */
#ifndef AZ_MQTT5_RPC_QOS
#define AZ_MQTT5_RPC_QOS 1
#endif

#define AZ_MQTT5_RPC_STATUS_PROPERTY_NAME "status"
#define AZ_MQTT5_RPC_STATUS_MESSAGE_PROPERTY_NAME "statusMessage"

/**
 * @brief The MQTT5 RPC status codes for the response.
 */
typedef enum
{
  // Default, unset value
  AZ_MQTT5_RPC_STATUS_UNKNOWN = 0,

  // Service success codes
  AZ_MQTT5_RPC_STATUS_OK = 200,
  // AZ_MQTT5_RPC_STATUS_ACCEPTED = 202,
  // AZ_MQTT5_RPC_STATUS_NO_CONTENT = 204,

  // Service error codes
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

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_RPC_H

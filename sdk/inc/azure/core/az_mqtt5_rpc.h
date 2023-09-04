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
#ifndef AZ_MQTT5_RPC_QOS
#define AZ_MQTT5_RPC_QOS 1
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
 * @brief The MQTT5 RPC correlation id length
 */
#define AZ_MQTT5_RPC_CORRELATION_ID_LENGTH 16

/**
 * @brief The MQTT5 RPC status codes for the response.
 */
typedef enum
{
  // Default, unset value
  AZ_MQTT5_RPC_STATUS_UNKNOWN = 0,

  // Service success codes
  AZ_MQTT5_RPC_STATUS_OK = 200,
  AZ_MQTT5_RPC_STATUS_ACCEPTED = 202,
  AZ_MQTT5_RPC_STATUS_NO_CONTENT = 204,

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

/**
 * @brief helper function to check if an az_span topic matches an az_span subscription
 */
AZ_NODISCARD AZ_INLINE bool az_span_topic_matches_sub(az_span sub, az_span topic)
{
  bool ret;
  // TODO: have this not be mosquitto specific
  if (MOSQ_ERR_SUCCESS
      != mosquitto_topic_matches_sub((char*)az_span_ptr(sub), (char*)az_span_ptr(topic), &ret))
  {
    ret = false;
  }
  return ret;
}

/**
 * @brief helper function to print a correlation id in a human readable format
 */
AZ_INLINE void print_correlation_id(az_span correlation_id)
{
  char* corr = (char*)az_span_ptr(correlation_id);
  printf("correlation id: ");
  for (int i = 0; i < AZ_MQTT5_RPC_CORRELATION_ID_LENGTH; i++)
  {
    printf("%d", corr[i]);
  }
  printf(" ");
}

/**
 * @brief helper function to check if an az_mqtt5_rpc_status indicates failure
 */
AZ_NODISCARD AZ_INLINE bool az_mqtt5_rpc_status_failed(az_mqtt5_rpc_status status)
{
  return (status < 200 || status >= 300);
}

AZ_NODISCARD AZ_INLINE az_result az_rpc_get_topic_from_format(az_span model_id, az_span executor_client_id, az_span invoker_client_id, az_span command_name, az_span format, az_span out_subscription_topic,
    int32_t* out_topic_length)
{
  int32_t format_size = az_span_size(format);

  // this is more than needed, but can never be less than needed
  int32_t subscription_max_length = az_span_size(model_id) + az_span_size(executor_client_id) + az_span_size(invoker_client_id)
      + (az_span_size(command_name) > 0 ? az_span_size(command_name) : 1) + format_size;

  char format_buf[subscription_max_length > az_span_size(out_subscription_topic) ? subscription_max_length : az_span_size(out_subscription_topic)];
  az_span temp_format_buf = AZ_SPAN_FROM_BUFFER(format_buf);
  az_span_copy(temp_format_buf, format);
  temp_format_buf = az_span_slice(temp_format_buf, 0, format_size);


  az_span_fill(out_subscription_topic, ' ');
  az_span temp_span = out_subscription_topic;
  
  int32_t index = az_span_find(temp_format_buf, AZ_SPAN_FROM_STR("{serviceId}"));
  if (index > 0)
  {
    _az_PRECONDITION_VALID_SPAN(model_id, 1, false);
    format_size += az_span_size(model_id);
    format_size -= 11;
    _az_PRECONDITION_VALID_SPAN(out_subscription_topic, format_size, true);

    temp_span = az_span_copy(
        temp_span, az_span_slice(temp_format_buf, 0, index));
    temp_span = az_span_copy(temp_span, model_id);
    temp_span = az_span_copy(
        temp_span, az_span_slice_to_end(temp_format_buf, index + 11));

    temp_format_buf = AZ_SPAN_FROM_BUFFER(format_buf);
    az_span_copy(temp_format_buf, out_subscription_topic);
    temp_format_buf = az_span_slice(temp_format_buf, 0, format_size);
    temp_span = out_subscription_topic;
  }

  index = az_span_find(temp_format_buf, AZ_SPAN_FROM_STR("{name}"));
  if (index > 0)
  {
    format_size += az_span_size(command_name);
    format_size -= 6;
    _az_PRECONDITION_VALID_SPAN(out_subscription_topic, format_size, true);

    temp_span = az_span_copy(
        temp_span, az_span_slice(temp_format_buf, 0, index));
    temp_span = az_span_copy(temp_span, command_name);
    temp_span = az_span_copy(
        temp_span, az_span_slice_to_end(temp_format_buf, index + 6));

    temp_format_buf = AZ_SPAN_FROM_BUFFER(format_buf);
    az_span_copy(temp_format_buf, out_subscription_topic);
    temp_format_buf = az_span_slice(temp_format_buf, 0, format_size);
    temp_span = out_subscription_topic;
  }

  index = az_span_find(temp_format_buf, AZ_SPAN_FROM_STR("{executorId}"));
  if (index > 0)
  {
    _az_PRECONDITION_VALID_SPAN(executor_client_id, 1, false);
    format_size += az_span_size(executor_client_id);
    format_size -= 12;
    _az_PRECONDITION_VALID_SPAN(out_subscription_topic, format_size, true);

    temp_span = az_span_copy(
        temp_span, az_span_slice(temp_format_buf, 0, index));
    temp_span = az_span_copy(temp_span, executor_client_id);
    temp_span = az_span_copy(
        temp_span, az_span_slice_to_end(temp_format_buf, index + 12));

    temp_format_buf = AZ_SPAN_FROM_BUFFER(format_buf);
    az_span_copy(temp_format_buf, out_subscription_topic);
    temp_format_buf = az_span_slice(temp_format_buf, 0, format_size);
    temp_span = out_subscription_topic;
  }

  index = az_span_find(temp_format_buf, AZ_SPAN_FROM_STR("{invokerId}"));
  if (index > 0)
  {
    _az_PRECONDITION_VALID_SPAN(invoker_client_id, 1, false);
    format_size += az_span_size(invoker_client_id);
    format_size -= 11;
    _az_PRECONDITION_VALID_SPAN(out_subscription_topic, format_size, true);

    temp_span = az_span_copy(
        temp_span, az_span_slice(temp_format_buf, 0, index));
    temp_span = az_span_copy(temp_span, invoker_client_id);
    temp_span = az_span_copy(
        temp_span, az_span_slice_to_end(temp_format_buf, index + 11));

    temp_format_buf = AZ_SPAN_FROM_BUFFER(format_buf);
    az_span_copy(temp_format_buf, out_subscription_topic);
    temp_format_buf = az_span_slice(temp_format_buf, 0, format_size);
    temp_span = out_subscription_topic;
  }

  if (out_topic_length != NULL)
  {
    *out_topic_length = format_size;
  }

  return AZ_OK;
}

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_RPC_H

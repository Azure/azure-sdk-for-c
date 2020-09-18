// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <azure/core/az_precondition.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/az_iot_hub_client.h>
#include <azure/iot/az_iot_pnp_client.h>

#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <azure/core/_az_cfg.h>

static const az_span commands_topic_prefix = AZ_SPAN_LITERAL_FROM_STR("$iothub/methods/");
static const az_span commands_topic_filter_suffix = AZ_SPAN_LITERAL_FROM_STR("POST/");
static const az_span commands_response_topic_properties = AZ_SPAN_LITERAL_FROM_STR("/?$rid=");
static const az_span command_separator = AZ_SPAN_LITERAL_FROM_STR("*");

AZ_NODISCARD az_result az_iot_pnp_client_commands_parse_received_topic(
    az_iot_pnp_client const* client,
    az_span received_topic,
    az_iot_pnp_client_command_request* out_request)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _az_PRECONDITION_NOT_NULL(out_request);

  (void)client;

  int32_t index = az_span_find(received_topic, commands_topic_prefix);

  if (index == -1)
  {
    return AZ_ERROR_IOT_TOPIC_NO_MATCH;
  }

  _az_LOG_WRITE(AZ_LOG_MQTT_RECEIVED_TOPIC, received_topic);

  received_topic = az_span_slice(
      received_topic, index + az_span_size(commands_topic_prefix), az_span_size(received_topic));

  index = az_span_find(received_topic, commands_topic_filter_suffix);

  if (index == -1)
  {
    return AZ_ERROR_IOT_TOPIC_NO_MATCH;
  }

  received_topic = az_span_slice(
      received_topic,
      index + az_span_size(commands_topic_filter_suffix),
      az_span_size(received_topic));

  index = az_span_find(received_topic, commands_response_topic_properties);

  if (index == -1)
  {
    return AZ_ERROR_IOT_TOPIC_NO_MATCH;
  }

  int32_t command_separator_index = az_span_find(received_topic, command_separator);
  if (command_separator_index > 0)
  {
    out_request->component = az_span_slice(received_topic, 0, command_separator_index);
    out_request->name
        = az_span_slice(received_topic, command_separator_index + 1, index);
  }
  else
  {
    out_request->component = AZ_SPAN_EMPTY;
    out_request->name = az_span_slice(received_topic, 0, index);
  }

  out_request->request_id = az_span_slice(
      received_topic,
      index + az_span_size(commands_response_topic_properties),
      az_span_size(received_topic));

  return AZ_OK;
}

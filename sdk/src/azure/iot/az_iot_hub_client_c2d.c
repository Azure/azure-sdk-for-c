// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <azure/iot/az_iot_hub_client.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_span_internal.h>

#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <azure/core/_az_cfg.h>

static const az_span c2d_topic_suffix = AZ_SPAN_LITERAL_FROM_STR("/messages/devicebound/");

AZ_NODISCARD az_result az_iot_hub_client_c2d_parse_received_topic(
    az_iot_hub_client const* client,
    az_span received_topic,
    az_iot_hub_client_c2d_request* out_request)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _az_PRECONDITION_NOT_NULL(out_request);
  (void)client;

  az_span reminder;
  az_span token = _az_span_token(received_topic, c2d_topic_suffix, &reminder);
  if (az_span_ptr(reminder) == NULL)
  {
    return AZ_ERROR_IOT_TOPIC_NO_MATCH;
  }

  _az_LOG_WRITE(AZ_LOG_MQTT_RECEIVED_TOPIC, received_topic);

  token = _az_span_token(reminder, c2d_topic_suffix, &reminder);
  AZ_RETURN_IF_FAILED(
      az_iot_hub_client_properties_init(&out_request->properties, token, az_span_size(token)));

  return AZ_OK;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_hub_client.h"
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>
#include <az_span_internal.h>

#include <_az_cfg.h>

static const az_span methods_topic_prefix = AZ_SPAN_LITERAL_FROM_STR("$iothub/methods/");
static const az_span methods_topic_filter_suffix = AZ_SPAN_LITERAL_FROM_STR("POST/");
static const az_span methods_response_topic_result = AZ_SPAN_LITERAL_FROM_STR("res/");
static const az_span methods_response_topic_properties = AZ_SPAN_LITERAL_FROM_STR("/?$rid=");

AZ_NODISCARD az_result az_iot_hub_client_methods_subscribe_topic_filter_get(
    az_iot_hub_client const* client,
    az_span mqtt_topic_filter,
    az_span* out_mqtt_topic_filter)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic_filter, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic_filter);

  UNUSED(client);

  int32_t required_length
      = az_span_size(methods_topic_prefix) + az_span_size(methods_topic_filter_suffix) + 1;

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_topic_filter, required_length);

  // TODO: Merge these two calls into one since they are copying two strings literals.
  az_span remainder = az_span_copy(mqtt_topic_filter, methods_topic_prefix);
  remainder = az_span_copy(remainder, methods_topic_filter_suffix);
  az_span_copy_uint8(remainder, '#');

  *out_mqtt_topic_filter = az_span_slice(mqtt_topic_filter, 0, required_length);

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_methods_received_topic_parse(
    az_iot_hub_client const* client,
    az_span received_topic,
    az_iot_hub_client_method_request* out_request)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  AZ_PRECONDITION_NOT_NULL(out_request);

  UNUSED(client);

  int32_t index = az_span_find(received_topic, methods_topic_prefix);

  if (index == -1)
  {
    return AZ_ERROR_IOT_TOPIC_NO_MATCH;
  }

  received_topic = az_span_slice(
      received_topic, index + az_span_size(methods_topic_prefix), az_span_size(received_topic));

  index = az_span_find(received_topic, methods_topic_filter_suffix);

  if (index == -1)
  {
    return AZ_ERROR_IOT_TOPIC_NO_MATCH;
  }

  received_topic = az_span_slice(
      received_topic,
      index + az_span_size(methods_topic_filter_suffix),
      az_span_size(received_topic));

  index = az_span_find(received_topic, methods_response_topic_properties);

  if (index == -1)
  {
    return AZ_ERROR_IOT_TOPIC_NO_MATCH;
  }

  out_request->name = az_span_slice(received_topic, 0, index);
  out_request->request_id = az_span_slice(
      received_topic,
      index + az_span_size(methods_response_topic_properties),
      az_span_size(received_topic));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_methods_response_publish_topic_get(
    az_iot_hub_client const* client,
    az_span request_id,
    uint16_t status,
    az_span mqtt_topic,
    az_span* out_mqtt_topic)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(request_id, 1, false);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic);

  UNUSED(client);

  int32_t required_length
      = az_span_size(methods_topic_prefix) + az_span_size(methods_response_topic_result);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_topic, required_length);

  az_span remainder = az_span_copy(mqtt_topic, methods_topic_prefix);
  remainder = az_span_copy(remainder, methods_response_topic_result);

  AZ_RETURN_IF_FAILED(az_span_utoa(remainder, (uint32_t)status, &remainder));

  required_length = az_span_size(methods_response_topic_properties) + az_span_size(request_id);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remainder, required_length);

  remainder = az_span_copy(remainder, methods_response_topic_properties);
  az_span_copy(remainder, request_id);

  *out_mqtt_topic = az_span_slice(mqtt_topic, 0, _az_span_diff(remainder, mqtt_topic));

  return AZ_OK;
}

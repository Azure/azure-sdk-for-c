// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_hub_client.h"
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

static const az_span methods_topic_prefix = AZ_SPAN_LITERAL_FROM_STR("$iothub/methods/");
static const az_span methods_topic_filter_suffix = AZ_SPAN_LITERAL_FROM_STR("POST/");
static const az_span methods_response_topic_result = AZ_SPAN_LITERAL_FROM_STR("res/");
static const az_span methods_response_topic_properties = AZ_SPAN_LITERAL_FROM_STR("/?$rid=");
static const uint8_t hash_tag = '#';

AZ_NODISCARD az_result az_iot_hub_client_methods_subscribe_topic_filter_get(
    az_iot_hub_client const* client,
    az_span mqtt_topic_filter,
    az_span* out_mqtt_topic_filter)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic_filter, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic_filter);

  AZ_RETURN_IF_FAILED(az_span_copy(mqtt_topic_filter, methods_topic_prefix, &mqtt_topic_filter));
  AZ_RETURN_IF_FAILED(
      az_span_append(mqtt_topic_filter, methods_topic_filter_suffix, &mqtt_topic_filter));
  AZ_RETURN_IF_FAILED(az_span_append_uint8(mqtt_topic_filter, hash_tag, &mqtt_topic_filter));

  *out_mqtt_topic_filter = mqtt_topic_filter;

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

  int32_t index = az_span_find(received_topic, methods_topic_prefix);

  if (index == -1)
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }

  received_topic = az_span_slice(
      received_topic, index + az_span_length(methods_topic_prefix), az_span_length(received_topic));

  index = az_span_find(received_topic, methods_topic_filter_suffix);

  if (index == -1)
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }

  received_topic = az_span_slice(
      received_topic,
      index + az_span_length(methods_topic_filter_suffix),
      az_span_length(received_topic));

  index = az_span_find(received_topic, methods_response_topic_properties);

  if (index == -1)
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }

  out_request->name = az_span_slice(received_topic, 0, index);
  out_request->request_id = az_span_slice(
      received_topic,
      index + az_span_length(methods_response_topic_properties),
      az_span_length(received_topic));

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

  AZ_RETURN_IF_FAILED(az_span_copy(mqtt_topic, methods_topic_prefix, &mqtt_topic));
  AZ_RETURN_IF_FAILED(az_span_append(mqtt_topic, methods_response_topic_result, &mqtt_topic));
  AZ_RETURN_IF_FAILED(az_span_append_u32toa(mqtt_topic, (uint32_t)status, &mqtt_topic));
  AZ_RETURN_IF_FAILED(az_span_append(mqtt_topic, methods_response_topic_properties, &mqtt_topic));
  AZ_RETURN_IF_FAILED(az_span_append(mqtt_topic, request_id, &mqtt_topic));

  *out_mqtt_topic = mqtt_topic;

  return AZ_OK;
}

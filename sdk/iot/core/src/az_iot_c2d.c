// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_hub_client.h"
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

static const uint8_t c2d_null_terminator = '\0';
static const az_span c2d_topic_prefix = AZ_SPAN_LITERAL_FROM_STR("devices/");
static const az_span c2d_topic_suffix = AZ_SPAN_LITERAL_FROM_STR("/messages/devicebound/#");

AZ_NODISCARD az_result az_iot_hub_client_c2d_subscribe_topic_filter_get(
    az_iot_hub_client const* client,
    az_span mqtt_topic_filter,
    az_span* out_mqtt_topic_filter)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic_filter, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic_filter);

  AZ_RETURN_IF_FAILED(az_span_append(mqtt_topic_filter, c2d_topic_prefix, out_mqtt_topic_filter));
  AZ_RETURN_IF_FAILED(
      az_span_append(*out_mqtt_topic_filter, client->_internal.device_id, out_mqtt_topic_filter));
  AZ_RETURN_IF_FAILED(
      az_span_append(*out_mqtt_topic_filter, c2d_topic_suffix, out_mqtt_topic_filter));
  AZ_RETURN_IF_FAILED(
      az_span_append_uint8(*out_mqtt_topic_filter, c2d_null_terminator, out_mqtt_topic_filter));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_c2d_received_topic_parse(
    az_iot_hub_client const* client,
    az_span received_topic,
    az_iot_hub_client_c2d_request* out_request)
{
  (void)client;
  (void)received_topic;
  (void)out_request;
  return AZ_OK;
}

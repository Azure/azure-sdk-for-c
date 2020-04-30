// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_hub_client.h"
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

static const az_span c2d_topic_prefix = AZ_SPAN_LITERAL_FROM_STR("devices/");
static const az_span c2d_topic_suffix = AZ_SPAN_LITERAL_FROM_STR("/messages/devicebound/");
static const uint8_t null_terminator = '\0';
static const uint8_t hash_tag = '#';

AZ_NODISCARD az_result az_iot_hub_client_c2d_get_subscribe_topic_filter(
    az_iot_hub_client const* client,
    char* mqtt_topic_filter,
    size_t mqtt_topic_filter_size,
    size_t* out_mqtt_topic_filter_length)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(mqtt_topic_filter);
  _az_PRECONDITION(mqtt_topic_filter_size > 0);

  az_span mqtt_topic_filter_span
      = az_span_init((uint8_t*)mqtt_topic_filter, (int32_t)mqtt_topic_filter_size);
  int32_t required_length = az_span_size(c2d_topic_prefix)
      + az_span_size(client->_internal.device_id) + az_span_size(c2d_topic_suffix)
      + (int32_t)sizeof(hash_tag);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(
      mqtt_topic_filter_span, required_length + (int32_t)sizeof(null_terminator));

  az_span remainder = az_span_copy(mqtt_topic_filter_span, c2d_topic_prefix);
  remainder = az_span_copy(remainder, client->_internal.device_id);
  remainder = az_span_copy(remainder, c2d_topic_suffix);
  remainder = az_span_copy_u8(remainder, hash_tag);
  az_span_copy_u8(remainder, null_terminator);

  if (out_mqtt_topic_filter_length)
  {
    *out_mqtt_topic_filter_length = (size_t)required_length;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_c2d_parse_received_topic(
    az_iot_hub_client const* client,
    az_span received_topic,
    az_iot_hub_client_c2d_request* out_request)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _az_PRECONDITION_NOT_NULL(out_request);
  (void)client;

  az_span token;
  token = az_span_token(received_topic, c2d_topic_suffix, &received_topic);
  token = az_span_token(received_topic, c2d_topic_suffix, &received_topic);
  AZ_RETURN_IF_FAILED(
      az_iot_hub_client_properties_init(&out_request->properties, token, az_span_size(token)));

  return AZ_OK;
}

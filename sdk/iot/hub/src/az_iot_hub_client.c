// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_hub_client.h"
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

static const uint8_t hub_client_forward_slash = '/';
static const az_span hub_client_param_separator_span = AZ_SPAN_LITERAL_FROM_STR("&");
static const az_span hub_client_param_equals_span = AZ_SPAN_LITERAL_FROM_STR("=");

static const az_span hub_service_api_version = AZ_SPAN_LITERAL_FROM_STR("/?api-version=2018-06-30");

AZ_NODISCARD az_iot_hub_client_options az_iot_hub_client_options_default()
{
  return (az_iot_hub_client_options){ .module_id = AZ_SPAN_NULL, .user_agent = AZ_SPAN_NULL };
}

AZ_NODISCARD az_result az_iot_hub_client_init(
    az_iot_hub_client* client,
    az_span iot_hub_hostname,
    az_span device_id,
    az_iot_hub_client_options const* options)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(iot_hub_hostname, 1, false);
  AZ_PRECONDITION_VALID_SPAN(device_id, 1, false);

  client->_internal.iot_hub_hostname = iot_hub_hostname;
  client->_internal.device_id = device_id;
  client->_internal.options = options == NULL ? az_iot_hub_client_options_default() : *options;

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_user_name_get(
    az_iot_hub_client const* client,
    az_span mqtt_user_name,
    az_span* out_mqtt_user_name)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_user_name, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_user_name);

  const az_span* const module_id = &(client->_internal.options.module_id);
  const az_span* const user_agent = &(client->_internal.options.user_agent);

  int32_t required_length = az_span_length(client->_internal.iot_hub_hostname)
      + az_span_length(client->_internal.device_id) + az_span_length(hub_service_api_version) + 1;
  if (az_span_length(*module_id) > 0)
  {
    required_length += az_span_length(*module_id) + 1;
  }
  if (az_span_length(*user_agent) > 0)
  {
    required_length += az_span_length(*user_agent) + 1;
  }

  AZ_RETURN_IF_SPAN_CAPACITY_TOO_SMALL(mqtt_user_name, required_length);

  mqtt_user_name = az_span_copy(mqtt_user_name, client->_internal.iot_hub_hostname);
  mqtt_user_name = az_span_append_uint8(mqtt_user_name, hub_client_forward_slash);
  mqtt_user_name = az_span_append(mqtt_user_name, client->_internal.device_id);

  if (az_span_length(*module_id) > 0)
  {
    mqtt_user_name = az_span_append_uint8(mqtt_user_name, hub_client_forward_slash);
    mqtt_user_name = az_span_append(mqtt_user_name, *module_id);
  }

  mqtt_user_name = az_span_append(mqtt_user_name, hub_service_api_version);

  if (az_span_length(*user_agent) > 0)
  {
    mqtt_user_name = az_span_append_uint8(mqtt_user_name, hub_client_param_separator_span);
    mqtt_user_name = az_span_append(mqtt_user_name, *user_agent);
  }

  *out_mqtt_user_name = mqtt_user_name;

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_id_get(
    az_iot_hub_client const* client,
    az_span mqtt_client_id,
    az_span* out_mqtt_client_id)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_client_id, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_client_id);

  const az_span* const module_id = &(client->_internal.options.module_id);

  int32_t required_length = az_span_length(client->_internal.device_id);
  if (az_span_length(*module_id) > 0)
  {
    required_length += az_span_length(*module_id) + 1;
  }

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(mqtt_client_id, required_length);

  mqtt_client_id = az_span_copy(mqtt_client_id, client->_internal.device_id);

  if (az_span_length(*module_id) > 0)
  {
    mqtt_client_id = az_span_append_uint8(mqtt_client_id, hub_client_forward_slash);
    mqtt_client_id = az_span_append(mqtt_client_id, *module_id);
  }

  *out_mqtt_client_id = mqtt_client_id;

  return AZ_OK;
}

AZ_NODISCARD az_result
az_iot_hub_client_properties_init(az_iot_hub_client_properties* properties, az_span buffer)
{
  AZ_PRECONDITION_NOT_NULL(properties);
  AZ_PRECONDITION_VALID_SPAN(buffer, 0, false);

  properties->_internal.properties = buffer;
  properties->_internal.current_property_index = 0;

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_properties_append(
    az_iot_hub_client_properties* properties,
    az_span name,
    az_span value)
{
  AZ_PRECONDITION_NOT_NULL(properties);
  AZ_PRECONDITION_VALID_SPAN(name, 1, false);
  AZ_PRECONDITION_VALID_SPAN(value, 1, false);

  az_span prop_span = properties->_internal.properties;

  int32_t required_length = az_span_length(name) + az_span_length(value) + 1;
  int32_t prop_length = az_span_length(prop_span);
  if (prop_length > 0)
  {
    required_length += prop_length + 1;
  }

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(prop_span, required_length);

  if (prop_length > 0)
  {
    prop_span = az_span_append_uint8(prop_span, hub_client_param_separator_span);
  }

  prop_span = az_span_append(prop_span, name);
  prop_span = az_span_append_uint8(prop_span, hub_client_param_equals_span);
  prop_span = az_span_append(prop_span, value);

  properties->_internal.properties = prop_span;

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_properties_find(
    az_iot_hub_client_properties* properties,
    az_span name,
    az_span* out_value)
{
  AZ_PRECONDITION_NOT_NULL(properties);
  AZ_PRECONDITION_VALID_SPAN(name, 1, false);
  AZ_PRECONDITION_NOT_NULL(out_value);

  az_span remaining = properties->_internal.properties;

  while (az_span_length(remaining) != 0)
  {
    az_span delim_span = az_span_token(remaining, hub_client_param_equals_span, &remaining);
    if (az_span_is_content_equal(delim_span, name))
    {
      *out_value = az_span_token(remaining, hub_client_param_separator_span, &remaining);
      return AZ_OK;
    }
    else
    {
      az_span value;
      value = az_span_token(remaining, hub_client_param_separator_span, &remaining);
      (void)value;
    }
  }

  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_NODISCARD az_result
az_iot_hub_client_properties_next(az_iot_hub_client_properties* properties, az_pair* out)
{
  AZ_PRECONDITION_NOT_NULL(properties);
  AZ_PRECONDITION_NOT_NULL(out);

  int32_t index = (int32_t)properties->_internal.current_property_index;
  int32_t prop_length = az_span_length(properties->_internal.properties);

  if (index == prop_length)
  {
    *out = AZ_PAIR_NULL;
    return AZ_ERROR_EOF;
  }

  az_span remainder;
  az_span prop_span = az_span_slice(properties->_internal.properties, index, prop_length);

  out->key = az_span_token(prop_span, hub_client_param_equals_span, &remainder);
  out->value = az_span_token(remainder, hub_client_param_separator_span, &remainder);
  if (az_span_length(remainder) == 0)
  {
    properties->_internal.current_property_index = (uint32_t)prop_length;
  }
  else
  {
    properties->_internal.current_property_index
        = (uint32_t)(az_span_ptr(remainder) - az_span_ptr(properties->_internal.properties));
  }

  return AZ_OK;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_hub_client.h"
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>
#include <az_version.h>

#include <_az_cfg.h>

static const uint8_t hub_client_forward_slash = '/';
static const az_span hub_client_param_separator_span = AZ_SPAN_LITERAL_FROM_STR("&");
static const az_span hub_client_param_equals_span = AZ_SPAN_LITERAL_FROM_STR("=");

static const az_span hub_service_api_version = AZ_SPAN_LITERAL_FROM_STR("/?api-version=2018-06-30");
static const az_span client_sdk_version
    = AZ_SPAN_LITERAL_FROM_STR("DeviceClientType=c%2F" AZ_SDK_VERSION_STRING);

AZ_NODISCARD az_iot_hub_client_options az_iot_hub_client_options_default()
{
  return (az_iot_hub_client_options){ .module_id = AZ_SPAN_NULL, .user_agent = client_sdk_version };
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

  int32_t required_length = az_span_size(client->_internal.iot_hub_hostname)
      + az_span_size(client->_internal.device_id) + az_span_size(hub_service_api_version) + 1;
  if (az_span_size(*module_id) > 0)
  {
    required_length += az_span_size(*module_id) + 1;
  }
  if (az_span_size(*user_agent) > 0)
  {
    required_length += az_span_size(*user_agent) + az_span_size(hub_client_param_separator_span);
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_user_name, required_length);

  az_span remainder = az_span_copy(mqtt_user_name, client->_internal.iot_hub_hostname);
  remainder = az_span_copy_u8(remainder, hub_client_forward_slash);
  remainder = az_span_copy(remainder, client->_internal.device_id);

  if (az_span_size(*module_id) > 0)
  {
    remainder = az_span_copy_u8(remainder, hub_client_forward_slash);
    remainder = az_span_copy(remainder, *module_id);
  }

  remainder = az_span_copy(remainder, hub_service_api_version);

  if (az_span_size(*user_agent) > 0)
  {
    remainder = az_span_copy_u8(remainder, *az_span_ptr(hub_client_param_separator_span));
    az_span_copy(remainder, *user_agent);
  }

  *out_mqtt_user_name = az_span_slice(mqtt_user_name, 0, required_length);

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

  int32_t required_length = az_span_size(client->_internal.device_id);
  if (az_span_size(*module_id) > 0)
  {
    required_length += az_span_size(*module_id) + 1;
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_client_id, required_length);

  az_span remainder = az_span_copy(mqtt_client_id, client->_internal.device_id);

  if (az_span_size(*module_id) > 0)
  {
    remainder = az_span_copy_u8(remainder, hub_client_forward_slash);
    az_span_copy(remainder, *module_id);
  }

  *out_mqtt_client_id = az_span_slice(mqtt_client_id, 0, required_length);

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_properties_init(
    az_iot_hub_client_properties* properties,
    az_span buffer,
    int32_t written_length)
{
  AZ_PRECONDITION_NOT_NULL(properties);
  AZ_PRECONDITION_VALID_SPAN(buffer, 0, false);
  AZ_PRECONDITION(written_length >= 0);

  properties->_internal.properties_buffer = buffer;
  properties->_internal.properties_written = written_length;
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

  int32_t prop_length = properties->_internal.properties_written;

  az_span remainder = az_span_slice_to_end(properties->_internal.properties_buffer, prop_length);

  int32_t required_length = az_span_size(name) + az_span_size(value) + 1;

  if (prop_length > 0)
  {
    required_length += 1;
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remainder, required_length);

  if (prop_length > 0)
  {
    remainder = az_span_copy_u8(remainder, *az_span_ptr(hub_client_param_separator_span));
  }

  remainder = az_span_copy(remainder, name);
  remainder = az_span_copy_u8(remainder, *az_span_ptr(hub_client_param_equals_span));
  az_span_copy(remainder, value);

  properties->_internal.properties_written += required_length;

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

  az_span remaining = az_span_slice(
      properties->_internal.properties_buffer, 0, properties->_internal.properties_written);

  while (az_span_size(remaining) != 0)
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
  int32_t prop_length = properties->_internal.properties_written;

  if (index == prop_length)
  {
    *out = AZ_PAIR_NULL;
    return AZ_ERROR_EOF;
  }

  az_span remainder;
  az_span prop_span = az_span_slice(properties->_internal.properties_buffer, index, prop_length);

  out->key = az_span_token(prop_span, hub_client_param_equals_span, &remainder);
  out->value = az_span_token(remainder, hub_client_param_separator_span, &remainder);
  if (az_span_size(remainder) == 0)
  {
    properties->_internal.current_property_index = (uint32_t)prop_length;
  }
  else
  {
    properties->_internal.current_property_index
        = (uint32_t)(az_span_ptr(remainder) - az_span_ptr(properties->_internal.properties_buffer));
  }

  return AZ_OK;
}

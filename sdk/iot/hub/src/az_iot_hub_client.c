// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_hub_client.h"
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

static const uint8_t hub_client_forward_slash = '/';
static const uint8_t hub_client_param_separator = '&';
static const uint8_t hub_client_param_equals = '=';
static const az_span hub_client_param_separator_span = AZ_SPAN_LITERAL_FROM_STR("&");
static const az_span hub_client_param_equals_span = AZ_SPAN_LITERAL_FROM_STR("=");

static const az_span hub_client_api_version = AZ_SPAN_LITERAL_FROM_STR("?api-version=2018-06-30");

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

  AZ_RETURN_IF_FAILED(
      az_span_copy(mqtt_user_name, client->_internal.iot_hub_hostname, &mqtt_user_name));
  AZ_RETURN_IF_FAILED(
      az_span_append_uint8(mqtt_user_name, hub_client_forward_slash, &mqtt_user_name));
  AZ_RETURN_IF_FAILED(az_span_append(mqtt_user_name, client->_internal.device_id, &mqtt_user_name));
  AZ_RETURN_IF_FAILED(
      az_span_append_uint8(mqtt_user_name, hub_client_forward_slash, &mqtt_user_name));

  if (az_span_length(*module_id) > 0)
  {
    AZ_RETURN_IF_FAILED(az_span_append(mqtt_user_name, *module_id, &mqtt_user_name));
    AZ_RETURN_IF_FAILED(
        az_span_append_uint8(mqtt_user_name, hub_client_forward_slash, &mqtt_user_name));
  }

  AZ_RETURN_IF_FAILED(az_span_append(mqtt_user_name, hub_client_api_version, &mqtt_user_name));

  if (az_span_length(*user_agent) > 0)
  {
    AZ_RETURN_IF_FAILED(
        az_span_append_uint8(mqtt_user_name, hub_client_param_separator, &mqtt_user_name));
    AZ_RETURN_IF_FAILED(az_span_append(mqtt_user_name, *user_agent, &mqtt_user_name));
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

  AZ_RETURN_IF_FAILED(az_span_copy(mqtt_client_id, client->_internal.device_id, &mqtt_client_id));

  if (az_span_length(*module_id) > 0)
  {
    AZ_RETURN_IF_FAILED(
        az_span_append_uint8(mqtt_client_id, hub_client_forward_slash, &mqtt_client_id));
    AZ_RETURN_IF_FAILED(az_span_append(mqtt_client_id, *module_id, &mqtt_client_id));
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
  properties->_internal.current_property = az_span_ptr(buffer);

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

  if (az_span_length(prop_span) > 0)
  {
    AZ_RETURN_IF_FAILED(az_span_append_uint8(prop_span, hub_client_param_separator, &prop_span));
  }

  AZ_RETURN_IF_FAILED(az_span_append(prop_span, name, &prop_span));
  AZ_RETURN_IF_FAILED(az_span_append_uint8(prop_span, hub_client_param_equals, &prop_span));
  AZ_RETURN_IF_FAILED(az_span_append(prop_span, value, &prop_span));

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

  int32_t index = 0;
  // Keep looking in case the passed name is a substring of a name in props
  while (az_span_length(remaining) != 0)
  {
    index = az_span_find(remaining, name);
    if (index != -1)
    {
      az_span value;
      az_span found_name;
      remaining = az_span_slice(remaining, index, -1);
      found_name = az_span_token(remaining, hub_client_param_equals_span, &value);

      // If found_name is not equal to the input name, then name is substring of found_name
      // Look again starting at the remainder
      if (az_span_length(found_name) != az_span_length(name))
      {
        // We do not care about value here, only remaining
        value = az_span_token(
            az_span_slice(remaining, index, -1), hub_client_param_separator_span, &remaining);
        continue;
      }

      // If lengths do match, and value length is not zero we have found the property.
      if (az_span_length(value) > 0)
      {
        *out_value = az_span_token(value, hub_client_param_separator_span, &value);
        return AZ_OK;
      }
      else
      {
        break;
      }
    }
    else
    {
      break;
    }
  }

  return AZ_ERROR_ITEM_NOT_FOUND;
}

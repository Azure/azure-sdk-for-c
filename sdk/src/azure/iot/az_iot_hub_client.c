// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/az_version.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_span_internal.h>
#include <azure/iot/az_iot_hub_client.h>
#include <azure/iot/internal/az_iot_common_internal.h>

#include <azure/core/_az_cfg.h>

static const uint8_t null_terminator = '\0';
static const uint8_t hub_client_forward_slash = '/';
static const az_span hub_client_param_separator_span = AZ_SPAN_LITERAL_FROM_STR("&");
static const az_span hub_client_param_equals_span = AZ_SPAN_LITERAL_FROM_STR("=");

static const az_span hub_digital_twin_model_id = AZ_SPAN_LITERAL_FROM_STR("model-id");
static const az_span hub_service_api_version = AZ_SPAN_LITERAL_FROM_STR("/?api-version=2018-06-30");
static const az_span hub_service_preview_api_version
    = AZ_SPAN_LITERAL_FROM_STR("/?api-version=2020-05-31-preview");
static const az_span client_sdk_version
    = AZ_SPAN_LITERAL_FROM_STR("DeviceClientType=c%2F" AZ_SDK_VERSION_STRING);

AZ_NODISCARD az_iot_hub_client_options az_iot_hub_client_options_default()
{
  return (az_iot_hub_client_options){ .module_id = AZ_SPAN_NULL,
                                      .user_agent = client_sdk_version,
                                      .model_id = AZ_SPAN_NULL };
}

AZ_NODISCARD az_result az_iot_hub_client_init(
    az_iot_hub_client* client,
    az_span iot_hub_hostname,
    az_span device_id,
    az_iot_hub_client_options const* options)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(iot_hub_hostname, 1, false);
  _az_PRECONDITION_VALID_SPAN(device_id, 1, false);

  client->_internal.iot_hub_hostname = iot_hub_hostname;
  client->_internal.device_id = device_id;
  client->_internal.options = options == NULL ? az_iot_hub_client_options_default() : *options;

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_get_user_name(
    az_iot_hub_client const* client,
    char* mqtt_user_name,
    size_t mqtt_user_name_size,
    size_t* out_mqtt_user_name_length)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(mqtt_user_name);
  _az_PRECONDITION(mqtt_user_name_size > 0);

  const az_span* const module_id = &(client->_internal.options.module_id);
  const az_span* const user_agent = &(client->_internal.options.user_agent);
  const az_span* const model_id = &(client->_internal.options.model_id);

  az_span mqtt_user_name_span
      = az_span_init((uint8_t*)mqtt_user_name, (int32_t)mqtt_user_name_size);

  int32_t required_length = az_span_size(client->_internal.iot_hub_hostname)
      + az_span_size(client->_internal.device_id) + (int32_t)sizeof(hub_client_forward_slash);
  required_length += az_span_size(*model_id) > 0 ? az_span_size(hub_service_preview_api_version)
                                                 : az_span_size(hub_service_api_version);
  if (az_span_size(*module_id) > 0)
  {
    required_length += az_span_size(*module_id) + (int32_t)sizeof(hub_client_forward_slash);
  }
  if (az_span_size(*user_agent) > 0)
  {
    required_length += az_span_size(*user_agent) + az_span_size(hub_client_param_separator_span);
  }
  // Note we skip the length of the model id since we have to url encode it. Bound checking is done
  // later.
  if (az_span_size(*model_id) > 0)
  {
    required_length += az_span_size(hub_client_param_separator_span)
        + az_span_size(hub_client_param_equals_span);
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(
      mqtt_user_name_span, required_length + (int32_t)sizeof(null_terminator));

  az_span remainder = az_span_copy(mqtt_user_name_span, client->_internal.iot_hub_hostname);
  remainder = az_span_copy_u8(remainder, hub_client_forward_slash);
  remainder = az_span_copy(remainder, client->_internal.device_id);

  if (az_span_size(*module_id) > 0)
  {
    remainder = az_span_copy_u8(remainder, hub_client_forward_slash);
    remainder = az_span_copy(remainder, *module_id);
  }

  if (az_span_size(*model_id) > 0)
  {
    remainder = az_span_copy(remainder, hub_service_preview_api_version);
  }
  else
  {
    remainder = az_span_copy(remainder, hub_service_api_version);
  }

  if (az_span_size(*user_agent) > 0)
  {
    remainder = az_span_copy_u8(remainder, *az_span_ptr(hub_client_param_separator_span));
    remainder = az_span_copy(remainder, *user_agent);
  }

  if (az_span_size(*model_id) > 0)
  {
    remainder = az_span_copy_u8(remainder, *az_span_ptr(hub_client_param_separator_span));
    remainder = az_span_copy(remainder, hub_digital_twin_model_id);
    remainder = az_span_copy_u8(remainder, *az_span_ptr(hub_client_param_equals_span));

    AZ_RETURN_IF_FAILED(_az_span_copy_url_encode(remainder, *model_id, &remainder));
  }
  if (az_span_size(remainder) > 0)
  {
    remainder = az_span_copy_u8(remainder, null_terminator);
  }
  else
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }

  if (out_mqtt_user_name_length)
  {
    *out_mqtt_user_name_length
        = mqtt_user_name_size - (size_t)az_span_size(remainder) - sizeof(null_terminator);
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_get_client_id(
    az_iot_hub_client const* client,
    char* mqtt_client_id,
    size_t mqtt_client_id_size,
    size_t* out_mqtt_client_id_length)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(mqtt_client_id);
  _az_PRECONDITION(mqtt_client_id_size > 0);

  az_span mqtt_client_id_span
      = az_span_init((uint8_t*)mqtt_client_id, (int32_t)mqtt_client_id_size);
  const az_span* const module_id = &(client->_internal.options.module_id);

  int32_t required_length = az_span_size(client->_internal.device_id);
  if (az_span_size(*module_id) > 0)
  {
    required_length += az_span_size(*module_id) + (int32_t)sizeof(hub_client_forward_slash);
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(
      mqtt_client_id_span, required_length + (int32_t)sizeof(null_terminator));

  az_span remainder = az_span_copy(mqtt_client_id_span, client->_internal.device_id);

  if (az_span_size(*module_id) > 0)
  {
    remainder = az_span_copy_u8(remainder, hub_client_forward_slash);
    remainder = az_span_copy(remainder, *module_id);
  }

  az_span_copy_u8(remainder, null_terminator);

  if (out_mqtt_client_id_length)
  {
    *out_mqtt_client_id_length = (size_t)required_length;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_properties_init(
    az_iot_hub_client_properties* properties,
    az_span buffer,
    int32_t written_length)
{
  _az_PRECONDITION_NOT_NULL(properties);
  _az_PRECONDITION_VALID_SPAN(buffer, 0, true);
  _az_PRECONDITION(written_length >= 0);

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
  _az_PRECONDITION_NOT_NULL(properties);
  _az_PRECONDITION_VALID_SPAN(name, 1, false);
  _az_PRECONDITION_VALID_SPAN(value, 1, false);

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
  _az_PRECONDITION_NOT_NULL(properties);
  _az_PRECONDITION_VALID_SPAN(name, 1, false);
  _az_PRECONDITION_NOT_NULL(out_value);

  az_span remaining = az_span_slice(
      properties->_internal.properties_buffer, 0, properties->_internal.properties_written);

  while (az_span_size(remaining) != 0)
  {
    az_span delim_span = _az_span_token(remaining, hub_client_param_equals_span, &remaining);
    if (az_span_is_content_equal(delim_span, name))
    {
      *out_value = _az_span_token(remaining, hub_client_param_separator_span, &remaining);
      return AZ_OK;
    }
    else
    {
      az_span value;
      value = _az_span_token(remaining, hub_client_param_separator_span, &remaining);
      (void)value;
    }
  }

  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_NODISCARD az_result
az_iot_hub_client_properties_next(az_iot_hub_client_properties* properties, az_pair* out)
{
  _az_PRECONDITION_NOT_NULL(properties);
  _az_PRECONDITION_NOT_NULL(out);

  int32_t index = (int32_t)properties->_internal.current_property_index;
  int32_t prop_length = properties->_internal.properties_written;

  if (index == prop_length)
  {
    *out = AZ_PAIR_NULL;
    return AZ_ERROR_EOF;
  }

  az_span remainder;
  az_span prop_span = az_span_slice(properties->_internal.properties_buffer, index, prop_length);

  out->key = _az_span_token(prop_span, hub_client_param_equals_span, &remainder);
  out->value = _az_span_token(remainder, hub_client_param_separator_span, &remainder);
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

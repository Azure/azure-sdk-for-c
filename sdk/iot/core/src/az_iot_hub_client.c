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
static const uint8_t hub_client_null_terminate = '\0';

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
  AZ_PRECONDITION_NOT_NULL(options);

  client->_internal.iot_hub_hostname = iot_hub_hostname;
  client->_internal.device_id = device_id;
  client->_internal.options.module_id = options->module_id;
  client->_internal.options.user_agent = options->user_agent;

  return AZ_OK;
}

//[Format without module id] {iothubhostname}/{device_id}/?api-version=2018-06-30
//[Format with module id] {iothubhostname}/{device_id}/{module_id}/?api-version=2018-06-30
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

  if (az_span_capacity(mqtt_user_name)
      < (int32_t)(
            az_span_length(client->_internal.iot_hub_hostname) + sizeof(hub_client_forward_slash)
            + az_span_length(client->_internal.device_id) + sizeof(hub_client_forward_slash)
            + az_span_length(*module_id)
            + (az_span_length(*module_id) > 0 ? sizeof(hub_client_forward_slash) : 0)
            + az_span_length(*user_agent)
            + (az_span_length(*user_agent) > 0 ? sizeof(hub_client_param_separator) : 0)
            + az_span_length(hub_client_api_version) + sizeof(hub_client_null_terminate)))
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY;
  }

  AZ_RETURN_IF_FAILED(
      az_span_append(mqtt_user_name, client->_internal.iot_hub_hostname, &mqtt_user_name));
  AZ_RETURN_IF_FAILED(
      az_span_append_uint8(mqtt_user_name, hub_client_forward_slash, &mqtt_user_name));
  AZ_RETURN_IF_FAILED(
      az_span_append(mqtt_user_name, client->_internal.device_id, &mqtt_user_name));
  AZ_RETURN_IF_FAILED(
      az_span_append_uint8(mqtt_user_name, hub_client_forward_slash, &mqtt_user_name));

  if (az_span_length(*module_id) > 0)
  {
    AZ_RETURN_IF_FAILED(az_span_append(mqtt_user_name, *module_id, &mqtt_user_name));
    AZ_RETURN_IF_FAILED(
        az_span_append_uint8(mqtt_user_name, hub_client_forward_slash, &mqtt_user_name));
  }

  AZ_RETURN_IF_FAILED(
      az_span_append(mqtt_user_name, hub_client_api_version, &mqtt_user_name));

  if (az_span_length(*user_agent) > 0)
  {
    AZ_RETURN_IF_FAILED(
        az_span_append_uint8(mqtt_user_name, hub_client_param_separator, &mqtt_user_name));
    AZ_RETURN_IF_FAILED(az_span_append(mqtt_user_name, *user_agent, &mqtt_user_name));
  }

  AZ_RETURN_IF_FAILED(
      az_span_append_uint8(mqtt_user_name, hub_client_null_terminate, &mqtt_user_name));

  *out_mqtt_user_name = mqtt_user_name;

  return AZ_OK;
}

//[Format without module id] {device_id}
//[Format with module id] {device_id}/{module_id}
AZ_NODISCARD az_result az_iot_hub_client_id_get(
    az_iot_hub_client const* client,
    az_span mqtt_client_id,
    az_span* out_mqtt_client_id)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_client_id, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_client_id);

  const az_span* const module_id = &(client->_internal.options.module_id);

  if (az_span_capacity(mqtt_client_id)
      < (int32_t)(
            az_span_length(client->_internal.device_id) + az_span_length(*module_id)
            + (az_span_length(*module_id) > 0 ? sizeof(hub_client_forward_slash) : 0)
            + sizeof(hub_client_null_terminate)))
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY;
  }

  AZ_RETURN_IF_FAILED(
      az_span_append(mqtt_client_id, client->_internal.device_id, &mqtt_client_id));

  if (az_span_length(*module_id) > 0)
  {
    AZ_RETURN_IF_FAILED(
        az_span_append_uint8(mqtt_client_id, hub_client_forward_slash, &mqtt_client_id));
    AZ_RETURN_IF_FAILED(az_span_append(mqtt_client_id, *module_id, &mqtt_client_id));
  }

  AZ_RETURN_IF_FAILED(
      az_span_append_uint8(mqtt_client_id, hub_client_null_terminate, &mqtt_client_id));

  *out_mqtt_client_id = mqtt_client_id;

  return AZ_OK;
}

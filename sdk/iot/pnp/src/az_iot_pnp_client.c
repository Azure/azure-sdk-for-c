// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stddef.h>
#include <stdint.h>

#include "az_iot_pnp_client.h"
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

static const uint8_t pnp_client_param_separator = '&';
static const uint8_t pnp_client_param_equals = '=';

static const az_span pnp_model_id = AZ_SPAN_LITERAL_FROM_STR("digital-twin-model-id");

AZ_NODISCARD az_iot_pnp_client_options az_iot_pnp_client_options_default()
{
  az_iot_pnp_client_options options = (az_iot_pnp_client_options) {
    .user_agent = AZ_SPAN_NULL,
    .content_encoding = AZ_SPAN_NULL,
    .content_type = AZ_SPAN_NULL,
  };

  return options;
}

AZ_NODISCARD az_result az_iot_pnp_client_init(
    az_iot_pnp_client* client,
    az_span iot_hub_hostname,
    az_span device_id,
    az_span root_interface_name,
    az_iot_pnp_client_options const* options)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(iot_hub_hostname, 1, false);
  AZ_PRECONDITION_VALID_SPAN(device_id, 1, false);
  AZ_PRECONDITION_VALID_SPAN(root_interface_name, 1, false);

  az_iot_hub_client_options hub_options = az_iot_hub_client_options_default();
  hub_options.user_agent = (options != NULL) ? options->user_agent : AZ_SPAN_NULL;

  AZ_RETURN_IF_FAILED(az_iot_hub_client_init(
      &client->_internal.iot_hub_client, iot_hub_hostname, device_id, &hub_options));

  client->_internal.root_interface_name = root_interface_name;
  client->_internal.options = options == NULL ? az_iot_pnp_client_options_default() : *options;

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_pnp_client_get_user_name(
    az_iot_pnp_client const* client,
    az_span mqtt_user_name,
    az_span* out_mqtt_user_name)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_user_name, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_user_name);

  AZ_RETURN_IF_FAILED(az_iot_hub_client_user_name_get(
      &client->_internal.iot_hub_client, mqtt_user_name, &mqtt_user_name));
  AZ_RETURN_IF_FAILED(
      az_span_append_uint8(mqtt_user_name, pnp_client_param_separator, &mqtt_user_name));
  AZ_RETURN_IF_FAILED(az_span_append(mqtt_user_name, pnp_model_id, &mqtt_user_name));
  AZ_RETURN_IF_FAILED(
      az_span_append_uint8(mqtt_user_name, pnp_client_param_equals, &mqtt_user_name));
  AZ_RETURN_IF_FAILED(
      az_span_append(mqtt_user_name, client->_internal.root_interface_name, &mqtt_user_name));

  *out_mqtt_user_name = mqtt_user_name;

  return AZ_OK;
}

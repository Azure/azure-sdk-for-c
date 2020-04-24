// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stddef.h>
#include <stdint.h>

#include "az_iot_pnp_client.h"
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

static const uint8_t null_terminator = '\0';
static const uint8_t pnp_client_param_separator = '&';
static const uint8_t pnp_client_param_equals = '=';

static const az_span pnp_model_id = AZ_SPAN_LITERAL_FROM_STR("digital-twin-model-id");

AZ_NODISCARD az_iot_pnp_client_options az_iot_pnp_client_options_default()
{
  az_iot_pnp_client_options options = (az_iot_pnp_client_options){
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
    char* mqtt_user_name,
    size_t mqtt_user_name_size,
    size_t* out_mqtt_user_name_length)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_NOT_NULL(mqtt_user_name);
  AZ_PRECONDITION(mqtt_user_name_size > 0);

  int32_t written;

  // First get hub user name since it is unknown how long it will be
  AZ_RETURN_IF_FAILED(az_iot_hub_client_user_name_get(
      &client->_internal.iot_hub_client, mqtt_user_name, mqtt_user_name_size, (size_t*)&written));

  az_span mqtt_user_name_span
      = az_span_init((uint8_t*)mqtt_user_name, (int32_t)mqtt_user_name_size);

  int32_t required_length = written + az_span_size(pnp_model_id)
      + az_span_size(client->_internal.root_interface_name) + 2;

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_user_name_span, required_length);

  az_span remainder = az_span_slice_to_end(mqtt_user_name_span, written);

  remainder = az_span_copy_u8(remainder, pnp_client_param_separator);
  remainder = az_span_copy(remainder, pnp_model_id);
  remainder = az_span_copy_u8(remainder, pnp_client_param_equals);
  remainder = az_span_copy(remainder, client->_internal.root_interface_name);
  az_span_copy_u8(remainder, null_terminator);

  if(out_mqtt_user_name_length)
  {
    *out_mqtt_user_name_length = (size_t)required_length;
  }

  return AZ_OK;
}

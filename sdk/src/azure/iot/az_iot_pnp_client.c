// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stddef.h>
#include <stdint.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/az_version.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/iot/az_iot_hub_client.h>
#include <azure/iot/az_iot_pnp_client.h>

#include <azure/core/_az_cfg.h>

static const az_span client_sdk_version
    = AZ_SPAN_LITERAL_FROM_STR("DeviceClientType=c%2F" AZ_SDK_VERSION_STRING);

AZ_NODISCARD az_iot_pnp_client_options az_iot_pnp_client_options_default()
{
  return (az_iot_pnp_client_options){ .module_id = AZ_SPAN_EMPTY,
                                      .user_agent = client_sdk_version,
                                      .component_names = NULL,
                                      .component_names_length = 0 };
}

AZ_NODISCARD az_result az_iot_pnp_client_init(
    az_iot_pnp_client* client,
    az_span iot_hub_hostname,
    az_span device_id,
    az_span model_id,
    az_iot_pnp_client_options const* options)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(iot_hub_hostname, 1, false);
  _az_PRECONDITION_VALID_SPAN(device_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(model_id, 1, false);

  client->_internal.options = (options == NULL) ? az_iot_pnp_client_options_default() : *options;

  az_iot_hub_client_options hub_options = az_iot_hub_client_options_default();
  hub_options.model_id = model_id;
  hub_options.module_id = client->_internal.options.module_id;
  hub_options.user_agent = client->_internal.options.user_agent;

  _az_RETURN_IF_FAILED(az_iot_hub_client_init(
      &client->_internal.iot_hub_client, iot_hub_hostname, device_id, &hub_options));

  return AZ_OK;
}

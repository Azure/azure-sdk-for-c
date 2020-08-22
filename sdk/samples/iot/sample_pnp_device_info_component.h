// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef SAMPLE_PNP_DEVICE_INFO_COMPONENT_H
#define SAMPLE_PNP_DEVICE_INFO_COMPONENT_H

#include "sample_pnp_mqtt_component.h"

#include <azure/core/az_result.h>
#include <azure/iot/az_iot_hub_client.h>

typedef struct
{
  az_span component_name;
} pnp_device_info_component;

/**
 * @brief Get the payload to send for device info
 *
 * @param client The `az_iot_hub_client` pointer to the hub client.
 * @param mqtt_message The `sample_pnp_mqtt_message` pointer to the struct to be populated.
 *
 * @return An #az_result with the result of the operation.
 */
az_result sample_pnp_device_info_get_report_data(
    const az_iot_hub_client* client,
    sample_pnp_mqtt_message* mqtt_message);

#endif // SAMPLE_PNP_DEVICE_INFO_COMPONENT_H

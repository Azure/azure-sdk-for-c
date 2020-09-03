// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef PNP_DEVICE_INFO_COMPONENT_H
#define PNP_DEVICE_INFO_COMPONENT_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

/**
 * @brief Get the payload to send for device info
 *
 * @param client The `az_iot_hub_client` pointer to the hub client.
 * @param mqtt_message The `sample_pnp_mqtt_message` pointer to the struct to be populated.
 *
 * @return An #az_result with the result of the operation.
 */
az_result pnp_device_info_build_reported_property(az_span payload, az_span* out_payload);

#endif // PNP_DEVICE_INFO_COMPONENT_H

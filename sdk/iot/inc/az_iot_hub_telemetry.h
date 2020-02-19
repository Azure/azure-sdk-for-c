// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span.h>
#include <az_result.h>
#include "az_iot_mqtt.h"
#include "az_iot_hub_client.h"
#include "az_iot_hub_properties.h"

#ifndef _az_IOT_HUB_TELEMETRY_H
#define _az_IOT_HUB_TELEMETRY_H

#include <_az_cfg_prefix.h>

az_result az_iot_sendtelemetry(az_iot_hub_client* client, az_span payload, az_iot_hub_properties* properties, az_iot_mqtt_publish *mqtt_pub);
// Module to module:
az_result az_iot_sendtelemetry(az_iot_hub_client* client, az_span destination, az_span payload, az_iot_hub_properties* properties, az_iot_mqtt_publish *mqtt_pub);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_TELEMETRY_H

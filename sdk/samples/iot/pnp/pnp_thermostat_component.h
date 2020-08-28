// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef PNP_THERMOSTAT_COMPONENT_H
#define PNP_THERMOSTAT_COMPONENT_H

#include "pnp_mqtt_message.h"

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

typedef struct
{
  az_span component_name;
  double current_temperature;
  double min_temperature;
  double max_temperature;
  int32_t device_temperature_avg_count;
  double device_temperature_avg_total;
  double avg_temperature;
  bool send_max_temp_property;
} pnp_thermostat_component;

az_result pnp_thermostat_init(
    pnp_thermostat_component* thermostat_component,
    az_span component_name,
    double initial_temp);

az_result pnp_thermostat_get_telemetry_message(
    const az_iot_hub_client* client,
    const pnp_thermostat_component* thermostat_component,
    pnp_mqtt_message* mqtt_message);

bool pnp_thermostat_get_max_temp_report(
    const az_iot_hub_client* client,
    pnp_thermostat_component* thermostat_component,
    pnp_mqtt_message* mqtt_message);

az_result pnp_thermostat_process_property_update(
    const az_iot_hub_client* client,
    pnp_thermostat_component* thermostat_component,
    az_span component_name,
    const az_json_token* property_name,
    const az_json_reader* property_value,
    int32_t version,
    pnp_mqtt_message* mqtt_message);

az_result pnp_thermostat_process_command(
    const az_iot_hub_client* client,
    const pnp_thermostat_component* thermostat_component,
    const az_iot_hub_client_method_request* command_request,
    az_span component_name,
    az_span command_name,
    az_span command_payload,
    pnp_mqtt_message* mqtt_message,
    az_iot_status* status);

#endif // PNP_THERMOSTAT_COMPONENT_H

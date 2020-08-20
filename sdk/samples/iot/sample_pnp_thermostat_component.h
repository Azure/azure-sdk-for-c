// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef SAMPLE_PNP_THERMOSTAT_COMPONENT_H
#define SAMPLE_PNP_THERMOSTAT_COMPONENT_H

#include <stdint.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#include "sample_pnp_component_mqtt.h"

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
} sample_pnp_thermostat_component;

az_result sample_pnp_thermostat_init(
    sample_pnp_thermostat_component* handle,
    az_span component_name,
    double initial_temp);

az_result sample_pnp_thermostat_get_telemetry_message(
    az_iot_hub_client* client,
    sample_pnp_thermostat_component* handle,
    sample_pnp_mqtt_message* mqtt_message);

bool sample_pnp_thermostat_get_max_temp_report(
    az_iot_hub_client* client,
    sample_pnp_thermostat_component* handle,
    sample_pnp_mqtt_message* mqtt_message);

az_result sample_pnp_thermostat_process_property_update(
    az_iot_hub_client* client,
    sample_pnp_thermostat_component* handle,
    az_span component_name,
    az_json_token* property_name,
    az_json_reader* property_value,
    int32_t version,
    sample_pnp_mqtt_message* mqtt_message);

az_result sample_pnp_thermostat_process_command(
    az_iot_hub_client* client,
    sample_pnp_thermostat_component* handle,
    az_iot_hub_client_method_request* command_request,
    az_span component_name,
    az_span command_name,
    az_span command_payload,
    sample_pnp_mqtt_message* mqtt_message,
    az_iot_status* status);

#endif // SAMPLE_PNP_THERMOSTAT_COMPONENT_H

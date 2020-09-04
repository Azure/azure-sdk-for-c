// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef PNP_THERMOSTAT_COMPONENT_H
#define PNP_THERMOSTAT_COMPONENT_H

#include "pnp_mqtt_message.h"

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

typedef struct
{
  az_span component_name;
  double average_temperature;
  double current_temperature;
  double maximum_temperature;
  double minimum_temperature;
  double temperature_summation;
  uint32_t temperature_count;
  bool send_maximum_temperature_property;
} pnp_thermostat_component;

az_result pnp_thermostat_init(
    pnp_thermostat_component* out_thermostat_component,
    az_span component_name,
    double initial_temperature);

void pnp_thermostat_build_maximum_temperature_reported_property(
    pnp_thermostat_component* thermostat_component,
    az_span* out_property_name,
    az_span payload,
    az_span* out_payload);

void pnp_thermostat_build_error_reported_property_with_status(
    az_span component_name,
    az_span property_name,
    az_json_reader* property_value,
    az_iot_status status,
    int32_t version,
    az_span payload,
    az_span* out_payload);

void pnp_thermostat_build_telemetry_message(
    pnp_thermostat_component const* thermostat_component,
    az_span payload,
    az_span* out_payload);

az_result pnp_thermostat_process_command_request(
    pnp_thermostat_component const* thermostat_component,
    az_span command_name,
    az_span command_payload,
    az_iot_status* out_status,
    az_span payload,
    az_span* out_payload);

az_result pnp_thermostat_process_property_update(
    pnp_thermostat_component* ref_thermostat_component,
    az_span component_name,
    az_json_token const* property_name,
    az_json_reader const* property_value,
    int32_t version,
    az_span payload,
    az_span* out_payload);

#endif // PNP_THERMOSTAT_COMPONENT_H

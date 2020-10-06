// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef PNP_THERMOSTAT_COMPONENT_H
#define PNP_THERMOSTAT_COMPONENT_H

#include <stdbool.h>
#include <stdint.h>

#include <azure/az_core.h>
#include <azure/az_iot.h>

#include "pnp_mqtt_message.h"

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

/**
 * @brief Initialize a #pnp_thermostat_component which holds device thermostat info.
 *
 * @param[out] out_thermostat_component A pointer to a #out_thermostat_component instance to
 * initialize.
 * @param[in] component_name The name of the component.
 * @param[in] initial_temperature The initial temperature to set all temperature member variables.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK #pnp_thermostat_component is initialied successfully.
 * @retval #AZ_ERROR_ARG The pointer to the #pnp_thermostat_component instance is NULL.
 */
az_result pnp_thermostat_init(
    pnp_thermostat_component* out_thermostat_component,
    az_span component_name,
    double initial_temperature);

/**
 * @brief Build the thermostat's temperature telemetry message.
 *
 * @param[in] thermostat_component A pointer to the themostat component to get data.
 * @param[in] payload An #az_span with sufficient capacity to hold the json payload.
 * @param[out] out_payload A pointer to the #az_span containing the output json payload.
 */
void pnp_thermostat_build_telemetry_message(
    pnp_thermostat_component* thermostat_component,
    az_span payload,
    az_span* out_payload);

/**
 * @brief Build the thermostat's maximum temperature reported property message.
 *
 * @param[in] thermostat_component A pointer to the themostat component to get data.
 * @param[in] payload An #az_span with sufficient capacity to hold the json payload.
 * @param[out] out_payload A pointer to the #az_span containing the output json payload.
 * @param[out] out_property_name The name of the reported property to be sent.
 */
void pnp_thermostat_build_maximum_temperature_reported_property(
    pnp_thermostat_component* thermostat_component,
    az_span payload,
    az_span* out_payload,
    az_span* out_property_name);

/**
 * @brief Build the thermostat's error message with status.
 *
 * @param[in] component_name The name of the component for the reported property.
 * @param[in] property_name The name of the property for which to send an update.
 * @param[in] property_value The property value to be appended.
 * @param[in] status The return status for the error message ack.
 * @param[in] version The version for the reported property ack.
 * @param[in] payload An #az_span with sufficient capacity to hold the json payload.
 * @param[out] out_payload A pointer to the #az_span containing the output json payload.
 */
void pnp_thermostat_build_error_reported_property_with_status(
    az_span component_name,
    az_span property_name,
    az_json_reader* property_value,
    az_iot_status status,
    int32_t version,
    az_span payload,
    az_span* out_payload);

/**
 * @brief Update the thermostat's member variables and prepare reported property message.
 *
 * @param[in,out] ref_thermostat_component A pointer to the themostat component to update data.
 * @param[in] property_name The name of the property to be updated.
 * @param[in] property_value The value used for the property update.
 * @param[in] version The version parsed from the received message, and used to prepare the returned
 * reported property message.
 * @param[in] payload An #az_span with sufficient capacity to hold the prepared reported property
 * json payload.
 * @param[out] out_payload A pointer to the #az_span containing the output reported property json
 * payload.
 *
 * @return A boolean indicating if property was updated.
 * @retval True if property updated. False if property does not belong to thermostat component.
 */
bool pnp_thermostat_process_property_update(
    pnp_thermostat_component* ref_thermostat_component,
    az_json_token const* property_name,
    az_json_reader const* property_value,
    int32_t version,
    az_span payload,
    az_span* out_payload);

/**
 * @brief Update the thermostat's member variables and prepare reported property message.
 *
 * @param[in] thermostat_component A pointer to the themostat on which to invoke the command.
 * @param[in] command_name The name of the command to be invoked.
 * @param[in] command_received_payload The received payload to be used by the invoked command.
 * @param[in] payload An #az_span with sufficient capacity to hold the command response json
 * payload.
 * @param[out] out_payload A pointer to the #az_span containing the output command response json
 * payload. On success will be filled. On failure will be empty.
 * @param[out] out_status The status resulting from the invoked command, to be used in the command
 * response. On success will be AZ_IOT_STATUS_OK. On failure will be AZ_IOT_STATUS_BAD_REQUEST or
 * AZ_IOT_STATUS_NOT_FOUND.
 *
 * @return A boolean indicating if command was successfully invoked.
 * @retval True if command successfully invoked. False if command failed to be invoked.
 */
bool pnp_thermostat_process_command_request(
    pnp_thermostat_component const* thermostat_component,
    az_span command_name,
    az_span command_received_payload,
    az_span payload,
    az_span* out_payload,
    az_iot_status* out_status);

#endif // PNP_THERMOSTAT_COMPONENT_H

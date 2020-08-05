// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef SAMPLE_PNP_H
#define SAMPLE_PNP_H

#include <stdint.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#define PNP_STATUS_SUCCESS 200
#define PNP_STATUS_BAD_FORMAT 400
#define PNP_STATUS_NOT_FOUND 404
#define PNP_STATUS_INTERNAL_ERROR 500

typedef void (*pnp_property_callback)(
    az_span component_name,
    az_json_token* property_name,
    az_json_token* property_value,
    int32_t version,
    void* user_context_callback);

/**
 * @brief Callback which is called for each property found by the #pnp_process_twin_data()
 * API.
 */
typedef az_result (*pnp_append_property_callback)(az_json_writer* json_writer, void* context);

/**
 * @brief Gets the MQTT topic that must be used for device to cloud telemetry messages.
 * @remark Telemetry MQTT Publish messages must have QoS At least once (1).
 * @remark This topic can also be used to set the MQTT Will message in the Connect message.
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[in] properties An optional #az_iot_hub_client_properties object (can be NULL).
 * @param[in] component_name An optional component name if the telemetry is being sent from a
 *                           sub-component.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If
 *                        successful, contains a null-terminated string with the topic that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of
 *                                                  \p mqtt_topic. Can be `NULL`.
 * @return #az_result
 */
az_result pnp_get_telemetry_topic(
    az_iot_hub_client const* client,
    az_iot_hub_client_properties* properties,
    az_span component_name,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 * @brief Parse a JSON payload for a PnP component command
 *
 * @param[in] component_command Input JSON payload containing the details for the component command.
 * @param[out] component_name The parsed component name (if it exists).
 * @param[out] command_name The parsed command name.
 */
az_result pnp_parse_command_name(
    az_span component_command,
    az_span* component_name,
    az_span* command_name);

/**
 * @brief Build a reported property
 *
 * @param[in] json_buffer The span into which the json payload will be placed.
 * @param[in] component_name The name of the component for the reported property.
 * @param[in] property_name The name of the property to which to send an update.
 * @param[in] append_callback The user callback to invoke to add the property value.
 * @param[in] context The user context which is passed to the callback.
 * @param[out] out_span The #az_span pointer to the output json payload.
 */
az_result pnp_create_reported_property(
    az_span json_buffer,
    az_span component_name,
    az_span property_name,
    pnp_append_property_callback append_callback,
    void* context,
    az_span* out_span);

/**
 * @brief Build a reported property with the ack status
 *
 * @param[in] json_buffer The span into which the json payload will be placed.
 * @param[in] component_name The name of the component for the reported property.
 * @param[in] property_name The name of the property to which to send an update.
 * @param[in] append_callback The user callback to invoke to add the property value.
 * @param[in] context The user context which is passed to the callback.
 * @param[in] ack_code The return value for the reported property.
 * @param[in] ack_version The ack version for the reported property.
 * @param[in] ack_description The optional description for the reported property.
 * @param[out] out_span The #az_span pointer to the output json payload.
 */
az_result pnp_create_reported_property_with_status(
    az_span json_buffer,
    az_span component_name,
    az_span property_name,
    pnp_append_property_callback append_callback,
    void* context,
    int32_t ack_code,
    int32_t ack_version,
    az_span ack_description,
    az_span* out_span);

/**
 * @brief Iteratively get the next desired property.
 *
 * @param[in] json_reader A pointer to the json reader from which the properties will be retrieved.
 * @param[in] is_partial Boolean stating whether the JSON document is partial or not.
 * @param[in] sample_components_ptr A pointer to a set of `az_span` pointers containing all the
 * names for components.
 * @param[in] sample_components_num Number of components in the set pointed to by
 * `sample_components_ptr`.
 * @param[in] property_callback The callback which is called on each twin property.
 * @param[in] context_ptr Pointer to user context.
 */
az_result pnp_process_twin_data(
    az_json_reader* json_reader,
    bool is_partial,
    const az_span** sample_components_ptr,
    int32_t sample_components_num,
    pnp_property_callback property_callback,
    void* context_ptr);

#endif // SAMPLE_PNP_H

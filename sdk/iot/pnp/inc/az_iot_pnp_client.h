// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT


/**
 * @file az_iot_pnp_client.h
 *
 * @brief definition for the Azure IoT PnP device SDK.
 */

#ifndef _az_IOT_PNP_CLIENT_H
#define _az_IOT_PNP_CLIENT_H

#include <az_result.h>
#include <az_span.h>

// NOTE: az_iot_hub_client.h is included for internal use only.
#include <az_iot_hub_client.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * @brief Azure IoT PnP Client options.
 *
 */
typedef struct az_iot_pnp_client_options
{
  az_span user_agent; /**< The user-agent is a formatted string that will be used for Azure IoT
                         usage statistics. */
} az_iot_pnp_client_options;

/**
 * @brief Azure IoT PnP Client.
 *
 */
typedef struct az_iot_pnp_client
{
  struct
  {
    az_iot_hub_client iot_hub_client;
	az_span root_interface_name;
  } _internal;
} az_iot_pnp_client;

/**
 * @brief Gets the default Azure IoT PnP Client options.
 * @details Call this to obtain an initialized #az_iot_pnp_client_options structure that can be
 *          afterwards modified and passed to #az_iot_pnp_client_init.
 *
 * @return #az_iot_pnp_client_options.
 */
AZ_NODISCARD az_iot_pnp_client_options az_iot_pnp_client_options_default();

/**
 * @brief Initializes an Azure IoT PnP Client.
 *
 * @param[out] client The #az_iot_pnp_client to use for this call.
 * @param[in] iot_hub_hostname The IoT Hub Hostname.
 * @param[in] device_id The Device ID.
 * @param[in] root_interface_name The root interface of the #az_iot_pnp_client.
 * @param[in] options A reference to an #az_iot_pnp_client_options structure. Can be NULL.
 * @return #az_result.
 */
AZ_NODISCARD az_result az_iot_pnp_client_init(
    az_iot_pnp_client* client,
    az_span iot_hub_hostname,
    az_span device_id,
    az_span root_interface_name,
    az_iot_pnp_client_options const* options);

/**
 * @brief Gets the MQTT user name.
 *
 * The user name will be of the following format:
 * {iothubhostname}/{device_id}/?api-version=2018-06-30&{user_agent}?digital-twin-model-id={root_interface_name}
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] mqtt_user_name An empty #az_span with sufficient capacity to hold the MQTT user name.
 * @param[out] out_mqtt_user_name The output #az_span containing the MQTT user name.
 * @return #az_result.
 */
AZ_NODISCARD az_result az_iot_pnp_client_user_name_get(
    az_iot_pnp_client const* client,
    az_span mqtt_user_name,
    az_span* out_mqtt_user_name);

/**
 * @brief Gets the MQTT client id.
 *
 * The client id will be of the following format:
 * {device_id}
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] mqtt_client_id An empty #az_span with sufficient capacity to hold the MQTT client id.
 * @param[out] out_mqtt_client_id The output #az_span containing the MQTT client id.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_pnp_client_id_get(
    az_iot_pnp_client const* client,
    az_span mqtt_client_id,
    az_span* out_mqtt_client_id);

/**
 *
 * SAS Token APIs
 *
 *   Use the following APIs when the Shared Access Key is available to the application or stored
 *   within a Hardware Security Module. The APIs are not necessary if X509 Client Certificate
 *   Authentication is used.
 */

/**
 * @brief Gets the Shared Access clear-text signature.
 * @details The application must obtain a valid clear-text signature using this API, sign it using
 *          HMAC-SHA256 using the Shared Access Key as password then Base64 encode the result.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] token_expiration_epoch_time The time, in seconds, from 1/1/1970.
 * @param[in] signature An empty #az_span with sufficient capacity to hold the SAS signature.
 * @param[out] out_signature The output #az_span containing the SAS signature.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_pnp_client_sas_signature_get(
    az_iot_pnp_client const* client,
    uint32_t token_expiration_epoch_time,
    az_span signature,
    az_span* out_signature);

/**
 * @brief Gets the MQTT password.
 * @note The MQTT password must be an empty string if X509 Client certificates are used. Use this
 *       API only when authenticating with SAS tokens.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] base64_hmac_sha256_signature The Base64 encoded value of the HMAC-SHA256(signature,
 *                                         SharedAccessKey). The signature is obtained by using
 *                                         #az_iot_pnp_client_sas_signature_get.
 * @param[in] key_name The Shared Access Key Name (Policy Name). This is optional. For security
 *                     reasons we recommend using one key per device instead of using a global
 *                     policy key.
 * @param[in] mqtt_password An empty #az_span with sufficient capacity to hold the MQTT password.
 * @param[out] out_mqtt_password The output #az_span containing the MQTT password.
 * @return #az_result.
 */
AZ_NODISCARD az_result az_iot_pnp_client_sas_password_get(
    az_iot_pnp_client const* client,
    az_span base64_hmac_sha256_signature,
    az_span key_name,
    az_span mqtt_password,
    az_span* out_mqtt_password);

/*
 *
 * PnP Telemetry APIs
 *
 */

/**
 * @brief Azure IoT PnP telemetry options.
 *
 */
typedef struct az_iot_pnp_client_telemetry_options
{
  struct
  {
    // OPEN: I can't find samples in Azure SDK modeling this.  Closest is
    // sdk\storage\blobs\inc\az_storage_blobs.h where it has 'az_span option" in
    // az_storage_blobs_blob_download_options that's ignored in practice.
    int _not_used;
  } _internal;
} az_iot_pnp_client_telemetry_options;

/**
 * @brief Gets the default Azure IoT PnP telemetry options.
 * @details Call this to obtain an initialized #az_iot_pnp_client_telemetry_options structure that
 * can be afterwards modified and passed to #az_iot_pnp_client_telemetry_publish_topic_get.
 *
 * @return #az_iot_pnp_client_telemetry_options.
 */
AZ_NODISCARD az_iot_pnp_client_telemetry_options az_iot_pnp_client_telemetry_options_default();

/**
 * @brief Gets the MQTT topic that must be used for device to cloud telemetry messages.
 * @note Telemetry MQTT Publish messages must have QoS At Least Once (1).
 * @note This topic can also be used to set the MQTT Will message in the Connect message.
 *
 * Should the user want a null terminated topic string, they may allocate a buffer large enough
 * to fit the topic plus a null terminator. They must set the last byte themselves or zero
 * initialize the buffer.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] component_name An #az_span specifying the component name to publish telemetry on.
 * @param[in] content_type An #az_span specifying the content type of the message payload.
 * @param[in] encoding_type An #az_span specifying the encoding type of the message payload.
 * @param[in] options A reference to an #az_iot_pnp_client_telemetry_options structure. Can be NULL.
 * @param[in] mqtt_topic An empty #az_span with sufficient capacity to hold the MQTT topic.
 * @param[out] out_mqtt_topic The output #az_span containing the MQTT topic.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_pnp_client_telemetry_publish_topic_get(
    az_iot_pnp_client const* client,
    az_span component_name,
    az_span content_type,
    az_span encoding_type,
    az_iot_pnp_client_telemetry_options const* options,
    az_span mqtt_topic,
    az_span* out_mqtt_topic);

/*
 *
 * PnP Command APIs
 *
 */

/**
 * @brief Azure IoT PnP command subscription options.
 *
 */
typedef struct az_iot_pnp_client_command_subscription_options
{
  struct
  {
    int _not_used;
  } _internal;
} az_iot_pnp_client_command_subscription_options;

/**
 * @brief Gets the default Azure IoT PnP command subscription options.
 * @details Call this to obtain an initialized #az_iot_pnp_client_command_subscription_options
 * structure that can be afterwards modified and passed to
 * #az_iot_pnp_client_commands_subscribe_topic_filter_get.
 *
 * @return #az_iot_pnp_client_command_subscription_options.
 */
AZ_NODISCARD az_iot_pnp_client_command_subscription_options az_iot_pnp_client_command_subscription_options_default();

/**
 * @brief Gets the MQTT topic filter to subscribe to PnP commands.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] options A reference to an #az_iot_pnp_client_command_subscription_options structure.
 * Can be NULL.
 * @param[in] mqtt_topic_filter An empty #az_span with sufficient capacity to hold the MQTT topic
 *                              filter.
 * @param[out] out_mqtt_topic_filter The output #az_span containing the MQTT topic filter.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_pnp_client_commands_subscribe_topic_filter_get(
    az_iot_pnp_client const* client,
    az_iot_pnp_client_command_subscription_options const* options,
    az_span mqtt_topic_filter,
    az_span* out_mqtt_topic_filter);

/**
 * @brief A command request received from the PnP service.
 *
 */
typedef struct az_iot_pnp_client_command_request
{
  az_span request_id; /**< The request id.
                       * @note The application must match the method request and method response. */
  az_span component; /**< The component the command is targeted towards. */
  az_span command_name; /**< The name of command to invoke */
  az_span payload; /**< The body of the request for the command, stripped of any envelope data from
                        MQTT payload */
} az_iot_pnp_client_command_request;

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[in] received_payload An #az_span containing the received payload.
 * @param[out] out_request If the message is a command request, this will contain the
 *                         #az_iot_pnp_client_command_request.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_pnp_client_command_received_topic_parse(
    az_iot_pnp_client const* client,
    az_span received_topic,
    az_span received_payload,
    az_iot_pnp_client_command_request* out_command_request);

/**
 * @brief Azure IoT PnP command response options.
 *
 */
typedef struct az_iot_pnp_client_command_response_options
{
  struct
  {
    int _not_used;
  } _internal;
} az_iot_pnp_client_command_response_options;

/**
 * @brief Gets the default Azure IoT PnP command response options.
 * @details Call this to obtain an initialized #az_iot_pnp_client_command_response_options structure
 * that can be afterwards modified and passed to
 * #az_iot_pnp_client_commands_response_publish_topic_get.
 *
 * @return #az_iot_pnp_client_command_response_options.
 */
AZ_NODISCARD az_iot_pnp_client_command_response_options az_iot_pnp_client_command_response_options_default();

/**
 * @brief Gets the MQTT topic that must be used to respond to command requests.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] request_id The request id. Must match a received #az_iot_pnp_client_command_request
 *                       request_id.
 * @param[in] status The status. (E.g. 200 for success.)
 * @param[in] options A reference to an #az_iot_pnp_client_command_response_options structure. Can
 * be NULL.
 * @param[in] mqtt_topic An empty #az_span with sufficient capacity to hold the MQTT topic.
 * @param[out] out_mqtt_topic The output #az_span containing the MQTT topic.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_pnp_client_commands_response_publish_topic_get(
    az_iot_pnp_client const* client,
    az_span request_id,
    uint16_t status,
    az_iot_pnp_client_command_response_options const* options,
    az_span mqtt_topic,
    az_span* out_mqtt_topic);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_PNP_CLIENT_H

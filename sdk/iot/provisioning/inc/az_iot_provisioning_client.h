// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_provisioning_client.h
 *
 * @brief definition for the Azure Device Provisioning client.
 */

#ifndef _az_IOT_PROVISIONING_CLIENT_H
#define _az_IOT_PROVISIONING_CLIENT_H

#include <az_iot_core.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

#define AZ_IOT_PROVISIONING_SERVICE_VERSION "2019-03-31"

/**
 * @brief Azure IoT Provisioning Client options.
 *
 */
typedef struct az_iot_provisioning_client_options
{
  az_span user_agent; /**< The user-agent is a formatted string that will be used for Azure IoT
                         usage statistics. */
} az_iot_provisioning_client_options;

/**
 * @brief Azure IoT Provisioning Client.
 *
 */
typedef struct az_iot_provisioning_client
{
  struct
  {
    az_span global_device_endpoint;
    az_span id_scope;
    az_span registration_id;
    az_iot_provisioning_client_options options;
  } _internal;
} az_iot_provisioning_client;

/**
 * @brief Gets the default Azure IoT Provisioning Client options.
 * @details Call this to obtain an initialized #az_iot_provisioning_client_options structure that
 *          can be afterwards modified and passed to #az_iot_provisioning_client_init.
 *
 * @return #az_iot_provisioning_client_options.
 */
AZ_NODISCARD az_iot_provisioning_client_options az_iot_provisioning_client_options_default();

/**
 * @brief Initializes an Azure IoT Provisioning Client.
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[in] global_device_endpoint The global device endpoint.
 * @param[in] id_scope The ID Scope.
 * @param[in] registration_id The Registration ID. This must match the client certificate name (CN
 *                            part of the certificate subject).
 * @param[in] options A reference to an #az_iot_provisioning_client_options structure. Can be NULL.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_provisioning_client_init(
    az_iot_provisioning_client* client,
    az_span global_device_endpoint,
    az_span id_scope,
    az_span registration_id,
    az_iot_provisioning_client_options const* options);

/**
 * @brief Gets the MQTT user name.
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[in] mqtt_user_name An empty #az_span with sufficient capacity to hold the MQTT user name.
 * @param[out] out_mqtt_user_name The output #az_span containing the MQTT user name.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_provisioning_client_user_name_get(
    az_iot_provisioning_client const* client,
    az_span mqtt_user_name,
    az_span* out_mqtt_user_name);

/**
 * @brief Gets the MQTT client id.
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[in] mqtt_client_id An empty #az_span with sufficient capacity to hold the MQTT client id.
 * @param[out] out_mqtt_client_id The output #az_span containing the MQTT client id.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_provisioning_client_id_get(
    az_iot_provisioning_client const* client,
    az_span mqtt_client_id,
    az_span* out_mqtt_client_id);

/**
 *
 * SAS Token APIs
 *
 *   Use the following APIs when the Shared Access Key is available to the application or stored
 *   within a Hardware Security Module. The APIs are not necessary if X509 Client Certificate
 *   Authentication is used.
 *
 *   The TPM Asymmetric Device Provisioning protocol is not supported on the MQTT protocol. TPMs can
 *   still be used to securely store and perform HMAC-SHA256 operations for SAS tokens.
 */

/**
 * @brief Gets the Shared Access clear-text signature.
 * @details The application must obtain a valid clear-text signature using
 *          this API, sign it using HMAC-SHA256 using the Shared Access Key as password then Base64
 *          encode the result.
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[in] token_expiration_epoch_time The time, in seconds, from 1/1/1970.
 * @param[in] signature An empty #az_span with sufficient capacity to hold the SAS signature.
 * @param[out] out_signature The output #az_span containing the SAS signature.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_provisioning_client_sas_signature_get(
    az_iot_provisioning_client const* client,
    uint32_t token_expiration_epoch_time,
    az_span signature,
    az_span* out_signature);

/**
 * @brief Gets the MQTT password.
 * @note The MQTT password must be an empty string if X509 Client certificates are used. Use this
 *       API only when authenticating with SAS tokens.
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[in] base64_hmac_sha256_signature The Base64 encoded value of the HMAC-SHA256(signature,
 *                                         SharedAccessKey). The signature is obtained by using
 *                                         #az_iot_hub_client_sas_signature_get.
 * @param[in] key_name The Shared Access Key Name (Policy Name). This is optional. For security
 *                     reasons we recommend using one key per device instead of using a global
 *                     policy key.
 * @param[in] mqtt_password An empty #az_span with sufficient capacity to hold the MQTT password.
 * @param[out] out_mqtt_password The output #az_span containing the MQTT password.
 * @return #az_result.
 */
AZ_NODISCARD az_result az_iot_hub_client_sas_password_get(
    az_iot_provisioning_client const* client,
    az_span base64_hmac_sha256_signature,
    az_span key_name,
    az_span mqtt_password,
    az_span* out_mqtt_password);

/**
 *
 * Register APIs
 *
 *   Use the following APIs when the Shared Access Key is available to the application or stored
 *   within a Hardware Security Module. The APIs are not necessary if X509 Client Certificate
 *   Authentication is used.
 */

/**
 * @brief Gets the MQTT topic filter to subscribe to register responses.
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[in] mqtt_topic_filter An empty #az_span with sufficient capacity to hold the MQTT topic
 *                              filter.
 * @param[out] out_mqtt_topic_filter The output #az_span containing the MQTT topic filter.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_provisioning_client_register_subscribe_topic_filter_get(
    az_iot_provisioning_client const* client,
    az_span mqtt_topic_filter,
    az_span* out_mqtt_topic_filter);

/**
 * @brief The registration operation state.
 * @note This is returned only when the operation completed.
 *
 */
typedef struct az_iot_provisioning_client_registration_state
{
  az_span assigned_hub_hostname; /**< Assigned Azure IoT Hub hostname. @note This is only
                                    available if error_code is success. */
  az_span device_id; /**< Assigned device ID. */
  az_span json_payload; /**< Additional JSON payload. */
  az_iot_status error_code; /**< The register operation status. */
  uint32_t extended_error_code; /**< The extended, 6 digit error code. */
  az_span error_message; /**< Error description. */
} az_iot_provisioning_client_registration_state;

/**
 * @brief Register or query operation response.
 *
 */
typedef struct az_iot_provisioning_client_register_response
{
  az_iot_status status; /**< The current request status.
                         * @note The response for the register operation is
                         * available through #registration_state only. */
  az_span operation_id; /**< Operation ID of the register operation. */
  az_span registration_state; /**< An #az_span containing the state of the register operation.
                               * @details This can be one of the following: 'unassigned',
                               * 'assigning', 'assigned', 'failed', 'disabled'. */
  uint32_t retry_after_seconds; /**< Recommended timeout before sending the next MQTT publish. */
  az_iot_provisioning_client_registration_state
      registration_information; /**< If the operation is complete (success or error), the
                                   registration state will contain the hub and device id in case of
                                   success. */
} az_iot_provisioning_client_register_response;

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[in] received_payload An #az_span containing the received topic.
 * @param[out] out_response If the message is register-operation related, this will contain the
 *                          #az_iot_provisioning_client_register_response.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_provisioning_client_received_topic_payload_parse(
    az_iot_provisioning_client const* client,
    az_span received_topic,
    az_span received_payload,
    az_iot_provisioning_client_register_response* out_response);

/**
 * @brief Gets the MQTT topic that must be used to submit a Register request.
 * @note The payload of the MQTT publish message may contain a JSON document formatted according to
 * the Provisioning Service's Register Device specification.
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[in] mqtt_topic An empty #az_span with sufficient capacity to hold the MQTT topic.
 * @param[out] out_mqtt_topic The output #az_span containing the MQTT topic.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_provisioning_client_register_publish_topic_get(
    az_iot_provisioning_client const* client,
    az_span mqtt_topic,
    az_span* out_mqtt_topic);

/**
 * @brief Gets the MQTT topic that must be used to submit a Register Status request.
 * @note The payload of the MQTT publish message should be empty.
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[in] register_response The received #az_iot_provisioning_client_register_response response.
 * @param[in] mqtt_topic An empty #az_span with sufficient capacity to hold the MQTT topic.
 * @param[out] out_mqtt_topic The output #az_span containing the MQTT topic.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_provisioning_client_get_operation_status_publish_topic_get(
    az_iot_provisioning_client const* client,
    az_iot_provisioning_client_register_response const* register_response,
    az_span mqtt_topic,
    az_span* out_mqtt_topic);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_PROVISIONING_CLIENT_H

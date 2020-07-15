// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_provisioning_client.h
 *
 * @brief definition for the Azure Device Provisioning client.
 * @remark The Device Provisioning MQTT protocol is described at
 * https://docs.microsoft.com/en-us/azure/iot-dps/iot-dps-mqtt-support
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_IOT_PROVISIONING_CLIENT_H
#define _az_IOT_PROVISIONING_CLIENT_H

#include <azure/iot/az_iot_common.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

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
 * @param[in] options __[nullable]__ A reference to an
 *                                   #az_iot_provisioning_client_options structure. Can be `NULL`
 *                                   for default options.
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
 * @param[out] mqtt_user_name A buffer with sufficient capacity to hold the MQTT user name.
 *                            If successful, contains a null-terminated string with the user name
 *                            that needs to be passed to the MQTT client.
 * @param[in] mqtt_user_name_size The size, in bytes of \p mqtt_user_name.
 * @param[out] out_mqtt_user_name_length __[nullable]__ Contains the string length, in bytes, of
 *                                                      \p mqtt_user_name. Can be `NULL`.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_provisioning_client_get_user_name(
    az_iot_provisioning_client const* client,
    char* mqtt_user_name,
    size_t mqtt_user_name_size,
    size_t* out_mqtt_user_name_length);

/**
 * @brief Gets the MQTT client id.
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[out] mqtt_client_id A buffer with sufficient capacity to hold the MQTT client id.
 *                            If successful, contains a null-terminated string with the client id
 *                            that needs to be passed to the MQTT client.
 * @param[in] mqtt_client_id_size The size, in bytes of \p mqtt_client_id.
 * @param[out] out_mqtt_client_id_length __[nullable]__ Contains the string length, in bytes, of
 *                                                      of \p mqtt_client_id. Can be `NULL`.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_provisioning_client_get_client_id(
    az_iot_provisioning_client const* client,
    char* mqtt_client_id,
    size_t mqtt_client_id_size,
    size_t* out_mqtt_client_id_length);

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
 * @remark More information available at
 * https://docs.microsoft.com/en-us/azure/iot-dps/concepts-symmetric-key-attestation#detailed-attestation-process
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[in] token_expiration_epoch_time The time, in seconds, from 1/1/1970.
 * @param[in] signature An empty #az_span with sufficient capacity to hold the SAS signature.
 * @param[out] out_signature The output #az_span containing the SAS signature.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_provisioning_client_sas_get_signature(
    az_iot_provisioning_client const* client,
    uint64_t token_expiration_epoch_time,
    az_span signature,
    az_span* out_signature);

/**
 * @brief Gets the MQTT password.
 * @remark The MQTT password must be an empty string if X509 Client certificates are used. Use this
 *       API only when authenticating with SAS tokens.
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[in] base64_hmac_sha256_signature The Base64 encoded value of the HMAC-SHA256(signature,
 *                                         SharedAccessKey). The signature is obtained by using
 *                                         #az_iot_provisioning_client_sas_get_signature.
 * @param[in] token_expiration_epoch_time The time, in seconds, from 1/1/1970.
 * @param[in] key_name The Shared Access Key Name (Policy Name). This is optional. For security
 *                     reasons we recommend using one key per device instead of using a global
 *                     policy key.
 * @param[out] mqtt_password A buffer with sufficient capacity to hold the MQTT password.
 *                           If successful, contains a null-terminated string with the password that
 *                           needs to be passed to the MQTT client.
 * @param[in] mqtt_password_size The size, in bytes of \p mqtt_password.
 * @param[out] out_mqtt_password_length __[nullable]__ Contains the string length, in bytes, of
 *                                                     \p mqtt_password. Can be `NULL`.
 * @return #az_result.
 */
AZ_NODISCARD az_result az_iot_provisioning_client_sas_get_password(
    az_iot_provisioning_client const* client,
    az_span base64_hmac_sha256_signature,
    uint64_t token_expiration_epoch_time,
    az_span key_name,
    char* mqtt_password,
    size_t mqtt_password_size,
    size_t* out_mqtt_password_length);

/**
 *
 * Register APIs
 *
 *   Use the following APIs when the Shared Access Key is available to the application or stored
 *   within a Hardware Security Module. The APIs are not necessary if X509 Client Certificate
 *   Authentication is used.
 */

/**
 * @brief The MQTT topic filter to subscribe to register responses.
 * @remark Register MQTT Publish messages will have QoS At most once (0).
 */
#define AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC "$dps/registrations/res/#"

/**
 * @brief The registration operation state.
 * @remark This is returned only when the operation completed.
 *
 */
typedef struct az_iot_provisioning_client_registration_result
{
  az_span assigned_hub_hostname; /**< Assigned Azure IoT Hub hostname. @remark This is only
                                    available if error_code is success. */
  az_span device_id; /**< Assigned device ID. */
  az_iot_status error_code; /**< The error code. */
  uint32_t extended_error_code; /**< The extended, 6 digit error code. */
  az_span error_message; /**< Error description. */
  az_span error_tracking_id; /**< Submit this ID when asking for Azure IoT service-desk help. */
  az_span
      error_timestamp; /**< Submit this timestamp when asking for Azure IoT service-desk help. */
} az_iot_provisioning_client_registration_result;

/**
 * @brief Register or query operation response.
 *
 */
typedef struct az_iot_provisioning_client_register_response
{
  az_iot_status status; /**< The current request status.
                         * @remark The authoritative response for the device registration operation
                         * (which may require several requests) is available only through
                         * #operation_status.  */
  az_span operation_id; /**< Operation ID of the register operation. */
  az_span operation_status; /**< An #az_span containing the status of the register operation.
                             * @details This can be one of the following: `unassigned`,
                             * `assigning`, `assigned`, `failed`, `disabled`.
                             * #az_iot_provisioning_client_parse_operation_status can optionally
                             * be used to convert this into
                             * the #az_iot_provisioning_client_operation_status enum. */
  uint32_t retry_after_seconds; /**< Recommended timeout before sending the next MQTT publish. */
  az_iot_provisioning_client_registration_result
      registration_result; /**< If the operation is complete (success or error), the
                                   registration state will contain the hub and device id in case of
                                   success. */
} az_iot_provisioning_client_register_response;

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[in] received_topic An #az_span containing the received MQTT topic.
 * @param[in] received_payload An #az_span containing the received MQTT payload.
 * @param[out] out_response If the message is register-operation related, this will contain the
 *                          #az_iot_provisioning_client_register_response.
 * @return #az_result
 *         - `AZ_ERROR_IOT_TOPIC_NO_MATCH` if the topic is not matching the expected format.
 */
AZ_NODISCARD az_result az_iot_provisioning_client_parse_received_topic_and_payload(
    az_iot_provisioning_client const* client,
    az_span received_topic,
    az_span received_payload,
    az_iot_provisioning_client_register_response* out_response);

/**
 * @brief Azure IoT Provisioning Service operation status.
 * 
 */
typedef enum
{
  // Device assignment in progress.
  AZ_IOT_PROVISIONING_STATUS_UNASSIGNED,
  AZ_IOT_PROVISIONING_STATUS_ASSIGNING,
  
  // Device assignment operation complete.
  AZ_IOT_PROVISIONING_STATUS_ASSIGNED,
  AZ_IOT_PROVISIONING_STATUS_FAILED,
  AZ_IOT_PROVISIONING_STATUS_DISABLED,
} az_iot_provisioning_client_operation_status;

/**
 * @brief Returns the #az_iot_provisioning_client_operation_status of a
 * #az_iot_provisioning_client_register_response object.
 *
 * @param[in] response The #az_iot_provisioning_client_register_response obtained after a successful
 *                     call to #az_iot_provisioning_client_parse_received_topic_and_payload.
 * @param[out] out_operation_status The registration operation status.
 * @return #az_result
 *         - #AZ_ERROR_UNEXPECTED_CHAR if the string contains an unexpected value.
 */
AZ_NODISCARD az_result az_iot_provisioning_client_parse_operation_status(
    az_iot_provisioning_client_register_response* response,
    az_iot_provisioning_client_operation_status* out_operation_status);

/**
 * @brief Checks if the status indicates that the service has an authoritative result of the
 * register operation. The operation may have completed in either success or error.
 *
 * @param[in] operation_status The #az_iot_provisioning_client_operation_status obtained by calling
 * #az_iot_provisioning_client_parse_operation_status.
 * @return `true` if the operation completed. `false` otherwise.
 */
AZ_INLINE bool az_iot_provisioning_client_operation_complete(
    az_iot_provisioning_client_operation_status operation_status)
{
  return (operation_status > AZ_IOT_PROVISIONING_STATUS_ASSIGNING);
}

/**
 * @brief Gets the MQTT topic that must be used to submit a Register request.
 * @remark The payload of the MQTT publish message may contain a JSON document formatted according
 * to the [Provisioning Service's Device Registration document]
 * (https://docs.microsoft.com/en-us/rest/api/iot-dps/runtimeregistration/registerdevice#deviceregistration)
 * specification.
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic filter. If
 *                        successful, contains a null-terminated string with the topic filter that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of
 *                                                  \p mqtt_topic. Can be `NULL`.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_provisioning_client_register_get_publish_topic(
    az_iot_provisioning_client const* client,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 * @brief Gets the MQTT topic that must be used to submit a Register Status request.
 * @remark The payload of the MQTT publish message should be empty.
 *
 * @param[in] client The #az_iot_provisioning_client to use for this call.
 * @param[in] register_response The received #az_iot_provisioning_client_register_response response.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic filter. If
 *                        successful, contains a null-terminated string with the topic filter that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of
 *                                                  \p mqtt_topic. Can be `NULL`.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_provisioning_client_query_status_get_publish_topic(
    az_iot_provisioning_client const* client,
    az_iot_provisioning_client_register_response const* register_response,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

#include <azure/core/_az_cfg_suffix.h>

#endif //!_az_IOT_PROVISIONING_CLIENT_H

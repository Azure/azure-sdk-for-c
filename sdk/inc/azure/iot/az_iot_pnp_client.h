// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition for the Azure IoT Plug and Play device SDK.
 *
 * @warning THIS LIBRARY IS IN PREVIEW. APIS ARE SUBJECT TO CHANGE UNTIL GENERAL AVAILABILITY.
 */

#ifndef _az_IOT_PNP_CLIENT_H
#define _az_IOT_PNP_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/iot/az_iot_hub_client.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Azure IoT Plug and Play Client options.
 *
 */
typedef struct
{
  /**
   * The module name (if a module identity is used).
   */
  az_span module_id;

  /**
   * The user-agent is a formatted string that will be used for Azure IoT usage statistics.
   */
  az_span user_agent;

  /**
   * The array of component names for this device.
   */
  az_span* component_names;

  /**
   * The number of component names in the `component_names` array.
   */
  int32_t component_names_length;
} az_iot_pnp_client_options;

/**
 * @brief Azure IoT PnP Client.
 *
 */
typedef struct
{
  struct
  {
    az_iot_hub_client iot_hub_client;
    az_span model_id;
    az_iot_pnp_client_options options;
  } _internal;
} az_iot_pnp_client;

/**
 * @brief Gets the default Azure IoT PnP Client options.
 * @details Call this to obtain an initialized #az_iot_pnp_client_options structure that can be
 *          afterwards modified and passed to az_iot_pnp_client_init().
 *
 * @return #az_iot_pnp_client_options.
 */
AZ_NODISCARD az_iot_pnp_client_options az_iot_pnp_client_options_default();

/**
 * @brief Initializes an Azure IoT PnP Client.
 *
 * @param[out] out_client The #az_iot_pnp_client to use for this call.
 * @param[in] iot_hub_hostname The IoT Hub Hostname.
 * @param[in] device_id The device ID.
 * @param[in] model_id The root DTDL interface of the #az_iot_pnp_client.
 * @param[in] options A reference to an #az_iot_pnp_client_options structure. Can be `NULL`.
 *
 * @pre \p out_client must not be `NULL`.
 * @pre \p iot_hub_hostname must be a valid, non-empty #az_span.
 * @pre \p device_id must be a valid, non-empty #az_span.
 * @pre \p model_id must be a valid, non-empty #az_span.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_iot_pnp_client_init(
    az_iot_pnp_client* out_client,
    az_span iot_hub_hostname,
    az_span device_id,
    az_span model_id,
    az_iot_pnp_client_options const* options);

/**
 * @brief The HTTP URL Path necessary when connecting to IoT Hub using WebSockets.
 */
#define AZ_IOT_PNP_CLIENT_WEB_SOCKET_PATH "/$iothub/websocket"

/**
 * @brief The HTTP URL Path necessary when connecting to IoT Hub using WebSockets without an X509
 * client certificate.
 * @note Most devices should use #AZ_IOT_PNP_CLIENT_WEB_SOCKET_PATH. This option is available for
 * devices not using X509 client certificates that fail to connect to IoT Hub.
 */
#define AZ_IOT_PNP_CLIENT_WEB_SOCKET_PATH_NO_X509_CLIENT_CERT \
  AZ_IOT_PNP_CLIENT_WEB_SOCKET_PATH "?iothub-no-client-cert=true"

/**
 * @brief Gets the MQTT user name.
 *
 * The user name will be of the following format:
 *
 * `{iothubhostname}/{device_id}/?api-version=2020-09-30&{user_agent}&model-id={model_id}`
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[out] mqtt_user_name A buffer with sufficient capacity to hold the MQTT user name. If
 * successful, contains a null-terminated string with the user name that needs to be passed to the
 * MQTT client.
 * @param[in] mqtt_user_name_size The size, in bytes, of \p mqtt_user_name.
 * @param[out] out_mqtt_user_name_length __[nullable]__ Contains the string length, in bytes, of \p
 * mqtt_user_name. Can be `NULL`.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p mqtt_user_name must not be `NULL`.
 * @pre \p mqtt_user_name_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_iot_pnp_client_get_user_name(
    az_iot_pnp_client const* client,
    char* mqtt_user_name,
    size_t mqtt_user_name_size,
    size_t* out_mqtt_user_name_length);

/**
 * @brief Gets the MQTT client ID.
 *
 * The client ID will be of the following format:
 *
 * **Without module ID**
 *
 * `{device_id}`
 *
 * **With module ID**
 *
 * `{device_id}/{module_id}`
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[out] mqtt_client_id A buffer with sufficient capacity to hold the MQTT client ID. If
 * successful, contains a null-terminated string with the client ID that needs to be passed to the
 * MQTT client.
 * @param[in] mqtt_client_id_size The size, in bytes, of \p mqtt_client_id.
 * @param[out] out_mqtt_client_id_length __[nullable]__ Contains the string length, in bytes, of \p
 * mqtt_client_id. Can be `NULL`.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p mqtt_client_id must not be `NULL`.
 * @pre \p mqtt_client_id_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_iot_pnp_client_get_client_id(
    az_iot_pnp_client const* client,
    char* mqtt_client_id,
    size_t mqtt_client_id_size,
    size_t* out_mqtt_client_id_length);

/*
 *
 * SAS Token APIs
 *
 *   Use the following APIs when the Shared Access Key is available to the application or stored
 *   within a Hardware Security Module. The APIs are not necessary if X509 Client Certificate
 *   Authentication is used.
 */

/**
 * @brief Gets the Shared Access clear-text signature.
 *
 * The application must obtain a valid clear-text signature using this API, sign it using
 * HMAC-SHA256 using the Shared Access Key as password then Base64 encode the result.
 *
 * Use the SAS APIs when the Shared Access Key is available to the application or stored
 * within a Hardware Security Module. The APIs are not necessary if X509 Client Certificate
 * Authentication is used.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] token_expiration_epoch_time The time, in seconds, from 1/1/1970.
 * @param[in] signature An empty #az_span with sufficient capacity to hold the SAS signature.
 * @param[out] out_signature The output #az_span containing the SAS signature.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p token_expiration_epoch_time must be greater than 0.
 * @pre \p signature must be a valid, non-empty #az_span.
 * @pre \p out_signature must not be `NULL`. It must point to an #az_span instance.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The signature was retrieved successfully.
 * @retval #AZ_ERROR_NOT_ENOUGH_SPACE The buffer is too small.
 */
AZ_NODISCARD az_result az_iot_pnp_client_sas_get_signature(
    az_iot_pnp_client const* client,
    uint32_t token_expiration_epoch_time,
    az_span signature,
    az_span* out_signature);

/**
 * @brief Gets the MQTT password.
 * @note The MQTT password must be an empty string if X509 Client certificates are used. Use this
 * API only when authenticating with SAS tokens.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] base64_hmac_sha256_signature The Base64 encoded value of the `HMAC-SHA256(signature,
 * SharedAccessKey)`. The signature is obtained by using az_iot_pnp_client_sas_get_signature().
 * @param[in] token_expiration_epoch_time The time, in seconds, from 1/1/1970.
 * @param[in] key_name The Shared Access Key Name (Policy Name). This is optional. For security
 * reasons we recommend using one key per device instead of using a global policy key.
 * @param[out] mqtt_password A buffer with sufficient capacity to hold the MQTT password. If
 * successful, contains a null-terminated string with the password that needs to be passed to the
 * MQTT client.
 * @param[in] mqtt_password_size The size, in bytes, of \p mqtt_password.
 * @param[out] out_mqtt_password_length __[nullable]__ Contains the string length, in bytes, of \p
 * mqtt_password. Can be `NULL`.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p base64_hmac_sha256_signature must be a valid, non-empty #az_span.
 * @pre \p token_expiration_epoch_time must be greater than 0.
 * @pre \p mqtt_password must not be `NULL`.
 * @pre \p mqtt_password_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The mqtt password was retrieved successfully.
 * @retval #AZ_ERROR_NOT_ENOUGH_SPACE The buffer is too small.
 */
AZ_NODISCARD az_result az_iot_pnp_client_sas_get_password(
    az_iot_pnp_client const* client,
    uint64_t token_expiration_epoch_time,
    az_span base64_hmac_sha256_signature,
    az_span key_name,
    char* mqtt_password,
    size_t mqtt_password_size,
    size_t* out_mqtt_password_length);

/*
 *
 * PnP Telemetry APIs
 *
 */

/**
 * @brief Gets the MQTT topic that is used for device to cloud telemetry messages.
 * @note This topic can also be used to set the MQTT Will message in the Connect message.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] component_name An #az_span specifying the component name to publish telemetry on. Can
 * be #AZ_SPAN_EMPTY if the telemetry is not for a component.
 * @param[in] properties Properties to attach to append to the topic.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If successful,
 * contains a null-terminated string with the topic that needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes, of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of \p
 * mqtt_topic. Can be `NULL`.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p mqtt_topic must not be `NULL`.
 * @pre \p mqtt_topic_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic was retrieved successfully.
 */
AZ_NODISCARD az_result az_iot_pnp_client_telemetry_get_publish_topic(
    az_iot_pnp_client const* client,
    az_span component_name,
    az_iot_message_properties* properties,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/*
 *
 * PnP Command APIs
 *
 */

/**
 * @brief The MQTT topic filter to subscribe to command requests.
 * @note Commands MQTT Publish messages will have QoS At most once (0).
 */
#define AZ_IOT_PNP_CLIENT_COMMANDS_SUBSCRIBE_TOPIC "$iothub/methods/POST/#"

/**
 * @brief A command request received from IoT Hub.
 *
 */
typedef struct
{
  /**
   * The request ID.
   * @note The application must match the command request and command response.
   */
  az_span request_id;

  /**
   * The name of the component which the command was invoked for.
   * @note Can be `AZ_SPAN_EMPTY` if for the root component
   */
  az_span component_name;

  /**
   * The command name.
   */
  az_span command_name;
} az_iot_pnp_client_command_request;

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_request If the message is a command request, this will contain the
 * #az_iot_pnp_client_command_request.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p received_topic must be a valid, non-empty #az_span.
 * @pre \p out_request must not be `NULL`. It must point to an #az_iot_pnp_client_command_request
 * instance.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic is meant for this feature and the \p out_request was populated
 * with relevant information.
 * @retval #AZ_ERROR_IOT_TOPIC_NO_MATCH The topic does not match the expected format. This could
 * be due to either a malformed topic OR the message which came in on this topic is not meant for
 * this feature.
 */
AZ_NODISCARD az_result az_iot_pnp_client_commands_parse_received_topic(
    az_iot_pnp_client const* client,
    az_span received_topic,
    az_iot_pnp_client_command_request* out_request);

/**
 * @brief Gets the MQTT topic that is used to respond to command requests.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] request_id The request ID. Must match a received #az_iot_pnp_client_command_request
 * request_id.
 * @param[in] status A code that indicates the result of the command, as defined by the application.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If successful,
 * contains a null-terminated string with the topic that needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes, of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of \p
 * mqtt_topic. Can be `NULL`.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p request_id must be a valid, non-empty #az_span.
 * @pre \p mqtt_topic must not be `NULL`.
 * @pre \p mqtt_topic_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic was retrieved successfully.
 */
AZ_NODISCARD az_result az_iot_pnp_client_commands_response_get_publish_topic(
    az_iot_pnp_client const* client,
    az_span request_id,
    uint16_t status,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 *
 * Property APIs
 *
 */

/**
 * @brief The MQTT topic filter to subscribe to property operation responses.
 * @note Property MQTT Publish messages will have QoS At most once (0).
 */
#define AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_SUBSCRIBE_TOPIC "$iothub/twin/res/#"

/**
 * @brief Gets the MQTT topic filter to subscribe to desired property changes.
 * @note Property MQTT Publish messages will have QoS At most once (0).
 */
#define AZ_IOT_PNP_CLIENT_PROPERTY_PATCH_SUBSCRIBE_TOPIC "$iothub/twin/PATCH/properties/desired/#"

/**
 * @brief Property response type.
 *
 */
typedef enum
{
  AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET
  = AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_GET, /**< A response from a property "GET" request. */
  AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES
  = AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES, /**< A "PATCH" response with a payload
                                                                containing desired properties. */
  AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_REPORTED_PROPERTIES
  = AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES, /**< A response with the result of the
                                                                 earlier reported properties. */
} az_iot_pnp_client_property_response_type;

/**
 * @brief Property response.
 *
 */
typedef struct
{
  az_iot_pnp_client_property_response_type response_type; /**< Property response type. */
  az_iot_status status; /**< The operation status. */
  az_span
      request_id; /**< Request ID matches the ID specified when issuing a Get or Patch command. */
  az_span
      version; /**< The property object version.
                * @note This is only set when
                * response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES
                * or
                * response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_REPORTED_PROPERTIES. */
} az_iot_pnp_client_property_response;

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_response If the message is property-operation related, this will contain the
 *                         #az_iot_pnp_client_property_response.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p received_topic must be a valid, non-empty #az_span.
 * @pre \p out_response must not be `NULL`. It must point to an #az_iot_pnp_client_property_response
 * instance.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic is meant for this feature and the \p out_response was populated
 * with relevant information.
 * @retval #AZ_ERROR_IOT_TOPIC_NO_MATCH The topic does not match the expected format. This could
 * be due to either a malformed topic OR the message which came in on this topic is not meant for
 * this feature.
 */
AZ_NODISCARD az_result az_iot_pnp_client_property_parse_received_topic(
    az_iot_pnp_client const* client,
    az_span received_topic,
    az_iot_pnp_client_property_response* out_response);

/**
 * @brief Gets the MQTT topic that is used to submit a property GET request.
 * @note The payload of the MQTT publish message should be empty.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] request_id The request ID.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If
 *                        successful, contains a null-terminated string with the topic that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes, of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of
 *                                                  \p mqtt_topic. Can be `NULL`.
 * @pre \p client must not be `NULL`.
 * @pre \p request_id must be a valid, non-empty #az_span.
 * @pre \p mqtt_topic must not be `NULL`.
 * @pre \p mqtt_topic_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic was retrieved successfully.
 */
AZ_NODISCARD az_result az_iot_pnp_client_property_document_get_publish_topic(
    az_iot_pnp_client const* client,
    az_span request_id,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 * @brief Gets the MQTT topic that is used to submit a Plug and Play Property PATCH request.
 * @note The payload of the MQTT publish message should contain a JSON document formatted according
 * to the DTDL specification.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] request_id The request ID.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If
 *                        successful, contains a null-terminated string with the topic that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes, of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of
 *                                                  \p mqtt_topic. Can be `NULL`.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p request_id must be a valid, non-empty #az_span.
 * @pre \p mqtt_topic must not be `NULL`.
 * @pre \p mqtt_topic_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic was retrieved successfully.
 */
AZ_NODISCARD az_result az_iot_pnp_client_property_patch_get_publish_topic(
    az_iot_pnp_client const* client,
    az_span request_id,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 * @brief Append the necessary characters to a reported property JSON payload belonging to a
 * component.
 *
 * The payload will be of the form:
 *
 * @code
 * "reported": {
 *     "<component_name>": {
 *         "__t": "c",
 *         "temperature": 23
 *     }
 * }
 * @endcode
 *
 * @note This API should be used in conjunction with
 * az_iot_pnp_client_property_builder_end_component().
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in,out] ref_json_writer The #az_json_writer to append the necessary characters for an IoT
 * Plug and Play component.
 * @param[in] component_name The component name associated with the reported property.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p ref_json_writer must not be `NULL`.
 * @pre \p component_name must be a valid, non-empty #az_span.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The JSON payload was prefixed successfully.
 */
AZ_NODISCARD az_result az_iot_pnp_client_property_builder_begin_component(
    az_iot_pnp_client const* client,
    az_json_writer* ref_json_writer,
    az_span component_name);

/**
 * @brief Append the necessary characters to end a reported property JSON payload belonging to a
 * component.
 *
 * @note This API should be used in conjunction with
 * az_iot_pnp_client_property_builder_begin_component().
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in,out] ref_json_writer The #az_json_writer to append the necessary characters for an IoT
 * Plug and Play component.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p ref_json_writer must not be `NULL`.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The JSON payload was suffixed successfully.
 */
AZ_NODISCARD az_result az_iot_pnp_client_property_builder_end_component(
    az_iot_pnp_client const* client,
    az_json_writer* ref_json_writer);

/**
 * @brief Begin a property response payload with confirmation status.
 *
 * This API should be used in response to an incoming desired property. More details can be found
 * here:
 *
 * https://docs.microsoft.com/en-us/azure/iot-pnp/concepts-convention#writable-properties
 *
 * The payload will be of the form:
 *
 * **Without component**
 * @code
 * //{
 * //  "<property_name>":{
 * //    "ac": <ack_code>,
 * //    "av": <ack_version>,
 * //    "ad": "<ack_description>",
 * //    "value": <user_value>
 * //  }
 * //}
 * @endcode
 *
 * To send a status for a property belonging to a component, first call the
 * az_iot_pnp_client_property_builder_begin_component() API to prefix the payload with the
 * necessary identification. The API call flow would look like the following with the listed JSON
 * payload being generated.
 *
 * **With component**
 * @code
 *
 * az_iot_pnp_client_property_builder_begin_component()
 * az_iot_pnp_client_property_builder_begin_reported_status()
 * // Append user value here (<user_value>)
 * az_iot_pnp_client_property_builder_end_reported_status()
 * az_iot_pnp_client_property_builder_end_component()
 *
 * //{
 * //  "<component_name>": {
 * //    "__t": "c",
 * //    "<property_name>": {
 * //      "ac": <ack_code>,
 * //      "av": <ack_version>,
 * //      "ad": "<ack_description>",
 * //      "value": <user_value>
 * //    }
 * //  }
 * //}
 * @endcode
 *
 * @note This API should be used in conjunction with
 * az_iot_pnp_client_property_builder_end_reported_status().
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in,out] ref_json_writer The initialized #az_json_writer to append data to.
 * @param[in] property_name The name of the property to build a response payload for.
 * @param[in] ack_code The HTTP-like status code to respond with. See #az_iot_status for
 * possible supported values.
 * @param[in] ack_version The version of the property the application is acknowledging.
 * @param[in] ack_description An optional description detailing the context or any details about
 * the acknowledgement. This can be #AZ_SPAN_EMPTY.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p ref_json_writer must not be `NULL`.
 * @pre \p property_name must be a valid, non-empty #az_span.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The JSON payload was prefixed successfully.
 */
AZ_NODISCARD az_result az_iot_pnp_client_property_builder_begin_reported_status(
    az_iot_pnp_client const* client,
    az_json_writer* ref_json_writer,
    az_span property_name,
    int32_t ack_code,
    int32_t ack_version,
    az_span ack_description);

/**
 * @brief End a property response payload with confirmation status.
 *
 * @note This API should be used in conjunction with
 * az_iot_pnp_client_property_builder_begin_reported_status().
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in,out] ref_json_writer The initialized #az_json_writer to append data to.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p ref_json_writer must not be `NULL`.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The JSON payload was suffixed successfully.
 */
AZ_NODISCARD az_result az_iot_pnp_client_property_builder_end_reported_status(
    az_iot_pnp_client const* client,
    az_json_writer* ref_json_writer);

/**
 * @brief Read the IoT Plug and Play property version.
 *
 * @warning This modifies the state of the json reader. To then use the same json reader
 * with az_iot_pnp_client_property_get_next_component_property(), you must call
 * az_json_reader_init() again after this call and before the call to
 * az_iot_pnp_client_property_get_next_component_property() or make an additional copy before
 * calling this API.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in,out] ref_json_reader The pointer to the #az_json_reader used to parse through the JSON
 * payload.
 * @param[in] response_type The #az_iot_pnp_client_property_response_type representing the message
 * type associated with the payload.
 * @param[out] out_version The numeric version of the properties in the JSON payload.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p ref_json_reader must not be `NULL`.
 * @pre \p out_version must not be `NULL`.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK If the function returned a valid version.
 */
AZ_NODISCARD az_result az_iot_pnp_client_property_get_property_version(
    az_iot_pnp_client const* client,
    az_json_reader* ref_json_reader,
    az_iot_pnp_client_property_response_type response_type,
    int32_t* out_version);

/**
 * @brief Iteratively read the IoT Plug and Play component properties.
 *
 * Note that between calls, the #az_span pointed to by \p out_component_name shall not be modified,
 * only checked and compared. Internally, the #az_span is only changed if the component name changes
 * in the JSON document and is not necessarily set every invocation of the function.
 *
 * On success, the `ref_json_reader` will be set on a valid property name. After checking the
 * property name, the reader can be advanced to the property value by calling
 * az_json_reader_next_token(). Note that on the subsequent call to this API, it is expected that
 * the json reader will be placed AFTER the read property name and value. That means that after
 * reading the property value (including single values or complex objects), the user must call
 * az_json_reader_next_token().
 *
 * Below is a code snippet which you can use as a starting point:
 *
 * @code
 *
 * while (az_result_succeeded(az_iot_pnp_client_property_get_next_component_property(
 *       &pnp_client, &jr, response_type, &component_name)))
 * {
 *   // Check if property is of interest (substitute user_property for your own property name)
 *   if (az_json_token_is_text_equal(&jr.token, user_property))
 *   {
 *     az_json_reader_next_token(&jr);
 *
 *     // Get the property value here
 *     // Example: az_json_token_get_int32(&jr.token, &user_int);
 *
 *     // Skip to next property value
 *     az_json_reader_next_token(&jr);
 *   }
 *   else
 *   {
 *     // The JSON reader must be advanced regardless of whether the property
 *     // is of interest or not.
 *     az_json_reader_next_token(&jr);
 *
 *     // Skip children in case the property value is an object
 *     az_json_reader_skip_children(&jr);
 *     az_json_reader_next_token(&jr);
 *   }
 * }
 *
 * @endcode
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in,out] ref_json_reader The #az_json_reader to parse through. The ownership of iterating
 * through this json reader is shared between the user and this API.
 * @param[in] response_type The #az_iot_pnp_client_property_response_type representing the message
 * type associated with the payload.
 * @param[out] out_component_name The #az_span* representing the value of the component.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p ref_json_reader must not be `NULL`.
 * @pre \p out_component_name must not be `NULL`. It must point to an #az_span instance.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK If the function returned a valid #az_json_reader pointing to the property name and
 * the #az_span with a component name.
 * @retval #AZ_ERROR_JSON_INVALID_STATE If the json reader is passed in at an unexpected location.
 * @retval #AZ_ERROR_IOT_END_OF_PROPERTIES If there are no more properties left for the component.
 */
AZ_NODISCARD az_result az_iot_pnp_client_property_get_next_component_property(
    az_iot_pnp_client const* client,
    az_json_reader* ref_json_reader,
    az_iot_pnp_client_property_response_type response_type,
    az_span* out_component_name);

#include <azure/core/_az_cfg_suffix.h>

#endif //_az_IOT_PNP_CLIENT_H

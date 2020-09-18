// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_pnp_client.h
 *
 * @brief Definition for the Azure IoT PnP device SDK.
 * 
 * @warning THIS LIBRARY IS IN PREVIEW. APIS ARE SUBJECT TO CHANGE UNTIL GENERAL AVAILABILITY.
 */

#ifndef _az_IOT_PNP_CLIENT_H
#define _az_IOT_PNP_CLIENT_H

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/iot/az_iot_hub_client.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Azure IoT PnP Client options.
 *
 */
typedef struct
{
  az_span module_id; /**< The module name (if a module identity is used). */
  az_span user_agent; /**< The user-agent is a formatted string that will be used for Azure IoT
                         usage statistics. */
  az_span* component_names; /**< The array of component names for this device. */
  int32_t
      component_names_length; /**< The number of component names in the `component_names` array. */
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
 * @param[in] model_id The root interface of the #az_iot_pnp_client.
 * @param[in] options A reference to an #az_iot_pnp_client_options structure. Can be NULL.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p iot_hub_hostname must be a valid, non-empty #az_span.
 * @pre \p device_id must be a valid, non-empty #az_span.
 * @pre \p model_id must be a valid, non-empty #az_span.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_iot_pnp_client_init(
    az_iot_pnp_client* client,
    az_span iot_hub_hostname,
    az_span device_id,
    az_span model_id,
    az_iot_pnp_client_options const* options);

/**
 * @brief The HTTP URI Path necessary when connecting to IoT Hub using WebSockets.
 */
#define AZ_IOT_PNP_CLIENT_WEB_SOCKET_PATH "/$iothub/websocket"

/**
 * @brief The HTTP URI Path necessary when connecting to IoT Hub using WebSockets without an X509
 * client certificate.
 * @remark Most devices should use #AZ_IOT_PNP_CLIENT_WEB_SOCKET_PATH. This option is available for
 * devices not using X509 client certificates that fail to connect to IoT Hub.
 */
#define AZ_IOT_PNP_CLIENT_WEB_SOCKET_PATH_NO_X509_CLIENT_CERT \
  AZ_IOT_PNP_CLIENT_WEB_SOCKET_PATH "?iothub-no-client-cert=true"

/**
 * @brief Gets the MQTT user name.
 *
 * The user name will be of the following format:
 * {iothubhostname}/{device_id}/?api-version=2018-06-30&{user_agent}&digital-twin-model-id={model_id}
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[out] mqtt_user_name A buffer with sufficient capacity to hold the MQTT user name.
 *                            If successful, contains a null-terminated string with the user name
 *                            that needs to be passed to the MQTT client.
 * @param[in] mqtt_user_name_size The size, in bytes of \p mqtt_user_name.
 * @param[out] out_mqtt_user_name_length __[nullable]__ Contains the string length, in bytes, of
 *                                                      \p mqtt_user_name. Can be `NULL`.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p mqtt_user_name must not be `NULL`.
 * @pre \p mqtt_user_name_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD AZ_INLINE az_result az_iot_pnp_client_get_user_name(
    az_iot_pnp_client const* client,
    char* mqtt_user_name,
    size_t mqtt_user_name_size,
    size_t* out_mqtt_user_name_length)
{
  return az_iot_hub_client_get_user_name(
      &(client->_internal.iot_hub_client),
      mqtt_user_name,
      mqtt_user_name_size,
      out_mqtt_user_name_length);
}

/**
 * @brief Gets the MQTT client id.
 *
 * The client id will be of the following format:
 * {device_id}
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[out] mqtt_client_id A buffer with sufficient capacity to hold the MQTT client id.
 *                            If successful, contains a null-terminated string with the client id
 *                            that needs to be passed to the MQTT client.
 * @param[in] mqtt_client_id_size The size, in bytes of \p mqtt_client_id.
 * @param[out] out_mqtt_client_id_length __[nullable]__ Contains the string length, in bytes, of
 *                                                      of \p mqtt_client_id. Can be `NULL`.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p mqtt_client_id must not be `NULL`.
 * @pre \p mqtt_client_id_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD AZ_INLINE az_result az_iot_pnp_client_get_client_id(
    az_iot_pnp_client const* client,
    char* mqtt_client_id,
    size_t mqtt_client_id_size,
    size_t* out_mqtt_client_id_length)
{
  return az_iot_hub_client_get_client_id(
      &client->_internal.iot_hub_client,
      mqtt_client_id,
      mqtt_client_id_size,
      out_mqtt_client_id_length);
}

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
 * @details The application must obtain a valid clear-text signature using this API, sign it using
 *          HMAC-SHA256 using the Shared Access Key as password then Base64 encode the result.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] token_expiration_epoch_time The time, in seconds, from 1/1/1970.
 * @param[in] signature An empty #az_span with sufficient capacity to hold the SAS signature.
 * @param[out] out_signature The output #az_span containing the SAS signature.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p token_expiration_epoch_time must be greater than 0.
 * @pre \p signature must be a valid, non-empty #az_span.
 * @pre \p out_signature must not be `NULL`. It must point to a valid #az_span instance.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD AZ_INLINE az_result az_iot_pnp_client_get_sas_signature(
    az_iot_pnp_client const* client,
    uint32_t token_expiration_epoch_time,
    az_span signature,
    az_span* out_signature)
{
  return az_iot_hub_client_sas_get_signature(
      &client->_internal.iot_hub_client, token_expiration_epoch_time, signature, out_signature);
}

/**
 * @brief Gets the MQTT password.
 * @remark The MQTT password must be an empty string if X509 Client certificates are used. Use this
 *         API only when authenticating with SAS tokens.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] base64_hmac_sha256_signature The Base64 encoded value of the HMAC-SHA256(signature,
 *                                         SharedAccessKey). The signature is obtained by using
 *                                         #az_iot_pnp_client_sas_signature_get.
 * @param[in] token_expiration_epoch_time The time, in seconds, from 1/1/1970.
 * @param[in] key_name The Shared Access Key Name (Policy Name). This is optional. For security
 *                     reasons we recommend using one key per device instead of using a global
 *                     policy key.
 * @param[out] mqtt_password A buffer with sufficient capacity to hold the MQTT password.
 *                           If successful, contains a null-terminated string with the password
 *                           that needs to be passed to the MQTT client.
 * @param[in] mqtt_password_size The size, in bytes of \p mqtt_password.
 * @param[out] out_mqtt_password_length __[nullable]__ Contains the string length, in bytes, of
 *                                                     \p mqtt_password. Can be `NULL`.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p base64_hmac_sha256_signature must be a valid, non-empty #az_span.
 * @pre \p token_expiration_epoch_time must be greater than 0.
 * @pre \p mqtt_password must not be `NULL`.
 * @pre \p mqtt_password_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD AZ_INLINE az_result az_iot_pnp_client_get_sas_password(
    az_iot_pnp_client const* client,
    uint64_t token_expiration_epoch_time,
    az_span base64_hmac_sha256_signature,
    az_span key_name,
    char* mqtt_password,
    size_t mqtt_password_size,
    size_t* out_mqtt_password_length)
{
  return az_iot_hub_client_sas_get_password(
      &client->_internal.iot_hub_client,
      token_expiration_epoch_time,
      base64_hmac_sha256_signature,
      key_name,
      mqtt_password,
      mqtt_password_size,
      out_mqtt_password_length);
}

/*
 *
 * PnP Telemetry APIs
 *
 */

/**
 * @brief Gets the MQTT topic that must be used for device to cloud telemetry messages.
 * @remark Telemetry MQTT Publish messages must have QoS At Least Once (1).
 * @remark This topic can also be used to set the MQTT Will message in the Connect message.
 *
 * Should the user want a null terminated topic string, they may allocate a buffer large enough
 * to fit the topic plus a null terminator. They must set the last byte themselves or zero
 * initialize the buffer.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] component_name An #az_span specifying the component name to publish telemetry on.
 * @param[in] properties Properties to attach to append to the topic.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If
 *                        successful, contains a null-terminated string with the topic that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of
 *                                                  \p mqtt_topic. Can be `NULL`.
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
 * @remark Commands MQTT Publish messages will have QoS At most once (0).
 */
#define AZ_IOT_PNP_CLIENT_COMMANDS_SUBSCRIBE_TOPIC "$iothub/methods/POST/#"

/**
 * @brief A command request received from IoT Hub.
 *
 */
typedef struct
{
  az_span
      request_id; /**< The request id.
                   * @note The application must match the command request and command response. */
  az_span component; /**< The name of the component which the command was invoked for.
                      * @note Can be `AZ_SPAN_EMPTY` if for the root component */
  az_span name; /**< The command name. */
} az_iot_pnp_client_command_request;

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_request If the message is a command request, this will contain the
 *                         #az_iot_pnp_client_command_request.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p received_topic must be a valid, non-empty #az_span.
 * @pre \p out_twin_response must not be `NULL`. It must point to a valid
 * #az_iot_pnp_client_command_request instance.
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
 * @brief Gets the MQTT topic that must be used to respond to command requests.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] request_id The request id. Must match a received #az_iot_pnp_client_command_request
 *                       request_id.
 * @param[in] status A code that indicates the result of the command, as defined by the user.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If
 *                        successful, contains a null-terminated string with the topic that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
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
AZ_NODISCARD AZ_INLINE az_result az_iot_pnp_client_commands_response_get_publish_topic(
    az_iot_pnp_client const* client,
    az_span request_id,
    uint16_t status,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  return az_iot_hub_client_methods_response_get_publish_topic(
      &client->_internal.iot_hub_client,
      request_id,
      status,
      mqtt_topic,
      mqtt_topic_size,
      out_mqtt_topic_length);
}

/**
 *
 * Twin APIs
 *
 */

/**
 * @brief The MQTT topic filter to subscribe to twin operation responses.
 * @remark Twin MQTT Publish messages will have QoS At most once (0).
 */
#define AZ_IOT_PNP_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC "$iothub/twin/res/#"

/**
 * @brief Gets the MQTT topic filter to subscribe to twin desired property changes.
 * @remark Twin MQTT Publish messages will have QoS At most once (0).
 */
#define AZ_IOT_PNP_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC "$iothub/twin/PATCH/properties/desired/#"

/**
 * @brief Twin response type.
 *
 */
typedef enum
{
  AZ_IOT_PNP_CLIENT_TWIN_RESPONSE_TYPE_GET = AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_GET,
  AZ_IOT_PNP_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES = AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES,
  AZ_IOT_PNP_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES = AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES,
} az_iot_pnp_client_twin_response_type;

/**
 * @brief Twin response.
 *
 */
typedef struct
{
  az_iot_pnp_client_twin_response_type response_type; /**< Twin response type. */
  az_iot_status status; /**< The operation status. */
  az_span
      request_id; /**< Request ID matches the ID specified when issuing a Get or Patch command. */
  az_span version; /**< The Twin object version.
                    * @remark This is only returned when
                    * response_type==AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES
                    * or
                    * response_type==AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES. */
} az_iot_pnp_client_twin_response;

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_twin_response If the message is twin-operation related, this will contain the
 *                         #az_iot_pnp_client_twin_response.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p received_topic must be a valid, non-empty #az_span.
 * @pre \p out_twin_response must not be `NULL`. It must point to a valid
 * #az_iot_pnp_client_twin_response instance.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic is meant for this feature and the \p out_response was populated
 * with relevant information.
 * @retval #AZ_ERROR_IOT_TOPIC_NO_MATCH The topic does not match the expected format. This could
 * be due to either a malformed topic OR the message which came in on this topic is not meant for
 * this feature.
 */
AZ_NODISCARD az_result az_iot_pnp_client_twin_parse_received_topic(
    az_iot_pnp_client const* client,
    az_span received_topic,
    az_iot_pnp_client_twin_response* out_twin_response);

/**
 * @brief Gets the MQTT topic that must be used to submit a Twin GET request.
 * @remark The payload of the MQTT publish message should be empty.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] request_id The request id.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If
 *                        successful, contains a null-terminated string with the topic that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
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
AZ_NODISCARD AZ_INLINE az_result az_iot_pnp_client_twin_document_get_publish_topic(
    az_iot_pnp_client const* client,
    az_span request_id,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  return az_iot_hub_client_twin_document_get_publish_topic(
      &client->_internal.iot_hub_client,
      request_id,
      mqtt_topic,
      mqtt_topic_size,
      out_mqtt_topic_length);
}

/**
 * @brief Gets the MQTT topic that must be used to submit a Twin PATCH request.
 * @remark The payload of the MQTT publish message should contain a JSON document
 *         formatted according to the Twin specification.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] request_id The request id.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If
 *                        successful, contains a null-terminated string with the topic that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
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
AZ_NODISCARD AZ_INLINE az_result az_iot_pnp_client_twin_patch_get_publish_topic(
    az_iot_pnp_client const* client,
    az_span request_id,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  return az_iot_hub_client_twin_patch_get_publish_topic(
      &client->_internal.iot_hub_client,
      request_id,
      mqtt_topic,
      mqtt_topic_size,
      out_mqtt_topic_length);
}

/**
 * @brief Append the necessary characters to a JSON payload to begin a twin component.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in,out] The #az_json_writer to append the necessary characters for an IoT Plug and Play
 * component.
 * @param[in] component_name The component name to begin.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p json_writer must not be `NULL`.
 * @pre \p component_name must be a valid, non-empty #az_span.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The json payload was prefixed successfully.
 */
AZ_NODISCARD az_result az_iot_pnp_client_twin_property_begin_component(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer,
    az_span component_name);

/**
 * @brief Append the necessary characters to a JSON payload to end a twin component.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in,out] The #az_json_writer to append the necessary characters for an IoT Plug and Play
 * component.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p json_writer must not be `NULL`.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The json payload was suffixed successfully.
 */
AZ_NODISCARD az_result az_iot_pnp_client_twin_property_end_component(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer);

/**
 * @brief Begin a property response payload with status
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] json_writer The initialized #az_json_writer to append data to.
 * @param[in] component_name The name of the component to use with this property payload. If this is
 * for a root or non-component, this can be #AZ_SPAN_EMPTY.
 * @param[in] property_name The name of the property to build a response payload for.
 * @param[in] ack_code The HTTP-like status code to respond with. Please see #az_iot_status for
 * possible supported values.
 * @param[in] ack_version The version of the property the application is acknowledging.
 * @param[in] ack_description The optional description detailing the context or any details about
 * the acknowledgement.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p json_writer must not be `NULL`.
 * @pre \p property_name must be a valid, non-empty #az_span.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The json payload was prefixed successfully.
 */
AZ_NODISCARD az_result az_iot_pnp_client_twin_begin_property_with_status(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer,
    az_span component_name,
    az_span property_name,
    int32_t ack_code,
    int32_t ack_version,
    az_span ack_description);

/**
 * @brief End a property response payload with status
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] json_writer The initialized #az_json_writer to append data to.
 * @param[in] component_name The name of the component to use with this property payload. If this is
 * for a root or non-component, this can be #AZ_SPAN_EMPTY.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p json_writer must not be `NULL`.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The json payload was suffixed successfully.
 */
AZ_NODISCARD az_result az_iot_pnp_client_twin_end_property_with_status(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer,
    az_span component_name);

/**
 * @brief Read the IoT Plug and Play twin properties version for a given component
 *
 * This API shall be used in conjunction with az_iot_pnp_client_twin_get_next_component(). For
 * usage, please see the documentation for az_iot_pnp_client_twin_get_next_component().
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] json_reader The #az_json_reader to parse through.
 * @param[in] is_partial The boolean representing whether the twin document is from a partial update
 * (#AZ_IOT_PNP_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES) or a full twin document
 * (#AZ_IOT_PNP_CLIENT_TWIN_RESPONSE_TYPE_GET).
 * @param[out] out_version The version of the properties in the json payload.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p json_reader must not be `NULL`.
 * @pre \p out_version must not be `NULL`. It must point to a valid int32_t variable.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK If the function returned a valid version.
 */
AZ_NODISCARD az_result az_iot_pnp_client_twin_get_property_version(
    az_iot_pnp_client const* client,
    az_json_reader* json_reader,
    bool is_partial,
    int32_t* out_version);

/**
 * @brief Read the IoT Plug and Play twin properties component-by-component.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] json_reader The #az_json_reader to parse through.
 * @param[in] is_partial The boolean representing whether the twin document is from a partial update
 * (#AZ_IOT_PNP_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES) or a full twin document
 * (#AZ_IOT_PNP_CLIENT_TWIN_RESPONSE_TYPE_GET).
 * @param[out] out_component_name The #az_span* representing the value of the component.
 * @param[out] out_property_name The #az_json_token* representing the name of the property.
 * @param[out] out_property_value The #az_json_reader* representing the value of the property.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p json_reader must not be `NULL`.
 * @pre \p out_component_name must not be `NULL`. It must point to a valid #az_span instance.
 * @pre \p out_property_name must not be `NULL`. It must point to a valid #az_json_token instance.
 * @pre \p out_property_value must not be `NULL`. It must point to a valid #az_json_reader instance.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK If the function returned a valid property name and value.
 * @retval #AZ_IOT_END_OF_PROPERTIES If there are no more properties left for the component.
 */
AZ_NODISCARD az_result az_iot_pnp_client_twin_get_next_component_property(
    az_iot_pnp_client const* client,
    az_json_reader* json_reader,
    bool is_partial,
    az_span* out_component_name,
    az_json_token* out_property_name,
    az_json_reader* out_property_value);

#include <azure/core/_az_cfg_suffix.h>

#endif //!_az_IOT_PNP_CLIENT_H

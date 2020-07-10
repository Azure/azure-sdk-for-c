// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_hub_client.h
 *
 * @brief definition for the Azure IoT Hub client.
 * @remark The IoT Hub MQTT protocol is described at
 * https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-mqtt-support
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_IOT_HUB_CLIENT_H
#define _az_IOT_HUB_CLIENT_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_common.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Azure IoT service MQTT bit field properties for telemetry publish messages.
 *
 */
enum
{
  AZ_HUB_CLIENT_DEFAULT_MQTT_TELEMETRY_QOS = 0,
  AZ_HUB_CLIENT_DEFAULT_MQTT_TELEMETRY_DUPLICATE = 0,
  AZ_HUB_CLIENT_DEFAULT_MQTT_TELEMETRY_RETAIN = 1
};

/**
 * @brief Azure IoT Hub Client options.
 *
 */
typedef struct az_iot_hub_client_options
{
  az_span module_id; /**< The module name (if a module identity is used). */
  az_span user_agent; /**< The user-agent is a formatted string that will be used for Azure IoT
                         usage statistics. */
  az_span model_id; /**< The model id used to identify the capabilities of a device based on the
                       Digital Twin document */
} az_iot_hub_client_options;

/**
 * @brief Azure IoT Hub Client.
 *
 */
typedef struct az_iot_hub_client
{
  struct
  {
    az_span iot_hub_hostname;
    az_span device_id;
    az_iot_hub_client_options options;
  } _internal;
} az_iot_hub_client;

/**
 * @brief Gets the default Azure IoT Hub Client options.
 * @details Call this to obtain an initialized #az_iot_hub_client_options structure that can be
 *          afterwards modified and passed to #az_iot_hub_client_init.
 *
 * @return #az_iot_hub_client_options.
 */
AZ_NODISCARD az_iot_hub_client_options az_iot_hub_client_options_default();

/**
 * @brief Initializes an Azure IoT Hub Client.
 *
 * @param[out] client The #az_iot_hub_client to use for this call.
 * @param[in] iot_hub_hostname The IoT Hub Hostname.
 * @param[in] device_id The Device ID.
 * @param[in] options A reference to an #az_iot_hub_client_options structure. If `NULL` is passed,
 * `az_iot_hub_client_init()` will use the default options.
 * @return #az_result.
 */
AZ_NODISCARD az_result az_iot_hub_client_init(
    az_iot_hub_client* client,
    az_span iot_hub_hostname,
    az_span device_id,
    az_iot_hub_client_options const* options);

/**
 * @brief The HTTP URI Path necessary when connecting to IoT Hub using WebSockets.
 */
#define AZ_IOT_HUB_CLIENT_WEB_SOCKET_PATH "/$iothub/websocket"

/**
 * @brief The HTTP URI Path necessary when connecting to IoT Hub using WebSockets without an X509
 * client certificate.
 * @remark Most devices should use #AZ_IOT_HUB_CLIENT_WEB_SOCKET_PATH. This option is available for
 * devices not using X509 client certificates that fail to connect to IoT Hub.
 */
#define AZ_IOT_HUB_CLIENT_WEB_SOCKET_PATH_NO_X509_CLIENT_CERT \
  AZ_IOT_HUB_CLIENT_WEB_SOCKET_PATH "?iothub-no-client-cert=true"

/**
 * @brief Gets the MQTT user name.
 *
 * The user name will be of the following format:
 * [Format without module id] {iothubhostname}/{device_id}/?api-version=2018-06-30&{user_agent}
 * [Format with module id]
 * {iothubhostname}/{device_id}/{module_id}/?api-version=2018-06-30&{user_agent}
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[out] mqtt_user_name A buffer with sufficient capacity to hold the MQTT user name.
 *                            If successful, contains a null-terminated string with the user name
 *                            that needs to be passed to the MQTT client.
 * @param[in] mqtt_user_name_size The size, in bytes of \p mqtt_user_name.
 * @param[out] out_mqtt_user_name_length __[nullable]__ Contains the string length, in bytes, of
 *                                                      \p mqtt_user_name. Can be `NULL`.
 * @return #az_result.
 */
AZ_NODISCARD az_result az_iot_hub_client_get_user_name(
    az_iot_hub_client const* client,
    char* mqtt_user_name,
    size_t mqtt_user_name_size,
    size_t* out_mqtt_user_name_length);

/**
 * @brief Gets the MQTT client id.
 *
 * The client id will be of the following format:
 * [Format without module id] {device_id}
 * [Format with module id] {device_id}/{module_id}
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[out] mqtt_client_id A buffer with sufficient capacity to hold the MQTT client id.
 *                            If successful, contains a null-terminated string with the client id
 *                            that needs to be passed to the MQTT client.
 * @param[in] mqtt_client_id_size The size, in bytes of \p mqtt_client_id.
 * @param[out] out_mqtt_client_id_length __[nullable]__ Contains the string length, in bytes, of
 *                                                      of \p mqtt_client_id. Can be `NULL`.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_get_client_id(
    az_iot_hub_client const* client,
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
 */

/**
 * @brief Gets the Shared Access clear-text signature.
 * @details The application must obtain a valid clear-text signature using this API, sign it using
 *          HMAC-SHA256 using the Shared Access Key as password then Base64 encode the result.
 *
 * @remark More information available at
 * https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-security#security-tokens
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[in] token_expiration_epoch_time The time, in seconds, from 1/1/1970.
 * @param[in] signature An empty #az_span with sufficient capacity to hold the SAS signature.
 * @param[out] out_signature The output #az_span containing the SAS signature.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_sas_get_signature(
    az_iot_hub_client const* client,
    uint64_t token_expiration_epoch_time,
    az_span signature,
    az_span* out_signature);

/**
 * @brief Gets the MQTT password.
 * @remark The MQTT password must be an empty string if X509 Client certificates are used. Use this
 *       API only when authenticating with SAS tokens.
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[in] base64_hmac_sha256_signature The Base64 encoded value of the HMAC-SHA256(signature,
 *                                         SharedAccessKey). The signature is obtained by using
 *                                         #az_iot_hub_client_sas_get_signature.
 * @param[in] token_expiration_epoch_time The time, in seconds, from 1/1/1970.
 *                                        It MUST be the same value passed to
 *                                        az_iot_hub_client_sas_get_signature.
 * @param[in] key_name The Shared Access Key Name (Policy Name). This is optional. For security
 *                     reasons we recommend using one key per device instead of using a global
 *                     policy key.
 * @param[out] mqtt_password A char buffer with sufficient capacity to hold the MQTT password.
 * @param[in] mqtt_password_size The size, in bytes of \p mqtt_password.
 * @param[out] out_mqtt_password_length __[nullable]__ Contains the string length, in bytes, of
 *                                                     \p mqtt_password. Can be `NULL`.
 * @return #az_result.
 *         #AZ_OK if successful. In this case, `mqtt_password` will contain a null-terminated string
 *           with the password that needs to be passed to the MQTT client.
 *         #AZ_ERROR_INSUFFICIENT_SPAN_SIZE If `mqtt_password` does not have enough size.
 */
AZ_NODISCARD az_result az_iot_hub_client_sas_get_password(
    az_iot_hub_client const* client,
    az_span base64_hmac_sha256_signature,
    uint64_t token_expiration_epoch_time,
    az_span key_name,
    char* mqtt_password,
    size_t mqtt_password_size,
    size_t* out_mqtt_password_length);

/**
 *
 * Properties APIs
 *
 *   IoT Hub message properties are used for Device-to-Cloud (D2C) as well as Cloud-to-Device (C2D).
 *   Properties are always appended to the MQTT topic of the published or received message and
 *   must contain Uri-encoded keys and values.
 */

/**
 * @brief Telemetry or C2D properties.
 *
 */
typedef struct az_iot_hub_client_properties
{
  struct
  {
    az_span properties_buffer;
    int32_t properties_written;
    uint32_t current_property_index;
  } _internal;
} az_iot_hub_client_properties;

/**
 * @brief Initializes the Telemetry or C2D properties.
 *
 * @note The properties init API will not encode properties. In order to support
 *       the following characters, they must be percent-encoded (RFC3986) as follows:
 *          `/` : `%2F`
 *          `%` : `%25`
 *          `#` : `%23`
 *          `&` : `%26`
 *       Only these characters would have to be encoded. If you would like to avoid the need to
 *       encode the names/values, avoid using these characters in names and values.
 *
 * @param[in] properties The #az_iot_hub_client_properties to initialize
 * @param[in] buffer Can either be an empty #az_span or an #az_span containing properly formatted
 *                   (with above mentioned characters encoded if applicable) properties with the
 *                   following format: {key}={value}&{key}={value}.
 * @param[in] written_length The length of the properly formatted properties already initialized
 * within the buffer. If the \p buffer is empty (uninitialized), this should be 0.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_properties_init(
    az_iot_hub_client_properties* properties,
    az_span buffer,
    int32_t written_length);

/**
 * @brief Appends a key-value property to the list of properties.
 *
 * @note The properties append API will not encode properties. In order to support
 *       the following characters, they must be percent-encoded (RFC3986) as follows:
 *          `/` : `%2F`
 *          `%` : `%25`
 *          `#` : `%23`
 *          `&` : `%26`
 *       Only these characters would have to be encoded. If you would like to avoid the need to
 *       encode the names/values, avoid using these characters in names and values.
 *
 * @param[in] properties The #az_iot_hub_client_properties to use for this call
 * @param[in] name The name of the property.
 * @param[in] value The value of the property.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_properties_append(
    az_iot_hub_client_properties* properties,
    az_span name,
    az_span value);

/**
 * @brief Finds the value of a property.
 * @remark This will return the first value of the property with the given name if multiple
 * properties with the same key exist.
 *
 * @param[in] properties The #az_iot_hub_client_properties to use for this call
 * @param[in] name The name of the property.
 * @param[out] out_value An #az_span containing the value of the property.
 * @return #az_result.
 */
AZ_NODISCARD az_result az_iot_hub_client_properties_find(
    az_iot_hub_client_properties* properties,
    az_span name,
    az_span* out_value);

/**
 * @brief Iterates over the list of properties.
 *
 * @param[in] properties The #az_iot_hub_client_properties to use for this call
 * @param[out] out An #az_pair containing the key and the value of the next property.
 * @return #az_result
 */
AZ_NODISCARD az_result
az_iot_hub_client_properties_next(az_iot_hub_client_properties* properties, az_pair* out);

/**
 *
 * Telemetry APIs
 *
 */

/**
 * @brief Gets the MQTT topic that must be used for device to cloud telemetry messages.
 * @remark Telemetry MQTT Publish messages must have QoS At least once (1).
 * @remark This topic can also be used to set the MQTT Will message in the Connect message.
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[in] properties An optional #az_iot_hub_client_properties object (can be NULL).
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If
 *                        successful, contains a null-terminated string with the topic that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of
 *                                                  \p mqtt_topic. Can be `NULL`.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_telemetry_get_publish_topic(
    az_iot_hub_client const* client,
    az_iot_hub_client_properties const* properties,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 *
 * Cloud-to-device (C2D) APIs
 *
 */

/**
 * @brief The MQTT topic filter to subscribe to Cloud-to-Device requests.
 * @remark C2D MQTT Publish messages will have QoS At least once (1).
 */
#define AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC "devices/+/messages/devicebound/#"

/**
 * @brief The Cloud-To-Device Request.
 *
 */
typedef struct az_iot_hub_client_c2d_request
{
  az_iot_hub_client_properties properties; /**< The properties associated with this C2D request. */
} az_iot_hub_client_c2d_request;

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_request If the message is a C2D request, this will contain the
 *                         #az_iot_hub_client_c2d_request
 * @return #az_result
 *         - `AZ_ERROR_IOT_TOPIC_NO_MATCH` if the topic is not matching the expected format.
 */
AZ_NODISCARD az_result az_iot_hub_client_c2d_parse_received_topic(
    az_iot_hub_client const* client,
    az_span received_topic,
    az_iot_hub_client_c2d_request* out_request);

/**
 *
 * Methods APIs
 *
 */

/**
 * @brief The MQTT topic filter to subscribe to method requests.
 * @remark Methods MQTT Publish messages will have QoS At most once (0).
 */
#define AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC "$iothub/methods/POST/#"

/**
 * @brief A method request received from IoT Hub.
 *
 */
typedef struct az_iot_hub_client_method_request
{
  az_span request_id; /**< The request id.
                       * @note The application must match the method request and method response. */
  az_span name; /**< The method name. */
} az_iot_hub_client_method_request;

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_request If the message is a method request, this will contain the
 *                         #az_iot_hub_client_method_request.
 * @return #az_result
 *         - `AZ_ERROR_IOT_TOPIC_NO_MATCH` if the topic is not matching the expected format.
 */
AZ_NODISCARD az_result az_iot_hub_client_methods_parse_received_topic(
    az_iot_hub_client const* client,
    az_span received_topic,
    az_iot_hub_client_method_request* out_request);

/**
 * @brief Gets the MQTT topic that must be used to respond to method requests.
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[in] request_id The request id. Must match a received #az_iot_hub_client_method_request
 *                       request_id.
 * @param[in] status A code that indicates the result of the method, as defined by the user.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If
 *                        successful, contains a null-terminated string with the topic that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of
 *                                                  \p mqtt_topic. Can be `NULL`.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_methods_response_get_publish_topic(
    az_iot_hub_client const* client,
    az_span request_id,
    uint16_t status,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 *
 * Twin APIs
 *
 */

/**
 * @brief The MQTT topic filter to subscribe to twin operation responses.
 * @remark Twin MQTT Publish messages will have QoS At most once (0).
 */
#define AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC "$iothub/twin/res/#"

/**
 * @brief Gets the MQTT topic filter to subscribe to twin desired property changes.
 * @remark Twin MQTT Publish messages will have QoS At most once (0).
 */
#define AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC "$iothub/twin/PATCH/properties/desired/#"

/**
 * @brief Twin response type.
 *
 */
typedef enum
{
  AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET = 1,
  AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES = 2,
  AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES = 3,
} az_iot_hub_client_twin_response_type;

/**
 * @brief Twin response.
 *
 */
typedef struct az_iot_hub_client_twin_response
{
  az_iot_hub_client_twin_response_type response_type; /**< Twin response type. */
  az_iot_status status; /**< The operation status. */
  az_span
      request_id; /**< Request ID matches the ID specified when issuing a Get or Patch command. */
  az_span version; /**< The Twin object version.
                    * @remark This is only returned when
                    * response_type==AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES
                    * or
                    * response_type==AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES. */
} az_iot_hub_client_twin_response;

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_twin_response If the message is twin-operation related, this will contain the
 *                         #az_iot_hub_client_twin_response.
 * @return #az_result
 *         - `AZ_ERROR_IOT_TOPIC_NO_MATCH` if the topic is not matching the expected format.
 */
AZ_NODISCARD az_result az_iot_hub_client_twin_parse_received_topic(
    az_iot_hub_client const* client,
    az_span received_topic,
    az_iot_hub_client_twin_response* out_twin_response);

/**
 * @brief Gets the MQTT topic that must be used to submit a Twin GET request.
 * @remark The payload of the MQTT publish message should be empty.
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[in] request_id The request id.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If
 *                        successful, contains a null-terminated string with the topic that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of
 *                                                  \p mqtt_topic. Can be `NULL`.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_twin_document_get_publish_topic(
    az_iot_hub_client const* client,
    az_span request_id,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 * @brief Gets the MQTT topic that must be used to submit a Twin PATCH request.
 * @remark The payload of the MQTT publish message should contain a JSON document
 *         formatted according to the Twin specification.
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[in] request_id The request id.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If
 *                        successful, contains a null-terminated string with the topic that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of
 *                                                  \p mqtt_topic. Can be `NULL`.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_twin_patch_get_publish_topic(
    az_iot_hub_client const* client,
    az_span request_id,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

#include <azure/core/_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_CLIENT_H

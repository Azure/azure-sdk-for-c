// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_hub_client.h
 *
 * @brief definition for the Azure IoT Hub device SDK.
 */

#ifndef _az_IOT_HUB_CLIENT_H
#define _az_IOT_HUB_CLIENT_H

#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

/**
 * @brief Azure IoT Hub Client options.
 *
 */
typedef struct az_iot_hub_client_options
{
  az_span module_id; /**< The module name (if a module identity is used). */
  az_span user_agent; /**< The user-agent is a formatted string that will be used for Azure IoT
                         usage statistics. */
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
 * @param[out] client
 * @param[in] iot_hub_hostname The IoT Hub Hostname.
 * @param[in] device_id The Device ID.
 * @param[in] options A reference to an #az_iot_hub_client_options structure. Can be NULL.
 * @return #az_result.
 */
AZ_NODISCARD az_result az_iot_hub_client_init(
    az_iot_hub_client* client,
    az_span iot_hub_hostname,
    az_span device_id,
    az_iot_hub_client_options const* options);

/**
 * @brief Gets the MQTT user name.
 *
 * @param[in] client
 * @param[in] mqtt_user_name An empty #az_span with sufficient capacity to hold the MQTT user name.
 * @param[out] out_mqtt_user_name The output #az_span containing the MQTT user name.
 * @return #az_result.
 */
AZ_NODISCARD az_result az_iot_hub_client_user_name_get(
    az_iot_hub_client const* client,
    az_span mqtt_user_name,
    az_span* out_mqtt_user_name);

/**
 * @brief Gets the MQTT client id.
 *
 * @param[in] client
 * @param[in] mqtt_client_id An empty #az_span with sufficient capacity to hold the MQTT client id.
 * @param[out] out_mqtt_client_id The output #az_span containing the MQTT client id.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_id_get(
    az_iot_hub_client const* client,
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
 * @param[in] client
 * @param[in] token_expiration_epoch_time The time, in seconds, from 1/1/1970.
 * @param[in] signature An empty #az_span with sufficient capacity to hold the SAS signature.
 * @param[out] out_signature The output #az_span containing the SAS signature.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_sas_signature_get(
    az_iot_hub_client const* client,
    uint32_t token_expiration_epoch_time,
    az_span signature,
    az_span* out_signature);

/**
 * @brief Gets the MQTT password.
 * @note The MQTT password must be an empty string if X509 Client certificates are used. Use this
 *       API only when authenticating with SAS tokens.
 *
 * @param[in] client
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
    az_iot_hub_client const* client,
    az_span base64_hmac_sha256_signature,
    az_span key_name,
    az_span mqtt_password,
    az_span* out_mqtt_password);

/**
 *
 * Properties APIs
 *
 *   IoT Hub message properties are used for Device to Cloud (D2C) as well as Cloud to Device (C2D).
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
    az_span properties;
    uint8_t* current_property;
  } _internal;
} az_iot_hub_client_properties;

/**
 * @brief Initializes the Telemetry or C2D properties.
 *
 * @param[in] properties
 * @param[in] buffer Can either be an empty #az_span or an #az_span containing properly formatted
 *                   properties starting with the '?' character.
 * @return #az_result
 */
AZ_NODISCARD az_result
az_iot_hub_client_properties_init(az_iot_hub_client_properties* properties, az_span buffer);

/**
 * @brief Appends a key-value property to the list of properties.
 *
 * @param[in] properties
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
 * @note This will return the first value of the property with the given name if multiple properties
 *       with the same key exist.
 *
 * @param[in] properties
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
 * @param[in] properties
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
 * @note Telemetry MQTT Publish messages must have QoS At Least Once (1).
 *
 * @param[in] client
 * @param[in] properties An optional #az_iot_hub_client_properties object (can be NULL).
 * @param[in] mqtt_topic An empty #az_span with sufficient capacity to hold the MQTT topic.
 * @param[out] out_mqtt_topic The output #az_span containing the MQTT topic.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_telemetry_publish_topic_get(
    az_iot_hub_client const* client,
    az_iot_hub_client_properties const* properties,
    az_span mqtt_topic,
    az_span* out_mqtt_topic);

/**
 *
 * Cloud to device (C2D) APIs
 *
 */

/**
 * @brief Gets the MQTT topic filter to subscribe to cloud to device requests.
 * @note C2D MQTT Publish messages will have QoS At Least Once (1).
 *
 * @param[in] client
 * @param[in] mqtt_topic_filter An empty #az_span with sufficient capacity to hold the MQTT topic
 *                              filter.
 * @param[out] out_mqtt_topic_filter The output #az_span containing the MQTT topic filter.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_c2d_subscribe_topic_filter_get(
    az_iot_hub_client const* client,
    az_span mqtt_topic_filter,
    az_span* out_mqtt_topic_filter);

/**
 * @brief The Cloud To Device Request.
 *
 */
typedef struct az_iot_hub_client_c2d_request
{
  az_iot_hub_client_properties properties; /**< The properties associated with this C2D request. */
} az_iot_hub_client_c2d_request;

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_request If the message is a C2D request, this will contain the
 *                         #az_iot_hub_client_c2d_request
 * @return az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_c2d_received_topic_parse(
    az_iot_hub_client const* client,
    az_span received_topic,
    az_iot_hub_client_c2d_request* out_request);

/**
 *
 * Methods APIs
 *
 */

/**
 * @brief Gets the MQTT topic filter to subscribe to method requests.
 *
 * @param[in] client
 * @param[in] mqtt_topic_filter An empty #az_span with sufficient capacity to hold the MQTT topic
 *                              filter.
 * @param[out] out_mqtt_topic_filter The output #az_span containing the MQTT topic filter.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_methods_subscribe_topic_filter_get(
    az_iot_hub_client const* client,
    az_span mqtt_topic_filter,
    az_span* out_mqtt_topic_filter);

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
 * @param[in] client
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_request If the message is a method request, this will contain the
 *                         #az_iot_hub_client_method_request.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_methods_received_topic_parse(
    az_iot_hub_client const* client,
    az_span received_topic,
    az_iot_hub_client_method_request* out_request);

/**
 * @brief Gets the MQTT topic that must be used to respond to method requests.
 *
 * @param[in] client
 * @param[in] request_id The request id. Must match a received #az_iot_hub_client_method_request
 *                       request_id.
 * @param[in] status The status. (E.g. 200 for success.)
 * @param[in] mqtt_topic An empty #az_span with sufficient capacity to hold the MQTT topic.
 * @param[out] out_mqtt_topic The output #az_span containing the MQTT topic.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_methods_response_publish_topic_get(
    az_iot_hub_client const* client,
    az_span request_id,
    uint16_t status,
    az_span mqtt_topic,
    az_span* out_mqtt_topic);

/**
 *
 * Twin APIs
 *
 */

/**
 * @brief Azure IoT Hub status codes.
 *
 */
typedef enum
{
  // Service success codes
  AZ_IOT_HUB_CLIENT_STATUS_OK = 200,
  AZ_IOT_HUB_CLIENT_STATUS_ACCEPTED = 202,
  AZ_IOT_HUB_CLIENT_STATUS_NO_CONTENT = 204,

  // Service error codes
  AZ_IOT_HUB_CLIENT_STATUS_BAD_REQUEST = 400,
  AZ_IOT_HUB_CLIENT_STATUS_UNAUTHORIZED = 401,
  AZ_IOT_HUB_CLIENT_STATUS_FORBIDDEN = 403,
  AZ_IOT_HUB_CLIENT_STATUS_NOT_FOUND = 404,
  AZ_IOT_HUB_CLIENT_STATUS_NOT_ALLOWED = 405,
  AZ_IOT_HUB_CLIENT_STATUS_NOT_CONFLICT = 409,
  AZ_IOT_HUB_CLIENT_STATUS_PRECONDITION_FAILED = 412,
  AZ_IOT_HUB_CLIENT_STATUS_REQUEST_TOO_LARGE = 413,
  AZ_IOT_HUB_CLIENT_STATUS_UNSUPPORTED_TYPE = 415,
  AZ_IOT_HUB_CLIENT_STATUS_THROTTLED = 429,
  AZ_IOT_HUB_CLIENT_STATUS_CLIENT_CLOSED = 499,
  AZ_IOT_HUB_CLIENT_STATUS_SERVER_ERROR = 500,
  AZ_IOT_HUB_CLIENT_STATUS_BAD_GATEWAY = 502,
  AZ_IOT_HUB_CLIENT_STATUS_SERVICE_UNAVAILABLE = 503,
  AZ_IOT_HUB_CLIENT_STATUS_TIMEOUT = 504,
} az_iot_hub_client_status;

/**
 * @brief Checks if the status indicates a successful operation.
 *
 * @param[in] status The #az_iot_hub_client_status to verify.
 * @return True if the status indicates success. False otherwise.
 */
AZ_NODISCARD bool az_iot_hub_client_is_success_status(az_iot_hub_client_status status);

/**
 * @brief Checks if the status indicates a retriable error occurred during the
 *        operation.
 *
 * @param[in] status The #az_iot_hub_client_status to verify.
 * @return True if the operation should be retried. False otherwise.
 */
AZ_NODISCARD bool az_iot_hub_client_is_retriable_status(az_iot_hub_client_status status);

/**
 * @brief Gets the MQTT topic filter to subscribe to twin operation responses.
 *
 * @param[in] client
 * @param[in] mqtt_topic_filter An empty #az_span with sufficient capacity to hold the MQTT topic
 *                              filter.
 * @param[out] out_mqtt_topic_filter The output #az_span containing the MQTT topic filter.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_twin_response_subscribe_topic_filter_get(
    az_iot_hub_client const* client,
    az_span mqtt_topic_filter,
    az_span* out_mqtt_topic_filter);

/**
 * @brief Gets the MQTT topic filter to subscribe to twin desired property changes.
 * @note The payload will contain only changes made to the desired properties.
 *
 * @param[in] client
 * @param[in] mqtt_topic_filter An empty #az_span with sufficient capacity to hold the MQTT topic
 *                              filter.
 * @param[out] out_mqtt_topic_filter The output #az_span containing the MQTT topic filter.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_twin_patch_subscribe_topic_filter_get(
    az_iot_hub_client const* client,
    az_span mqtt_topic_filter,
    az_span* out_mqtt_topic_filter);

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
  az_iot_hub_client_status status; /**< The operation status. */
  az_span
      request_id; /**< Request ID matches the ID specified when issuing a Get or Patch command. */
  az_span version; /**< The Twin object version.
                    * @note This is only returned when
                    * response_type==AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES
                    * or
                    * response_type==AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES. */
} az_iot_hub_client_twin_response;

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_request If the message is twin-operation related, this will contain the
 *                         #az_iot_hub_client_twin_response.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_twin_received_topic_parse(
    az_iot_hub_client const* client,
    az_span received_topic,
    az_iot_hub_client_twin_response* out_twin_response);

/**
 * @brief Gets the MQTT topic that must be used to submit a Twin GET request.
 * @note The payload of the MQTT publish message should be empty.
 *
 * @param[in] client
 * @param[in] request_id The request id.
 * @param[in] mqtt_topic An empty #az_span with sufficient capacity to hold the MQTT topic.
 * @param[out] out_mqtt_topic The output #az_span containing the MQTT topic.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_twin_get_publish_topic_get(
    az_iot_hub_client const* client,
    az_span request_id,
    az_span mqtt_topic,
    az_span* out_mqtt_topic);

/**
 * @brief Gets the MQTT topic that must be used to submit a Twin PATCH request.
 * @note The payload of the MQTT publish message should contain a JSON document
 *       formatted according to the Twin specification.
 *
 * @param[in] client
 * @param[in] request_id The request id.
 * @param[in] if_match_version Can be either "*" to overwrite or the twin version to limit the
 *                             reported properties patch to that version only. The version is
 *                             available from the JSON document or from
 *                             #az_iot_hub_client_twin_response::version.
 * @param[in] mqtt_topic An empty #az_span with sufficient capacity to hold the MQTT topic.
 * @param[out] out_mqtt_topic The output #az_span containing the MQTT topic.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_hub_client_twin_patch_publish_topic_get(
    az_iot_hub_client const* client,
    az_span request_id,
    az_span if_match_version,
    az_span mqtt_topic,
    az_span* out_mqtt_topic);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_CLIENT_H

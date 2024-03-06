// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_rpc_client_codec.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_RPC_CLIENT_CODEC_H
#define _az_MQTT5_RPC_CLIENT_CODEC_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief MQTT5 RPC Client Codec options.
 *
 */
typedef struct
{
  /**
   * @brief The topic format to use for the subscription topic.
   *
   *
   * @note Can include {commandName} for command name, {modelId} for model id, {executorId} for the
   * server's client_id, and/or {invokerClientId} for the client's client_id. The default value is
   * "clients/{invokerClientId}/services/{modelId}/{executorId}/command/{commandName}/response".
   *
   * @note Tokens must be surrounded by slashes, unless they are at the beginning or end of the
   * topic.
   */
  az_span subscription_topic_format;

  /**
   * @brief The topic format to use for the request topic.
   *
   * @note Can include {commandName} for command name, {modelId} for model id, and/or {executorId}
   * for the server's client_id. The default value is
   * "services/{modelId}/{executorId}/command/{commandName}/request".
   *
   * @note Tokens must be surrounded by slashes, unless they are at the beginning or end of the
   * topic.
   */
  az_span request_topic_format;

} az_mqtt5_rpc_client_codec_options;

/**
 * @brief The MQTT5 RPC Client Codec.
 *
 */
typedef struct
{
  struct
  {
    /**
     * @brief Client ID to use for the response topic.
     */
    az_span client_id;
    /**
     * @brief Model ID to use for the topics.
     */
    az_span model_id;
    /**
     * @brief Options for the MQTT5 RPC Client Codec.
     */
    az_mqtt5_rpc_client_codec_options options;
  } _internal;
} az_mqtt5_rpc_client_codec;

/**
 * @brief Represents the parsed topic obtained from the server as a response to a RPC request.
 */
typedef struct
{
  /**
   * @brief The server id that executed the command.
   */
  az_span executor_id;
  /**
   * @brief The command that was executed.
   */
  az_span command_name;
} az_mqtt5_rpc_client_codec_request_response;

/**
 * @brief Returns the default options for the MQTT5 RPC Client Codec.
 *
 * @return An #az_mqtt5_rpc_client_codec_options object with default values.
 */
AZ_NODISCARD az_mqtt5_rpc_client_codec_options az_mqtt5_rpc_client_codec_options_default();

/**
 * @brief Generates the publish topic for this RPC Client Codec.
 *
 * @param[in] client The #az_mqtt5_rpc_client_codec to use.
 * @param[in] executor_id The client id of the server to send the request to.
 * @param[in] command_name The command name to use for the request.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic filter. If
 * successful, contains a null-terminated string with the topic filter that needs to be passed to
 * the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of \p
 * mqtt_topic. Can be `NULL`.
 * @pre \p client must not be `NULL`.
 * @pre \p mqtt_topic must not be `NULL`.
 * @pre \p mqtt_topic_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic was created successfully.
 * @retval #AZ_ERROR_NOT_ENOUGH_SPACE The buffer is too small. \p out_mqtt_topic_length will contain
 * the required size.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_client_codec_get_publish_topic(
    az_mqtt5_rpc_client_codec* client,
    az_span executor_id,
    az_span command_name,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 * @brief Generates the response topic property for this RPC Client Codec.
 *
 * @param[in] client The #az_mqtt5_rpc_client_codec to use.
 * @param[in] executor_id The client id of the server to send the request to.
 * @param[in] command_name The command name to use for the request.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic response
 * property. If successful, contains a null-terminated string with the topic response property that
 * needs to be added as a property in the request.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of \p
 * mqtt_topic. Can be `NULL`.
 * @pre \p client must not be `NULL`.
 * @pre \p mqtt_topic must not be `NULL`.
 * @pre \p mqtt_topic_size must be greater than 0.
 * @pre \p mqtt_topic must be large enough to hold the topic.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic was created successfully.
 * @retval #AZ_ERROR_NOT_ENOUGH_SPACE The buffer is too small. \p out_mqtt_topic_length will contain
 * the required size.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_client_codec_get_response_property_topic(
    az_mqtt5_rpc_client_codec* client,
    az_span executor_id,
    az_span command_name,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 * @brief Generates the subscribe topic for this RPC Client Codec in which the client will receive
 * the response.
 *
 * @param[in] client The #az_mqtt5_rpc_client_codec to use.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic filter. If
 * successful, contains a null-terminated string with the topic filter that needs to be passed to
 * the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of \p
 * mqtt_topic. Can be `NULL`.
 * @pre \p client must not be `NULL`.
 * @pre \p mqtt_topic must not be `NULL`.
 * @pre \p mqtt_topic_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic was created successfully.
 * @retval #AZ_ERROR_NOT_ENOUGH_SPACE The buffer is too small. \p out_mqtt_topic_length will
 * contain the required size.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_client_codec_get_subscribe_topic(
    az_mqtt5_rpc_client_codec* client,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client The #az_mqtt5_rpc_client_codec to use.
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_response If the message is meant for this client, contains the executor id and
 * command name of the message.
 * @pre \p client must not be `NULL`.
 * @pre \p received_topic must be a valid #az_span of size greater than or equal to 1.
 * @pre \p out_response must not be `NULL`.
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic was parsed successfully.
 * @retval #AZ_ERROR_IOT_TOPIC_NO_MATCH If the topic is not matching the expected format or is not
 * meant for this client.
 *
 */
AZ_NODISCARD az_result az_mqtt5_rpc_client_codec_parse_received_topic(
    az_mqtt5_rpc_client_codec* client,
    az_span received_topic,
    az_mqtt5_rpc_client_codec_request_response* out_response);

/**
 * @brief Initializes an MQTT5 RPC Client Codec.
 *
 * @param[out] client The #az_mqtt5_rpc_client_codec to initialize.
 * @param[in] client_id The client id to use for the response topic.
 * @param[in] model_id The model id to use for the topics.
 * @param[in] options #az_mqtt5_rpc_client_codec_options to use for the RPC Client or NULL to use
 * the defaults.
 * @pre \p client must not be `NULL`.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The client was initialized successfully.
 * @retval #AZ_ERROR_ARG An invalid argument was provided.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_client_codec_init(
    az_mqtt5_rpc_client_codec* client,
    az_span client_id,
    az_span model_id,
    az_mqtt5_rpc_client_codec_options* options);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_RPC_CLIENT_CODEC_H

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_rpc_server_codec.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_RPC_SERVER_CODEC_H
#define _az_MQTT5_RPC_SERVER_CODEC_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief MQTT5 RPC Server Codec options.
 *
 */
typedef struct
{
  /**
   * @brief The service group id to use for the subscription topic, specifying this allows the
   * server to subscribe to a shared subscription topic. It can be AZ_SPAN_EMPTY if not using a
   * shared subscription topic.
   */
  az_span service_group_id;

  /**
   * @brief The topic format to use for the subscription topic.
   *
   * @note Can include {name} for command name if server can accept multiple commands, {serviceId}
   * for model id, and/or {executorId} for the server's client_id, and {cmdPhase} for
   * differentiating between request and response topics. The default value is
   * services/{serviceId}/{executorId}/command/{name}/{cmdPhase}.
   *
   * @note Tokens must be surrounded by slashes, unless they are at the beginning or end of the
   * topic.
   *
   */
  az_span topic_format;

} az_mqtt5_rpc_server_codec_options;

/**
 * @brief MQTT5 RPC Server Codec.
 *
 */
typedef struct
{
  struct
  {
    az_span model_id;
    az_span client_id;

    az_mqtt5_rpc_server_codec_options options;
  } _internal;
} az_mqtt5_rpc_server_codec;

/**
 * @brief Represents the parsed response topic from a RPC client request.
 */
typedef struct
{
  /**
   * @brief The model id of the service.
   */
  az_span service_id;

  /**
   * @brief The client id of the specified server that should execute the command.
   */
  az_span executor_id;

  /**
   * @brief The name of the command to execute.
   */
  az_span command_name;
} az_mqtt5_rpc_server_codec_request;

/**
 * @brief Initializes a RPC server codec options object with default values.
 *
 * @return An #az_mqtt5_rpc_server_codec_options object with default values.
 */
AZ_NODISCARD az_mqtt5_rpc_server_codec_options az_mqtt5_rpc_server_codec_options_default();

/**
 * @brief Generates the subscription topic for the RPC Server Codec.
 *
 * @param[in] server The #az_mqtt5_rpc_server_codec to use.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic filter. If
 * successful, contains a null-terminated string with the topic filter that needs to be passed to
 * the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of \p
 * mqtt_topic. Can be `NULL`.
 * @pre \p server must not be `NULL`.
 * @pre \p mqtt_topic must not be `NULL`.
 * @pre \p mqtt_topic_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic was created successfully.
 * @retval #AZ_ERROR_NOT_ENOUGH_SPACE The buffer is too small.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_server_codec_get_subscribe_topic(
    az_mqtt5_rpc_server_codec* server,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] server The #az_mqtt5_rpc_server_codec to use.
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_request If the message is meant for this server, contains the model id,
 * client id of the server, and command name of the request.
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic was parsed successfully.
 * @retval #AZ_ERROR_IOT_TOPIC_NO_MATCH If the topic is not matching the expected format or is not
 * meant for this server.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_server_codec_parse_received_topic(
    az_mqtt5_rpc_server_codec* server,
    az_span received_topic,
    az_mqtt5_rpc_server_codec_request* out_request);

/**
 * @brief Initializes an MQTT5 RPC Server Codec.
 *
 * @param[out] server The #az_mqtt5_rpc_server_codec to initialize.
 * @param[in] model_id The model id to use for the subscription topic. May be AZ_SPAN_EMPTY if not
 * used in the subscription topic.
 * @param[in] client_id The client id to use for the subscription topic. May be AZ_SPAN_EMPTY if not
 * used in the subscription topic.
 * @param[in] options Any #az_mqtt5_rpc_server_codec_options to use for the RPC Server or NULL to
 * use the defaults.
 * @pre \p server must not be `NULL`.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The client was initialized successfully.
 * @retval #AZ_ERROR_ARG An invalid argument was provided.
 */
AZ_NODISCARD az_result az_mqtt5_rpc_server_codec_init(
    az_mqtt5_rpc_server_codec* server,
    az_span model_id,
    az_span client_id,
    az_mqtt5_rpc_server_codec_options* options);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_RPC_SERVER_CODEC_H

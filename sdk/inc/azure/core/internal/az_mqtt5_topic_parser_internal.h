// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines the internal functions used by the MQTT5 topic parser.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_TOPIC_PARSER_INTERNAL_H
#define _az_MQTT5_TOPIC_PARSER_INTERNAL_H

#include <azure/core/az_span.h>
#include <stdio.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Key appended to a topic to indicate a shared subscription.
 */
#define _az_MQTT5_TOPIC_PARSER_SERVICE_GROUP_ID_KEY "$share/"
/**
 * @brief Key used to replace the executor id in a topic format with any executor id.
 */
#define _az_MQTT5_TOPIC_PARSER_ANY_EXECUTOR_ID "_any_"

/**
 * @brief Key used to indicate the invoker client id in a topic format.
 */
#define _az_MQTT5_TOPIC_PARSER_CLIENT_ID_KEY "{invokerClientId}"
/**
 * @brief Hash of the client id key. Exact string used to calculate the hash is "invokerClientId".
 *
 * @details The value below is a hash of a key used in a topic format, it has been calculated
 * using the #_az_mqtt5_rpc_calculate_hash function. To re-calculate it call the function with a
 * #az_span containing the key (ex. "serviceId"). If the key changes, update the hash here.
 */
#define _az_MQTT5_TOPIC_PARSER_CLIENT_ID_HASH 3426466449

/**
 * @brief Key used to indicate the service id in a topic format.
 */
#define _az_MQTT5_TOPIC_PARSER_SERVICE_ID_KEY "{serviceId}"
/**
 * @brief Hash of the service id key. Exact string used to calculate the hash is "serviceId".
 *
 * @details The value below is a hash of a key used in a topic format, it has been calculated
 * using the #_az_mqtt5_rpc_calculate_hash function. To re-calculate it call the function with a
 * #az_span containing the key (ex. "serviceId"). If the key changes, update the hash here.
 */
#define _az_MQTT5_TOPIC_PARSER_SERVICE_ID_HASH 4175641829

/**
 * @brief Key used to indicate the executor id in a topic format.
 */
#define _az_MQTT5_RPC_EXECUTOR_ID_KEY "{executorId}"
/**
 * @brief Hash of the executor id key. Exact string used to calculate the hash is "executorId".
 *
 * @details The value below is a hash of a key used in a topic format, it has been calculated
 * using the #_az_mqtt5_rpc_calculate_hash function. To re-calculate it call the function with a
 * #az_span containing the key (ex. "serviceId"). If the key changes, update the hash here.
 */
#define _az_MQTT5_TOPIC_PARSER_EXECUTOR_ID_HASH 3913329219

/**
 * @brief Key used to indicate the command id in a topic format.
 */
#define _az_MQTT5_TOPIC_PARSER_COMMAND_ID_KEY "{name}"
/**
 * @brief Hash of the command id key. Exact string used to calculate the hash is "name".
 *
 * @details The value below is a hash of a key used in a topic format, it has been calculated
 * using the #_az_mqtt5_rpc_calculate_hash function. To re-calculate it call the function with a
 * #az_span containing the key (ex. "serviceId"). If the key changes, update the hash here.
 */
#define _az_MQTT5_TOPIC_PARSER_COMMAND_ID_HASH 2624200456

/**
 * @brief Function to calculate the hash of a token.
 *
 * @param token[in] The token to calculate the hash of.
 *
 * @return The hash of the token.
 */
AZ_INLINE uint32_t _az_mqtt5_topic_parser_calculate_hash(az_span token)
{
  uint32_t hash = 5831;
  for (int32_t i = 0; i < az_span_size(token); i++)
  {
    hash = ((hash << 5) + hash) + az_span_ptr(token)[i];
  }
  return hash;
}

/**
 * @brief Helper function to replace tokens in a topic format.
 *
 * @param[out] mqtt_topic_span Buffer to write the result to.
 * @param[in] topic_format The topic format to replace tokens in.
 * @param[in] service_group_id #az_span containing the service group id or #AZ_SPAN_EMPTY.
 * @param[in] client_id #az_span containing the client id or #AZ_SPAN_EMPTY.
 * @param[in] service_id #az_span containing the service id.
 * @param[in] executor_id #az_span containing the executor id or #AZ_SPAN_EMPTY.
 * @param[in] command_id #az_span containing the command id.
 * @param[out] required_length The required length of the buffer to write the result to.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result _az_mqtt5_topic_parser_replace_tokens_in_format(
    az_span mqtt_topic_span,
    az_span topic_format,
    az_span service_group_id,
    az_span client_id,
    az_span service_id,
    az_span executor_id,
    az_span command_id,
    uint32_t* required_length);

/**
 * @brief Helper function to extract tokens from a topic.
 *
 * @param[in] topic_format The topic format to reference when extracting tokens.
 * @param[in] received_topic The topic to extract tokens from.
 * @param[in] client_id #az_span containing the client id or #AZ_SPAN_EMPTY.
 * @param[in] service_id #az_span containing the service id or #AZ_SPAN_EMPTY.
 * @param[in] executor_id #az_span containing the executor id or #AZ_SPAN_EMPTY.
 * @param[out] extracted_client_id Pointer to an #az_span to write the extracted client id to or
 * NULL if not needed.
 * @param[out] extracted_service_id Pointer to an #az_span to write the extracted service id to or
 * NULL if not needed.
 * @param[out] extracted_executor_id Pointer to an #az_span to write the extracted executor id to or
 * NULL if not needed.
 * @param[out] extracted_command_name Pointer to an #az_span to write the extracted command name to
 * or NULL if not needed.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result _az_mqtt5_topic_parser_extract_tokens_from_topic(
    az_span topic_format,
    az_span received_topic,
    az_span client_id,
    az_span service_id,
    az_span executor_id,
    az_span* extracted_client_id,
    az_span* extracted_service_id,
    az_span* extracted_executor_id,
    az_span* extracted_command_name);

/**
 * @brief Helper function to check if a topic format is valid.
 *
 * @param[in] topic_format An #az_span containing the topic format to check.
 * @return true if the topic format is valid, false otherwise.
 */
AZ_NODISCARD bool _az_mqtt5_topic_parser_valid_topic_format(az_span topic_format);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_TOPIC_PARSER_INTERNAL_H

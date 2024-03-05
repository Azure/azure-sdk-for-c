// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_telemetry_producer_codec.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_TELEMETRY_PRODUCER_CODEC_H
#define _az_MQTT5_TELEMETRY_PRODUCER_CODEC_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief MQTT5 Telemetry Producer Codec options.
 *
 */
typedef struct
{
  /**
   * @brief The topic format to use for the publish topic to send telemetry.
   *
   * @note Can include {name} for telemetry name, {modelId} for model id, and/or {senderId} for
   * the sender's client_id. The default value is services/{modelId}/{senderId}/telemetry.
   *
   * @note Tokens must be surrounded by slashes, unless they are at the beginning or end of the
   * topic.
   *
   */
  az_span telemetry_topic_format;
} az_mqtt5_telemetry_producer_codec_options;

/**
 * @brief MQTT5 Telemetry Producer Codec.
 *
 */
typedef struct
{
  struct
  {
    az_span model_id;
    az_span client_id;

    az_mqtt5_telemetry_producer_codec_options options;
  } _internal;
} az_mqtt5_telemetry_producer_codec;

/**
 * @brief Initializes a telemetry producer codec options with default values.
 *
 * @return An #az_mqtt5_telemetry_producer_codec_options object with default values.
 */
AZ_NODISCARD az_mqtt5_telemetry_producer_codec_options
az_mqtt5_telemetry_producer_codec_options_default();

/**
 * @brief Generates the publish topic to publish telemetry for the telemetry producer.
 *
 * @param[in] producer The #az_mqtt5_telemetry_producer_codec to use.
 * @param[in] telemetry_name An #az_span containing the name of the telemetry.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic filter. If
 * successful, contains a null-terminated string with the topic filter that needs to be passed to
 * the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of \p
 * mqtt_topic. Can be `NULL`.
 * @pre \p producer must not be `NULL`.
 * @pre \p mqtt_topic must not be `NULL`.
 * @pre \p mqtt_topic_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic was created successfully.
 * @retval #AZ_ERROR_NOT_ENOUGH_SPACE The buffer is too small.
 */
AZ_NODISCARD az_result az_mqtt5_telemetry_producer_codec_get_publish_topic(
    az_mqtt5_telemetry_producer_codec* producer,
    az_span telemetry_name,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 * @brief Initializes an MQTT5 Telemetry Producer Codec.
 *
 * @param[out] producer The #az_mqtt5_telemetry_producer_codec to initialize.
 * @param[in] model_id The model id to use for the publish topic. May be AZ_SPAN_EMPTY if not
 * used in the publish topic.
 * @param[in] sender_id The sender id to use for the publish topic. May be AZ_SPAN_EMPTY if not
 * token is not used in the subscription topic format.
 * @param[in] options Any #az_mqtt5_telemetry_producer_codec_options to use for the telemetry
 * producer or NULL to use the defaults.
 * @pre \p producer must not be `NULL`.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The telemetry producer was initialized successfully.
 * @retval #AZ_ERROR_ARG An invalid argument was provided.
 */
AZ_NODISCARD az_result az_mqtt5_telemetry_producer_codec_init(
    az_mqtt5_telemetry_producer_codec* producer,
    az_span model_id,
    az_span sender_id,
    az_mqtt5_telemetry_producer_codec_options* options);

#endif // _az_MQTT5_TELEMETRY_PRODUCER_CODEC_H

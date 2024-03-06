// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_telemetry_consumer_codec.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_TELEMETRY_CONSUMER_CODEC_H
#define _az_MQTT5_TELEMETRY_CONSUMER_CODEC_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief MQTT5 Telemetry Consumer Codec options.
 *
 */
typedef struct
{
  /**
   * @brief The service group id to use for the subscription topic, specifying this allows the
   * telemetry consumer to subscribe to a shared subscription topic. It can be AZ_SPAN_EMPTY if not
   * using a shared subscription topic.
   */
  az_span service_group_id;

  /**
   * @brief The topic format to use for the subscription topic to receive telemetry.
   *
   * @note Can include {telemetryName} for telemetry name, {modelId} for model id, and/or {senderId}
   * for the sender's client_id. The default value is services/{modelId}/{senderId}/telemetry.
   *
   * @note Tokens must be surrounded by slashes, unless they are at the beginning or end of the
   * topic.
   *
   */
  az_span telemetry_topic_format;
} az_mqtt5_telemetry_consumer_codec_options;

/**
 * @brief MQTT5 Telemetry Consumer Codec.
 *
 */
typedef struct
{
  struct
  {
    az_span model_id;
    az_span sender_id;

    az_mqtt5_telemetry_consumer_codec_options options;
  } _internal;
} az_mqtt5_telemetry_consumer_codec;

/**
 * @brief Represents the parsed data from telemetry.
 */
typedef struct
{
  /**
   * @brief The identifier of the service model.
   */
  az_span model_id;

  /**
   * @brief The id of the sender of the telemetry.
   *
   */
  az_span sender_id;

  /**
   * @brief The name of the telemetry.
   */
  az_span telemetry_name;
} az_mqtt5_telemetry_consumer_codec_data;

/**
 * @brief Initializes a telemetry consumer codec options with default values.
 *
 * @return An #az_mqtt5_telemetry_consumer_codec_options object with default values.
 */
AZ_NODISCARD az_mqtt5_telemetry_consumer_codec_options
az_mqtt5_telemetry_consumer_codec_options_default();

/**
 * @brief Generates the subscription topic to receive telemetry for the telemetry consumer.
 *
 * @param[in] consumer The #az_mqtt5_telemetry_consumer_codec to use.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic filter. If
 * successful, contains a null-terminated string with the topic filter that needs to be passed to
 * the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of \p
 * mqtt_topic. Can be `NULL`.
 * @pre \p consumer must not be `NULL`.
 * @pre \p mqtt_topic must not be `NULL`.
 * @pre \p mqtt_topic_size must be greater than 0.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic was created successfully.
 * @retval #AZ_ERROR_NOT_ENOUGH_SPACE The buffer is too small. \p out_mqtt_topic_length will contain
 * the required size.
 */
AZ_NODISCARD az_result az_mqtt5_telemetry_consumer_codec_get_subscribe_topic(
    az_mqtt5_telemetry_consumer_codec* consumer,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 * @brief Attempts to parse the topic of a received telemetry.
 *
 * @param[in] consumer The #az_mqtt5_telemetry_consumer_codec to use.
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_data If the message is meant for this consumer, contains the model id,
 * sender id, and name of the telemetry.
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The topic was parsed successfully.
 * @retval #AZ_ERROR_IOT_TOPIC_NO_MATCH If the topic is not matching the expected format or is not
 * meant for this consumer.
 */
AZ_NODISCARD az_result az_mqtt5_telemetry_consumer_codec_parse_received_topic(
    az_mqtt5_telemetry_consumer_codec* consumer,
    az_span received_topic,
    az_mqtt5_telemetry_consumer_codec_data* out_data);

/**
 * @brief Initializes an MQTT5 Telemetry Consumer Codec.
 *
 * @param[out] consumer The #az_mqtt5_telemetry_consumer_codec to initialize.
 * @param[in] model_id The model id to use for the subscription topic. May be AZ_SPAN_EMPTY if not
 * used in the subscription topic.
 * @param[in] sender_id The sender id to use for the subscription topic. May be AZ_SPAN_EMPTY if not
 * used in the subscription topic, or if the sender id is not needed.
 * @param[in] options Any #az_mqtt5_telemetry_consumer_codec_options to use for the telemetry
 * consumer or NULL to use the defaults.
 * @pre \p consumer must not be `NULL`.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The telemetry consumer was initialized successfully.
 * @retval #AZ_ERROR_ARG An invalid argument was provided.
 */
AZ_NODISCARD az_result az_mqtt5_telemetry_consumer_codec_init(
    az_mqtt5_telemetry_consumer_codec* consumer,
    az_span model_id,
    az_span sender_id,
    az_mqtt5_telemetry_consumer_codec_options* options);

#endif // _az_MQTT5_TELEMETRY_CONSUMER_CODEC_H

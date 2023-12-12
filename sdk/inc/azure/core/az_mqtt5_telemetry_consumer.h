// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_telemetry_consumer. You use the telemetry consumer to receive
 * telemetry messages.
 *
 * @note The state diagram is in sdk/docs/core/resources/telemetry_consumer.puml
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_TELEMETRY_CONSUMER_H
#define _az_MQTT5_TELEMETRY_CONSUMER_H

#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_mqtt5_telemetry_consumer_codec.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Event types for the MQTT5 Telemetry Consumer.
 *
 */
enum az_event_type_mqtt5_telemetry_consumer
{
  /**
   * @brief Event representing the Telemetry Consumer receiving a telemetry message and
   * sending it to the application.
   *
   */
  AZ_MQTT5_EVENT_TELEMETRY_CONSUMER_IND = _az_MAKE_EVENT(_az_FACILITY_TELEMETRY_CONSUMER, 1),
};

/**
 * @brief The MQTT5 Telemetry Consumer.
 *
 */
typedef struct az_mqtt5_telemetry_consumer
{
  struct
  {
    /**
     * @brief Telemetry Consumer HFSM for the MQTT5 Telemetry Consumer.
     *
     */
    _az_hfsm telemetry_consumer_hfsm;

    /**
     * @brief The subclient used by the MQTT5 Telemetry Consumer.
     */
    _az_event_client subclient;

    /**
     * @brief The MQTT5 connection linked to the MQTT5 Telemetry Consumer.
     */
    az_mqtt5_connection* connection;

    /**
     * @brief Timeout in seconds for subscribing acknowledgement.
     */
    int32_t subscribe_timeout_in_seconds;

    /**
     * @brief the message id of the pending subscribe for the telemetry topic.
     */
    int32_t pending_subscription_id;

    /**
     * @brief timer used for the subscribe of the telemetry topic.
     */
    _az_event_pipeline_timer telemetry_consumer_timer;

    /**
     * @brief The subscription topic to use for the Telemetry Consumer.
     */
    az_span subscription_topic;

    /**
     * @brief #az_mqtt5_telemetry_consumer_codec associated with this telemetry consumer.
     */
    az_mqtt5_telemetry_consumer_codec* telemetry_consumer_codec;
  } _internal;
} az_mqtt5_telemetry_consumer;

// Event data types

/**
 * @brief Event data for #AZ_MQTT5_EVENT_TELEMETRY_CONSUMER_IND.
 */
typedef struct az_mqtt5_telemetry_consumer_ind_event_data
{
  /**
   * @brief The telemetry topic to make sure the right Telemetry Consumer processes the telemetry.
   */
  az_span telemetry_topic;
  /**
   * @brief The payload of the telemetry.
   */
  az_span telemetry_payload;
  /**
   * @brief The content type of the telemetry.
   */
  az_span content_type;
} az_mqtt5_telemetry_consumer_ind_event_data;

/**
 * @brief Triggers an #AZ_MQTT5_EVENT_SUB_REQ event from the application.
 *
 * @note This should be called from the application to subscribe to the telemetry topic.
 *
 * @param[in] client The #az_mqtt5_telemetry_consumer to use.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The event was triggered successfully or the client is already subscribing.
 * @retval #AZ_ERROR_NOT_SUPPORTED if the client is not connected.
 * @retval Other on other failures creating/sending the subscribe message.
 */
AZ_NODISCARD az_result
az_mqtt5_telemetry_consumer_subscribe_begin(az_mqtt5_telemetry_consumer* client);

/**
 * @brief Triggers an #AZ_MQTT5_EVENT_UNSUB_REQ event from the application.
 *
 * @note This should be called from the application to unsubscribe from the telemetry topic. This
 * may be used if the application doesn't want to receive telemetry anymore.
 *
 * @param[in] client The #az_mqtt5_telemetry_consumer to use.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The event was triggered successfully.
 * @retval #AZ_ERROR_NOT_SUPPORTED if the client is not connected.
 * @retval Other on other failures creating/sending the unsubscribe message.
 */
AZ_NODISCARD az_result
az_mqtt5_telemetry_consumer_unsubscribe_begin(az_mqtt5_telemetry_consumer* client);

/**
 * @brief Initializes an MQTT5 Telemetry Consumer.
 *
 * @param[out] client The #az_mqtt5_telemetry_consumer to initialize.
 * @param[in] telemetry_consumer_codec The #az_mqtt5_telemetry_consumer_codec to initialize and use
 * within the Telemetry Consumer.
 * @param[in] connection The #az_mqtt5_connection to use for the Telemetry Consumer.
 * @param[in] subscription_topic The application allocated #az_span to use for the subscription
 * topic.
 * @param[in] model_id The model id to use for the subscription topic. May be AZ_SPAN_EMPTY if not
 * used in the subscription topic.
 * @param[in] sender_id The sender client id to use for the subscription topic. May be AZ_SPAN_EMPTY
 * if not used in the subscription topic.
 * @param[in] subscribe_timeout_in_seconds Timeout in seconds for subscribing acknowledgement (must
 * be > 0).
 * @param[in] options Any #az_mqtt5_telemetry_consumer_codec_options to use for the Telemetry
 * Consumer.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_telemetry_consumer_init(
    az_mqtt5_telemetry_consumer* client,
    az_mqtt5_telemetry_consumer_codec* telemetry_consumer_codec,
    az_mqtt5_connection* connection,
    az_span subscription_topic,
    az_span model_id,
    az_span sender_id,
    int32_t subscribe_timeout_in_seconds,
    az_mqtt5_telemetry_consumer_codec_options* options);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_TELEMETRY_CONSUMER_H

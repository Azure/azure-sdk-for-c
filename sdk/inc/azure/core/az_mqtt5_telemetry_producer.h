// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_telemetry_producer. You use the Telemetry Producer to send
 * telemetry.
 *
 * @note The state diagram is in sdk/docs/core/resources/telemetry_producer.puml
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_TELEMETRY_PRODUCER_H
#define _az_MQTT5_TELEMETRY_PRODUCER_H

#include <stdio.h>

#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_mqtt5_telemetry_producer_codec.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Event types for the MQTT5 Telemetry Producer.
 *
 */
enum az_mqtt5_event_type_telemetry_producer
{
  /**
   * @brief Event representing the application requesting to send a telemetry message.
   */
  AZ_MQTT5_EVENT_TELEMETRY_PRODUCER_SEND_REQ = _az_MAKE_EVENT(_az_FACILITY_TELEMETRY_PRODUCER, 1),
  /**
   * @brief Event representing the Telemetry Producer getting an error from the broker when sending
   * the telemetry message. It is then sent to the application. This event indicates the failure is
   * from the producer/broker.
   *
   */
  AZ_MQTT5_EVENT_TELEMETRY_PRODUCER_ERROR_RSP = _az_MAKE_EVENT(_az_FACILITY_TELEMETRY_PRODUCER, 2),
};

/**
 * @brief The type represents the various #az_result error conditions specific to the
 * Telemetry Producer.
 */
enum az_result_telemetry_producer
{
  // === Telemetry Producer error codes ===
  /**
   * @brief Another publish is already in progress and the puback hasn't been received yet.
   */
  AZ_ERROR_TELEMETRY_PRODUCER_PUB_IN_PROGRESS
  = _az_RESULT_MAKE_ERROR(_az_FACILITY_TELEMETRY_PRODUCER, 1),
};

/**
 * @brief The MQTT5 Telemetry Producer.
 *
 */
typedef struct az_mqtt5_telemetry_producer
{
  struct
  {
    /**
     * @brief Telemetry Producer hfsm for the MQTT5 Telemetry Producer.
     *
     */
    _az_hfsm telemetry_producer_hfsm;

    /**
     * @brief The subclient used by the MQTT5 Telemetry Producer.
     */
    _az_event_client subclient;

    /**
     * @brief The MQTT5 connection linked to the MQTT5 Telemetry Producer.
     */
    az_mqtt5_connection* connection;

    /**
     * @brief Timeout, in seconds, for publishing (must be > 0).
     */
    int32_t publish_timeout_in_seconds;

    /**
     * @brief The MQTT message id of the pending publish.
     */
    int32_t pending_pub_id;

    /**
     * @brief The application allocated #az_span to use for the telemetry topic.
     */
    az_span telemetry_topic_buffer;

    /**
     * @brief timer used for the publishes.
     */
    _az_event_pipeline_timer telemetry_producer_timer;

    /**
     * @brief #az_mqtt5_telemetry_producer_codec associated with this client.
     */
    az_mqtt5_telemetry_producer_codec* telemetry_producer_codec;

    /**
     * @brief The property bag used by the Telemetry Producer for sending telemetry messages.
     */
    az_mqtt5_property_bag property_bag;

  } _internal;
} az_mqtt5_telemetry_producer;

// Event data types

/**
 * @brief Event data for #AZ_MQTT5_EVENT_TELEMETRY_PRODUCER_SEND_REQ.
 */
typedef struct az_mqtt5_telemetry_producer_send_req_event_data
{
  /**
   * @brief The content type of the telemetry.
   */
  az_span content_type;

  /**
   * @brief The payload of the telemetry.
   */
  az_span telemetry_payload;

  /**
   * @brief The telemetry name.
   */
  az_span telemetry_name;

  /**
   * @brief The QOS for this telemetry message.
   */
  int8_t qos;

} az_mqtt5_telemetry_producer_send_req_event_data;

/**
 * @brief Event data for #AZ_MQTT5_EVENT_TELEMETRY_PRODUCER_ERROR_RSP.
 */
typedef struct az_mqtt5_telemetry_producer_error_rsp_event_data
{
  /**
   * @brief The error message of the telemetry.
   */
  az_span error_message;

  /**
   * @brief The reason_code of the puback error.
   */
  int32_t reason_code;

} az_mqtt5_telemetry_producer_error_rsp_event_data;

/**
 * @brief Triggers an #AZ_MQTT5_EVENT_TELEMETRY_PRODUCER_SEND_REQ event from the application.
 *
 * @note This should be called from the application when it wants to request that a telemetry
 * message is sent.
 *
 * @param[in] client The #az_mqtt5_telemetry_producer to use.
 * @param[in] data The #az_mqtt5_telemetry_producer_send_req_event_data with information for the
 * telemetry.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The event was triggered successfully.
 * @retval #AZ_ERROR_HFSM_INVALID_STATE If called when the client is in a faulted state.
 * @retval #AZ_ERROR_NOT_SUPPORTED if the client is not connected.
 * @retval #AZ_ERROR_TELEMETRY_PRODUCER_PUB_IN_PROGRESS if another publish is already in progress
 * and neither are QOS 0.
 * @retval Other on other failures creating/sending the telemetry message.
 */
AZ_NODISCARD az_result az_mqtt5_telemetry_producer_send_begin(
    az_mqtt5_telemetry_producer* client,
    az_mqtt5_telemetry_producer_send_req_event_data* data);

/**
 * @brief Initializes an MQTT5 Telemetry Producer.
 *
 * @param[out] client The #az_mqtt5_telemetry_producer to initialize.
 * @param[out] telemetry_producer_codec The #az_mqtt5_telemetry_producer_codec to initialize and use
 * within the Telemetry Producer.
 * @param[in] connection The #az_mqtt5_connection to use for the Telemetry Producer.
 * @param[in] property_bag The application allocated #az_mqtt5_property_bag to use for the
 * Telemetry Producer.
 * @param[in] client_id The client id to use for the telemetry topic.
 * @param[in] model_id The model id to use for the telemetry topic.
 * @param[in] telemetry_topic_buffer The application allocated #az_span to use for the telemetry
 * topic.
 * @param[in] publish_timeout_in_seconds Timeout in seconds for publishing (must be > 0).
 * @param[in] options Any #az_mqtt5_telemetry_producer_codec_options to use for the Telemetry
 * Producer or NULL to use the defaults.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_telemetry_producer_init(
    az_mqtt5_telemetry_producer* client,
    az_mqtt5_telemetry_producer_codec* telemetry_producer_codec,
    az_mqtt5_connection* connection,
    az_mqtt5_property_bag property_bag,
    az_span client_id,
    az_span model_id,
    az_span telemetry_topic_buffer,
    int32_t publish_timeout_in_seconds,
    az_mqtt5_telemetry_producer_codec_options* options);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_TELEMETRY_PRODUCER_H

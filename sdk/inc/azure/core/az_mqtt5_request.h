// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_request.
 *
 * @note The state diagram is in sdk/docs/core/resources/mqtt_request.puml
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_REQUEST_H
#define _az_MQTT5_REQUEST_H

#include <stdio.h>

#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Event types for the MQTT5 Request.
 *
 */
enum az_mqtt5_event_type_request
{
  /**
   * @brief Event representing the RPC Client HFSM sending the Request's Pub ID.
   */
  AZ_MQTT5_EVENT_REQUEST_INIT = _az_MAKE_EVENT(_az_FACILITY_MQTT_REQUEST, 1),

  /**
   * @brief Event representing the RPC Client HFSM indicating the Request is completed.
   */
  AZ_MQTT5_EVENT_REQUEST_COMPLETE = _az_MAKE_EVENT(_az_FACILITY_MQTT_REQUEST, 2),

  /**
   * @brief Event representing the RPC Client HFSM indicating the Request should be considered
   * faulted.
   */
  AZ_MQTT5_EVENT_REQUEST_FAULTED = _az_MAKE_EVENT(_az_FACILITY_MQTT_REQUEST, 3),

};

/**
 * @brief The MQTT5 Request.
 *
 */
typedef struct az_mqtt5_request
{
  struct
  {
    /**
     * @brief Request HFSM for the MQTT5 Request.
     *
     */
    _az_hfsm request_hfsm;

    /**
     * @brief The subclient used by the MQTT5 Request.
     */
    _az_event_client subclient;

    /**
     * @brief The MQTT5 connection linked to the MQTT5 Request.
     */
    az_mqtt5_connection* connection;

    /**
     * @brief The policy collection that all MQTT Request policies are a part of.
     */
    _az_event_policy_collection* request_policy_collection;

    /**
     * @brief The message id of the pending publish for the request.
     */
    int32_t pending_pub_id;

    /**
     * @brief Timeout in seconds for publishing (must be > 0).
     */
    int32_t publish_timeout_in_seconds;

    /**
     * @brief the correlation id of the request.
     */
    az_span correlation_id;

    /**
     * @brief Timeout in seconds for request completion (must be > 0).
     */
    int32_t request_completion_timeout_in_seconds;

    /**
     * @brief Timer used for publishes.
     */
    _az_event_pipeline_timer request_pub_timer;

    /**
     * @brief Timer used to track the execution of the request.
     */
    _az_event_pipeline_timer request_completion_timer;

  } _internal;
} az_mqtt5_request;

// Event data types

/**
 * @brief Event data for #AZ_MQTT5_EVENT_REQUEST_INIT.
 */
typedef struct init_event_data
{
  /**
   * @brief The correlation id of the request to track this pub ID with.
   */
  az_span correlation_id;
  /**
   * @brief The pub ID of the pending publish associated with this request.
   */
  int32_t pub_id;
} init_event_data;

/**
 * @brief Initializes an MQTT5 Request.
 *
 * @param[out] request The #az_mqtt5_request to initialize.
 * @param[in] connection The #az_mqtt5_connection to use for the request.
 * @param[in] request_policy_collection The #_az_event_policy_collection that this MQTT Request
 * policies will be a part of.
 * @param[in] correlation_id The correlation id of the request.
 * @param[in] publish_timeout_in_seconds Timeout in seconds for publishing (must be > 0).
 * @param[in] request_completion_timeout_in_seconds Timeout in seconds for request completion (must
 * be > 0).
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_request_init(
    az_mqtt5_request* request,
    az_mqtt5_connection* connection,
    _az_event_policy_collection* request_policy_collection,
    az_span correlation_id,
    int32_t publish_timeout_in_seconds,
    int32_t request_completion_timeout_in_seconds);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_PUB_QUEUE_H

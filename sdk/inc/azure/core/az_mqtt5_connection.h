// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_connection. You use the MQTT 5 connection to connect to a MQTT
 * broker.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_CONNECTION_H
#define _az_MQTT5_CONNECTION_H

#include <azure/core/az_context.h>
#include <azure/core/az_mqtt5.h>
#include <azure/core/az_mqtt5_config.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_event_pipeline_internal.h>
#include <azure/core/internal/az_event_policy_collection_internal.h>
#include <azure/core/internal/az_mqtt5_policy_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/core/az_event_policy.h>
#include <azure/core/internal/az_event_pipeline_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief The MQTT 5 connection.
 *
 */
typedef struct az_mqtt5_connection az_mqtt5_connection;

/**
 * @brief Event types for the MQTT 5 connection.
 *
 */
enum az_event_type_mqtt5_connection
{
  /**
   * @brief Event representing a request to open the MQTT 5 connection.
   *
   */
  AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ = _az_MAKE_EVENT(_az_FACILITY_IOT, 10),

  /**
   * @brief Event representing a request to close the MQTT 5 connection.
   *
   */
  AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ = _az_MAKE_EVENT(_az_FACILITY_IOT, 11),
};

/**
 * @brief Callback for MQTT 5 connection events.
 *
 */
typedef az_result (*az_mqtt5_connection_callback)(az_mqtt5_connection* client, az_event event);

/**
 * @brief MQTT 5 connection options.
 *
 */
typedef struct
{
  /**
   * @brief The hostname of the MQTT 5 broker.
   *
   */
  az_span hostname;

  /**
   * @brief The port of the MQTT 5 broker.
   *
   */
  int16_t port;

  /**
   * @brief Set to true to disable the SDK's connection management. Set to false by default.
   *
   * @details If set to true, the application is responsible for managing the MQTT 5 connection.
   *
   */
  bool disable_sdk_connection_management;

  /**
   * @brief The client id for the MQTT 5 connection. REQUIRED if disable_sdk_connection_management
   * is false.
   *
   */
  az_span client_id_buffer;

  /**
   * @brief The username for the MQTT 5 connection.
   */
  az_span username_buffer;

  /**
   * @brief The password for the MQTT 5 connection.
   *
   */
  az_span password_buffer;

  /**
   * @brief Contains the client certificates for the MQTT 5 connection.
   */
  az_mqtt5_x509_client_certificate client_certificates[MQTT5_CLIENT_CERTIFICATES_MAX];
} az_mqtt5_connection_options;

/**
 * @brief The MQTT 5 connection.
 *
 */
struct az_mqtt5_connection
{
  struct
  {
    /**
     * @brief Connection policy for the MQTT 5 connection.
     *
     */
    _az_hfsm connection_policy;

    /**
     * @brief Policy collection.
     *
     */
    _az_event_policy_collection policy_collection;

    /**
     * @brief MQTT 5 policy.
     *
     */
    _az_mqtt5_policy mqtt5_policy;

    /**
     * @brief Event pipeline for the MQTT 5 connection.
     *
     */
    _az_event_pipeline event_pipeline;

    /**
     * @brief Callback for MQTT 5 connection events.
     *
     */
    az_mqtt5_connection_callback event_callback;

    /**
     * @brief Options for the MQTT 5 connection.
     *
     */
    az_mqtt5_connection_options options;
  } _internal;
};

/**
 * @brief Initializes a MQTT 5 connection options object with default values.
 * @return An #az_mqtt5_connection_options object with default values.
 */
AZ_NODISCARD az_mqtt5_connection_options az_mqtt5_connection_options_default();

/**
 * @brief Initializes a MQTT 5 connection object.
 *
 * @param client Client to initialize.
 * @param context Context to use for the MQTT connection.
 * @param mqtt_client MQTT 5 client to be used by the MQTT connection.
 * @param event_callback Callback for MQTT 5 connection events.
 * @param options MQTT 5 connection options.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_connection_init(
    az_mqtt5_connection* client,
    az_context* context,
    az_mqtt5* mqtt_client,
    az_mqtt5_connection_callback event_callback,
    az_mqtt5_connection_options* options);

/**
 * @brief Opens the connection to the broker.
 *
 * @param client MQTT 5 connection client.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_INLINE az_result az_mqtt5_connection_open(az_mqtt5_connection* client)
{
  return _az_event_pipeline_post_outbound_event(
      &client->_internal.event_pipeline, (az_event){ AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ, NULL });
}

/**
 * @brief Closes the connection to the broker.
 *
 * @param client MQTT 5 connection client.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_INLINE az_result az_mqtt5_connection_close(az_mqtt5_connection* client)
{
  return _az_event_pipeline_post_outbound_event(
      &client->_internal.event_pipeline, (az_event){ AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ, NULL });
}

/**
 * @brief Internal callback for MQTT 5 connection events, will call the user's callback if it
 * exists.
 *
 * @param client MQTT 5 connection client.
 * @param event Event to handle.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_INLINE az_result _az_mqtt5_connection_api_callback(az_mqtt5_connection* client, az_event event)
{
  if (client->_internal.event_callback != NULL)
  {
    _az_RETURN_IF_FAILED(client->_internal.event_callback(client, event));
  }

  return AZ_OK;
}

/**
 * @brief Internal policy init function for the MQTT 5 connection.
 *
 * @param hfsm State machine to use for the MQTT 5 connection.
 * @param outbound Outbound policy.
 * @param inbound Inbound policy.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result _az_mqtt5_connection_policy_init(
    _az_hfsm* hfsm,
    az_event_policy* outbound,
    az_event_policy* inbound);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_CONNECTION_H

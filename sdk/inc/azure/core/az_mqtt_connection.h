// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt_connection. You use the MQTT connection to connect to a MQTT
 * broker.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT_CONNECTION
#define _az_MQTT_CONNECTION

#include <azure/core/az_context.h>
#include <azure/core/az_mqtt.h>
#include <azure/core/az_mqtt_config.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_event_pipeline_internal.h>
#include <azure/core/internal/az_event_policy_subclients_internal.h>
#include <azure/core/internal/az_mqtt_policy_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/core/az_event_policy.h>
#include <azure/core/internal/az_event_pipeline_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief The MQTT connection.
 *
 */
typedef struct az_mqtt_connection az_mqtt_connection;

/**
 * @brief Event types for the MQTT connection.
 *
 */
enum az_event_type_mqtt_connection
{
  /**
   * @brief Event representing a request to open the MQTT connection.
   *
   */
  AZ_EVENT_MQTT_CONNECTION_OPEN_REQ = _az_MAKE_EVENT(_az_FACILITY_IOT, 10),

  /**
   * @brief Event representing a request to close the MQTT connection.
   *
   */
  AZ_EVENT_MQTT_CONNECTION_CLOSE_REQ = _az_MAKE_EVENT(_az_FACILITY_IOT, 11),
};

/**
 * @brief Callback for MQTT connection events.
 *
 */
typedef az_result (*az_mqtt_connection_callback)(az_mqtt_connection* client, az_event event);

/**
 * @brief MQTT connection options.
 *
 */
typedef struct
{
  /**
   * @brief The hostname of the MQTT broker.
   *
   */
  az_span hostname;

  /**
   * @brief The port of the MQTT broker.
   *
   */
  int16_t port;

  /**
   * @brief Denotes whether the MQTT connection should be managed by the SDK.
   *
   */
  bool connection_management;

  /**
   * @brief The client id for the MQTT connection. REQUIRED if connection_management is true.
   *
   */
  az_span client_id_buffer;

  /**
   * @brief The username for the MQTT connection. REQUIRED if connection_management is true.
   *
   */
  az_span username_buffer;

  /**
   * @brief The password for the MQTT connection. REQUIRED if connection_management is true.
   *
   */
  az_span password_buffer;

  /**
   * @brief Specifies whether the MQTT connection should use a username and password. REQUIRED if
   * connection_management is true.
   */
  bool use_username_password;

  /**
   * @brief Contains the client certificates for the MQTT connection. REQUIRED if
   * connection_management is true.
   */
  az_mqtt_x509_client_certificate client_certificates[MQTT_CLIENT_CERTIFICATES_MAX];
} az_mqtt_connection_options;

/**
 * @brief The MQTT connection.
 *
 */
struct az_mqtt_connection
{
  struct
  {
    /**
     * @brief Connection policy for the MQTT connection.
     *
     */
    _az_hfsm connection_policy;

    /**
     * @brief Subclient policies.
     *
     */
    _az_event_policy_subclients subclient_policy;

    /**
     * @brief MQTT policy.
     *
     */
    _az_mqtt_policy mqtt_policy;

    /**
     * @brief Event pipeline for the MQTT connection.
     *
     */
    _az_event_pipeline event_pipeline;

    /**
     * @brief Callback for MQTT connection events.
     *
     */
    az_mqtt_connection_callback event_callback;

    /**
     * @brief Options for the MQTT connection.
     *
     */
    az_mqtt_connection_options options;
  } _internal;
};

/**
 * @brief Initializes a MQTT connection options object with default values.
 * @return An #az_mqtt_connection_options object with default values.
 */
AZ_NODISCARD az_mqtt_connection_options az_mqtt_connection_options_default();

/**
 * @brief Initializes a MQTT connection object.
 *
 * @param client Client to initialize.
 * @param context Context to use for the MQTT connection.
 * @param mqtt_client MQTT client to be used by the MQTT connection.
 * @param event_callback Callback for MQTT connection events.
 * @param options MQTT connection options.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt_connection_init(
    az_mqtt_connection* client,
    az_context* context,
    az_mqtt* mqtt_client,
    az_mqtt_connection_callback event_callback,
    az_mqtt_connection_options* options);

/**
 * @brief Opens the connection to the broker.
 *
 * @param client MQTT connection client.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_INLINE az_result az_mqtt_connection_open(az_mqtt_connection* client)
{
  return _az_event_pipeline_post_outbound_event(
      &client->_internal.event_pipeline, (az_event){ AZ_EVENT_MQTT_CONNECTION_OPEN_REQ, NULL });
}

/**
 * @brief Closes the connection to the broker.
 *
 * @param client MQTT connection client.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_INLINE az_result az_mqtt_connection_close(az_mqtt_connection* client)
{
  return _az_event_pipeline_post_outbound_event(
      &client->_internal.event_pipeline, (az_event){ AZ_EVENT_MQTT_CONNECTION_CLOSE_REQ, NULL });
}

/**
 * @brief Internal callback for MQTT connection events, will call the user's callback if it exists.
 *
 * @param client MQTT connection client.
 * @param event Event to handle.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_INLINE az_result _az_mqtt_connection_api_callback(az_mqtt_connection* client, az_event event)
{
  if (client->_internal.event_callback != NULL)
  {
    _az_RETURN_IF_FAILED(client->_internal.event_callback(client, event));
  }

  return AZ_OK;
}

/**
 * @brief Internal policy init function for the MQTT connection.
 *
 * @param hfsm State machine to use for the MQTT connection.
 * @param outbound Outbound policy.
 * @param inbound Inbound policy.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result _az_mqtt_connection_policy_init(
    _az_hfsm* hfsm,
    az_event_policy* outbound,
    az_event_policy* inbound);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_CONNECTION

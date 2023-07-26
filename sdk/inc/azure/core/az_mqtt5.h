// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief This header defines the types and functions your application uses to leverage MQTT v5
 * pub/sub functionality.
 *
 * @details For more details on Azure IoT MQTT requirements please see
 * https://docs.microsoft.com/azure/iot-hub/iot-hub-mqtt-support.
 * - Outbound APIs will be called by the SDK to send data over the network.
 * - Inbound API calls must be called by the MQTT 5 implementation to send data towards the SDK.
 *
 * @note Object lifetime: all APIs have run-to-completion semantics. Data passed into the APIs
 *       is owned by the API for the duration of the call.
 *
 * @note API I/O model: It is generally expected that all operations are asynchronous. An outbound
 *       call should not block, and may occur on the same call-stack, as a result of an inbound
 *       call.
 *
 * @note The SDK expects that the network stack is stalled for the duration of the API calls.
 *
 * @note All API implementations must return az_result.
 * We recommend using `_az_RESULT_MAKE_ERROR(_az_FACILITY_IOT_MQTT, mqtt_stack_ret);` if compatible.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_H
#define _az_MQTT5_H

#include <azure/core/az_config.h>
#include <azure/core/az_event.h>
#include <azure/core/az_log.h>
#include <azure/core/az_mqtt5_property_bag.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(TRANSPORT_MOSQUITTO)
#include <azure/platform/az_mqtt5_mosquitto.h>
#else
#include <azure/platform/az_mqtt5_notransport.h>
#endif

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief x509 certificate definition.
 */
typedef struct
{
  /**
   * @brief The x509 certificate.
   */
  az_span cert;

  /**
   * @brief The x509 certificate key.
   */
  az_span key;
} az_mqtt5_x509_client_certificate;

/**
 * @brief MQTT 5 publish data.
 *
 */
typedef struct
{
  /**
   * @brief The properties of the publish request.
   */
  az_mqtt5_property_bag* properties;

  /**
   * @brief The topic to publish to.
   */
  az_span topic;

  /**
   * @brief The payload to publish.
   */
  az_span payload;

  /**
   * @brief The ID of the publish request.
   *
   * @details The MQTT 5 stack should set this ID upon returning.
   */
  int32_t out_id;

  /**
   * @brief The quality of service of the publish request.
   */
  int8_t qos;
} az_mqtt5_pub_data;

/**
 * @brief MQTT 5 receive data.
 *
 */
typedef struct
{
  /**
   * @brief The received properties.
   */
  az_mqtt5_property_bag* properties;

  /**
   * @brief The topic the message was received on.
   */
  az_span topic;

  /**
   * @brief The payload of the message.
   */
  az_span payload;

  /**
   * @brief The quality of service of the message.
   */
  int8_t qos;

  /**
   * @brief The ID of the message.
   */
  int32_t id;
} az_mqtt5_recv_data;

/**
 * @brief MQTT 5 publish acknowledgement data.
 *
 */
typedef struct
{
  /**
   * @brief The puback properties.
   */
  az_mqtt5_property_bag* properties;

  /**
   * @brief The publish request ID.
   */
  int32_t id;
} az_mqtt5_puback_data;

/**
 * @brief MQTT 5 subscribe data.
 *
 */
typedef struct
{
  /**
   * @brief The properties of subscribe request.
   */
  az_mqtt5_property_bag* properties;
  /**
   * @brief Topic filter to subscribe to.
   */
  az_span topic_filter;

  /**
   * @brief Quality of service of the subscription.
   */
  int8_t qos;

  /**
   * @brief Id to correlate the subscription request with the response acknowledgement.
   */
  int32_t out_id;
} az_mqtt5_sub_data;

/**
 * @brief MQTT 5 subscribe acknowledgement data.
 *
 */
typedef struct
{
  /**
   * @brief The suback properties.
   */
  az_mqtt5_property_bag* properties;

  /**
   * @brief The subscribe request ID.
   */
  int32_t id;
} az_mqtt5_suback_data;

/**
 * @brief MQTT 5 connect data.
 *
 */
typedef struct
{
  /**
   * @brief The properties of the connect request.
   */
  az_mqtt5_property_bag* properties;

  /**
   * @brief Hostname or IP address of the MQTT 5 broker.
   */
  az_span host;

  /**
   * @brief Port number broker is listening on.
   */
  int16_t port;

  /**
   * @brief The username to send to the broker.
   */
  az_span username;

  /**
   * @brief The password to send to the broker.
   */
  az_span password;

  /**
   * @brief The client ID to send to the broker.
   */
  az_span client_id;

  /**
   * @brief The certificate to use for authentication.
   */
  az_mqtt5_x509_client_certificate certificate;
} az_mqtt5_connect_data;

/**
 * @brief MQTT 5 connect acknowledgement data.
 *
 */
typedef struct
{

  /**
   * @brief The properties of the connect acknowledgement.
   */
  az_mqtt5_property_bag* properties;

  /**
   * @brief Connection acknowledgement reason code. Indicates success or reason for failure.
   */
  int32_t connack_reason;

  /**
   * @brief Indicates whether a TLS authentication error occurred.
   */
  bool tls_authentication_error;
} az_mqtt5_connack_data;

/**
 * @brief MQTT 5 disconnect data.
 *
 */
typedef struct
{
  /**
   * @brief Indicates whether a TLS authentication error occurred.
   */
  bool tls_authentication_error;

  /**
   * @brief Indicates whether the disconnect was requested by the client.
   */
  bool disconnect_requested;
} az_mqtt5_disconnect_data;

/**
 * @brief Log classifications for MQTT 5.
 *
 */
enum az_log_classification_mqtt5
{
  /// Log classification for MQTT 5.
  AZ_LOG_MQTT_STACK = _az_LOG_MAKE_CLASSIFICATION(_az_FACILITY_CORE_MQTT5, 3),
};

/**
 * @brief Azure MQTT 5 HFSM event types.
 *
 */
enum az_event_type_mqtt5
{
  /// MQTT 5 Connect Request event.
  AZ_MQTT5_EVENT_CONNECT_REQ = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 10),

  /// MQTT 5 Connect Response event.
  AZ_MQTT5_EVENT_CONNECT_RSP = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 11),

  /// MQTT 5 Disconnect Request event.
  AZ_MQTT5_EVENT_DISCONNECT_REQ = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 12),

  /// MQTT 5 Disconnect Response event.
  AZ_MQTT5_EVENT_DISCONNECT_RSP = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 13),

  /// MQTT 5 Publish Receive Indication event.
  AZ_MQTT5_EVENT_PUB_RECV_IND = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 14),

  /// MQTT 5 Publish Request event.
  AZ_MQTT5_EVENT_PUB_REQ = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 15),

  /// MQTT 5 Publish Acknowledge Response event.
  AZ_MQTT5_EVENT_PUBACK_RSP = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 16),

  /// MQTT 5 Subscribe Request event.
  AZ_MQTT5_EVENT_SUB_REQ = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 17),

  /// MQTT 5 Subscribe Acknowledge Response event.
  AZ_MQTT5_EVENT_SUBACK_RSP = _az_MAKE_EVENT(_az_FACILITY_CORE_MQTT5, 18),
};

// Porting 1. The following functions must be called by the implementation when data is received:

/**
 * @brief Posts a MQTT 5 publish receive indication event to the event pipeline.
 *
 * @param mqtt5 The MQTT 5 instance.
 * @param recv_data The MQTT 5 receive data.
 *
 * @return An #az_result value indicating the result of the operation.
 *
 */
AZ_NODISCARD AZ_INLINE az_result
az_mqtt5_inbound_recv(az_mqtt5* mqtt5, az_mqtt5_recv_data* recv_data)
{
  _az_event_pipeline* pipeline = mqtt5->_internal.platform_mqtt5.pipeline;
  if (!pipeline)
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }

  return _az_event_pipeline_post_inbound_event(
      pipeline, (az_event){ .type = AZ_MQTT5_EVENT_PUB_RECV_IND, .data = recv_data });
}

/**
 * @brief Posts a MQTT 5 connect acknowledgement event to the event pipeline.
 *
 * @param mqtt5 The MQTT 5 instance.
 * @param connack_data The MQTT 5 connect acknowledgement data.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD AZ_INLINE az_result
az_mqtt5_inbound_connack(az_mqtt5* mqtt5, az_mqtt5_connack_data* connack_data)
{
  _az_event_pipeline* pipeline = mqtt5->_internal.platform_mqtt5.pipeline;
  if (!pipeline)
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }

  return _az_event_pipeline_post_inbound_event(
      pipeline, (az_event){ .type = AZ_MQTT5_EVENT_CONNECT_RSP, .data = connack_data });
}

/**
 * @brief Posts a MQTT 5 subscribe acknowledgement event to the event pipeline.
 *
 * @param mqtt5 The MQTT 5 instance.
 * @param suback_data The MQTT 5 subscribe acknowledgement data.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD AZ_INLINE az_result
az_mqtt5_inbound_suback(az_mqtt5* mqtt5, az_mqtt5_suback_data* suback_data)
{
  _az_event_pipeline* pipeline = mqtt5->_internal.platform_mqtt5.pipeline;
  if (!pipeline)
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }

  return _az_event_pipeline_post_inbound_event(
      pipeline, (az_event){ .type = AZ_MQTT5_EVENT_SUBACK_RSP, .data = suback_data });
}

/**
 * @brief Posts a MQTT 5 publish acknowledgement event to the event pipeline.
 *
 * @param mqtt5 The MQTT 5 instance.
 * @param puback_data The MQTT 5 publish acknowledgement data.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD AZ_INLINE az_result
az_mqtt5_inbound_puback(az_mqtt5* mqtt5, az_mqtt5_puback_data* puback_data)
{
  _az_event_pipeline* pipeline = mqtt5->_internal.platform_mqtt5.pipeline;
  if (!pipeline)
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }

  return _az_event_pipeline_post_inbound_event(
      pipeline, (az_event){ .type = AZ_MQTT5_EVENT_PUBACK_RSP, .data = puback_data });
}

/**
 * @brief Posts a MQTT 5 disconnect response event to the event pipeline.
 *
 * @param mqtt5 The MQTT 5 instance.
 * @param disconnect_data The MQTT 5 disconnect response data.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD AZ_INLINE az_result
az_mqtt5_inbound_disconnect(az_mqtt5* mqtt5, az_mqtt5_disconnect_data* disconnect_data)
{
  _az_event_pipeline* pipeline = mqtt5->_internal.platform_mqtt5.pipeline;
  if (!pipeline)
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }

  return _az_event_pipeline_post_inbound_event(
      pipeline, (az_event){ .type = AZ_MQTT5_EVENT_DISCONNECT_RSP, .data = disconnect_data });
}

// Porting 2. The following functions must be implemented and will be called by the SDK to
//            send data:

/**
 * @brief The default MQTT 5 options.
 *
 * @return An #az_mqtt5_options value.
 */
AZ_NODISCARD az_mqtt5_options az_mqtt5_options_default();

/**
 * @brief Initializes the MQTT 5 instance.
 *
 * @param mqtt5 The MQTT 5 instance.
 * @param options The MQTT 5 options.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_init(az_mqtt5* mqtt5, az_mqtt5_options const* options);

/**
 * @brief Sends a MQTT 5 connect data packet to broker.
 *
 * @param mqtt5 The MQTT 5 instance.
 * @param connect_data The MQTT 5 connect data.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
az_mqtt5_outbound_connect(az_mqtt5* mqtt5, az_mqtt5_connect_data* connect_data);

/**
 * @brief Sends a MQTT 5 subscribe data packet to broker.
 *
 * @param mqtt5 The MQTT 5 instance.
 * @param sub_data The MQTT 5 subscribe data.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_outbound_sub(az_mqtt5* mqtt5, az_mqtt5_sub_data* sub_data);

/**
 * @brief Sends a MQTT 5 publish data packet to broker.
 *
 * @param mqtt5 The MQTT 5 instance.
 * @param pub_data The MQTT 5 publish data.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_outbound_pub(az_mqtt5* mqtt5, az_mqtt5_pub_data* pub_data);

/**
 * @brief Sends a MQTT 5 disconnect to broker.
 *
 * @param mqtt5 The MQTT 5 instance.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_outbound_disconnect(az_mqtt5* mqtt5);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_H

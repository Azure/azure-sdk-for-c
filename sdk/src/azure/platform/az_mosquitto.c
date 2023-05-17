// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Contains the az_mqtt.h interface implementation with Mosquitto MQTT
 * (https://github.com/eclipse/mosquitto).
 *
 * @note The Mosquitto Lib documentation is available at:
 * https://mosquitto.org/api/files/mosquitto-h.html
 */

#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/core/internal/az_span_internal.h>

#include <mosquitto.h>
#include <pthread.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

static void _az_mosquitto_critical_error() { az_platform_critical_error(); }

AZ_INLINE az_result _az_result_from_mosq(int mosquitto_ret)
{
  az_result ret;

  if (mosquitto_ret == MOSQ_ERR_SUCCESS)
  {
    ret = AZ_OK;
  }
  else
  {
    ret = _az_RESULT_MAKE_ERROR(_az_FACILITY_IOT_MQTT, mosquitto_ret);
  }

  return ret;
}

static void _az_mosquitto_on_connect(struct mosquitto* mosq, void* obj, int reason_code)
{
  az_result ret;
  az_mqtt* me = (az_mqtt*)obj;

  _az_PRECONDITION(mosq == me->mosquitto_handle);

  if (reason_code != 0)
  {
    /* If the connection fails for any reason, we don't want to keep on
     * retrying in this example, so disconnect. Without this, the client
     * will attempt to reconnect. */
    mosquitto_disconnect(mosq);
  }

  ret = az_mqtt_inbound_connack(
      me,
      &(az_mqtt_connack_data){ .connack_reason = reason_code, .tls_authentication_error = false });

  if (az_result_failed(ret))
  {
    _az_mosquitto_critical_error();
  }
}

static void _az_mosquitto_on_disconnect(struct mosquitto* mosq, void* obj, int rc)
{
  (void)mosq;
  az_mqtt* me = (az_mqtt*)obj;

  az_result ret = az_mqtt_inbound_disconnect(
      me,
      &(az_mqtt_disconnect_data){ .tls_authentication_error = false,
                                  .disconnect_requested = (rc == 0) });

  if (az_result_failed(ret))
  {
    _az_mosquitto_critical_error();
  }
}

/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
static void _az_mosquitto_on_publish(struct mosquitto* mosq, void* obj, int mid)
{
  (void)mosq;
  az_mqtt* me = (az_mqtt*)obj;

  az_result ret = az_mqtt_inbound_puback(me, &(az_mqtt_puback_data){ mid });

  if (az_result_failed(ret))
  {
    _az_mosquitto_critical_error();
  }
}

static void _az_mosquitto_on_subscribe(
    struct mosquitto* mosq,
    void* obj,
    int mid,
    int qos_count,
    const int* granted_qos)
{
  (void)mosq;
  (void)qos_count;
  (void)granted_qos;

  az_mqtt* me = (az_mqtt*)obj;

  az_result ret = az_mqtt_inbound_suback(me, &(az_mqtt_suback_data){ mid });

  if (az_result_failed(ret))
  {
    _az_mosquitto_critical_error();
  }
}

static void _az_mosquitto_on_unsubscribe(struct mosquitto* mosq, void* obj, int mid)
{
  (void)mosq;
  (void)obj;
  (void)mid;

  // Unsubscribe is not handled or implemented. We should never get here.
  _az_mosquitto_critical_error();
}

static void _az_mosquitto_on_message(
    struct mosquitto* mosq,
    void* obj,
    const struct mosquitto_message* message)
{
  (void)mosq;
  az_mqtt* me = (az_mqtt*)obj;

  az_result ret = az_mqtt_inbound_recv(
      me,
      &(az_mqtt_recv_data){ .qos = (int8_t)message->qos,
                            .id = (int32_t)message->mid,
                            .payload = az_span_create(message->payload, message->payloadlen),
                            .topic = az_span_create_from_str(message->topic) });

  if (az_result_failed(ret))
  {
    _az_mosquitto_critical_error();
  }
}

static void _az_mosquitto_on_log(struct mosquitto* mosq, void* obj, int level, const char* str)
{
  (void)mosq;
  (void)obj;
  (void)level;
  if (_az_LOG_SHOULD_WRITE(AZ_LOG_MQTT_STACK))
  {
    _az_LOG_WRITE(AZ_LOG_MQTT_STACK, az_span_create_from_str((char*)(uintptr_t)str));
  }
}

AZ_NODISCARD az_mqtt_options az_mqtt_options_default()
{
  return (az_mqtt_options){ .certificate_authority_trusted_roots = NULL,
                            .openssl_engine = NULL,
                            .mosquitto_handle = NULL };
}

AZ_NODISCARD az_result az_mqtt_init(az_mqtt* mqtt, az_mqtt_options const* options)
{
  _az_PRECONDITION_NOT_NULL(mqtt);
  mqtt->_internal.options = options == NULL ? az_mqtt_options_default() : *options;
  mqtt->mosquitto_handle = mqtt->_internal.options.mosquitto_handle;
  mqtt->_internal.platform_mqtt.pipeline = NULL;

  return AZ_OK;
}

AZ_NODISCARD az_result
az_mqtt_outbound_connect(az_mqtt* mqtt, az_context* context, az_mqtt_connect_data* connect_data)
{
  (void)context;
  az_result ret = AZ_OK;
  az_mqtt* me = (az_mqtt*)mqtt;

  // IMPORTANT: application must call mosquitto_lib_init() before any Mosquitto clients are created.

  if (me->mosquitto_handle == NULL)
  {
    me->mosquitto_handle = mosquitto_new(
        (char*)az_span_ptr(connect_data->client_id),
        false, // clean-session
        me); // callback context - i.e. user data.
  }

  if (me->mosquitto_handle == NULL)
  {
    ret = AZ_ERROR_OUT_OF_MEMORY;
    return ret;
  }

  /* Configure callbacks. This should be done before connecting ideally. */
  mosquitto_log_callback_set(me->mosquitto_handle, _az_mosquitto_on_log);
  mosquitto_connect_callback_set(me->mosquitto_handle, _az_mosquitto_on_connect);
  mosquitto_disconnect_callback_set(me->mosquitto_handle, _az_mosquitto_on_disconnect);
  mosquitto_publish_callback_set(me->mosquitto_handle, _az_mosquitto_on_publish);
  mosquitto_subscribe_callback_set(me->mosquitto_handle, _az_mosquitto_on_subscribe);
  mosquitto_unsubscribe_callback_set(me->mosquitto_handle, _az_mosquitto_on_unsubscribe);
  mosquitto_message_callback_set(me->mosquitto_handle, _az_mosquitto_on_message);

  if (az_span_size(me->_internal.options.certificate_authority_trusted_roots) != 0)
  {
    _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_tls_set(
        me->mosquitto_handle,
        (const char*)az_span_ptr(me->_internal.options.certificate_authority_trusted_roots),
        NULL,
        (const char*)az_span_ptr(connect_data->certificate.cert),
        (const char*)az_span_ptr(connect_data->certificate.key),
        NULL)));
  }

  _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_username_pw_set(
      me->mosquitto_handle,
      (const char*)az_span_ptr(connect_data->username),
      (const char*)az_span_ptr(connect_data->password))));

  _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_connect_async(
      (struct mosquitto*)me->mosquitto_handle,
      (char*)az_span_ptr(connect_data->host),
      connect_data->port,
      AZ_MQTT_KEEPALIVE_SECONDS)));

  _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_loop_start(me->mosquitto_handle)));

  return ret;
}

AZ_NODISCARD az_result
az_mqtt_outbound_sub(az_mqtt* mqtt, az_context* context, az_mqtt_sub_data* sub_data)
{
  (void)context;
  return _az_result_from_mosq(mosquitto_subscribe(
      mqtt->mosquitto_handle,
      &sub_data->out_id,
      (char*)az_span_ptr(sub_data->topic_filter),
      sub_data->qos));
}

AZ_NODISCARD az_result
az_mqtt_outbound_pub(az_mqtt* mqtt, az_context* context, az_mqtt_pub_data* pub_data)
{
  (void)context;
  return _az_result_from_mosq(mosquitto_publish(
      mqtt->mosquitto_handle,
      &pub_data->out_id,
      (char*)az_span_ptr(pub_data->topic), // Assumes properly formed NULL terminated string.
      az_span_size(pub_data->payload),
      az_span_ptr(pub_data->payload),
      pub_data->qos,
      false));
}

AZ_NODISCARD az_result az_mqtt_outbound_disconnect(az_mqtt* mqtt, az_context* context)
{
  (void)context;
  return _az_result_from_mosq(mosquitto_disconnect(mqtt->mosquitto_handle));
}

AZ_NODISCARD az_result az_mqtt_wait_for_event(az_mqtt* mqtt, int32_t timeout)
{
  return _az_result_from_mosq(mosquitto_loop(mqtt->mosquitto_handle, timeout, 1));
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Contains the az_mqtt5.h interface implementation with Mosquitto MQTT
 * (https://github.com/eclipse/mosquitto).
 *
 * @note The Mosquitto Lib documentation is available at:
 * https://mosquitto.org/api/files/mosquitto-h.html
 */

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_mqtt5_config.h>
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

/// A certificate path (any string) is required when configuring mosquitto to use OS certificates.
#define REQUIRED_TLS_SET_CERT_PATH "L"

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

static void _az_mosquitto5_on_connect(
    struct mosquitto* mosq,
    void* obj,
    int rc,
    int flags,
    const mosquitto_property* props)
{
#ifdef AZ_NO_PRECONDITION_CHECKING
  (void)mosq;
#endif // AZ_NO_PRECONDITION_CHECKING
  (void)flags;
  (void)props;
  az_result ret;
  az_mqtt5* me = (az_mqtt5*)obj;

  _az_PRECONDITION(mosq == me->_internal.mosquitto_handle);

  if (rc != 0)
  {
    /* If the connection fails for any reason, we don't want to keep on
     * retrying in this example, so disconnect. Without this, the client
     * will attempt to reconnect. */
    ret = mosquitto_disconnect(me->_internal.mosquitto_handle);

    if (ret != MOSQ_ERR_SUCCESS && ret != MOSQ_ERR_NO_CONN)
    {
      _az_mosquitto_critical_error();
    }
  }

  ret = az_mqtt5_inbound_connack(
      me, &(az_mqtt5_connack_data){ .connack_reason = rc, .tls_authentication_error = false });

  if (az_result_failed(ret))
  {
    _az_mosquitto_critical_error();
  }
}

static void _az_mosquitto5_on_disconnect(
    struct mosquitto* mosq,
    void* obj,
    int rc,
    const mosquitto_property* props)
{
#ifdef AZ_NO_PRECONDITION_CHECKING
  (void)mosq;
#endif // AZ_NO_PRECONDITION_CHECKING
  (void)props;
  az_mqtt5* me = (az_mqtt5*)obj;

  _az_PRECONDITION(mosq == me->_internal.mosquitto_handle);

  az_result ret = az_mqtt5_inbound_disconnect(
      me,
      &(az_mqtt5_disconnect_data){ .tls_authentication_error = false,
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
static void _az_mosquitto5_on_publish(
    struct mosquitto* mosq,
    void* obj,
    int mid,
    int rc,
    const mosquitto_property* props)
{
#ifdef AZ_NO_PRECONDITION_CHECKING
  (void)mosq;
#endif // AZ_NO_PRECONDITION_CHECKING
  (void)rc;
  (void)props;
  az_mqtt5* me = (az_mqtt5*)obj;

  _az_PRECONDITION(mosq == me->_internal.mosquitto_handle);

  az_result ret = az_mqtt5_inbound_puback(me, &(az_mqtt5_puback_data){ .id = mid });

  if (az_result_failed(ret))
  {
    _az_mosquitto_critical_error();
  }
}

static void _az_mosquitto5_on_subscribe(
    struct mosquitto* mosq,
    void* obj,
    int mid,
    int qos_count,
    const int* granted_qos,
    const mosquitto_property* props)
{
#ifdef AZ_NO_PRECONDITION_CHECKING
  (void)mosq;
#endif // AZ_NO_PRECONDITION_CHECKING
  (void)qos_count;
  (void)granted_qos;
  (void)props;

  az_mqtt5* me = (az_mqtt5*)obj;

  _az_PRECONDITION(mosq == me->_internal.mosquitto_handle);

  az_result ret = az_mqtt5_inbound_suback(me, &(az_mqtt5_suback_data){ .id = mid });

  if (az_result_failed(ret))
  {
    _az_mosquitto_critical_error();
  }
}

static void _az_mosquitto5_on_unsubscribe(
    struct mosquitto* mosq,
    void* obj,
    int mid,
    const mosquitto_property* props)
{
  (void)mosq;
  (void)obj;
  (void)mid;
  (void)props;

  // Unsubscribe is not handled or implemented. We should never get here.
  _az_mosquitto_critical_error();
}

static void _az_mosquitto5_on_message(
    struct mosquitto* mosq,
    void* obj,
    const struct mosquitto_message* message,
    const mosquitto_property* props)
{
#ifdef AZ_NO_PRECONDITION_CHECKING
  (void)mosq;
#endif // AZ_NO_PRECONDITION_CHECKING
  az_result ret;
  az_mqtt5* me = (az_mqtt5*)obj;
  az_mqtt5_property_bag property_bag;
  az_mqtt5_property_bag_options property_bag_options;

  _az_PRECONDITION(mosq == me->_internal.mosquitto_handle);

  property_bag_options = az_mqtt5_property_bag_options_default();
  property_bag_options.properties = (mosquitto_property*)(uintptr_t)props;

  ret = az_mqtt5_property_bag_init(&property_bag, me, &property_bag_options);

  if (az_result_failed(ret))
  {
    _az_mosquitto_critical_error();
  }

  ret = az_mqtt5_inbound_recv(
      me,
      &(az_mqtt5_recv_data){ .qos = (int8_t)message->qos,
                             .id = (int32_t)message->mid,
                             .payload = az_span_create(message->payload, message->payloadlen),
                             .topic = az_span_create_from_str(message->topic),
                             .properties = &property_bag });

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
#ifdef AZ_NO_LOGGING
  (void)str;
#endif // AZ_NO_LOGGING
  if (_az_LOG_SHOULD_WRITE(AZ_LOG_MQTT_STACK))
  {
    _az_LOG_WRITE(AZ_LOG_MQTT_STACK, az_span_create_from_str((char*)(uintptr_t)str));
  }
}

AZ_NODISCARD az_mqtt5_options az_mqtt5_options_default()
{
  return (az_mqtt5_options){
    .certificate_authority_trusted_roots = AZ_SPAN_EMPTY,
    .openssl_engine = NULL,
    .mosquitto_handle = NULL,
    .disable_tls = false,
  };
}

// TODO_L: Should we have az_mosquitto5_init(..., mosquitto_handle h)
AZ_NODISCARD az_result az_mqtt5_init(az_mqtt5* mqtt5, az_mqtt5_options const* options)
{
  _az_PRECONDITION_NOT_NULL(mqtt5);
  mqtt5->_internal.options = options == NULL ? az_mqtt5_options_default() : *options;
  mqtt5->_internal.mosquitto_handle = mqtt5->_internal.options.mosquitto_handle;
  mqtt5->_internal.platform_mqtt5.pipeline = NULL;

  return AZ_OK;
}

AZ_NODISCARD az_result
az_mqtt5_outbound_connect(az_mqtt5* mqtt5, az_mqtt5_connect_data* connect_data)
{
  az_result ret = AZ_OK;
  az_mqtt5* me = (az_mqtt5*)mqtt5;

  // IMPORTANT: application must call mosquitto_lib_init() before any Mosquitto clients are created.

  if (me->_internal.mosquitto_handle == NULL)
  {
    me->_internal.mosquitto_handle = mosquitto_new(
        (char*)az_span_ptr(connect_data->client_id),
        false, // clean-session
        me); // callback context - i.e. user data.
  }

  if (me->_internal.mosquitto_handle == NULL)
  {
    ret = AZ_ERROR_OUT_OF_MEMORY;
    return ret;
  }

  _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_int_option(
      me->_internal.mosquitto_handle, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5)));

  // Configure callbacks. This should be done before connecting ideally.
  mosquitto_log_callback_set(me->_internal.mosquitto_handle, _az_mosquitto_on_log);
  mosquitto_connect_v5_callback_set(me->_internal.mosquitto_handle, _az_mosquitto5_on_connect);
  mosquitto_disconnect_v5_callback_set(
      me->_internal.mosquitto_handle, _az_mosquitto5_on_disconnect);
  mosquitto_publish_v5_callback_set(me->_internal.mosquitto_handle, _az_mosquitto5_on_publish);
  mosquitto_subscribe_v5_callback_set(me->_internal.mosquitto_handle, _az_mosquitto5_on_subscribe);
  mosquitto_unsubscribe_v5_callback_set(
      me->_internal.mosquitto_handle, _az_mosquitto5_on_unsubscribe);
  mosquitto_message_v5_callback_set(me->_internal.mosquitto_handle, _az_mosquitto5_on_message);

  if (me->_internal.options.disable_tls == 0)
  {
    bool use_os_certs
        = (az_span_ptr(me->_internal.options.certificate_authority_trusted_roots) == NULL);

    if (use_os_certs)
    {
      _az_RETURN_IF_FAILED(_az_result_from_mosq(
          mosquitto_int_option(me->_internal.mosquitto_handle, MOSQ_OPT_TLS_USE_OS_CERTS, 1)));
    }

    _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_tls_set(
        me->_internal.mosquitto_handle,
        (const char*)az_span_ptr(me->_internal.options.certificate_authority_trusted_roots),
        use_os_certs ? REQUIRED_TLS_SET_CERT_PATH : NULL,
        (const char*)az_span_ptr(connect_data->certificate.cert),
        (const char*)az_span_ptr(connect_data->certificate.key),
        NULL)));
  }

  if (az_span_ptr(connect_data->username) != NULL)
  {
    _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_username_pw_set(
        me->_internal.mosquitto_handle,
        (const char*)az_span_ptr(connect_data->username),
        (const char*)az_span_ptr(connect_data->password))));
  }

  _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_connect_async(
      (struct mosquitto*)me->_internal.mosquitto_handle,
      (char*)az_span_ptr(connect_data->host),
      connect_data->port,
      AZ_MQTT5_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS)));

  _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_loop_start(me->_internal.mosquitto_handle)));

  return ret;
}

AZ_NODISCARD az_result az_mqtt5_outbound_sub(az_mqtt5* mqtt5, az_mqtt5_sub_data* sub_data)
{
  return _az_result_from_mosq(mosquitto_subscribe_v5(
      mqtt5->_internal.mosquitto_handle,
      &sub_data->out_id,
      (char*)az_span_ptr(sub_data->topic_filter),
      sub_data->qos,
      0,
      sub_data->properties ? sub_data->properties->properties : NULL));
}

AZ_NODISCARD az_result az_mqtt5_outbound_pub(az_mqtt5* mqtt5, az_mqtt5_pub_data* pub_data)
{
  return _az_result_from_mosq(mosquitto_publish_v5(
      mqtt5->_internal.mosquitto_handle,
      &pub_data->out_id,
      (char*)az_span_ptr(pub_data->topic), // Assumes properly formed NULL terminated string.
      az_span_size(pub_data->payload),
      az_span_ptr(pub_data->payload),
      pub_data->qos,
      false,
      pub_data->properties ? pub_data->properties->properties : NULL));
}

AZ_NODISCARD az_result az_mqtt5_outbound_disconnect(az_mqtt5* mqtt5)
{
  return _az_result_from_mosq(mosquitto_disconnect_v5(mqtt5->_internal.mosquitto_handle, 0, NULL));
}

AZ_NODISCARD az_result az_mqtt5_property_bag_init(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5* mqtt5,
    az_mqtt5_property_bag_options const* options)
{
  (void)mqtt5;
  property_bag->properties = options == NULL ? NULL : options->properties;

  return AZ_OK;
}

AZ_NODISCARD az_mqtt5_property_bag_options az_mqtt5_property_bag_options_default()
{
  return (az_mqtt5_property_bag_options){
    .properties = NULL,
  };
}

AZ_NODISCARD az_result az_mqtt5_property_bag_empty(az_mqtt5_property_bag* property_bag)
{
  _az_PRECONDITION_NOT_NULL(property_bag);

  mosquitto_property_free_all(&property_bag->properties);

  property_bag->properties = NULL;

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_string_append(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_string* prop_str)
{
  _az_PRECONDITION_NOT_NULL(property_bag);
  _az_PRECONDITION_NOT_NULL(prop_str);

  return _az_result_from_mosq(mosquitto_property_add_string(
      (mosquitto_property**)&property_bag->properties,
      (int)type,
      (const char*)az_span_ptr(prop_str->str)));
}

AZ_NODISCARD az_result az_mqtt5_property_bag_stringpair_append(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_stringpair* prop_strpair)
{
  _az_PRECONDITION_NOT_NULL(property_bag);
  _az_PRECONDITION_NOT_NULL(prop_strpair);

  return _az_result_from_mosq(mosquitto_property_add_string_pair(
      (mosquitto_property**)&property_bag->properties,
      (int)type,
      (const char*)az_span_ptr(prop_strpair->key),
      (const char*)az_span_ptr(prop_strpair->value)));
}

AZ_NODISCARD az_result az_mqtt5_property_bag_byte_append(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint8_t prop_byte)
{
  _az_PRECONDITION_NOT_NULL(property_bag);

  return _az_result_from_mosq(mosquitto_property_add_byte(
      (mosquitto_property**)&property_bag->properties, (int)type, prop_byte));
}

AZ_NODISCARD az_result az_mqtt5_property_bag_int_append(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint32_t prop_int)
{
  _az_PRECONDITION_NOT_NULL(property_bag);

  return _az_result_from_mosq(mosquitto_property_add_int32(
      (mosquitto_property**)&property_bag->properties, (int)type, prop_int));
}

AZ_NODISCARD az_result az_mqtt5_property_bag_binary_append(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_binarydata* prop_bindata)
{
  _az_PRECONDITION_NOT_NULL(property_bag);
  _az_PRECONDITION_NOT_NULL(prop_bindata);

  return _az_result_from_mosq(mosquitto_property_add_binary(
      (mosquitto_property**)&property_bag->properties,
      (int)type,
      (const void*)az_span_ptr(prop_bindata->bindata),
      (uint16_t)az_span_size(prop_bindata->bindata)));
}

AZ_NODISCARD az_result az_mqtt5_property_bag_string_read(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_string* out_prop_str)
{
  _az_PRECONDITION_NOT_NULL(property_bag);
  _az_PRECONDITION_NOT_NULL(out_prop_str);

  char* out_str = NULL;
  const mosquitto_property* prop = mosquitto_property_read_string(
      (const mosquitto_property*)property_bag->properties, (int)type, &out_str, false);

  if (prop == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  out_prop_str->str._internal.ptr = (uint8_t*)out_str;
  out_prop_str->str._internal.size = (int32_t)strlen(out_str);

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_stringpair_find(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_span key,
    az_mqtt5_property_stringpair* out_prop_strpair)
{
  _az_PRECONDITION_NOT_NULL(property_bag);
  _az_PRECONDITION_NOT_NULL(out_prop_strpair);

  const mosquitto_property* prop = NULL;
  const mosquitto_property* props = (const mosquitto_property*)property_bag->properties;
  int identifier;

  for (prop = props; prop != NULL; prop = mosquitto_property_next(prop))
  {
    identifier = mosquitto_property_identifier(prop);
    if (identifier == (int)type)
    {
      char* key_str = NULL;
      char* value_str = NULL;

      mosquitto_property_read_string_pair(prop, (int)type, &key_str, &value_str, false);

      if (az_span_is_content_equal(key, az_span_create_from_str(key_str)))
      {
        out_prop_strpair->key._internal.ptr = (uint8_t*)key_str;
        out_prop_strpair->key._internal.size = (int32_t)strlen(key_str);
        out_prop_strpair->value._internal.ptr = (uint8_t*)value_str;
        out_prop_strpair->value._internal.size = (int32_t)strlen(value_str);
        return AZ_OK;
      }
      free((void*)key_str);
      free((void*)value_str);
    }
  }

  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_byte_read(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint8_t* out_prop_byte)
{
  _az_PRECONDITION_NOT_NULL(property_bag);
  _az_PRECONDITION_NOT_NULL(out_prop_byte);

  const mosquitto_property* property = mosquitto_property_read_byte(
      (const mosquitto_property*)property_bag->properties, (int)type, out_prop_byte, false);
  if (property == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }
  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_int_read(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint32_t* out_prop_int)
{
  _az_PRECONDITION_NOT_NULL(property_bag);

  const mosquitto_property* property = mosquitto_property_read_int32(
      (const mosquitto_property*)property_bag->properties, (int)type, out_prop_int, false);
  if (property == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }
  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_binarydata_read(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_binarydata* out_prop_bindata)
{
  _az_PRECONDITION_NOT_NULL(property_bag);
  _az_PRECONDITION_NOT_NULL(out_prop_bindata);

  uint8_t* out_bin = NULL;
  uint16_t out_bin_size;
  const mosquitto_property* prop = mosquitto_property_read_binary(
      (const mosquitto_property*)property_bag->properties,
      (int)type,
      (void**)&out_bin,
      &out_bin_size,
      false);

  if (prop == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  out_prop_bindata->bindata._internal.ptr = out_bin;
  out_prop_bindata->bindata._internal.size = out_bin_size;

  return AZ_OK;
}

void az_mqtt5_property_string_free(az_mqtt5_property_string* prop_str)
{
  _az_PRECONDITION_NOT_NULL(prop_str);

  free((void*)prop_str->str._internal.ptr);
  prop_str->str._internal.ptr = NULL;
  prop_str->str._internal.size = 0;
}

void az_mqtt5_property_stringpair_free(az_mqtt5_property_stringpair* prop_strpair)
{
  _az_PRECONDITION_NOT_NULL(prop_strpair);

  free((void*)prop_strpair->key._internal.ptr);
  free((void*)prop_strpair->value._internal.ptr);
  prop_strpair->key._internal.ptr = NULL;
  prop_strpair->key._internal.size = 0;
  prop_strpair->value._internal.ptr = NULL;
  prop_strpair->value._internal.size = 0;
}

void az_mqtt5_property_binarydata_free(az_mqtt5_property_binarydata* prop_bindata)
{
  _az_PRECONDITION_NOT_NULL(prop_bindata);

  free(prop_bindata->bindata._internal.ptr);
  prop_bindata->bindata._internal.ptr = NULL;
  prop_bindata->bindata._internal.size = 0;
}

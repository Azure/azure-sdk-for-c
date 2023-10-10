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

AZ_INLINE az_result _az_result_from_mosq5(int mosquitto_ret)
{
  az_result ret;

  if (mosquitto_ret == MOSQ_ERR_SUCCESS)
  {
    ret = AZ_OK;
  }
  else
  {
    ret = _az_RESULT_MAKE_ERROR(_az_FACILITY_CORE_MQTT5, mosquitto_ret);
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

  _az_PRECONDITION(mosq == *me->_internal.mosquitto_handle);

  if (rc != 0)
  {
    /* If the connection fails for any reason, we don't want to keep on
     * retrying in this example, so disconnect. Without this, the client
     * will attempt to reconnect. */
    ret = mosquitto_disconnect(*me->_internal.mosquitto_handle);

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

  _az_PRECONDITION(mosq == *me->_internal.mosquitto_handle);

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
  (void)props;
  az_mqtt5* me = (az_mqtt5*)obj;

  _az_PRECONDITION(mosq == *me->_internal.mosquitto_handle);

  az_result ret
      = az_mqtt5_inbound_puback(me, &(az_mqtt5_puback_data){ .id = mid, .reason_code = rc });

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

  _az_PRECONDITION(mosq == *me->_internal.mosquitto_handle);

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
#ifdef AZ_NO_PRECONDITION_CHECKING
  (void)mosq;
#endif // AZ_NO_PRECONDITION_CHECKING
  (void)props;

  az_mqtt5* me = (az_mqtt5*)obj;

  _az_PRECONDITION(mosq == *me->_internal.mosquitto_handle);

  az_result ret = az_mqtt5_inbound_unsuback(me, &(az_mqtt5_unsuback_data){ .id = mid });

  if (az_result_failed(ret))
  {
    _az_mosquitto_critical_error();
  }
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

  _az_PRECONDITION(mosq == *me->_internal.mosquitto_handle);

  ret = az_mqtt5_property_bag_init(&property_bag, me, (mosquitto_property**)(uintptr_t)&props);

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
    .disable_tls_validation = false,
  };
}

AZ_NODISCARD az_result
az_mqtt5_init(az_mqtt5* mqtt5, struct mosquitto** mosquitto_handle, az_mqtt5_options const* options)
{
  _az_PRECONDITION_NOT_NULL(mqtt5);
  _az_PRECONDITION_NOT_NULL(mosquitto_handle);

  mqtt5->_internal.options = options == NULL ? az_mqtt5_options_default() : *options;
  mqtt5->_internal.mosquitto_handle = mosquitto_handle;
  mqtt5->_internal.platform_mqtt5.pipeline = NULL;

  return AZ_OK;
}

AZ_NODISCARD az_result
az_mqtt5_outbound_connect(az_mqtt5* mqtt5, az_mqtt5_connect_data* connect_data)
{
  _az_PRECONDITION_NOT_NULL(mqtt5);
  _az_PRECONDITION_NOT_NULL(connect_data);
  _az_PRECONDITION_VALID_SPAN(connect_data->host, 1, false);
  _az_PRECONDITION_VALID_SPAN(connect_data->client_id, 1, false);

  az_result ret = AZ_OK;
  az_mqtt5* me = (az_mqtt5*)mqtt5;

  // IMPORTANT: application must call mosquitto_lib_init() before any Mosquitto clients are created.

  if (*me->_internal.mosquitto_handle == NULL)
  {
    *me->_internal.mosquitto_handle = mosquitto_new(
        (char*)az_span_ptr(connect_data->client_id),
        false, // clean-session
        me); // callback context - i.e. user data.
  }

  if (*me->_internal.mosquitto_handle == NULL)
  {
    ret = AZ_ERROR_OUT_OF_MEMORY;
    return ret;
  }

  _az_RETURN_IF_FAILED(_az_result_from_mosq5(mosquitto_int_option(
      *me->_internal.mosquitto_handle, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5)));

  // Configure callbacks. This should be done before connecting ideally.
  mosquitto_log_callback_set(*me->_internal.mosquitto_handle, _az_mosquitto_on_log);
  mosquitto_connect_v5_callback_set(*me->_internal.mosquitto_handle, _az_mosquitto5_on_connect);
  mosquitto_disconnect_v5_callback_set(
      *me->_internal.mosquitto_handle, _az_mosquitto5_on_disconnect);
  mosquitto_publish_v5_callback_set(*me->_internal.mosquitto_handle, _az_mosquitto5_on_publish);
  mosquitto_subscribe_v5_callback_set(*me->_internal.mosquitto_handle, _az_mosquitto5_on_subscribe);
  mosquitto_unsubscribe_v5_callback_set(
      *me->_internal.mosquitto_handle, _az_mosquitto5_on_unsubscribe);
  mosquitto_message_v5_callback_set(*me->_internal.mosquitto_handle, _az_mosquitto5_on_message);

  if (me->_internal.options.disable_tls_validation == 0)
  {
    bool use_os_certs
        = (az_span_ptr(me->_internal.options.certificate_authority_trusted_roots) == NULL);

    if (use_os_certs)
    {
      _az_RETURN_IF_FAILED(_az_result_from_mosq5(
          mosquitto_int_option(*me->_internal.mosquitto_handle, MOSQ_OPT_TLS_USE_OS_CERTS, 1)));
    }

    _az_RETURN_IF_FAILED(_az_result_from_mosq5(mosquitto_tls_set(
        *me->_internal.mosquitto_handle,
        (const char*)az_span_ptr(me->_internal.options.certificate_authority_trusted_roots),
        use_os_certs ? REQUIRED_TLS_SET_CERT_PATH : NULL,
        (const char*)az_span_ptr(connect_data->certificate.cert),
        (const char*)az_span_ptr(connect_data->certificate.key),
        NULL)));
  }

  if (az_span_ptr(connect_data->username) != NULL)
  {
    _az_RETURN_IF_FAILED(_az_result_from_mosq5(mosquitto_username_pw_set(
        *me->_internal.mosquitto_handle,
        (const char*)az_span_ptr(connect_data->username),
        (const char*)az_span_ptr(connect_data->password))));
  }

  _az_RETURN_IF_FAILED(_az_result_from_mosq5(mosquitto_connect_async(
      *me->_internal.mosquitto_handle,
      (char*)az_span_ptr(connect_data->host),
      connect_data->port,
      AZ_MQTT5_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS)));

  _az_RETURN_IF_FAILED(
      _az_result_from_mosq5(mosquitto_loop_start(*me->_internal.mosquitto_handle)));

  return ret;
}

AZ_NODISCARD az_result az_mqtt5_outbound_sub(az_mqtt5* mqtt5, az_mqtt5_sub_data* sub_data)
{
  return _az_result_from_mosq5(mosquitto_subscribe_v5(
      *mqtt5->_internal.mosquitto_handle,
      &sub_data->out_id,
      (char*)az_span_ptr(sub_data->topic_filter),
      sub_data->qos,
      0,
      (sub_data->properties == NULL) ? NULL : *(sub_data->properties->mosq_properties)));
}

AZ_NODISCARD az_result az_mqtt5_outbound_unsub(az_mqtt5* mqtt5, az_mqtt5_unsub_data* unsub_data)
{
  return _az_result_from_mosq5(mosquitto_unsubscribe_v5(
      *mqtt5->_internal.mosquitto_handle,
      &unsub_data->out_id,
      (char*)az_span_ptr(unsub_data->topic_filter),
      (unsub_data->properties == NULL) ? NULL : *(unsub_data->properties->mosq_properties)));
}

AZ_NODISCARD az_result az_mqtt5_outbound_pub(az_mqtt5* mqtt5, az_mqtt5_pub_data* pub_data)
{
  return _az_result_from_mosq5(mosquitto_publish_v5(
      *mqtt5->_internal.mosquitto_handle,
      &pub_data->out_id,
      (char*)az_span_ptr(pub_data->topic), // Assumes properly formed NULL terminated string.
      az_span_size(pub_data->payload),
      az_span_ptr(pub_data->payload),
      pub_data->qos,
      false,
      (pub_data->properties == NULL) ? NULL : *(pub_data->properties->mosq_properties)));
}

AZ_NODISCARD az_result az_mqtt5_outbound_disconnect(az_mqtt5* mqtt5)
{
  return _az_result_from_mosq5(
      mosquitto_disconnect_v5(*mqtt5->_internal.mosquitto_handle, 0, NULL));
}

AZ_NODISCARD az_result az_mqtt5_property_bag_init(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5* mqtt5,
    mosquitto_property** mosq_properties)
{
  (void)mqtt5;
  property_bag->mosq_properties = mosq_properties;

  return AZ_OK;
}

void az_mqtt5_property_bag_clear(az_mqtt5_property_bag* property_bag)
{
  _az_PRECONDITION_NOT_NULL(property_bag);

  mosquitto_property_free_all(property_bag->mosq_properties);

  *(property_bag->mosq_properties) = NULL;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_append_string(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_span prop_str)
{
  _az_PRECONDITION_NOT_NULL(property_bag);
  _az_PRECONDITION_VALID_SPAN(prop_str, 1, false);

  return _az_result_from_mosq5(mosquitto_property_add_string(
      property_bag->mosq_properties, (int)type, (const char*)az_span_ptr(prop_str)));
}

AZ_NODISCARD az_result az_mqtt5_property_bag_append_stringpair(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_span prop_key,
    az_span prop_value)
{
  _az_PRECONDITION_NOT_NULL(property_bag);
  _az_PRECONDITION_VALID_SPAN(prop_key, 1, false);
  _az_PRECONDITION_VALID_SPAN(prop_value, 1, false);

  return _az_result_from_mosq5(mosquitto_property_add_string_pair(
      property_bag->mosq_properties,
      (int)type,
      (const char*)az_span_ptr(prop_key),
      (const char*)az_span_ptr(prop_value)));
}

AZ_NODISCARD az_result az_mqtt5_property_bag_append_byte(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint8_t prop_byte)
{
  _az_PRECONDITION_NOT_NULL(property_bag);

  return _az_result_from_mosq5(
      mosquitto_property_add_byte(property_bag->mosq_properties, (int)type, prop_byte));
}

AZ_NODISCARD az_result az_mqtt5_property_bag_append_int(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint32_t prop_int)
{
  _az_PRECONDITION_NOT_NULL(property_bag);

  return _az_result_from_mosq5(
      mosquitto_property_add_int32(property_bag->mosq_properties, (int)type, prop_int));
}

AZ_NODISCARD az_result az_mqtt5_property_bag_append_binary(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_span prop_bindata)
{
  _az_PRECONDITION_NOT_NULL(property_bag);
  _az_PRECONDITION_VALID_SPAN(prop_bindata, 1, false);

  return _az_result_from_mosq5(mosquitto_property_add_binary(
      property_bag->mosq_properties,
      (int)type,
      (const void*)az_span_ptr(prop_bindata),
      (uint16_t)az_span_size(prop_bindata)));
}

AZ_NODISCARD az_mqtt5_property_string
az_mqtt5_property_bag_read_string(az_mqtt5_property_bag* property_bag, az_mqtt5_property_type type)
{
  if (property_bag == NULL)
  {
    return NULL;
  }

  char* out_str = NULL;
  const mosquitto_property* prop = mosquitto_property_read_string(
      (const mosquitto_property*)*property_bag->mosq_properties, (int)type, &out_str, false);

  return prop == NULL ? NULL : out_str;
}

AZ_NODISCARD az_mqtt5_property_stringpair az_mqtt5_property_bag_find_stringpair(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_span key)
{
  if (property_bag == NULL)
  {
    return (az_mqtt5_property_stringpair){ .key = NULL, .value = NULL };
  }

  const mosquitto_property* prop = NULL;
  const mosquitto_property* props = (const mosquitto_property*)*property_bag->mosq_properties;
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
        return (az_mqtt5_property_stringpair){ .key = key_str, .value = value_str };
      }
      free((void*)key_str);
      free((void*)value_str);
    }
  }

  return (az_mqtt5_property_stringpair){ .key = NULL, .value = NULL };
}

AZ_NODISCARD az_result az_mqtt5_property_bag_read_byte(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint8_t* out_prop_byte)
{
  _az_PRECONDITION_NOT_NULL(out_prop_byte);
  if (property_bag == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  const mosquitto_property* property = mosquitto_property_read_byte(
      (const mosquitto_property*)*property_bag->mosq_properties, (int)type, out_prop_byte, false);
  if (property == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }
  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_read_int(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint32_t* out_prop_int)
{
  _az_PRECONDITION_NOT_NULL(out_prop_int);
  if (property_bag == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  const mosquitto_property* property = mosquitto_property_read_int32(
      (const mosquitto_property*)*property_bag->mosq_properties, (int)type, out_prop_int, false);
  if (property == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }
  return AZ_OK;
}

AZ_NODISCARD az_mqtt5_property_binarydata az_mqtt5_property_bag_read_binarydata(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type)
{
  if (property_bag == NULL)
  {
    return (az_mqtt5_property_binarydata){ .bindata = NULL, .bindata_length = 0 };
  }

  uint8_t* out_bin = NULL;
  uint16_t out_bin_size;
  const mosquitto_property* prop = mosquitto_property_read_binary(
      (const mosquitto_property*)*property_bag->mosq_properties,
      (int)type,
      (void**)&out_bin,
      &out_bin_size,
      false);

  return prop == NULL
      ? (az_mqtt5_property_binarydata){ .bindata = NULL, .bindata_length = 0 }
      : (az_mqtt5_property_binarydata){ .bindata = out_bin, .bindata_length = out_bin_size };
}

AZ_NODISCARD az_span az_mqtt5_property_get_string(az_mqtt5_property_string* prop_str)
{
  return *prop_str == NULL ? AZ_SPAN_EMPTY : az_span_create_from_str((char*)(uintptr_t)*prop_str);
}

AZ_NODISCARD az_span
az_mqtt5_property_stringpair_get_key(az_mqtt5_property_stringpair* prop_strpair)
{
  _az_PRECONDITION_NOT_NULL(prop_strpair);

  return prop_strpair->key == NULL ? AZ_SPAN_EMPTY : az_span_create_from_str(prop_strpair->key);
}

AZ_NODISCARD az_span
az_mqtt5_property_stringpair_get_value(az_mqtt5_property_stringpair* prop_strpair)
{
  _az_PRECONDITION_NOT_NULL(prop_strpair);

  return prop_strpair->value == NULL ? AZ_SPAN_EMPTY : az_span_create_from_str(prop_strpair->value);
}

AZ_NODISCARD az_span az_mqtt5_property_get_binarydata(az_mqtt5_property_binarydata* prop_bindata)
{
  _az_PRECONDITION_NOT_NULL(prop_bindata);

  return prop_bindata->bindata == NULL
      ? AZ_SPAN_EMPTY
      : az_span_create(prop_bindata->bindata, (int32_t)prop_bindata->bindata_length);
}

void az_mqtt5_property_read_free_string(az_mqtt5_property_string* prop_str)
{
  _az_PRECONDITION_NOT_NULL(prop_str);

  if (*prop_str != NULL)
  {
    free((void*)(uintptr_t)*prop_str);
    *prop_str = NULL;
  }
}

void az_mqtt5_property_read_free_stringpair(az_mqtt5_property_stringpair* prop_strpair)
{
  _az_PRECONDITION_NOT_NULL(prop_strpair);

  if (prop_strpair->key != NULL)
  {
    free((void*)prop_strpair->key);
    prop_strpair->key = NULL;
  }

  if (prop_strpair->value != NULL)
  {
    free((void*)prop_strpair->value);
    prop_strpair->value = NULL;
  }
}

void az_mqtt5_property_read_free_binarydata(az_mqtt5_property_binarydata* prop_bindata)
{
  _az_PRECONDITION_NOT_NULL(prop_bindata);

  if (prop_bindata->bindata != NULL)
  {
    free((void*)prop_bindata->bindata);
    prop_bindata->bindata = NULL;
    prop_bindata->bindata_length = 0;
  }
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Contains the az_mqtt5.h interface implementation with Eclipse Paho Async.
 * (https://github.com/eclipse/paho.mqtt.c).
 *
 * @note The Paho MQTT Async documentation is available at:
 * https://eclipse.github.io/paho.mqtt.c/MQTTAsync/html/
 */

#include <MQTTAsync.h>
#include <azure/core/az_mqtt5.h>
#include <azure/core/az_mqtt5_config.h>
#include <azure/core/az_mqtt5_property_bag.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/core/_az_cfg.h>

#define SERVER_URI_COLON_LENGTH 1
#define SERVER_URI_PORT_MAX_LENGTH 5

static void _az_pahoasync5_critical_error() { az_platform_critical_error(); }

AZ_INLINE az_result _az_result_from_pahoasync5(int pahoasync_ret)
{
  az_result ret;

  if (pahoasync_ret == MQTTASYNC_SUCCESS)
  {
    ret = AZ_OK;
  }
  else
  {
    ret = _az_RESULT_MAKE_ERROR(_az_FACILITY_CORE_MQTT5, pahoasync_ret);
  }

  return ret;
}

AZ_INLINE void _clear_pahoasync5_property(az_mqtt5_property_bag* property_bag)
{
  memset(&(property_bag->pahoasync_property), 0, sizeof(property_bag->pahoasync_property));
}

static void _az_pahoasync5_on_connect_success(void* client, MQTTAsync_successData5* response)
{
  az_result ret;
  az_mqtt5* me = (az_mqtt5*)client;

  ret = az_mqtt5_inbound_connack(
      me,
      &(az_mqtt5_connack_data){ .connack_reason = response->reasonCode,
                                .tls_authentication_error = false });

  if (az_result_failed(ret))
  {
    _az_pahoasync5_critical_error();
  }
}

static void _az_pahoasync5_on_connect_failure(void* client, MQTTAsync_failureData5* response)
{
  az_result ret;
  az_mqtt5* me = (az_mqtt5*)client;

  /* If the connection fails for any reason, we don't want to keep on
   * retrying in this example, so disconnect. Without this, the client
   * will attempt to reconnect. */
  ret = MQTTAsync_disconnect(*me->_internal.pahoasync_handle, NULL);

  if (ret != MQTTASYNC_SUCCESS && ret != MQTTASYNC_DISCONNECTED)
  {
    _az_pahoasync5_critical_error();
  }

  // TODO_L: Failure to connect returns a successful return code in response->reasonCode.
  // Investigation needed to determine if this is a bug in Paho.
  ret = az_mqtt5_inbound_connack(
      me,
      &(az_mqtt5_connack_data){ .connack_reason = response->code,
                                .tls_authentication_error = false });

  if (az_result_failed(ret))
  {
    _az_pahoasync5_critical_error();
  }
}

static void _az_pahoasync5_on_disconnect(void* client, char* cause)
{
  (void)cause; // cause is always set to NULL currently based on the Paho Async documentation.
  az_mqtt5* me = (az_mqtt5*)client;

  az_result ret = az_mqtt5_inbound_disconnect(
      me,
      &(az_mqtt5_disconnect_data){ .tls_authentication_error = false,
                                   .disconnect_requested = false });

  if (az_result_failed(ret))
  {
    _az_pahoasync5_critical_error();
  }
}

static void _az_pahoasync5_on_disconnect_success(void* client, MQTTAsync_successData5* response)
{
  az_mqtt5* me = (az_mqtt5*)client;

  az_result ret = az_mqtt5_inbound_disconnect(
      me,
      &(az_mqtt5_disconnect_data){ .tls_authentication_error = false,
                                   .disconnect_requested = (response->reasonCode == 0) });

  if (az_result_failed(ret))
  {
    _az_pahoasync5_critical_error();
  }
}

static void _az_pahoasync5_on_disconnect_failure(void* client, MQTTAsync_failureData5* response)
{
  (void)client;
  (void)response;

  // TODO_L: Implement logic to handle failing to disconnect.
  _az_pahoasync5_critical_error();
}

static void _az_pahoasync5_on_publish_success(void* client, MQTTAsync_successData5* response)
{
  az_mqtt5* me = (az_mqtt5*)client;

  az_result ret = az_mqtt5_inbound_puback(
      me, &(az_mqtt5_puback_data){ .id = response->token, .reason_code = response->reasonCode });

  if (az_result_failed(ret))
  {
    _az_pahoasync5_critical_error();
  }
}

static void _az_pahoasync5_on_publish_failure(void* client, MQTTAsync_failureData5* response)
{
  az_mqtt5* me = (az_mqtt5*)client;

  az_result ret = az_mqtt5_inbound_puback(
      me, &(az_mqtt5_puback_data){ .id = response->token, .reason_code = response->reasonCode });

  if (az_result_failed(ret))
  {
    _az_pahoasync5_critical_error();
  }
}

static void _az_pahoasync5_on_subscribe_success(void* client, MQTTAsync_successData5* response)
{
  az_mqtt5* me = (az_mqtt5*)client;

  az_result ret = az_mqtt5_inbound_suback(me, &(az_mqtt5_suback_data){ .id = response->token });

  if (az_result_failed(ret))
  {
    _az_pahoasync5_critical_error();
  }
}

static void _az_pahoasync5_on_subscribe_failure(void* client, MQTTAsync_failureData5* response)
{
  (void)client;
  (void)response;

  // TODO_L: Implement logic to handle failing to subscribe.
  _az_pahoasync5_critical_error();
}

static void _az_pahoasync5_on_unsubscribe_success(void* client, MQTTAsync_successData5* response)
{
  az_mqtt5* me = (az_mqtt5*)client;

  az_result ret = az_mqtt5_inbound_unsuback(me, &(az_mqtt5_unsuback_data){ .id = response->token });

  if (az_result_failed(ret))
  {
    _az_pahoasync5_critical_error();
  }
}

static void _az_pahoasync5_on_unsubscribe_failure(void* client, MQTTAsync_failureData5* response)
{
  (void)client;
  (void)response;

  // TODO_L: Implement logic to handle failing to unsubscribe.
  _az_pahoasync5_critical_error();
}

static int _az_pahoasync5_on_message(
    void* client,
    char* topicName,
    int topicLen,
    MQTTAsync_message* m)
{
  az_result ret;
  az_mqtt5* me = (az_mqtt5*)client;
  az_mqtt5_property_bag property_bag;

  ret = az_mqtt5_property_bag_init(&property_bag, me, &(m->properties));

  if (az_result_failed(ret))
  {
    _az_pahoasync5_critical_error();
  }

  ret = az_mqtt5_inbound_recv(
      me,
      &(az_mqtt5_recv_data){ .qos = (int8_t)m->qos,
                             .id = m->msgid,
                             .payload = az_span_create(m->payload, m->payloadlen),
                             .topic = az_span_create((uint8_t*)topicName, topicLen),
                             .properties = &property_bag });

  if (az_result_failed(ret))
  {
    _az_pahoasync5_critical_error();
  }

  MQTTAsync_freeMessage(&m);
  MQTTAsync_free(topicName);
  return 1;
}

AZ_NODISCARD az_mqtt5_options az_mqtt5_options_default()
{
  return (az_mqtt5_options){
    .certificate_authority_trusted_roots = AZ_SPAN_EMPTY,
    .disable_tls_validation = false,
  };
}

AZ_NODISCARD az_result
az_mqtt5_init(az_mqtt5* mqtt5, MQTTAsync* pahoasync_handle, az_mqtt5_options const* options)
{
  _az_PRECONDITION_NOT_NULL(mqtt5);
  _az_PRECONDITION_NOT_NULL(pahoasync_handle);

  mqtt5->_internal.options = options == NULL ? az_mqtt5_options_default() : *options;
  mqtt5->_internal.pahoasync_handle = pahoasync_handle;
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
  MQTTAsync_createOptions create_opts = MQTTAsync_createOptions_initializer5;
  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer5;
  MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;

  if (me->_internal.options.disable_tls_validation == false)
  {
    ssl_opts.verify = 1;
    bool use_os_certs
        = (az_span_ptr(me->_internal.options.certificate_authority_trusted_roots) == NULL);

    if (!use_os_certs)
    {
      ssl_opts.disableDefaultTrustStore = 1;
      ssl_opts.CApath
          = (const char*)az_span_ptr(me->_internal.options.certificate_authority_trusted_roots);
    }

    ssl_opts.keyStore = (const char*)az_span_ptr(connect_data->certificate.cert);
    ssl_opts.privateKey = (const char*)az_span_ptr(connect_data->certificate.key);
  }

  if (az_span_ptr(connect_data->username) != NULL)
  {
    conn_opts.username = (const char*)az_span_ptr(connect_data->username);
    conn_opts.password = (const char*)az_span_ptr(connect_data->password);
  }

  char port_buffer[SERVER_URI_PORT_MAX_LENGTH];
  sprintf(port_buffer, "%d", connect_data->port);
  az_span port_span = az_span_create_from_str(port_buffer);

  int32_t server_uri_length
      = az_span_size(connect_data->host) + SERVER_URI_COLON_LENGTH + SERVER_URI_PORT_MAX_LENGTH + 1;
  char server_uri_buffer[server_uri_length];
  az_span server_uri = az_span_create((uint8_t*)server_uri_buffer, server_uri_length);
  az_span server_uri_temp = server_uri;

  server_uri_temp = az_span_copy(server_uri_temp, connect_data->host);
  server_uri_temp = az_span_copy_u8(server_uri_temp, ':');
  server_uri_temp = az_span_copy(server_uri_temp, port_span);
  server_uri_temp = az_span_copy_u8(server_uri_temp, '\0');

  _az_RETURN_IF_FAILED(_az_result_from_pahoasync5(MQTTAsync_createWithOptions(
      me->_internal.pahoasync_handle,
      (const char*)az_span_ptr(server_uri),
      (const char*)az_span_ptr(connect_data->client_id),
      MQTTCLIENT_PERSISTENCE_NONE,
      NULL,
      &create_opts)));

  if (me->_internal.pahoasync_handle == NULL)
  {
    ret = AZ_ERROR_OUT_OF_MEMORY;
    return ret;
  }

  _az_RETURN_IF_FAILED(_az_result_from_pahoasync5(MQTTAsync_setCallbacks(
      *me->_internal.pahoasync_handle,
      mqtt5,
      _az_pahoasync5_on_disconnect,
      _az_pahoasync5_on_message,
      NULL)));

  conn_opts.keepAliveInterval = AZ_MQTT5_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;
  conn_opts.cleansession = false;
  conn_opts.onSuccess5 = _az_pahoasync5_on_connect_success;
  conn_opts.onFailure5 = _az_pahoasync5_on_connect_failure;
  conn_opts.context = me;
  conn_opts.ssl = &ssl_opts;

  _az_RETURN_IF_FAILED(
      _az_result_from_pahoasync5(MQTTAsync_connect(*me->_internal.pahoasync_handle, &conn_opts)));

  return ret;
}

AZ_NODISCARD az_result az_mqtt5_outbound_sub(az_mqtt5* mqtt5, az_mqtt5_sub_data* sub_data)
{
  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
  opts.onSuccess5 = _az_pahoasync5_on_subscribe_success;
  opts.onFailure5 = _az_pahoasync5_on_subscribe_failure;
  opts.context = mqtt5;

  _az_RETURN_IF_FAILED(_az_result_from_pahoasync5(MQTTAsync_subscribe(
      *mqtt5->_internal.pahoasync_handle,
      (const char*)az_span_ptr(sub_data->topic_filter),
      sub_data->qos,
      &opts)));

  sub_data->out_id = opts.token;

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_outbound_unsub(az_mqtt5* mqtt5, az_mqtt5_unsub_data* unsub_data)
{
  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
  opts.onSuccess5 = _az_pahoasync5_on_unsubscribe_success;
  opts.onFailure5 = _az_pahoasync5_on_unsubscribe_failure;
  opts.context = mqtt5;

  _az_RETURN_IF_FAILED(_az_result_from_pahoasync5(MQTTAsync_unsubscribe(
      *mqtt5->_internal.pahoasync_handle,
      (const char*)az_span_ptr(unsub_data->topic_filter),
      &opts)));

  unsub_data->out_id = opts.token;

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_outbound_pub(az_mqtt5* mqtt5, az_mqtt5_pub_data* pub_data)
{
  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
  MQTTAsync_message msg = MQTTAsync_message_initializer;

  opts.onSuccess5 = _az_pahoasync5_on_publish_success;
  opts.onFailure5 = _az_pahoasync5_on_publish_failure;
  opts.context = mqtt5;

  msg.payload = (void*)az_span_ptr(pub_data->payload);
  msg.payloadlen = az_span_size(pub_data->payload);
  msg.qos = pub_data->qos;
  if (pub_data->properties != NULL)
  {
    msg.properties = *(pub_data->properties->pahoasync_properties);
  }

  _az_RETURN_IF_FAILED(_az_result_from_pahoasync5(MQTTAsync_sendMessage(
      *mqtt5->_internal.pahoasync_handle, (const char*)az_span_ptr(pub_data->topic), &msg, &opts)));

  pub_data->out_id = opts.token;

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_outbound_disconnect(az_mqtt5* mqtt5)
{
  MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;

  opts.onSuccess5 = _az_pahoasync5_on_disconnect_success;
  opts.onFailure5 = _az_pahoasync5_on_disconnect_failure;
  opts.context = mqtt5;

  return _az_result_from_pahoasync5(
      MQTTAsync_disconnect(*mqtt5->_internal.pahoasync_handle, &opts));
}

AZ_NODISCARD az_result az_mqtt5_property_bag_init(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5* mqtt5,
    MQTTProperties* pahoasync_properties)
{
  (void)mqtt5;
  property_bag->pahoasync_properties = pahoasync_properties;

  return AZ_OK;
}

void az_mqtt5_property_bag_clear(az_mqtt5_property_bag* property_bag)
{
  _az_PRECONDITION_NOT_NULL(property_bag);

  MQTTProperties_free(property_bag->pahoasync_properties);
  property_bag->pahoasync_properties->count = 0;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_append_string(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_span prop_str)
{
  _az_PRECONDITION_NOT_NULL(property_bag);
  _az_PRECONDITION_VALID_SPAN(prop_str, 1, false);

  az_result ret = AZ_OK;

  property_bag->pahoasync_property.identifier = (enum MQTTPropertyCodes)type;
  property_bag->pahoasync_property.value.data.data = (char*)az_span_ptr(prop_str);
  property_bag->pahoasync_property.value.data.len = az_span_size(prop_str);

  ret = _az_result_from_pahoasync5(
      MQTTProperties_add(property_bag->pahoasync_properties, &(property_bag->pahoasync_property)));

  _clear_pahoasync5_property(property_bag);

  return ret;
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

  az_result ret = AZ_OK;

  property_bag->pahoasync_property.identifier = (enum MQTTPropertyCodes)type;
  property_bag->pahoasync_property.value.data.data = (char*)az_span_ptr(prop_key);
  property_bag->pahoasync_property.value.data.len = az_span_size(prop_key);
  property_bag->pahoasync_property.value.value.data = (char*)az_span_ptr(prop_value);
  property_bag->pahoasync_property.value.value.len = az_span_size(prop_value);

  ret = _az_result_from_pahoasync5(
      MQTTProperties_add(property_bag->pahoasync_properties, &(property_bag->pahoasync_property)));

  _clear_pahoasync5_property(property_bag);

  return ret;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_append_byte(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint8_t prop_byte)
{
  _az_PRECONDITION_NOT_NULL(property_bag);

  az_result ret = AZ_OK;

  property_bag->pahoasync_property.identifier = (enum MQTTPropertyCodes)type;
  property_bag->pahoasync_property.value.byte = prop_byte;

  ret = _az_result_from_pahoasync5(
      MQTTProperties_add(property_bag->pahoasync_properties, &(property_bag->pahoasync_property)));

  _clear_pahoasync5_property(property_bag);

  return ret;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_append_int(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint32_t prop_int)
{
  _az_PRECONDITION_NOT_NULL(property_bag);

  az_result ret = AZ_OK;

  property_bag->pahoasync_property.identifier = (enum MQTTPropertyCodes)type;
  property_bag->pahoasync_property.value.integer4 = prop_int; // TODO_L: Add append for two byte int

  ret = _az_result_from_pahoasync5(
      MQTTProperties_add(property_bag->pahoasync_properties, &(property_bag->pahoasync_property)));

  _clear_pahoasync5_property(property_bag);

  return ret;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_append_binary(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_span prop_bindata)
{
  _az_PRECONDITION_NOT_NULL(property_bag);
  _az_PRECONDITION_VALID_SPAN(prop_bindata, 1, false);

  az_result ret = AZ_OK;

  property_bag->pahoasync_property.identifier = (enum MQTTPropertyCodes)type;
  property_bag->pahoasync_property.value.data.data = (char*)az_span_ptr(prop_bindata);
  property_bag->pahoasync_property.value.data.len = az_span_size(prop_bindata);

  ret = _az_result_from_pahoasync5(
      MQTTProperties_add(property_bag->pahoasync_properties, &(property_bag->pahoasync_property)));

  _clear_pahoasync5_property(property_bag);

  return ret;
}

AZ_NODISCARD az_mqtt5_property_string
az_mqtt5_property_bag_read_string(az_mqtt5_property_bag* property_bag, az_mqtt5_property_type type)
{
  return property_bag == NULL ? NULL
                              : MQTTProperties_getProperty(
                                  property_bag->pahoasync_properties, (enum MQTTPropertyCodes)type);
}

AZ_NODISCARD az_mqtt5_property_stringpair az_mqtt5_property_bag_find_stringpair(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_span key)
{
  if (property_bag == NULL)
  {
    return NULL;
  }
  _az_PRECONDITION_VALID_SPAN(key, 1, false);
  MQTTProperty* prop = NULL;
  int stringpair_index = 0;

  for (prop = MQTTProperties_getPropertyAt(
           property_bag->pahoasync_properties, (enum MQTTPropertyCodes)type, stringpair_index);
       prop != NULL;
       prop = MQTTProperties_getPropertyAt(
           property_bag->pahoasync_properties, (enum MQTTPropertyCodes)type, stringpair_index++))
  {
    az_span prop_key = az_span_create((uint8_t*)prop->value.data.data, prop->value.data.len);
    if (az_span_is_content_equal(key, prop_key))
    {
      return prop;
    }
  }

  return prop;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_read_byte(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint8_t* out_prop_byte)
{
  if (property_bag == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }
  _az_PRECONDITION_NOT_NULL(out_prop_byte);
  MQTTProperty* prop;

  prop = MQTTProperties_getProperty(
      property_bag->pahoasync_properties, (enum MQTTPropertyCodes)type);

  if (prop == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  *out_prop_byte = prop->value.byte;

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_read_int(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint32_t* out_prop_int)
{
  if (property_bag == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }
  _az_PRECONDITION_NOT_NULL(out_prop_int);
  MQTTProperty* prop;

  prop = MQTTProperties_getProperty(
      property_bag->pahoasync_properties, (enum MQTTPropertyCodes)type);

  if (prop == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  // TODO_L: Add reading capabilities for integer 2.
  *out_prop_int = prop->value.integer4;

  return AZ_OK;
}

AZ_NODISCARD az_mqtt5_property_binarydata az_mqtt5_property_bag_read_binarydata(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type)
{
  return property_bag == NULL ? NULL
                              : MQTTProperties_getProperty(
                                  property_bag->pahoasync_properties, (enum MQTTPropertyCodes)type);
}

AZ_NODISCARD az_span az_mqtt5_property_get_string(az_mqtt5_property_string* prop_str)
{
  MQTTProperty* paho_prop_str = (MQTTProperty*)*prop_str;
  return paho_prop_str == NULL
      ? AZ_SPAN_EMPTY
      : az_span_create((uint8_t*)paho_prop_str->value.data.data, paho_prop_str->value.data.len);
}

AZ_NODISCARD az_span
az_mqtt5_property_stringpair_get_key(az_mqtt5_property_stringpair* prop_strpair)
{
  MQTTProperty* paho_prop_strpair = (MQTTProperty*)*prop_strpair;
  return paho_prop_strpair == NULL
      ? AZ_SPAN_EMPTY
      : az_span_create(
          (uint8_t*)paho_prop_strpair->value.data.data, paho_prop_strpair->value.data.len);
}

AZ_NODISCARD az_span
az_mqtt5_property_stringpair_get_value(az_mqtt5_property_stringpair* prop_strpair)
{
  MQTTProperty* paho_prop_strpair = (MQTTProperty*)*prop_strpair;
  return paho_prop_strpair == NULL
      ? AZ_SPAN_EMPTY
      : az_span_create(
          (uint8_t*)paho_prop_strpair->value.value.data, paho_prop_strpair->value.value.len);
}

AZ_NODISCARD az_span az_mqtt5_property_get_binarydata(az_mqtt5_property_binarydata* prop_bindata)
{
  MQTTProperty* paho_prop_bindata = (MQTTProperty*)*prop_bindata;
  return paho_prop_bindata == NULL
      ? AZ_SPAN_EMPTY
      : az_span_create(
          (uint8_t*)paho_prop_bindata->value.data.data, paho_prop_bindata->value.data.len);
}

void az_mqtt5_property_read_free_string(az_mqtt5_property_string* prop_str) { (void)prop_str; }

void az_mqtt5_property_read_free_stringpair(az_mqtt5_property_stringpair* prop_strpair)
{
  (void)prop_strpair;
}

void az_mqtt5_property_read_free_binarydata(az_mqtt5_property_binarydata* prop_bindata)
{
  (void)prop_bindata;
}

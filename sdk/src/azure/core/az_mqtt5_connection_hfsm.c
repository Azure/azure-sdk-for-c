// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_mqtt5_connection_config.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>
/**
 * @brief Convenience macro to transition connection to faulted
 */
#define _az_FAULTED_IF_FAILED(exp, conn, current_state, event)                                  \
  do                                                                                            \
  {                                                                                             \
    az_result const _az_result = (exp);                                                         \
    if (az_result_failed(_az_result))                                                           \
    {                                                                                           \
      if (_az_hfsm_transition_peer((_az_hfsm*)conn, current_state, faulted) != AZ_OK)           \
      {                                                                                         \
        az_platform_critical_error();                                                           \
      }                                                                                         \
      az_result const _az_result_inb = az_event_policy_send_inbound_event(                      \
          (az_event_policy*)conn,                                                               \
          (az_event){ .type = AZ_HFSM_EVENT_ERROR,                                              \
                      .data = &(az_hfsm_event_data_error){                                      \
                          .error_type = event.type, .sender = conn, .sender_event = event } }); \
      if (az_result_failed(_az_result_inb))                                                     \
      {                                                                                         \
        return _az_result_inb;                                                                  \
      }                                                                                         \
      return _az_result;                                                                        \
    }                                                                                           \
  } while (0)

static az_result root(az_event_policy* me, az_event event);

static az_result idle(az_event_policy* me, az_event event);
static az_result started(az_event_policy* me, az_event event);
static az_result faulted(az_event_policy* me, az_event event);

static az_result connecting(az_event_policy* me, az_event event);
static az_result connected(az_event_policy* me, az_event event);
static az_result disconnecting(az_event_policy* me, az_event event);
static az_result reconnect_timeout(az_event_policy* me, az_event event);

static az_event_policy_handler _get_parent(az_event_policy_handler child_state)
{
  az_event_policy_handler parent_state;

  if (child_state == root)
  {
    parent_state = NULL;
  }
  else if (child_state == idle || child_state == started || child_state == faulted)
  {
    parent_state = root;
  }
  else if (
      child_state == connecting || child_state == connected || child_state == disconnecting
      || child_state == reconnect_timeout)
  {
    parent_state = started;
  }
  else
  {
    // Unknown state.
    az_platform_critical_error();
    parent_state = NULL;
  }

  return parent_state;
}

static az_result root(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mqtt_connection"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // No-op.
      break;

    case AZ_HFSM_EVENT_ERROR:
      _az_FAULTED_IF_FAILED(
          az_event_policy_send_inbound_event(me, event), (az_mqtt5_connection*)me, root, event);
      break;

    case AZ_HFSM_EVENT_EXIT:
      if (_az_LOG_SHOULD_WRITE(AZ_HFSM_EVENT_EXIT))
      {
        _az_LOG_WRITE(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_mqtt_connection: PANIC!"));
      }

      az_platform_critical_error();
      break;

    case AZ_MQTT5_EVENT_PUB_REQ:
    case AZ_MQTT5_EVENT_SUB_REQ:
    case AZ_MQTT5_EVENT_UNSUB_REQ:
      _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event((az_event_policy*)me, event));
      break;

    default:
      // TODO_L Event filtering. We should not ignore events.
      break;
  }

  return ret;
}

static az_result faulted(az_event_policy* me, az_event event)
{
  (void)me;
  az_result ret = AZ_OK;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mqtt_connection/faulted"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      break;
    default:
      ret = AZ_ERROR_HFSM_INVALID_STATE;
      break;
  }

  return ret;
}

AZ_INLINE az_result _start_connect(az_mqtt5_connection* me)
{
  az_mqtt5_x509_client_certificate* current_client_certificate
      = &me->_internal.options.client_certificates
             [me->_internal.client_certificate_index
              % me->_internal.options.client_certificate_count];
  az_span current_client_certificate_cert = AZ_SPAN_EMPTY;
  az_span current_client_certificate_key = AZ_SPAN_EMPTY;

  if (me->_internal.options.client_certificate_count > 0)
  {
    current_client_certificate_cert = current_client_certificate->cert;
    current_client_certificate_key = current_client_certificate->key;
  }

  az_mqtt5_connect_data connect_data = (az_mqtt5_connect_data){
    .host = me->_internal.options.hostname,
    .port = me->_internal.options.port,
    .client_id = me->_internal.options.client_id_buffer,
    .username = me->_internal.options.username_buffer,
    .password = me->_internal.options.password_buffer,
    .certificate.cert = current_client_certificate_cert,
    .certificate.key = current_client_certificate_key,
    .properties = NULL,
  };

  _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event(
      (az_event_policy*)me,
      (az_event){
          .type = AZ_MQTT5_EVENT_CONNECT_REQ,
          .data = &connect_data,
      }));
  return AZ_OK;
}

static az_result idle(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_connection* this_policy = (az_mqtt5_connection*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mqtt5_connection/idle"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
      // No-op.
      break;

    case AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ:
      _az_FAULTED_IF_FAILED(
          _az_hfsm_transition_peer((_az_hfsm*)me, idle, started), this_policy, idle, event);
      _az_FAULTED_IF_FAILED(
          _az_hfsm_transition_substate((_az_hfsm*)me, started, connecting),
          this_policy,
          started,
          event);
      _az_RETURN_IF_FAILED(_start_connect(this_policy));
      break;

    default:
      // TODO_L: To add debuggability, call a macro here.
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE az_result _disconnect(az_mqtt5_connection* me)
{
  return az_event_policy_send_outbound_event(
      (az_event_policy*)me,
      (az_event){
          .type = AZ_MQTT5_EVENT_DISCONNECT_REQ,
          .data = NULL,
      });
}

static az_result started(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_connection* this_policy = (az_mqtt5_connection*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mqtt5_connection/started"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      this_policy->_internal.reconnect_counter = 0;
      break;

    case AZ_HFSM_EVENT_EXIT:
    case AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ:
      // No-op.
      break;

    case AZ_MQTT5_EVENT_CONNECT_RSP:
      // There should be no connect response in this state as a connect request has not been sent or
      // a disconnect response has been received.
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, started, faulted));
      break;

    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
      if (az_result_failed(_az_mqtt5_connection_api_callback(this_policy, event)))
      {
        // Callback failed: fault the connection object.
        _az_RETURN_IF_FAILED(_az_hfsm_send_event(
            (_az_hfsm*)me, (az_event){ .type = AZ_HFSM_EVENT_ERROR, .data = NULL }));
      }

      _az_FAULTED_IF_FAILED(
          _az_hfsm_transition_peer((_az_hfsm*)me, started, idle), this_policy, started, event);
      _az_RETURN_IF_FAILED(
          az_event_policy_send_inbound_event((az_event_policy*)this_policy, event));
      break;

    case AZ_HFSM_EVENT_ERROR:
    case AZ_HFSM_EVENT_TIMEOUT:
      if (event.data == &this_policy->_internal.connection_timer)
      {
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, started, faulted));
        _az_RETURN_IF_FAILED(az_event_policy_send_inbound_event(
            (az_event_policy*)this_policy,
            (az_event){ .type = AZ_HFSM_EVENT_ERROR,
                        .data = &(az_hfsm_event_data_error){ .error_type = event.type,
                                                             .sender = this_policy,
                                                             .sender_event = event } }));
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE int32_t _get_elapsed_time(int64_t current_time_msec, int64_t start_time_msec)
{
  int64_t elapsed_time_msec = current_time_msec - start_time_msec;
  if (elapsed_time_msec < 0 || elapsed_time_msec > INT32_MAX)
  {
    az_platform_critical_error();
  }
  return (int32_t)elapsed_time_msec;
}

static az_result connecting(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_connection* this_policy = (az_mqtt5_connection*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mqtt5_connection/started/connecting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    {
      if (az_result_failed(az_platform_clock_msec(&this_policy->_internal.connect_start_time_msec)))
      {
        az_platform_critical_error();
      }
      break;
    }
    case AZ_HFSM_EVENT_EXIT:
    {
      int64_t current_time_msec = 0;
      if (az_result_failed(az_platform_clock_msec(&current_time_msec)))
      {
        az_platform_critical_error();
      }
      this_policy->_internal.connect_time_msec
          = _get_elapsed_time(current_time_msec, this_policy->_internal.connect_start_time_msec);
      break;
    }
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    {
      az_mqtt5_connack_data* data = (az_mqtt5_connack_data*)event.data;

      if (data->connack_reason == 0)
      {
        _az_FAULTED_IF_FAILED(
            _az_hfsm_transition_peer((_az_hfsm*)me, connecting, connected),
            this_policy,
            connecting,
            event);
        _az_RETURN_IF_FAILED(
            az_event_policy_send_inbound_event((az_event_policy*)this_policy, event));
      }
      else
      {
        _az_FAULTED_IF_FAILED(
            _az_hfsm_transition_peer((_az_hfsm*)me, connecting, reconnect_timeout),
            this_policy,
            connecting,
            event);

        if (data->connack_reason == AZ_MQTT5_CONNACK_UNSPECIFIED_ERROR
            || data->connack_reason == AZ_MQTT5_CONNACK_NOT_AUTHORIZED
            || data->connack_reason == AZ_MQTT5_CONNACK_SERVER_BUSY
            || data->connack_reason == AZ_MQTT5_CONNACK_BANNED
            || data->connack_reason == AZ_MQTT5_CONNACK_BAD_AUTHENTICATION_METHOD)
        {
          this_policy->_internal.client_certificate_index++;
        }
      }

      if (az_result_failed(_az_mqtt5_connection_api_callback(this_policy, event)))
      {
        // Callback failed: fault the connection object.
        _az_RETURN_IF_FAILED(_az_hfsm_send_event(
            (_az_hfsm*)me, (az_event){ .type = AZ_HFSM_EVENT_ERROR, .data = NULL }));
      }
      break;
    }

    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
      // Transition occurs due to protocol violation, client sent connect request and server
      // replied with disconnect.
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, connecting, faulted));
      break;

    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
      _az_FAULTED_IF_FAILED(
          _az_hfsm_transition_peer((_az_hfsm*)me, connecting, disconnecting),
          this_policy,
          connecting,
          event);
      _az_RETURN_IF_FAILED(_disconnect(this_policy));
      break;

    case AZ_MQTT5_EVENT_PUB_REQ:
    case AZ_MQTT5_EVENT_SUB_REQ:
    case AZ_MQTT5_EVENT_UNSUB_REQ:
      ret = AZ_ERROR_HFSM_INVALID_STATE;
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE az_result _start_reconnect_timer(az_mqtt5_connection* conn, az_event event)
{
  az_result ret = AZ_OK;
  int32_t random_num = 0;
  _az_event_pipeline* pipeline = (_az_event_pipeline*)&conn->_internal.event_pipeline;
  _az_event_pipeline_timer* timer = (_az_event_pipeline_timer*)&conn->_internal.connection_timer;

  if (az_result_failed(az_platform_get_random(&random_num)))
  {
    az_platform_critical_error();
  }
  double normalized_random_num = random_num / (double)RAND_MAX;
  int32_t random_jitter_msec
      = (int32_t)(normalized_random_num * AZ_MQTT5_CONNECTION_MAX_RANDOM_JITTER_MSEC);

  ret = _az_event_pipeline_timer_create(pipeline, timer);
  if (az_result_failed(ret))
  {
    if (ret == AZ_ERROR_OUT_OF_MEMORY)
    {
      if (az_result_failed(_az_hfsm_transition_peer((_az_hfsm*)conn, reconnect_timeout, faulted)))
      {
        az_platform_critical_error();
      }
      _az_RETURN_IF_FAILED(az_event_policy_send_inbound_event(
          (az_event_policy*)conn,
          (az_event){ .type = ret,
                      .data = &(az_hfsm_event_data_error){
                          .error_type = event.type, .sender = conn, .sender_event = event } }));

      return ret;
    }
    else
    {
      az_platform_critical_error();
    }
  }

  conn->_internal.reconnect_counter++;
  if (conn->_internal.reconnect_counter > AZ_MQTT5_CONNECTION_MAX_CONNECT_ATTEMPTS
      && AZ_MQTT5_CONNECTION_MAX_CONNECT_ATTEMPTS != -1)
  {
    _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(
        conn, (az_event){ .type = AZ_EVENT_MQTT5_CONNECTION_RETRY_EXHAUSTED_IND, .data = NULL }));
    _az_FAULTED_IF_FAILED(
        _az_hfsm_transition_peer((_az_hfsm*)conn, reconnect_timeout, idle),
        conn,
        reconnect_timeout,
        event);
  }

  int32_t retry_delay_msec = conn->_internal.options.retry_delay_function(
      conn->_internal.reconnect_counter == 0 ? 0 : conn->_internal.connect_time_msec,
      conn->_internal.reconnect_counter,
      AZ_MQTT5_CONNECTION_MIN_RETRY_DELAY_MSEC,
      AZ_MQTT5_CONNECTION_MAX_RETRY_DELAY_MSEC,
      random_jitter_msec);

  if (az_result_failed(az_platform_timer_start(&timer->platform_timer, retry_delay_msec)))
  {
    az_platform_critical_error();
  }

  return ret;
}

static az_result reconnect_timeout(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_connection* this_policy = (az_mqtt5_connection*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mqtt5_connection/started/reconnect_timeout"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    {
      _start_reconnect_timer(this_policy, event);
      break;
    }

    case AZ_HFSM_EVENT_EXIT:
    {
      _az_event_pipeline_timer* timer
          = (_az_event_pipeline_timer*)&this_policy->_internal.connection_timer;
      if (az_result_failed(az_platform_timer_destroy(&timer->platform_timer)))
      {
        az_platform_critical_error();
      }
      break;
    }

    case AZ_HFSM_EVENT_TIMEOUT:
      if (event.data == &this_policy->_internal.connection_timer)
      {
        _az_FAULTED_IF_FAILED(
            _az_hfsm_transition_peer((_az_hfsm*)me, reconnect_timeout, connecting),
            this_policy,
            reconnect_timeout,
            event);
        _start_connect(this_policy);
      }
      break;

    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
      _az_FAULTED_IF_FAILED(
          _az_hfsm_transition_superstate((_az_hfsm*)me, reconnect_timeout, started),
          this_policy,
          reconnect_timeout,
          event);
      if (az_result_failed(_az_mqtt5_connection_api_callback(
              this_policy, (az_event){ .type = AZ_MQTT5_EVENT_DISCONNECT_RSP, .data = NULL })))
      {
        // Callback failed: fault the connection object.
        _az_RETURN_IF_FAILED(_az_hfsm_send_event(
            (_az_hfsm*)me, (az_event){ .type = AZ_HFSM_EVENT_ERROR, .data = NULL }));
      }
      _az_RETURN_IF_FAILED(az_event_policy_send_inbound_event(
          (az_event_policy*)this_policy,
          (az_event){ .type = AZ_MQTT5_EVENT_DISCONNECT_RSP, .data = NULL }));
      _az_FAULTED_IF_FAILED(
          _az_hfsm_transition_peer((_az_hfsm*)me, started, idle), this_policy, started, event);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result connected(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_connection* this_policy = (az_mqtt5_connection*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mqtt5_connection/started/connected"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      this_policy->_internal.reconnect_counter = 0;
      break;
    case AZ_HFSM_EVENT_TIMEOUT:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
      _az_FAULTED_IF_FAILED(
          _az_hfsm_transition_peer((_az_hfsm*)me, connected, connecting),
          this_policy,
          connected,
          event);
      _az_RETURN_IF_FAILED(
          az_event_policy_send_inbound_event((az_event_policy*)this_policy, event));
      _start_connect(this_policy);

      break;

    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
      _az_FAULTED_IF_FAILED(
          _az_hfsm_transition_substate((_az_hfsm*)me, connected, disconnecting),
          this_policy,
          connected,
          event);
      _az_RETURN_IF_FAILED(_disconnect((az_mqtt5_connection*)me));
      break;

    case AZ_MQTT5_EVENT_PUB_RECV_IND:
    case AZ_MQTT5_EVENT_PUBACK_RSP:
    case AZ_MQTT5_EVENT_SUBACK_RSP:
    case AZ_MQTT5_EVENT_UNSUBACK_RSP:
      _az_RETURN_IF_FAILED(az_event_policy_send_inbound_event((az_event_policy*)me, event));
      break;

    case AZ_MQTT5_EVENT_PUB_REQ:
    case AZ_MQTT5_EVENT_SUB_REQ:
    case AZ_MQTT5_EVENT_UNSUB_REQ:
      _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event((az_event_policy*)me, event));
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result disconnecting(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_connection* this_policy = (az_mqtt5_connection*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mqtt5_connection/started/disconnecting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    {
      _az_event_pipeline* pipeline = (_az_event_pipeline*)&this_policy->_internal.event_pipeline;
      _az_event_pipeline_timer* timer
          = (_az_event_pipeline_timer*)&this_policy->_internal.connection_timer;

      _az_RETURN_IF_FAILED(_az_event_pipeline_timer_create(pipeline, timer));
      if (az_result_failed(az_platform_timer_start(
              &timer->platform_timer, AZ_MQTT5_CONNECTION_DISCONNECT_HANDSHAKE_TIMEOUT_MSEC)))
      {
        az_platform_critical_error();
      }
      break;
    }

    case AZ_HFSM_EVENT_EXIT:
    {
      _az_event_pipeline_timer* timer
          = (_az_event_pipeline_timer*)&this_policy->_internal.connection_timer;
      if (az_result_failed(az_platform_timer_destroy(&timer->platform_timer)))
      {
        az_platform_critical_error();
      }
      break;
    }
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    {
      az_mqtt5_connack_data* data = (az_mqtt5_connack_data*)event.data;

      if (data->connack_reason == 0)
      {
        _disconnect(this_policy);
      }
      break;
    }
    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
      // No-op.
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_NODISCARD az_result _az_mqtt5_connection_policy_init(
    _az_hfsm* hfsm,
    az_event_policy* outbound,
    az_event_policy* inbound)
{
  _az_RETURN_IF_FAILED(_az_hfsm_init(hfsm, root, _get_parent, outbound, inbound));
  _az_RETURN_IF_FAILED(_az_hfsm_transition_substate(hfsm, root, idle));

  return AZ_OK;
}

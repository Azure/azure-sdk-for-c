// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

static az_result root(az_event_policy* me, az_event event);

static az_result idle(az_event_policy* me, az_event event);
static az_result started(az_event_policy* me, az_event event);
static az_result faulted(az_event_policy* me, az_event event);

static az_result connecting(az_event_policy* me, az_event event);
static az_result connected(az_event_policy* me, az_event event);
static az_result disconnecting(az_event_policy* me, az_event event);
static az_result reconnect_timeout(az_event_policy* me, az_event event);

static void _connection_timeout_callback(void* context)
{
  az_mqtt5_connection* this_policy = (az_mqtt5_connection*)context;

  if (_az_hfsm_send_event(
          (_az_hfsm*)this_policy,
          (az_event){
              .type = AZ_HFSM_EVENT_TIMEOUT,
              .data = NULL,
          })
      != AZ_OK)
  {
    az_platform_critical_error();
  }
}

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
      if (az_result_failed(az_event_policy_send_inbound_event(me, event)))
      {
        az_platform_critical_error();
      }
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
      // TODO_L Event filtering. We should not ignore events. Pipeline should not be sending them
      // down.
      break;
  }

  return ret;
}

static az_result faulted(az_event_policy* me, az_event event)
{
  az_result ret = AZ_ERROR_HFSM_INVALID_STATE;
  (void)me;
#ifdef AZ_NO_LOGGING
  (void)event;
#endif // AZ_NO_LOGGING

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mqtt_connection/faulted"));
  }

  return ret;
}

AZ_INLINE az_result _connect(az_mqtt5_connection* me)
{
  az_mqtt5_connect_data connect_data = (az_mqtt5_connect_data){
    .host = me->_internal.options.hostname,
    .port = me->_internal.options.port,
    .client_id = me->_internal.options.client_id_buffer,
    .username = me->_internal.options.username_buffer,
    .password = me->_internal.options.password_buffer,
    .certificate.cert = me->_internal.options.client_certificate_count == 0
        ? AZ_SPAN_EMPTY
        : me->_internal.options
              .client_certificates
                  [me->_internal.client_certificate_index
                   % me->_internal.options.client_certificate_count]
              .cert,
    .certificate.key = me->_internal.options.client_certificate_count == 0
        ? AZ_SPAN_EMPTY
        : me->_internal.options
              .client_certificates
                  [me->_internal.client_certificate_index
                   % me->_internal.options.client_certificate_count]
              .key,
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
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, idle, started));
      _az_RETURN_IF_FAILED(_az_hfsm_transition_substate((_az_hfsm*)me, started, connecting));
      _az_RETURN_IF_FAILED(_connect(this_policy));
      break;

    default:
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

    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, started, idle));
      break;

    case AZ_MQTT5_EVENT_CONNECT_RSP:
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, started, faulted));
      break;

    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
      if (az_result_failed(_az_mqtt5_connection_api_callback(this_policy, event)))
      {
        // Callback failed: fault the connection object.
        _az_RETURN_IF_FAILED(_az_hfsm_send_event(
            (_az_hfsm*)me, (az_event){ .type = AZ_HFSM_EVENT_ERROR, .data = NULL }));
      }

      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, started, idle));
      _az_RETURN_IF_FAILED(
          az_event_policy_send_inbound_event((az_event_policy*)this_policy, event));
      break;

    case AZ_HFSM_EVENT_ERROR:
    case AZ_HFSM_EVENT_TIMEOUT:
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, started, faulted));
      _az_RETURN_IF_FAILED(
          az_event_policy_send_inbound_event((az_event_policy*)this_policy, event));
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result connecting(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_connection* this_policy = (az_mqtt5_connection*)me;

  // Start and stop the timer on entering and exiting.

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mqtt5_connection/started/connecting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    {
      _az_RETURN_IF_FAILED(az_platform_clock_msec(&this_policy->_internal.connect_start_time_msec));
      break;
    }
    case AZ_HFSM_EVENT_EXIT:
    {
      int64_t current_time_msec = 0;
      _az_RETURN_IF_FAILED(az_platform_clock_msec(&current_time_msec));
      int64_t elapsed_time_msec
          = current_time_msec - this_policy->_internal.connect_start_time_msec;
      if (elapsed_time_msec < 0 || elapsed_time_msec > INT32_MAX)
      {
        az_platform_critical_error();
      }
      this_policy->_internal.connect_time_msec = (int32_t)elapsed_time_msec;
      break;
    }
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    {
      az_mqtt5_connack_data* data = (az_mqtt5_connack_data*)event.data;

      if (data->connack_reason == 0)
      {
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, connecting, connected));
        _az_RETURN_IF_FAILED(
            az_event_policy_send_inbound_event((az_event_policy*)this_policy, event));
      }
      else
      {
        _az_RETURN_IF_FAILED(
            _az_hfsm_transition_peer((_az_hfsm*)me, connecting, reconnect_timeout));
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
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, connecting, faulted));
      break;

    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, connecting, disconnecting));
      _az_RETURN_IF_FAILED(_disconnect(this_policy));
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
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
      int32_t random_num = 0;

      _az_RETURN_IF_FAILED(az_platform_get_random(&random_num));
      double normalized_random_num = random_num / (double)RAND_MAX;
      int32_t random_jitter_msec = (int32_t)(
          normalized_random_num * this_policy->_internal.options.max_random_jitter_msec);

      _az_RETURN_IF_FAILED(az_platform_timer_create(
          &this_policy->_internal.connection_timer, _connection_timeout_callback, this_policy));
      _az_RETURN_IF_FAILED(az_platform_timer_start(
          &this_policy->_internal.connection_timer,
          this_policy->_internal.options.retry_delay_function(
              this_policy->_internal.reconnect_counter == 0
                  ? 0
                  : this_policy->_internal.connect_time_msec,
              this_policy->_internal.reconnect_counter,
              this_policy->_internal.options.min_retry_delay_msec,
              this_policy->_internal.options.max_retry_delay_msec,
              random_jitter_msec)));

      this_policy->_internal.reconnect_counter++;
      if (this_policy->_internal.reconnect_counter
          > this_policy->_internal.options.max_connect_attempts)
      {
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, reconnect_timeout, faulted));
      }

      this_policy->_internal.client_certificate_index++;

      break;
    }

    case AZ_HFSM_EVENT_EXIT:
      _az_RETURN_IF_FAILED(az_platform_timer_destroy(&this_policy->_internal.connection_timer));
      break;

    case AZ_HFSM_EVENT_TIMEOUT:
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, reconnect_timeout, connecting));
      _connect(this_policy);
      break;

    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
      _az_RETURN_IF_FAILED(_az_hfsm_send_event(
          (_az_hfsm*)me, (az_event){ .type = AZ_MQTT5_EVENT_DISCONNECT_RSP, .data = NULL }));
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
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, connected, connecting));
      _az_RETURN_IF_FAILED(
          az_event_policy_send_inbound_event((az_event_policy*)this_policy, event));
      _connect(this_policy);

      break;

    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
      _az_RETURN_IF_FAILED(_az_hfsm_transition_substate((_az_hfsm*)me, connected, disconnecting));
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
      _az_RETURN_IF_FAILED(az_platform_timer_create(
          &this_policy->_internal.connection_timer, _connection_timeout_callback, this_policy));
      _az_RETURN_IF_FAILED(az_platform_timer_start(
          &this_policy->_internal.connection_timer,
          this_policy->_internal.disconnecting_timeout_msec));
      break;
    case AZ_HFSM_EVENT_EXIT:
      _az_RETURN_IF_FAILED(az_platform_timer_destroy(&this_policy->_internal.connection_timer));
      break;
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

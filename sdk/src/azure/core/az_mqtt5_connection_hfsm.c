// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>

#include <azure/core/_az_cfg.h>

static az_result root(az_event_policy* me, az_event event);

static az_result idle(az_event_policy* me, az_event event);
static az_result started(az_event_policy* me, az_event event);
// static az_result faulted(az_event_policy* me, az_event event);

static az_result connecting(az_event_policy* me, az_event event);
static az_result connected(az_event_policy* me, az_event event);
static az_result disconnecting(az_event_policy* me, az_event event);
// static az_result reconnect_timeout(az_event_policy* me, az_event event);

static az_event_policy_handler _get_parent(az_event_policy_handler child_state)
{
  az_event_policy_handler parent_state;

  if (child_state == root)
  {
    parent_state = NULL;
  }
  else if (child_state == idle || child_state == started) // ||  child_state == faulted) TODO_L:
                                                          // Unused, will implement later.
  {
    parent_state = root;
  }
  else if (child_state == connecting || child_state == connected || child_state == disconnecting)
  //|| child_state == reconnect_timeout) TODO_L: Unused, will implement later.
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

    default:
      // TODO_L Event filtering. We should not ignore events.
      break;
  }

  return ret;
}

/* TODO_L: Unused, will implement later.
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
*/

AZ_INLINE az_result _connect(az_mqtt5_connection* me)
{
  // TODO_L: Key rotation to be implemented. For now, using first one.
  _az_PRECONDITION_VALID_SPAN(me->_internal.options.hostname, 1, false);
  _az_PRECONDITION_VALID_SPAN(me->_internal.options.client_id_buffer, 1, false);
  _az_PRECONDITION_VALID_SPAN(me->_internal.options.username_buffer, 1, false);
  _az_PRECONDITION_VALID_SPAN(me->_internal.options.password_buffer, 0, true);
  _az_PRECONDITION_NOT_NULL(me->_internal.options.client_certificates);
  _az_PRECONDITION_VALID_SPAN(me->_internal.options.client_certificates[0].cert, 1, false);
  _az_PRECONDITION_VALID_SPAN(me->_internal.options.client_certificates[0].key, 1, false);

  az_mqtt5_connect_data connect_data = (az_mqtt5_connect_data){
    .host = me->_internal.options.hostname,
    .port = me->_internal.options.port,
    .client_id = me->_internal.options.client_id_buffer,
    .username = me->_internal.options.username_buffer,
    .password = me->_internal.options.password_buffer,
    .certificate.cert = me->_internal.options.client_certificates[0].cert,
    .certificate.key = me->_internal.options.client_certificates[0].key,
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
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, started, idle));
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

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mqtt5_connection/started/connecting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

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
        // TODO_L: Implement credential rotation & retry.
        _az_RETURN_IF_FAILED(_az_hfsm_send_event(
            (_az_hfsm*)me, (az_event){ .type = AZ_HFSM_EVENT_ERROR, .data = NULL }));

        _az_RETURN_IF_FAILED(
            az_event_policy_send_inbound_event((az_event_policy*)this_policy, event));
      }

      if (az_result_failed(_az_mqtt5_connection_api_callback(this_policy, event)))
      {
        // Callback failed: fault the connection object.
        _az_RETURN_IF_FAILED(_az_hfsm_send_event(
            (_az_hfsm*)me, (az_event){ .type = AZ_HFSM_EVENT_ERROR, .data = NULL }));
      }
      break;
    }

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

// TODO_L: Implement reconnect logic.
/*
static az_result reconnect_timeout(az_event_policy* me, az_event event)
{

  (void)me;
  (void)event;
  return AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;

  az_result ret = AZ_OK;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mqtt5_connection/started/reconnect_timeout"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // TODO_L: reconnect timer (apply reconnect policy), counter not implemented.
      break;

    case AZ_HFSM_EVENT_EXIT:
      // TODO_L
      break;

    case AZ_HFSM_EVENT_TIMEOUT:
      // TODO_L
      break;

    default:
      // TODO_L
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}
*/

static az_result connected(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mqtt5_connection/started/connected"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_TIMEOUT:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
      _az_RETURN_IF_FAILED(_az_hfsm_transition_substate((_az_hfsm*)me, connected, disconnecting));
      _az_RETURN_IF_FAILED(_disconnect((az_mqtt5_connection*)me));
      break;

    case AZ_MQTT5_EVENT_PUBACK_RSP:
      _az_RETURN_IF_FAILED(az_event_policy_send_inbound_event((az_event_policy*)me, event));
      // send to application to handle
      // if ((az_event_policy*)this_policy->inbound_policy != NULL)
      // {
      // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type =
      // AZ_EVENT_RPC_CLIENT_INVOKE_COMMAND_RSP, .data = data});
      // }
      _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback((az_mqtt5_connection*)me, event));
      break;
    case AZ_MQTT5_EVENT_PUB_RECV_IND:
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
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mqtt5_connection/started/disconnecting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
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

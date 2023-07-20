// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt.h>
#include <azure/core/az_mqtt_rpc_server.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdio.h>
#include <stdlib.h>

#include "mqtt_protocol.h"

#include <azure/core/_az_cfg.h>

static az_result root(az_event_policy* me, az_event event);
static az_result subscribing(az_event_policy* me, az_event event);
static az_result waiting(az_event_policy* me, az_event event);

AZ_INLINE az_result _handle_request(az_mqtt_rpc_server* me, az_mqtt_recv_data* data);

static az_event_policy_handler _get_parent(az_event_policy_handler child_state)
{
  az_event_policy_handler parent_state;

  if (child_state == root)
  {
    parent_state = NULL;
  }
  else if (child_state == subscribing || child_state == waiting)
  {
    parent_state = root;
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
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_server"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
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
        _az_LOG_WRITE(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_mqtt_rpc_server: PANIC!"));
      }

      az_platform_critical_error();
      break;

    case AZ_MQTT_EVENT_PUBACK_RSP:
    case AZ_EVENT_MQTT_CONNECTION_OPEN_REQ:
    case AZ_MQTT_EVENT_CONNECT_RSP:
    case AZ_EVENT_MQTT_CONNECTION_CLOSE_REQ:
      break;

    default:
      // TODO 
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_NODISCARD AZ_INLINE bool az_span_topic_matches_sub(az_span sub, az_span topic)
{
  bool ret;
  if (MOSQ_ERR_SUCCESS != mosquitto_topic_matches_sub(az_span_ptr(sub), az_span_ptr(topic), &ret))
  {
    ret = false;
  }
  return ret;
}

static az_result subscribing(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt_rpc_server* this_policy = (az_mqtt_rpc_server*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_server/subscribing"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // TODO: start timer
      break;

    case AZ_HFSM_EVENT_EXIT:
      // TODO: stop timer
      break;

    case AZ_MQTT_EVENT_SUBACK_RSP:
      // if get suback that matches the sub we sent, transition to waiting
      az_mqtt_suback_data* data = (az_mqtt_suback_data*)event.data;
      if (data->id == this_policy->_internal.options._az_mqtt_rpc_server_pending_sub_id)
      {
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, subscribing, waiting));
      }
      break;

    case AZ_MQTT_EVENT_PUB_RECV_IND:
      // if get relevent incoming publish (which implies that we're subscribed), transition to waiting
      az_mqtt_recv_data* recv_data = (az_mqtt_recv_data*)event.data;
      // Ensure pub is of the right topic
      if (az_span_topic_matches_sub(this_policy->_internal.options.sub_topic, recv_data->topic))
      {
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, subscribing, waiting));
        _az_RETURN_IF_FAILED(_handle_request(this_policy, recv_data));
      }
      break;

    case AZ_HFSM_EVENT_TIMEOUT:
      // resend sub request
      _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event((az_event_policy*)me, (az_event)
            { .type = AZ_MQTT_EVENT_SUB_REQ,
              .data = &(az_mqtt_sub_data)
                { .topic_filter = this_policy->_internal.options.sub_topic,
                  .qos = this_policy->_internal.options.sub_qos,
                  .out_id = &this_policy->_internal.options._az_mqtt_rpc_server_pending_sub_id
                }
            }));
      break;

    case AZ_MQTT_EVENT_PUBACK_RSP:
    case AZ_EVENT_MQTT_CONNECTION_OPEN_REQ:
    case AZ_MQTT_EVENT_CONNECT_RSP:
      break;

    default:
      // TODO 
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

// TEMPORARY FOR MOSQUITTO MQTT5 PROPERTIES WORKAROUND
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

AZ_INLINE az_result _build_response(az_mqtt_rpc_server* me, az_mqtt_pub_data *out_data, az_mqtt_rpc_status status, az_span payload)
{
  az_mqtt_rpc_server* this_policy = (az_mqtt_rpc_server*)me;

  out_data->props = NULL;

  _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_property_add_binary(&out_data->props, MQTT_PROP_CORRELATION_DATA, az_span_ptr(this_policy->_internal.options.pending_command.correlation_id), az_span_size(this_policy->_internal.options.pending_command.correlation_id))));
  char status_str[5];
  sprintf(status_str, "%d", status);
  _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_property_add_string_pair(&out_data->props, MQTT_PROP_USER_PROPERTY, "status", status_str)));
  _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_property_add_string(&out_data->props, MQTT_PROP_CONTENT_TYPE, "application/json")));

  out_data->topic = this_policy->_internal.options.pending_command.response_topic;
  out_data->payload = payload;
  out_data->qos = this_policy->_internal.options.response_qos;

  return AZ_OK;

}

AZ_INLINE az_result _build_finished_response(az_mqtt_rpc_server* me, az_event event, az_mqtt_pub_data *out_data)
{
  // add validation
  az_mqtt_rpc_server_execution_data* data = (az_mqtt_rpc_server_execution_data*)event.data;
  return _build_response(me, out_data, data->status, data->response);
}

AZ_INLINE az_result _build_error_response(az_mqtt_rpc_server* me, az_span error_message, az_mqtt_pub_data *out_data)
{
  // add handling for different error types
  return _build_response(me, out_data, AZ_MQTT_RPC_STATUS_SERVER_ERROR, error_message);
}

AZ_INLINE az_result _handle_request(az_mqtt_rpc_server* this_policy, az_mqtt_recv_data* data)
{
  char* response_topic;
  void* correlation_data;
  uint16_t correlation_data_len;
  
  // parse MQTT5 properties
  if (mosquitto_property_read_string(data->props, MQTT_PROP_RESPONSE_TOPIC, &response_topic, false)
      == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  if (mosquitto_property_read_binary(
            data->props, MQTT_PROP_CORRELATION_DATA, &correlation_data, &correlation_data_len, false)
      == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }
  this_policy->_internal.options.pending_command.correlation_id = az_span_create(correlation_data, correlation_data_len);
  this_policy->_internal.options.pending_command.response_topic = az_span_create_from_str(response_topic); 

  //error validation on presence of properties (correlation id, response topic)

  //validate request isn't expired?

  //deserialize request payload

  //send to app for execution
  // if ((az_event_policy*)this_policy->inbound_policy != NULL)
  // {
    // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type = AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND, .data = data});
  // }
  _az_RETURN_IF_FAILED(_az_mqtt_connection_api_callback(
    this_policy->_internal.connection,
    (az_event){ .type = AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND, .data = &this_policy->_internal.options.pending_command }));
    
  free(response_topic);
  free(correlation_data);
}


static az_result waiting(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt_rpc_server* this_policy = (az_mqtt_rpc_server*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_server/waiting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // No-op
      break;

    case AZ_MQTT_EVENT_PUB_RECV_IND:
      az_mqtt_recv_data* recv_data = (az_mqtt_recv_data*)event.data;
      // Ensure pub is of the right topic
      if (az_span_topic_matches_sub(this_policy->_internal.options.sub_topic, recv_data->topic))
      {
        _az_RETURN_IF_FAILED(_handle_request(this_policy, recv_data));
      }
      // start timer
      break;

    case AZ_EVENT_MQTT_RPC_SERVER_EXECUTION_FINISH:
      // check that correlation id matches
      // stop timer
      // clear pending correlation id
      // create response message/payload
      az_mqtt_pub_data data;
      _az_RETURN_IF_FAILED(_build_finished_response(this_policy, event, &data));
      
      // send publish
      _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event((az_event_policy*)me, (az_event){.type = AZ_MQTT_EVENT_PUB_REQ, .data = &data}));
      mosquitto_property_free_all(&data.props);
      // else log and ignore
      break;

    case AZ_HFSM_EVENT_TIMEOUT:
      // send response that command execution failed
      az_mqtt_pub_data timeout_pub_data;
      // TODO: "Command Server {_name} timeout after {_timeout}."
      _az_RETURN_IF_FAILED(_build_error_response(this_policy, AZ_SPAN_FROM_STR("Command Server timeout"), &timeout_pub_data));
      _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event((az_event_policy*)me, (az_event){.type = AZ_MQTT_EVENT_PUB_REQ, .data = &timeout_pub_data}));
      mosquitto_property_free_all(&timeout_pub_data.props);
      break;

    case AZ_MQTT_EVENT_SUBACK_RSP:
    case AZ_MQTT_EVENT_PUBACK_RSP:
    case AZ_EVENT_MQTT_CONNECTION_OPEN_REQ:
    case AZ_MQTT_EVENT_CONNECT_RSP:
      break;

    case AZ_HFSM_EVENT_EXIT:
      // No-op
      break;

    default:
      // TODO 
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_NODISCARD az_result
_az_rpc_server_policy_init(_az_hfsm* hfsm,
    _az_event_client* event_client,
    az_mqtt_connection* connection)
{
  _az_RETURN_IF_FAILED(_az_hfsm_init(hfsm, root, _get_parent, NULL, NULL));
  _az_RETURN_IF_FAILED(_az_hfsm_transition_substate(hfsm, root, subscribing));

  event_client->policy = (az_event_policy*)hfsm;
  _az_RETURN_IF_FAILED(
      _az_event_policy_collection_add_client(&connection->_internal.policy_collection, event_client));

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt_rpc_server_register(
    az_mqtt_rpc_server* client)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }

  return az_event_policy_send_outbound_event((az_event_policy*)client, (az_event)
    { .type = AZ_MQTT_EVENT_SUB_REQ,
      .data = &(az_mqtt_sub_data)
        { .topic_filter = client->_internal.options.sub_topic,
          .qos = client->_internal.options.sub_qos,
          .out_id = &client->_internal.options._az_mqtt_rpc_server_pending_sub_id
        }
    });
}

AZ_NODISCARD az_result az_rpc_server_init(
    az_mqtt_rpc_server* client,
    az_mqtt_connection* connection,
    az_mqtt_rpc_server_options* options)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(options->sub_topic, 1, false);
  // _az_PRECONDITION_VALID_SPAN(options->command_handled, 1, false);
  _az_PRECONDITION_VALID_SPAN(options->pending_command.correlation_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(options->pending_command.response_topic, 1, false);

   // client->_internal.options.command_handled = options->command_handled;
  client->_internal.options.pending_command.correlation_id = options->pending_command.correlation_id;
  client->_internal.options.pending_command.response_topic = options->pending_command.response_topic;

  client->_internal.options.sub_qos = 1;
  client->_internal.options.response_qos = 1;
  client->_internal.options.sub_topic = AZ_SPAN_FROM_STR("vehicles/dtmi:rpc:samples:vehicle;1/commands/vehicle03/unlock"); //TODO: generate

  client->_internal.connection = connection;

  // Initialize the stateful sub-client.
  if ((connection != NULL) && (az_span_size(connection->_internal.options.hostname) == 0))
  {
    // connection->_internal.options.hostname = AZ_SPAN_FROM_STR("hostname");
    connection->_internal.options.client_id_buffer = AZ_SPAN_FROM_STR("vehicle03");
    connection->_internal.options.username_buffer = AZ_SPAN_FROM_STR("vehicle03");
    connection->_internal.options.password_buffer = AZ_SPAN_EMPTY;

    _az_RETURN_IF_FAILED(_az_rpc_server_policy_init(
        (_az_hfsm*)client, &client->_internal.subclient, connection));
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt_rpc_server_execution_finish(
    az_mqtt_rpc_server* client,
    az_mqtt_rpc_server_execution_data* data)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }

  _az_PRECONDITION_VALID_SPAN(data->correlation_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(data->response_topic, 1, false);
  _az_PRECONDITION_VALID_SPAN(data->response, 1, false);
  // _az_PRECONDITION_VALID_SPAN(data->error_message, 1, false);

  return _az_event_pipeline_post_outbound_event(
      &client->_internal.connection->_internal.event_pipeline,
      (az_event){ .type = AZ_EVENT_MQTT_RPC_SERVER_EXECUTION_FINISH, .data = data });
}
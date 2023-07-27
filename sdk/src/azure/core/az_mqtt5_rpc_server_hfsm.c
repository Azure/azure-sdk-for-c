// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_mqtt5_rpc_server.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdio.h>
#include <stdlib.h>

#include "mqtt_protocol.h"

#include <azure/core/_az_cfg.h>

#define AZ_RPC_CONTENT_TYPE "application/json"

static az_result root(az_event_policy* me, az_event event);
static az_result subscribing(az_event_policy* me, az_event event);
static az_result waiting(az_event_policy* me, az_event event);

AZ_INLINE az_result _handle_request(az_mqtt5_rpc_server* me, az_mqtt5_recv_data* data);

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
        _az_LOG_WRITE(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_mqtt5_rpc_server: PANIC!"));
      }

      az_platform_critical_error();
      break;

    case AZ_MQTT5_EVENT_PUBACK_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ:
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
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
  az_mqtt5_rpc_server* this_policy = (az_mqtt5_rpc_server*)me;

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

    case AZ_MQTT5_EVENT_SUBACK_RSP:
      // if get suback that matches the sub we sent, transition to waiting
      az_mqtt5_suback_data* data = (az_mqtt5_suback_data*)event.data;
      if (data->id == this_policy->_internal.rpc_server_data._internal._az_mqtt5_rpc_server_pending_sub_id)
      {
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, subscribing, waiting));
      }
      break;

    case AZ_MQTT5_EVENT_PUB_RECV_IND:
      // if get relevent incoming publish (which implies that we're subscribed), transition to waiting
      az_mqtt5_recv_data* recv_data = (az_mqtt5_recv_data*)event.data;
      // Ensure pub is of the right topic
      if (az_span_topic_matches_sub(this_policy->_internal.options.sub_topic, recv_data->topic))
      {
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, subscribing, waiting));
        _az_RETURN_IF_FAILED(_handle_request(this_policy, recv_data));
      }
      break;

    case AZ_HFSM_EVENT_TIMEOUT:
      // resend sub request
      az_mqtt5_sub_data sub_data = { 
        .topic_filter = this_policy->_internal.options.sub_topic,
        .qos = this_policy->_internal.options.sub_qos,
        .out_id = 0
      };
      _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event((az_event_policy*)me, (az_event)
            { .type = AZ_MQTT5_EVENT_SUB_REQ,
              .data = &sub_data
            }));
      this_policy->_internal.rpc_server_data._internal._az_mqtt5_rpc_server_pending_sub_id = sub_data.out_id;
      break;

    case AZ_MQTT5_EVENT_PUBACK_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ:
    case AZ_MQTT5_EVENT_CONNECT_RSP:
      break;

    default:
      // TODO 
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE az_result _build_response(az_mqtt5_rpc_server* me, az_mqtt5_pub_data *out_data, az_mqtt5_rpc_status status, az_span payload)
{
  az_mqtt5_rpc_server* this_policy = (az_mqtt5_rpc_server*)me;
  char status_str[5];
  sprintf(status_str, "%d", status);

  az_mqtt5_property_string content_type
      = { .str = AZ_SPAN_FROM_STR(AZ_RPC_CONTENT_TYPE) };
  az_mqtt5_property_stringpair status_property
      = { .key = AZ_SPAN_FROM_STR("status"),
          .value = az_span_create_from_str(status_str) };

  _az_RETURN_IF_FAILED(az_mqtt5_property_bag_string_append(
          this_policy->_internal.rpc_server_data.property_bag,
          AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE,
          &content_type));
  _az_RETURN_IF_FAILED(az_mqtt5_property_bag_stringpair_append(
          this_policy->_internal.rpc_server_data.property_bag,
          AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
          &status_property));
  _az_RETURN_IF_FAILED(az_mqtt5_property_bag_binary_append(
          this_policy->_internal.rpc_server_data.property_bag,
          AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA,
          &this_policy->_internal.rpc_server_data._internal.pending_command.correlation_data_property));

  out_data->properties = this_policy->_internal.rpc_server_data.property_bag;
  out_data->topic = az_mqtt5_property_string_get(&this_policy->_internal.rpc_server_data._internal.pending_command.response_topic_property);
  out_data->payload = payload;
  out_data->qos = this_policy->_internal.options.response_qos;

  return AZ_OK;
}

AZ_INLINE az_result _build_finished_response(az_mqtt5_rpc_server* me, az_mqtt5_rpc_server_execution_data* event_data, az_mqtt5_pub_data *out_data)
{
  // add validation
  return _build_response(me, out_data, event_data->status, event_data->response);
}

AZ_INLINE az_result _build_error_response(az_mqtt5_rpc_server* me, az_span error_message, az_mqtt5_pub_data *out_data)
{
  // add handling for different error types
  return _build_response(me, out_data, AZ_MQTT5_RPC_STATUS_SERVER_ERROR, error_message);
}

AZ_INLINE az_result _handle_request(az_mqtt5_rpc_server* this_policy, az_mqtt5_recv_data* data)
{
  _az_PRECONDITION_NOT_NULL(data->properties);
  _az_PRECONDITION_NOT_NULL(this_policy);

  _az_RETURN_IF_FAILED(az_mqtt5_property_bag_string_read(
    data->properties,
    MQTT_PROP_RESPONSE_TOPIC,
    &this_policy->_internal.rpc_server_data._internal.pending_command.response_topic_property));

  _az_RETURN_IF_FAILED(az_mqtt5_property_bag_binary_read(
    data->properties,
    AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA,
    &this_policy->_internal.rpc_server_data._internal.pending_command.correlation_data_property));

  //validate request isn't expired?

  //deserialize request payload
  az_mqtt5_property_string content_type;
  _az_RETURN_IF_FAILED(az_mqtt5_property_bag_string_read(
      data->properties,
      AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE,
      &content_type));

  az_mqtt5_rpc_server_command_data command_data = (az_mqtt5_rpc_server_command_data){
    .correlation_id = az_mqtt5_property_binary_data_get(&this_policy->_internal.rpc_server_data._internal.pending_command.correlation_data_property),
    .response_topic = az_mqtt5_property_string_get(&this_policy->_internal.rpc_server_data._internal.pending_command.response_topic_property),
    .request_data = data->payload,
  };

  //send to app for execution
  // if ((az_event_policy*)this_policy->inbound_policy != NULL)
  // {
    // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type = AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND, .data = data});
  // }
  _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(
    this_policy->_internal.connection,
    (az_event){ .type = AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND, .data = &command_data }));
    
  az_mqtt5_property_bag_string_free(&content_type);
}

AZ_INLINE az_result _rpc_start_timer(az_mqtt5_rpc_server* me)
{
  _az_event_pipeline* pipeline = &me->_internal.connection->_internal.event_pipeline;
  _az_event_pipeline_timer* timer = &me->_internal.rpc_server_data._internal.rpc_execution_timer;

  _az_RETURN_IF_FAILED(_az_event_pipeline_timer_create(pipeline, timer));

  int32_t delay_milliseconds
      = (int32_t)me->_internal.rpc_server_data._internal.retry_after_seconds * 1000;
  if (delay_milliseconds <= 0)
  {
    delay_milliseconds = AZ_MQTT5_RPC_SERVER_MINIMUM_TIMEOUT_SECONDS * 1000;
  }

  _az_RETURN_IF_FAILED(az_platform_timer_start(&timer->platform_timer, delay_milliseconds));
}

AZ_INLINE az_result _rpc_stop_timer(az_mqtt5_rpc_server* me)
{
  _az_event_pipeline_timer* timer = &me->_internal.rpc_server_data._internal.rpc_execution_timer;
  return az_platform_timer_destroy(&timer->platform_timer);
}


static az_result waiting(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_rpc_server* this_policy = (az_mqtt5_rpc_server*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_server/waiting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // No-op
      break;

    case AZ_MQTT5_EVENT_PUB_RECV_IND:
      az_mqtt5_recv_data* recv_data = (az_mqtt5_recv_data*)event.data;
      // Ensure pub is of the right topic
      if (az_span_topic_matches_sub(this_policy->_internal.options.sub_topic, recv_data->topic))
      {
        _rpc_start_timer(this_policy);
        _az_RETURN_IF_FAILED(_handle_request(this_policy, recv_data));
      }
      // start timer
      break;

    case AZ_EVENT_MQTT5_RPC_SERVER_EXECUTION_FINISH:
      // check that correlation id matches
      az_mqtt5_rpc_server_execution_data* event_data = (az_mqtt5_rpc_server_execution_data*)event.data;
      if (az_span_is_content_equal(event_data->correlation_id, az_mqtt5_property_binary_data_get(&this_policy->_internal.rpc_server_data._internal.pending_command.correlation_data_property)) && az_span_is_content_equal(event_data->response_topic, az_mqtt5_property_string_get(&this_policy->_internal.rpc_server_data._internal.pending_command.response_topic_property)))
      {
        // stop timer
        _rpc_stop_timer(this_policy);
        // create response message/payload
        az_mqtt5_pub_data data;
        _az_RETURN_IF_FAILED(_build_finished_response(this_policy, event_data, &data));
        
        // send publish
        _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event((az_event_policy*)me, (az_event){.type = AZ_MQTT5_EVENT_PUB_REQ, .data = &data}));

        // clear pending command
        az_mqtt5_property_bag_binary_free(&this_policy->_internal.rpc_server_data._internal.pending_command.correlation_data_property);
        az_mqtt5_property_bag_string_free(&this_policy->_internal.rpc_server_data._internal.pending_command.response_topic_property);

        mosquitto_property_free_all(&this_policy->_internal.rpc_server_data.property_bag->_internal.options.properties);
        this_policy->_internal.rpc_server_data.property_bag->_internal.options.properties = NULL;
      }
      else{
        // log and ignore
      }
      break;

    case AZ_HFSM_EVENT_TIMEOUT:
      if (event.data == &this_policy->_internal.rpc_server_data._internal.rpc_execution_timer)
      {
        // send response that command execution failed
        az_mqtt5_pub_data timeout_pub_data;
        // TODO: "Command Server {_name} timeout after {_timeout}."
        _az_RETURN_IF_FAILED(_build_error_response(this_policy, AZ_SPAN_FROM_STR("Command Server timeout"), &timeout_pub_data));
        _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event((az_event_policy*)me, (az_event){.type = AZ_MQTT5_EVENT_PUB_REQ, .data = &timeout_pub_data}));

        // clear pending command
        az_mqtt5_property_bag_binary_free(&this_policy->_internal.rpc_server_data._internal.pending_command.correlation_data_property);
        az_mqtt5_property_bag_string_free(&this_policy->_internal.rpc_server_data._internal.pending_command.response_topic_property);

        mosquitto_property_free_all(&this_policy->_internal.rpc_server_data.property_bag->_internal.options.properties);
        this_policy->_internal.rpc_server_data.property_bag->_internal.options.properties = NULL;
      }
      break;

    case AZ_MQTT5_EVENT_SUBACK_RSP:
    case AZ_MQTT5_EVENT_PUBACK_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ:
    case AZ_MQTT5_EVENT_CONNECT_RSP:
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
    az_mqtt5_connection* connection)
{
  _az_RETURN_IF_FAILED(_az_hfsm_init(hfsm, root, _get_parent, NULL, NULL));
  _az_RETURN_IF_FAILED(_az_hfsm_transition_substate(hfsm, root, subscribing));

  event_client->policy = (az_event_policy*)hfsm;
  _az_RETURN_IF_FAILED(
      _az_event_policy_collection_add_client(&connection->_internal.policy_collection, event_client));

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_rpc_server_register(
    az_mqtt5_rpc_server* client)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }

  az_mqtt5_sub_data sub_data = { 
        .topic_filter = client->_internal.options.sub_topic,
        .qos = client->_internal.options.sub_qos,
        .out_id = 0
      };
  _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event((az_event_policy*)client, (az_event)
        { .type = AZ_MQTT5_EVENT_SUB_REQ,
          .data = &sub_data
        }));
      client->_internal.rpc_server_data._internal._az_mqtt5_rpc_server_pending_sub_id = sub_data.out_id;
    return AZ_OK;
}

AZ_NODISCARD az_result az_rpc_server_init(
    az_mqtt5_rpc_server* client,
    az_mqtt5_connection* connection,
    az_mqtt5_rpc_server_options* options,
    az_mqtt5_rpc_server_data* rpc_server_data)
{
  _az_PRECONDITION_NOT_NULL(client);
  // _az_PRECONDITION_NOT_NULL(options->property_bag);
  _az_PRECONDITION_VALID_SPAN(options->sub_topic, 1, false);
  _az_PRECONDITION_VALID_SPAN(options->command_name, 1, false);
  _az_PRECONDITION_VALID_SPAN(options->model_id, 1, false);

  client->_internal.options.command_name = options->command_name;
  client->_internal.options.model_id = options->model_id;
  client->_internal.rpc_server_data.property_bag = rpc_server_data->property_bag;

  client->_internal.options.sub_qos = 1;
  client->_internal.options.response_qos = 1;

  az_span temp_span = options->sub_topic;
  temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("vehicles/"));
  temp_span = az_span_copy(temp_span, client->_internal.options.model_id);
  temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("/commands/"));
  temp_span = az_span_copy(temp_span, connection->_internal.options.client_id_buffer);
  temp_span = az_span_copy_u8(temp_span, '/');
  temp_span = az_span_copy(temp_span, client->_internal.options.command_name);
  temp_span = az_span_copy_u8(temp_span, '\0');

  client->_internal.options.sub_topic = options->sub_topic;

  client->_internal.connection = connection;

  // Initialize the stateful sub-client.
  if ((connection != NULL))
  {
    _az_RETURN_IF_FAILED(_az_rpc_server_policy_init(
        (_az_hfsm*)client, &client->_internal.subclient, connection));
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_rpc_server_execution_finish(
    az_mqtt5_rpc_server* client,
    az_mqtt5_rpc_server_execution_data* data)
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
      (az_event){ .type = AZ_EVENT_MQTT5_RPC_SERVER_EXECUTION_FINISH, .data = data });
}
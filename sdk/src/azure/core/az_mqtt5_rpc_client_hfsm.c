// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_mqtt5_rpc_client.h>
#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdio.h>
#include <stdlib.h>

#include "mqtt_protocol.h"

#include <azure/core/_az_cfg.h>

static az_result root(az_event_policy* me, az_event event);
static az_result idle(az_event_policy* me, az_event event);
static az_result subscribing(az_event_policy* me, az_event event);
static az_result ready(az_event_policy* me, az_event event);
static az_result faulted(az_event_policy* me, az_event event);

AZ_NODISCARD az_result _az_rpc_client_hfsm_policy_init(
    _az_hfsm* hfsm,
    _az_event_client* event_client,
    az_mqtt5_connection* connection);

static az_event_policy_handler _get_parent(az_event_policy_handler child_state)
{
  az_event_policy_handler parent_state;

  if (child_state == root)
  {
    parent_state = NULL;
  }
  else if (child_state == idle || child_state == subscribing || child_state == ready || child_state == faulted)
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

AZ_INLINE az_mqtt5_rpc_client_rsp_event_data _parse_response(az_mqtt5_recv_data* recv_data,
  az_mqtt5_property_binarydata* correlation_data,
  az_mqtt5_property_stringpair* status,
  az_mqtt5_property_stringpair* error_message,
  az_mqtt5_property_string* content_type
  )
{
  // parse response
  printf("Received response: %s\n", az_span_ptr(recv_data->payload));

  az_mqtt5_rpc_client_rsp_event_data resp_data = {
    .response_payload = recv_data->payload,
    .parsing_failure = false,
    .status = AZ_MQTT5_RPC_STATUS_UNKNOWN,
    .error_message = AZ_SPAN_EMPTY,
    .content_type = AZ_SPAN_EMPTY,
    .correlation_id = AZ_SPAN_EMPTY
  };

  // az_mqtt5_property_binarydata correlation_data;
  if (az_result_failed(az_mqtt5_property_bag_read_binarydata(
      recv_data->properties, AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA, correlation_data)))
  {
    resp_data.parsing_failure = true;
    resp_data.error_message = AZ_SPAN_FROM_STR("Cannot process response message without CorrelationData");
    return resp_data;
  }
  resp_data.correlation_id = az_mqtt5_property_get_binarydata(correlation_data);
  printf("Processing response for: %s\n", az_span_ptr(resp_data.correlation_id));

  // read the status of the response
  // az_mqtt5_property_stringpair status;
  if (az_result_failed(az_mqtt5_property_bag_find_stringpair(
      recv_data->properties, AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY, AZ_SPAN_FROM_STR(AZ_MQTT5_RPC_STATUS_PROPERTY_NAME), status)))
  {
    resp_data.parsing_failure = true;
    resp_data.error_message = AZ_SPAN_FROM_STR("Response does not have the 'status' property.");
    return resp_data;
  }

  // parse status
  az_span status_str = az_mqtt5_property_stringpair_get_value(status);
  if (az_result_failed((az_span_atoi32(status_str, (int32_t*)&resp_data.status))))
  {
    resp_data.parsing_failure = true;
    resp_data.error_message = AZ_SPAN_FROM_STR("Status property contains invalid value.");
    return resp_data;
  }

  if (resp_data.status < 200 || resp_data.status >= 300 )
  {
    // read the error message if there is one
    // az_mqtt5_property_stringpair error_message;
    if (!az_result_failed(az_mqtt5_property_bag_find_stringpair(
        recv_data->properties, AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY, AZ_SPAN_FROM_STR(AZ_MQTT5_RPC_STATUS_MESSAGE_PROPERTY_NAME), error_message)))
    {
      resp_data.error_message = az_mqtt5_property_stringpair_get_value(error_message);
    }
  }
  else
  {
    if(az_span_is_content_equal(recv_data->payload, AZ_SPAN_EMPTY))
    {
      resp_data.parsing_failure = true;
      resp_data.error_message = AZ_SPAN_FROM_STR("Empty payload in 200");
      return resp_data;
    }
    // read the content type so the application can properly deserialize the request
    // az_mqtt5_property_string content_type;
    if (!az_result_failed(az_mqtt5_property_bag_read_string(
        recv_data->properties, AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE, content_type)))
    {
      resp_data.content_type = az_mqtt5_property_get_string(content_type);
    }
  }

  return resp_data;
}

static az_result root(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_rpc_client_hfsm* this_policy = (az_mqtt5_rpc_client_hfsm*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_client_hfsm"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      break;

    case AZ_HFSM_EVENT_ERROR:
    {
      _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(	
            this_policy->_internal.connection,	
            (az_event){ .type = AZ_HFSM_EVENT_ERROR, .data = event.data }));
      // TODO: uncomment when this won't fail every time
      // if (az_result_failed(az_event_policy_send_inbound_event(me, event)))
      // {
      //   az_platform_critical_error();
      // }
      break;
    }

    case AZ_HFSM_EVENT_EXIT:
    {
      if (_az_LOG_SHOULD_WRITE(AZ_HFSM_EVENT_EXIT))
      {
        _az_LOG_WRITE(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_mqtt5_rpc_client_hfsm: PANIC!"));
      }

      az_platform_critical_error();
      break;
    }

    case AZ_MQTT5_EVENT_PUBACK_RSP:
    case AZ_MQTT5_EVENT_SUBACK_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ:
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
    case AZ_MQTT5_EVENT_UNSUBACK_RSP:
      break;


    case AZ_MQTT5_EVENT_PUB_RECV_IND:
    {
      az_mqtt5_recv_data* recv_data = (az_mqtt5_recv_data*)event.data;

      // Ensure pub is of the right topic
      if (az_span_is_content_equal(this_policy->_internal.rpc_client->response_topic, recv_data->topic))
      {
        az_mqtt5_property_binarydata correlation_data;
        az_mqtt5_property_stringpair status;
        az_mqtt5_property_stringpair error_message;
        az_mqtt5_property_string content_type;

        az_mqtt5_rpc_client_rsp_event_data resp_data = _parse_response(recv_data, &correlation_data, &status, &error_message, &content_type);

        // send to application to handle
        // if ((az_event_policy*)this_policy->inbound_policy != NULL)	
        // {	
        // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type =	
        // AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND_REQ, .data = data});	
        // }	
        _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(	
            this_policy->_internal.connection,	
            (az_event){ .type = AZ_EVENT_RPC_CLIENT_RSP, .data = &resp_data }));

        az_mqtt5_property_free_binarydata(&correlation_data);
        az_mqtt5_property_free_stringpair(&status);
        if (resp_data.status < 200 || resp_data.status >= 300 )
        {
          // TODO: Might need to check if this isn't empty
          az_mqtt5_property_free_stringpair(&error_message);
        }
        else
        {
          az_mqtt5_property_free_string(&content_type);
        }
      }

      break;
    }

    default:
      // TODO
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

/**
 * @brief start subscription timer
 */
AZ_INLINE az_result _rpc_start_timer(az_mqtt5_rpc_client_hfsm* me)
{
  _az_event_pipeline* pipeline = &me->_internal.connection->_internal.event_pipeline;
  _az_event_pipeline_timer* timer = &me->_internal.rpc_client_timer;

  _az_RETURN_IF_FAILED(_az_event_pipeline_timer_create(pipeline, timer));

  int32_t delay_milliseconds = (int32_t)me->_internal.rpc_client->options.subscribe_timeout_in_seconds * 1000;

  _az_RETURN_IF_FAILED(az_platform_timer_start(&timer->platform_timer, delay_milliseconds));

  return AZ_OK;
}

/**
 * @brief stop subscription timer
 */
AZ_INLINE az_result _rpc_stop_timer(az_mqtt5_rpc_client_hfsm* me)
{
  _az_event_pipeline_timer* timer = &me->_internal.rpc_client_timer;
  return az_platform_timer_destroy(&timer->platform_timer);
}

static az_result idle(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_rpc_client_hfsm* this_policy = (az_mqtt5_rpc_client_hfsm*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_client_hfsm/idle"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      break;

    case AZ_MQTT5_EVENT_CONNECT_RSP:
    case AZ_MQTT5_EVENT_UNSUBACK_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ:
      // ignore
      break;

    case AZ_EVENT_RPC_CLIENT_SUB_REQ:
    {
      // Send subscribe
      az_mqtt5_sub_data subscription_data = { .topic_filter = this_policy->_internal.rpc_client->response_topic,
                                          .qos = this_policy->_internal.rpc_client->options.subscribe_qos,
                                          .out_id = 0 };
      // transition to subscribing
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer(&this_policy->_internal.rpc_client_policy, idle, subscribing));

      _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event(
        (az_event_policy*)this_policy,
        (az_event){ .type = AZ_MQTT5_EVENT_SUB_REQ, .data = &subscription_data }));
      this_policy->_internal.pending_subscription_id = subscription_data.out_id;
      break;
    }

    case AZ_EVENT_RPC_CLIENT_INVOKE_REQ:
    {
      return AZ_ERROR_HFSM_INVALID_STATE;
      break;
    }

    case AZ_MQTT5_EVENT_PUB_RECV_IND:
    {
      az_mqtt5_recv_data* recv_data = (az_mqtt5_recv_data*)event.data;

      // Ensure pub is of the right topic
      if (az_span_is_content_equal(this_policy->_internal.rpc_client->response_topic, recv_data->topic))
      {
        // transition straight to ready - we must already be subscribed
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, idle, ready));

        // handle the response (same in all states, so handle in root)
        ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      }
      break;
    }

    default:
      // TODO 
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result subscribing(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_rpc_client_hfsm* this_policy = (az_mqtt5_rpc_client_hfsm*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_client_hfsm/subscribing"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      _rpc_start_timer(this_policy);
      break;

    case AZ_HFSM_EVENT_EXIT:
      _rpc_stop_timer(this_policy);
      this_policy->_internal.pending_subscription_id = 0;
      break;

    case AZ_MQTT5_EVENT_SUBACK_RSP:
    {
      // if get suback that matches the sub we sent, stop waiting for the suback
      az_mqtt5_suback_data* data = (az_mqtt5_suback_data*)event.data;
      if (data->id == this_policy->_internal.pending_subscription_id)
      {
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, subscribing, ready));
      }
      break;
    }

    case AZ_HFSM_EVENT_TIMEOUT:
    {
      if (event.data == &this_policy->_internal.rpc_client_timer)
      {
        // if subscribing times out, go to faulted state - this is not recoverable
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, subscribing, faulted));
      }
      break;
    }

    case AZ_MQTT5_EVENT_PUB_RECV_IND:
    {
      az_mqtt5_recv_data* recv_data = (az_mqtt5_recv_data*)event.data;

      // Ensure pub is of the right topic
      if (az_span_is_content_equal(this_policy->_internal.rpc_client->response_topic, recv_data->topic))
      {
        // transition to ready - we must already be subscribed
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, subscribing, ready));

        // handle the response (same in all states, so handle in root)
        ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      }
      break;
    }

    case AZ_EVENT_RPC_CLIENT_INVOKE_REQ:
    {
      return AZ_ERROR_HFSM_INVALID_STATE;
      break;
    }

    case AZ_EVENT_RPC_CLIENT_SUB_REQ:
      // ignore
      break;

    default:
      // TODO 
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result ready(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_rpc_client_hfsm* this_policy = (az_mqtt5_rpc_client_hfsm*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_client_hfsm/ready"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // notify application that it can now send invoke requests
      // if ((az_event_policy*)this_policy->inbound_policy != NULL)	
      // {	
      // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type =	
      // AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND_REQ, .data = data});	
      // }	
      // TODO: figure out data type here that allows app to know what rpc client sent this
      _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(	
          this_policy->_internal.connection,
          (az_event){ .type = AZ_EVENT_RPC_CLIENT_READY_IND, .data = NULL }));
      break;

    case AZ_HFSM_EVENT_EXIT:
      break;

    case AZ_EVENT_RPC_CLIENT_SUB_REQ:
      // ignore
      break;

    case AZ_EVENT_RPC_CLIENT_UNSUB_REQ:
    {
      // Send unsubscribe
      az_mqtt5_unsub_data unsubscription_data = { .topic_filter = this_policy->_internal.rpc_client->response_topic };
      // transition to idle
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer(&this_policy->_internal.rpc_client_policy, ready, idle));

      _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event(
        (az_event_policy*)this_policy,
        (az_event){ .type = AZ_MQTT5_EVENT_UNSUB_REQ, .data = &unsubscription_data }));
      break;
    }

    case AZ_EVENT_RPC_CLIENT_INVOKE_REQ:
    {
      // TODO: need to make sure this is for this RPC client
      az_mqtt5_rpc_client_invoke_req_event_data* event_data
          = (az_mqtt5_rpc_client_invoke_req_event_data*)event.data;

      if (!_az_span_is_valid(event_data->correlation_id, 1, false))
      {
        return AZ_ERROR_ARG;
      }
      az_mqtt5_property_binarydata correlation_data = { .bindata = event_data->correlation_id };	
      _az_RETURN_IF_FAILED(az_mqtt5_property_bag_append_binary(	
          &this_policy->_internal.property_bag,	
          AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA,	
          &correlation_data));	

      if (!_az_span_is_valid(event_data->content_type, 1, false))
      {
        return AZ_ERROR_ARG;
      }
      az_mqtt5_property_string content_type = { .str = event_data->content_type };	
      _az_RETURN_IF_FAILED(az_mqtt5_property_bag_append_string(	
        &this_policy->_internal.property_bag, AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE, &content_type));	

      az_mqtt5_property_string response_topic_property = { .str = this_policy->_internal.rpc_client->response_topic };	
      _az_RETURN_IF_FAILED(az_mqtt5_property_bag_append_string(	
        &this_policy->_internal.property_bag, AZ_MQTT5_PROPERTY_TYPE_RESPONSE_TOPIC, &response_topic_property));	
      
      // send pub request
      az_mqtt5_pub_data data
       = (az_mqtt5_pub_data) {
        .topic = this_policy->_internal.rpc_client->request_topic,
        .qos = this_policy->_internal.rpc_client->options.request_qos,
        .payload = event_data->request_payload,
        .out_id = 0,
        .properties = &this_policy->_internal.property_bag,
       };
      // send publish
      ret = az_event_policy_send_outbound_event(
          (az_event_policy*)me, (az_event){ .type = AZ_MQTT5_EVENT_PUB_REQ, .data = &data });

      event_data->mid = data.out_id;

      // empty the property bag so it can be reused
      _az_RETURN_IF_FAILED(az_mqtt5_property_bag_clear(&this_policy->_internal.property_bag));

      break;
    }
    case AZ_MQTT5_EVENT_PUBACK_RSP:
        // send to application to handle
        // if ((az_event_policy*)this_policy->inbound_policy != NULL)	
        // {	
        // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type =	
        // AZ_EVENT_RPC_CLIENT_INVOKE_COMMAND_RSP, .data = data});	
        // }	
        _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(	
            this_policy->_internal.connection,
            (az_event){ .type = AZ_MQTT5_EVENT_PUBACK_RSP, .data = event.data }));
      // }
      
      break;

    case AZ_MQTT5_EVENT_PUB_RECV_IND:
    {
      // handle the response (same in all states, so handle in root)
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
    }

    default:
      // TODO 
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}


/**
 * @brief Failure state - locks up all execution of this hfsm
 */
static az_result faulted(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_rpc_client_hfsm* this_policy = (az_mqtt5_rpc_client_hfsm*)me;
#ifdef AZ_NO_LOGGING
  (void)event;
#endif // AZ_NO_LOGGING

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_client_hfsm/faulted"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    {
      _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(	
            this_policy->_internal.connection,	
            (az_event){ .type = AZ_HFSM_EVENT_ERROR, .data = NULL }));
      break;
    }
    default:
      ret = AZ_ERROR_HFSM_INVALID_STATE;
      break;
  }

  return ret;
}

AZ_NODISCARD az_result az_mqtt5_rpc_client_invoke_req(
    az_mqtt5_rpc_client_hfsm* client,
    az_mqtt5_rpc_client_invoke_req_event_data* data)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }

  return _az_event_pipeline_post_outbound_event(
      &client->_internal.connection->_internal.event_pipeline,
      (az_event){ .type = AZ_EVENT_RPC_CLIENT_INVOKE_REQ, .data = data });
}

AZ_NODISCARD az_result az_mqtt5_rpc_client_subscribe_req(az_mqtt5_rpc_client_hfsm* client)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }
  return _az_event_pipeline_post_outbound_event(
      &client->_internal.connection->_internal.event_pipeline,
      (az_event){ .type = AZ_EVENT_RPC_CLIENT_SUB_REQ, .data = NULL });
}

AZ_NODISCARD az_result az_mqtt5_rpc_client_unsubscribe_req(az_mqtt5_rpc_client_hfsm* client)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }
  return _az_event_pipeline_post_outbound_event(
      &client->_internal.connection->_internal.event_pipeline,
      (az_event){ .type = AZ_EVENT_RPC_CLIENT_UNSUB_REQ, .data = NULL });
}

AZ_NODISCARD az_result _az_rpc_client_hfsm_policy_init(
    _az_hfsm* hfsm,
    _az_event_client* event_client,
    az_mqtt5_connection* connection)
{
  _az_RETURN_IF_FAILED(_az_hfsm_init(hfsm, root, _get_parent, NULL, NULL));
  _az_RETURN_IF_FAILED(_az_hfsm_transition_substate(hfsm, root, idle));

  event_client->policy = (az_event_policy*)hfsm;
  _az_RETURN_IF_FAILED(_az_event_policy_collection_add_client(
      &connection->_internal.policy_collection, event_client));

  return AZ_OK;
}

AZ_NODISCARD az_result az_rpc_client_hfsm_init(
    az_mqtt5_rpc_client_hfsm* client,
    az_mqtt5_rpc_client* rpc_client,
    az_mqtt5_connection* connection,
    az_mqtt5_property_bag property_bag,
    az_span client_id,
    az_span model_id,
    az_span executor_client_id,
    az_span command_name,
    az_span response_topic_buffer,
    az_span request_topic_buffer,
    az_mqtt5_rpc_client_options* options)
{
  _az_PRECONDITION_NOT_NULL(client);

  // az_mqtt5_rpc_client rpc_client;
  client->_internal.rpc_client = rpc_client;
  
  _az_RETURN_IF_FAILED(az_rpc_client_init(client->_internal.rpc_client, client_id, model_id, executor_client_id, command_name, response_topic_buffer, request_topic_buffer, options));
  // client->_internal.rpc_client = &rpc_client;
  client->_internal.property_bag = property_bag;
  client->_internal.connection = connection;

  // Initialize the stateful sub-client.
  if ((connection != NULL))
  {
    _az_RETURN_IF_FAILED(
        _az_rpc_client_hfsm_policy_init((_az_hfsm*)client, &client->_internal.subclient, connection));
  }

  return AZ_OK;
}


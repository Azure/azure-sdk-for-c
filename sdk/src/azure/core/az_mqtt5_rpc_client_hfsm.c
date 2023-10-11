// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_mqtt5_rpc_client.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

#define _RETURN_AND_CLEAR_PROPERTY_BAG_IF_FAILED(exp, property_bag) \
  do                                                                \
  {                                                                 \
    az_result const _az_result = (exp);                             \
    if (az_result_failed(_az_result))                               \
    {                                                               \
      az_mqtt5_property_bag_clear(property_bag);                    \
      return _az_result;                                            \
    }                                                               \
  } while (0)

static az_result root(az_event_policy* me, az_event event);
static az_result idle(az_event_policy* me, az_event event);
static az_result subscribing(az_event_policy* me, az_event event);
static az_result ready(az_event_policy* me, az_event event);
static az_result publishing(az_event_policy* me, az_event event);
static az_result faulted(az_event_policy* me, az_event event);

AZ_NODISCARD az_result _az_mqtt5_rpc_client_hfsm_policy_init(
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
  else if (
      child_state == idle || child_state == subscribing || child_state == ready
      || child_state == faulted)
  {
    parent_state = root;
  }
  else if (child_state == publishing)
  {
    parent_state = ready;
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
  az_mqtt5_rpc_client* this_policy = (az_mqtt5_rpc_client*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_client_policy"));
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
        _az_LOG_WRITE(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_mqtt5_rpc_client: PANIC!"));
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
    case AZ_MQTT5_EVENT_RPC_CLIENT_UNSUB_REQ:
    case AZ_MQTT5_EVENT_PUB_RECV_IND:
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

/**
 * @brief If the incoming pub is of the correct topic, this transitions to the ready state if needed
 * and passes the event for the ready state to handle
 */
AZ_INLINE az_result send_to_ready_if_topic_matches(
    az_mqtt5_rpc_client* this_policy,
    az_event event,
    az_event_policy_handler source_state)
{
  az_mqtt5_recv_data* recv_data = (az_mqtt5_recv_data*)event.data;

  // Ensure pub is of the right topic
  if (_az_span_topic_matches_filter(
          this_policy->_internal.rpc_client_codec->_internal.subscription_topic, recv_data->topic))
  {
    // transition states if requested
    if (source_state != NULL)
    {
      _az_RETURN_IF_FAILED(
          _az_hfsm_transition_peer(&this_policy->_internal.rpc_client_hfsm, source_state, ready));
      // notify application that it can now send invoke requests
      // if ((az_event_policy*)this_policy->inbound_policy != NULL)
      // {
      // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type =
      // AZ_MQTT5_EVENT_RPC_CLIENT_READY_IND, .data = this_policy->_internal.rpc_client});
      // }
      _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(
          this_policy->_internal.connection,
          (az_event){ .type = AZ_MQTT5_EVENT_RPC_CLIENT_READY_IND,
                      .data = this_policy->_internal.rpc_client_codec }));

      // pass event to Ready to be processed there
      _az_RETURN_IF_FAILED(_az_hfsm_send_event(&this_policy->_internal.rpc_client_hfsm, event));
    }
  }
  return AZ_OK;
}

/**
 * @brief Transitions to idle if needed and sends an unsubscribe request
 */
AZ_INLINE az_result
unsubscribe(az_mqtt5_rpc_client* this_policy, az_event_policy_handler source_state)
{
  // transition states if requested
  if (source_state != NULL)
  {
    _az_RETURN_IF_FAILED(
        _az_hfsm_transition_peer(&this_policy->_internal.rpc_client_hfsm, source_state, idle));
  }

  // Send unsubscribe
  az_mqtt5_unsub_data unsubscription_data
      = { .topic_filter = this_policy->_internal.rpc_client_codec->_internal.subscription_topic };

  _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event(
      (az_event_policy*)this_policy,
      (az_event){ .type = AZ_MQTT5_EVENT_UNSUB_REQ, .data = &unsubscription_data }));

  return AZ_OK;
}

static az_result idle(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_rpc_client* this_policy = (az_mqtt5_rpc_client*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_client_policy/idle"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    case AZ_MQTT5_EVENT_UNSUBACK_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ:
      // ignore
      break;

    case AZ_MQTT5_EVENT_RPC_CLIENT_SUB_REQ:
    {
      // transition to subscribing
      _az_RETURN_IF_FAILED(
          _az_hfsm_transition_peer(&this_policy->_internal.rpc_client_hfsm, idle, subscribing));

      // Send subscribe
      az_mqtt5_sub_data subscription_data
          = { .topic_filter = this_policy->_internal.rpc_client_codec->_internal.subscription_topic,
              .qos = AZ_MQTT5_DEFAULT_RPC_QOS,
              .out_id = 0 };
      _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event(
          (az_event_policy*)this_policy,
          (az_event){ .type = AZ_MQTT5_EVENT_SUB_REQ, .data = &subscription_data }));

      // Save message id so we can correlate the suback later
      this_policy->_internal.pending_subscription_id = subscription_data.out_id;
      break;
    }

    case AZ_MQTT5_EVENT_RPC_CLIENT_INVOKE_REQ:
    {
      // can't invoke if not subscribed
      return AZ_ERROR_HFSM_INVALID_STATE;
      break;
    }

    case AZ_MQTT5_EVENT_PUB_RECV_IND:
    {
      // If the pub is of the right topic, we must already be subscribed so transition to ready &
      // send response to the application
      _az_RETURN_IF_FAILED(send_to_ready_if_topic_matches(this_policy, event, idle));

      break;
    }

    case AZ_MQTT5_EVENT_RPC_CLIENT_UNSUB_REQ:
    {
      // send unsubscribe if this request is for this policy. Doesn't change state
      _az_RETURN_IF_FAILED(unsubscribe(this_policy, NULL));
      break;
    }

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

/**
 * @brief start subscription/publishing timer
 */
AZ_INLINE az_result _rpc_start_timer(az_mqtt5_rpc_client* me, int32_t timeout_in_seconds)
{
  _az_event_pipeline* pipeline = &me->_internal.connection->_internal.event_pipeline;
  _az_event_pipeline_timer* timer = &me->_internal.rpc_client_timer;

  _az_RETURN_IF_FAILED(_az_event_pipeline_timer_create(pipeline, timer));

  int32_t delay_milliseconds = (int32_t)timeout_in_seconds * 1000;

  _az_RETURN_IF_FAILED(az_platform_timer_start(&timer->platform_timer, delay_milliseconds));

  return AZ_OK;
}

/**
 * @brief stop subscription/publishing timer
 */
AZ_INLINE az_result _rpc_stop_timer(az_mqtt5_rpc_client* me)
{
  _az_event_pipeline_timer* timer = &me->_internal.rpc_client_timer;
  return az_platform_timer_destroy(&timer->platform_timer);
}

static az_result subscribing(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_rpc_client* this_policy = (az_mqtt5_rpc_client*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_client_policy/subscribing"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      _rpc_start_timer(
          this_policy,
          this_policy->_internal.rpc_client_codec->_internal.options.subscribe_timeout_in_seconds);
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
        // notify application that it can now send invoke requests
        // if ((az_event_policy*)this_policy->inbound_policy != NULL)
        // {
        // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type =
        // AZ_MQTT5_EVENT_RPC_CLIENT_READY_IND, .data = this_policy->_internal.rpc_client});
        // }
        _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(
            this_policy->_internal.connection,
            (az_event){ .type = AZ_MQTT5_EVENT_RPC_CLIENT_READY_IND,
                        .data = this_policy->_internal.rpc_client_codec }));
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
      // If the pub is of the right topic, we must already be subscribed so transition to ready &
      // send response to the application
      _az_RETURN_IF_FAILED(send_to_ready_if_topic_matches(this_policy, event, subscribing));

      break;
    }

    case AZ_MQTT5_EVENT_RPC_CLIENT_UNSUB_REQ:
    {
      // send unsubscribe and transition to idle if this request is for this policy
      _az_RETURN_IF_FAILED(unsubscribe(this_policy, subscribing));
      break;
    }

    case AZ_MQTT5_EVENT_RPC_CLIENT_INVOKE_REQ:
    {
      // can't invoke until subscribed
      return AZ_ERROR_HFSM_INVALID_STATE;
      break;
    }

    case AZ_MQTT5_EVENT_RPC_CLIENT_SUB_REQ:
      // already subscribing, application doesn't need to take any action
      ret = AZ_OK;
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE az_result
send_resp_inbound_if_topic_matches(az_mqtt5_rpc_client* this_policy, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_recv_data* recv_data = (az_mqtt5_recv_data*)event.data;

  if (_az_span_topic_matches_filter(
          this_policy->_internal.rpc_client_codec->_internal.subscription_topic, recv_data->topic))
  {
    // Reading properties
    az_mqtt5_property_binarydata correlation_data = az_mqtt5_property_bag_read_binarydata(
        recv_data->properties, AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA);
    az_mqtt5_property_stringpair status = az_mqtt5_property_bag_find_stringpair(
        recv_data->properties,
        AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
        AZ_SPAN_FROM_STR(AZ_MQTT5_RPC_STATUS_PROPERTY_NAME));
    az_mqtt5_property_stringpair error_message = az_mqtt5_property_bag_find_stringpair(
        recv_data->properties,
        AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
        AZ_SPAN_FROM_STR(AZ_MQTT5_RPC_STATUS_MESSAGE_PROPERTY_NAME));
    az_mqtt5_property_string content_type = az_mqtt5_property_bag_read_string(
        recv_data->properties, AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE);

    az_span status_val_str = az_mqtt5_property_stringpair_get_value(&status);
    az_span error_message_val_str = az_mqtt5_property_stringpair_get_value(&error_message);

    az_mqtt5_rpc_client_rsp_event_data resp_data
        = { .response_payload = recv_data->payload,
            .status = AZ_MQTT5_RPC_STATUS_UNKNOWN,
            .error_message = AZ_SPAN_EMPTY,
            .content_type = az_mqtt5_property_get_string(&content_type),
            .correlation_id = az_mqtt5_property_get_binarydata(&correlation_data) };

    az_result rc = AZ_OK;
    if (az_span_is_content_equal(resp_data.correlation_id, AZ_SPAN_EMPTY))
    {
      resp_data.error_message
          = AZ_SPAN_FROM_STR("Cannot process response message without CorrelationData");
      rc = AZ_ERROR_ITEM_NOT_FOUND;
    }
    else if (az_span_is_content_equal(status_val_str, AZ_SPAN_EMPTY))
    {
      resp_data.error_message = AZ_SPAN_FROM_STR("Response does not have the 'status' property.");
      rc = AZ_ERROR_ITEM_NOT_FOUND;
    }
    else if (az_result_failed((az_span_atoi32(status_val_str, (int32_t*)&resp_data.status))))
    {
      resp_data.error_message = AZ_SPAN_FROM_STR("Status property contains invalid value.");
      rc = AZ_ERROR_UNEXPECTED_CHAR;
    }
    else if (az_mqtt5_rpc_status_failed(resp_data.status))
    {
      // read the error message if there is one
      resp_data.error_message = error_message_val_str;
    }
    else if (az_span_is_content_equal(recv_data->payload, AZ_SPAN_EMPTY))
    {
      resp_data.error_message = AZ_SPAN_FROM_STR("Empty payload in 200");
      rc = AZ_ERROR_ITEM_NOT_FOUND;
    }
    else if (az_span_is_content_equal(resp_data.content_type, AZ_SPAN_EMPTY))
    {
      resp_data.error_message
          = AZ_SPAN_FROM_STR("Response does not have the 'content type' property.");
      rc = AZ_ERROR_ITEM_NOT_FOUND;
    }

    // TODO_L: send to application to handle
    // if ((az_event_policy*)this_policy->inbound_policy != NULL)
    // {
    // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type =
    // AZ_MQTT5_EVENT_RPC_SERVER_EXECUTE_COMMAND_REQ, .data = &resp_data});
    // }
    ret = _az_mqtt5_connection_api_callback(
        this_policy->_internal.connection,
        (az_event){ .type = az_result_failed(rc) ? AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP
                                                 : AZ_MQTT5_EVENT_RPC_CLIENT_RSP,
                    .data = &resp_data });

    az_mqtt5_property_read_free_binarydata(&correlation_data);
    az_mqtt5_property_read_free_stringpair(&status);
    az_mqtt5_property_read_free_stringpair(&error_message);
    az_mqtt5_property_read_free_string(&content_type);
  }

  return ret;
}

static az_result ready(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_rpc_client* this_policy = (az_mqtt5_rpc_client*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_client_policy/ready"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      break;

    case AZ_HFSM_EVENT_EXIT:
      break;

    case AZ_MQTT5_EVENT_RPC_CLIENT_SUB_REQ:
    {
      // already subscribed, application doesn't need to wait for
      // AZ_MQTT5_EVENT_RPC_CLIENT_READY_IND to invoke commands
      ret = AZ_ERROR_HFSM_INVALID_STATE;
      break;
    }

    case AZ_MQTT5_EVENT_RPC_CLIENT_UNSUB_REQ:
    {
      // send unsubscribe and transition to idle if this request is for this policy
      _az_RETURN_IF_FAILED(unsubscribe(this_policy, ready));
      break;
    }

    case AZ_MQTT5_EVENT_RPC_CLIENT_INVOKE_REQ:
    {
      az_mqtt5_rpc_client_invoke_req_event_data* event_data
          = (az_mqtt5_rpc_client_invoke_req_event_data*)event.data;

      if (!_az_span_is_valid(event_data->correlation_id, 1, false))
      {
        return AZ_ERROR_ARG;
      }

      _RETURN_AND_CLEAR_PROPERTY_BAG_IF_FAILED(
          az_mqtt5_property_bag_append_binary(
              &this_policy->_internal.property_bag,
              AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA,
              event_data->correlation_id),
          &this_policy->_internal.property_bag);

      if (!_az_span_is_valid(event_data->content_type, 1, false))
      {
        az_mqtt5_property_bag_clear(&this_policy->_internal.property_bag);
        return AZ_ERROR_ARG;
      }

      _RETURN_AND_CLEAR_PROPERTY_BAG_IF_FAILED(
          az_mqtt5_property_bag_append_string(
              &this_policy->_internal.property_bag,
              AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE,
              event_data->content_type),
          &this_policy->_internal.property_bag);

      if (!_az_span_is_valid(event_data->rpc_server_client_id, 1, false))
      {
        az_mqtt5_property_bag_clear(&this_policy->_internal.property_bag);
        return AZ_ERROR_ARG;
      }
      int32_t response_topic_length;
      _RETURN_AND_CLEAR_PROPERTY_BAG_IF_FAILED(
          az_mqtt5_rpc_client_codec_get_response_topic(
              this_policy->_internal.rpc_client_codec,
              event_data->rpc_server_client_id,
              _az_span_is_valid(event_data->command_name, 1, 0) ? event_data->command_name
                                                                : AZ_SPAN_EMPTY,
              this_policy->_internal.rpc_client_codec->_internal.response_topic_buffer,
              &response_topic_length),
          &this_policy->_internal.property_bag);
      az_span response_topic_property_span = az_span_create(
          az_span_ptr(this_policy->_internal.rpc_client_codec->_internal.response_topic_buffer),
          response_topic_length - 1);
      _RETURN_AND_CLEAR_PROPERTY_BAG_IF_FAILED(
          az_mqtt5_property_bag_append_string(
              &this_policy->_internal.property_bag,
              AZ_MQTT5_PROPERTY_TYPE_RESPONSE_TOPIC,
              response_topic_property_span),
          &this_policy->_internal.property_bag);

      _RETURN_AND_CLEAR_PROPERTY_BAG_IF_FAILED(
          az_mqtt5_rpc_client_codec_get_request_topic(
              this_policy->_internal.rpc_client_codec,
              event_data->rpc_server_client_id,
              _az_span_is_valid(event_data->command_name, 1, 0) ? event_data->command_name
                                                                : AZ_SPAN_EMPTY,
              this_policy->_internal.rpc_client_codec->_internal.request_topic_buffer),
          &this_policy->_internal.property_bag);

      // send pub request
      az_mqtt5_pub_data data = (az_mqtt5_pub_data){
        .topic = this_policy->_internal.rpc_client_codec->_internal.request_topic_buffer,
        .qos = AZ_MQTT5_DEFAULT_RPC_QOS,
        .payload = event_data->request_payload,
        .out_id = 0,
        .properties = &this_policy->_internal.property_bag,
      };

      if (az_span_size(this_policy->_internal.pending_pub_correlation_id) < az_span_size(event_data->correlation_id))
      {
        return AZ_ERROR_NOT_ENOUGH_SPACE;
      }

      az_span_copy(this_policy->_internal.pending_pub_correlation_id, event_data->correlation_id);
      _RETURN_AND_CLEAR_PROPERTY_BAG_IF_FAILED(
          _az_hfsm_transition_substate((_az_hfsm*)me, ready, publishing),
          &this_policy->_internal.property_bag);
      // send publish
      ret = az_event_policy_send_outbound_event(
          (az_event_policy*)me, (az_event){ .type = AZ_MQTT5_EVENT_PUB_REQ, .data = &data });

      this_policy->_internal.pending_pub_id = data.out_id;

      // empty the property bag so it can be reused
      az_mqtt5_property_bag_clear(&this_policy->_internal.property_bag);
      break;
    }
    case AZ_MQTT5_EVENT_PUBACK_RSP:
      break;

    case AZ_MQTT5_EVENT_PUB_RECV_IND:
    {
      // If the pub is of the right topic, send response to the application.
      _az_RETURN_IF_FAILED(send_resp_inbound_if_topic_matches(this_policy, event));
      break;
    }

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result publishing(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_rpc_client* this_policy = (az_mqtt5_rpc_client*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_client_policy/ready/publishing"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      _rpc_start_timer(
          this_policy,
          this_policy->_internal.rpc_client_codec->_internal.options.publish_timeout_in_seconds);
      break;

    case AZ_HFSM_EVENT_EXIT:
      _rpc_stop_timer(this_policy);
      this_policy->_internal.pending_pub_id = 0;
      break;

    case AZ_MQTT5_EVENT_PUBACK_RSP:
    {
      az_mqtt5_puback_data* puback_data = (az_mqtt5_puback_data*)event.data;
      if (puback_data->id == this_policy->_internal.pending_pub_id)
      {
        if (puback_data->reason_code != 0)
        {
          az_mqtt5_rpc_client_rsp_event_data resp_data
              = { .response_payload = AZ_SPAN_EMPTY,
                  .status = puback_data->reason_code,
                  .error_message = AZ_SPAN_FROM_STR("Puback has failure code."),
                  .content_type = AZ_SPAN_EMPTY,
                  .correlation_id = this_policy->_internal.pending_pub_correlation_id };

          // send to application to handle
          // if ((az_event_policy*)this_policy->inbound_policy != NULL)
          // {
          // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type =
          // AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP, .data = &resp_data});
          // }
          _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(
              this_policy->_internal.connection,
              (az_event){ .type = AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP, .data = &resp_data }));
        }
        _az_RETURN_IF_FAILED(_az_hfsm_transition_superstate((_az_hfsm*)me, publishing, ready));
      }
      break;
    }

    case AZ_HFSM_EVENT_TIMEOUT:
    {
      if (event.data == &this_policy->_internal.rpc_client_timer)
      {
        // if publishing times out, send failure to application and return to Ready
        az_mqtt5_rpc_client_rsp_event_data resp_data
            = { .response_payload = AZ_SPAN_EMPTY,
                .status = AZ_MQTT5_RPC_STATUS_TIMEOUT,
                .error_message = AZ_SPAN_FROM_STR("Publish timed out."),
                .content_type = AZ_SPAN_EMPTY,
                .correlation_id = this_policy->_internal.pending_pub_correlation_id };

        // send to application to handle
        // if ((az_event_policy*)this_policy->inbound_policy != NULL)
        // {
        // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type =
        // AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP, .data = &resp_data});
        // }
        _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(
            this_policy->_internal.connection,
            (az_event){ .type = AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP, .data = &resp_data }));

        _az_RETURN_IF_FAILED(_az_hfsm_transition_superstate((_az_hfsm*)me, publishing, ready));
      }
      break;
    }

    case AZ_MQTT5_EVENT_RPC_CLIENT_INVOKE_REQ:
    {
      // Can't invoke until outgoing pub has been ack'd or times out
      ret = AZ_ERROR_RPC_PUB_IN_PROGRESS;
      break;
    }

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

/**
 * @brief Failure state - locks up all execution of this policy
 */
static az_result faulted(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_rpc_client* this_policy = (az_mqtt5_rpc_client*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_client_policy/faulted"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    {
      // if ((az_event_policy*)this_policy->inbound_policy != NULL)
      // {
      // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type =
      // AZ_HFSM_EVENT_ERROR, .data = NULL});
      // }
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

AZ_NODISCARD az_result az_mqtt5_rpc_client_invoke_begin(
    az_mqtt5_rpc_client* client,
    az_mqtt5_rpc_client_invoke_req_event_data* data)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }

  return _az_hfsm_send_event(
      &client->_internal.rpc_client_hfsm,
      (az_event){ .type = AZ_MQTT5_EVENT_RPC_CLIENT_INVOKE_REQ, .data = data });
}

AZ_NODISCARD az_result az_mqtt5_rpc_client_subscribe_begin(az_mqtt5_rpc_client* client)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }
  return _az_hfsm_send_event(
      &client->_internal.rpc_client_hfsm,
      (az_event){ .type = AZ_MQTT5_EVENT_RPC_CLIENT_SUB_REQ, .data = NULL });
}

AZ_NODISCARD az_result az_mqtt5_rpc_client_unsubscribe_begin(az_mqtt5_rpc_client* client)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }
  return _az_hfsm_send_event(
      &client->_internal.rpc_client_hfsm,
      (az_event){ .type = AZ_MQTT5_EVENT_RPC_CLIENT_UNSUB_REQ, .data = NULL });
}

AZ_NODISCARD az_result _az_mqtt5_rpc_client_hfsm_policy_init(
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

AZ_NODISCARD az_result az_mqtt5_rpc_client_init(
    az_mqtt5_rpc_client* client,
    az_mqtt5_rpc_client_codec* rpc_client_codec,
    az_mqtt5_connection* connection,
    az_mqtt5_property_bag property_bag,
    az_span client_id,
    az_span model_id,
    az_span command_name,
    az_span response_topic_buffer,
    az_span request_topic_buffer,
    az_span subscribe_topic_buffer,
    az_span correlation_id_buffer,
    az_mqtt5_rpc_client_codec_options* options)
{
  _az_PRECONDITION_NOT_NULL(client);

  client->_internal.rpc_client_codec = rpc_client_codec;

  _az_RETURN_IF_FAILED(az_mqtt5_rpc_client_codec_init(
      client->_internal.rpc_client_codec,
      client_id,
      model_id,
      command_name,
      response_topic_buffer,
      request_topic_buffer,
      subscribe_topic_buffer,
      options));
  client->_internal.property_bag = property_bag;
  client->_internal.connection = connection;
  client->_internal.pending_pub_correlation_id = correlation_id_buffer;

  // Initialize the stateful sub-client.
  if ((connection != NULL))
  {
    _az_RETURN_IF_FAILED(_az_mqtt5_rpc_client_hfsm_policy_init(
        (_az_hfsm*)client, &client->_internal.subclient, connection));
  }

  return AZ_OK;
}

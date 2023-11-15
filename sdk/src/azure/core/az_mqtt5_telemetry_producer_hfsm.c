// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_mqtt5_telemetry_producer.h>
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
static az_result ready(az_event_policy* me, az_event event);
static az_result publishing(az_event_policy* me, az_event event);
static az_result faulted(az_event_policy* me, az_event event);

AZ_NODISCARD az_result _az_mqtt5_telemetry_producer_hfsm_policy_init(
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
  else if (child_state == ready || child_state == faulted)
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
  az_mqtt5_telemetry_producer* this_policy = (az_mqtt5_telemetry_producer*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_telemetry_producer"));
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
        _az_LOG_WRITE(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_mqtt5_telemetry_producer: PANIC!"));
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
    case AZ_MQTT5_EVENT_PUB_RECV_IND:
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

/**
 * @brief start publishing timer
 */
AZ_INLINE az_result _publish_start_timer(az_mqtt5_telemetry_producer* me)
{
  _az_event_pipeline* pipeline = &me->_internal.connection->_internal.event_pipeline;
  _az_event_pipeline_timer* timer = &me->_internal.telemetry_producer_timer;

  _az_RETURN_IF_FAILED(_az_event_pipeline_timer_create(pipeline, timer));

  int32_t delay_milliseconds = (int32_t)me->_internal.publish_timeout_in_seconds * 1000;

  _az_RETURN_IF_FAILED(az_platform_timer_start(&timer->platform_timer, delay_milliseconds));

  return AZ_OK;
}

/**
 * @brief stop publishing timer
 */
AZ_INLINE az_result _publish_stop_timer(az_mqtt5_telemetry_producer* me)
{
  _az_event_pipeline_timer* timer = &me->_internal.telemetry_producer_timer;
  return az_platform_timer_destroy(&timer->platform_timer);
}

AZ_NODISCARD static az_result prep_telemetry(az_mqtt5_telemetry_producer* this_policy, az_mqtt5_telemetry_producer_send_req_event_data* event_data, az_mqtt5_pub_data* out_data)
{
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

  _RETURN_AND_CLEAR_PROPERTY_BAG_IF_FAILED(
      az_mqtt5_telemetry_producer_codec_get_publish_topic(
          this_policy->_internal.telemetry_producer_codec,
          event_data->telemetry_name,
          (char*)az_span_ptr(this_policy->_internal.telemetry_topic_buffer),
          (size_t)az_span_size(this_policy->_internal.telemetry_topic_buffer),
          NULL),
      &this_policy->_internal.property_bag);

  // send pub request
  out_data->topic = this_policy->_internal.telemetry_topic_buffer;
  out_data->qos = event_data->qos;
  out_data->payload = event_data->telemetry_payload;
  out_data->out_id = 0;
  out_data->properties = &this_policy->_internal.property_bag;

  return AZ_OK;
}

static az_result ready(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_telemetry_producer* this_policy = (az_mqtt5_telemetry_producer*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_telemetry_producer/ready"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      break;

    case AZ_HFSM_EVENT_EXIT:
      break;

    case AZ_MQTT5_EVENT_TELEMETRY_PRODUCER_SEND_REQ:
    {
      az_mqtt5_telemetry_producer_send_req_event_data* event_data
          = (az_mqtt5_telemetry_producer_send_req_event_data*)event.data;
      az_mqtt5_pub_data data = {0};
      _RETURN_AND_CLEAR_PROPERTY_BAG_IF_FAILED(prep_telemetry(this_policy, event_data, &data), &this_policy->_internal.property_bag);

      if (data.qos != AZ_MQTT5_QOS_AT_MOST_ONCE)
      {
         _RETURN_AND_CLEAR_PROPERTY_BAG_IF_FAILED(
          _az_hfsm_transition_substate((_az_hfsm*)me, ready, publishing),
          &this_policy->_internal.property_bag);
      }
     
      // send publish
      ret = az_event_policy_send_outbound_event(
          (az_event_policy*)me, (az_event){ .type = AZ_MQTT5_EVENT_PUB_REQ, .data = &data });
      
      if (data.qos != AZ_MQTT5_QOS_AT_MOST_ONCE)
      {
        // If the pub failed to send, don't wait for the puback
        if (az_result_failed(ret))
        {
          _RETURN_AND_CLEAR_PROPERTY_BAG_IF_FAILED(_az_hfsm_transition_superstate((_az_hfsm*)me, publishing, ready),
          &this_policy->_internal.property_bag);
        }
        // otherwise, save the message id to correlate the puback
        else
        {
          this_policy->_internal.pending_pub_id = data.out_id;
        }
      }

      // empty the property bag so it can be reused
      az_mqtt5_property_bag_clear(&this_policy->_internal.property_bag);
      break;
    }
    case AZ_MQTT5_EVENT_PUBACK_RSP:
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result publishing(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_telemetry_producer* this_policy = (az_mqtt5_telemetry_producer*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_telemetry_producer/ready/publishing"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      _publish_start_timer(this_policy);
      break;

    case AZ_HFSM_EVENT_EXIT:
      _publish_stop_timer(this_policy);
      this_policy->_internal.pending_pub_id = 0;
      break;

    case AZ_MQTT5_EVENT_PUBACK_RSP:
    {
      az_mqtt5_puback_data* puback_data = (az_mqtt5_puback_data*)event.data;
      if (puback_data->id == this_policy->_internal.pending_pub_id)
      {
        if (puback_data->reason_code != 0)
        {
          // TODO: need a way for application to know what failed to send
          az_mqtt5_telemetry_producer_error_rsp_event_data resp_data
              = { .reason_code = puback_data->reason_code,
                  .error_message = AZ_SPAN_FROM_STR("Puback has failure code.") };

          // send to application to handle
          // if ((az_event_policy*)this_policy->inbound_policy != NULL)
          // {
          // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type =
          // AZ_MQTT5_EVENT_TELEMETRY_PRODUCER_ERROR_RSP, .data = &resp_data});
          // }
          _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(
              this_policy->_internal.connection,
              (az_event){ .type = AZ_MQTT5_EVENT_TELEMETRY_PRODUCER_ERROR_RSP, .data = &resp_data }));
        }
        _az_RETURN_IF_FAILED(_az_hfsm_transition_superstate((_az_hfsm*)me, publishing, ready));
      }
      break;
    }

    case AZ_HFSM_EVENT_TIMEOUT:
    {
      if (event.data == &this_policy->_internal.telemetry_producer_timer)
      {
        // if publishing times out, go to faulted state - this is not recoverable
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, ready, faulted));
      }
      break;
    }

    case AZ_MQTT5_EVENT_TELEMETRY_PRODUCER_SEND_REQ:
    {
      az_mqtt5_telemetry_producer_send_req_event_data* event_data
          = (az_mqtt5_telemetry_producer_send_req_event_data*)event.data;
      if (event_data->qos != AZ_MQTT5_QOS_AT_MOST_ONCE)
      {
        // Can't send another QOS 1 telemetry until outgoing pub has been ack'd or times out
        ret = AZ_ERROR_TELEMETRY_PRODUCER_PUB_IN_PROGRESS;
        break;
      }
      else
      {
        az_mqtt5_pub_data data = {0};
        _RETURN_AND_CLEAR_PROPERTY_BAG_IF_FAILED(prep_telemetry(this_policy, event_data, &data), &this_policy->_internal.property_bag);
        // send publish
        ret = az_event_policy_send_outbound_event(
            (az_event_policy*)me, (az_event){ .type = AZ_MQTT5_EVENT_PUB_REQ, .data = &data });

        // empty the property bag so it can be reused
        az_mqtt5_property_bag_clear(&this_policy->_internal.property_bag);
      }
      
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
  az_mqtt5_telemetry_producer* this_policy = (az_mqtt5_telemetry_producer*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_telemetry_producer/faulted"));
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

AZ_NODISCARD az_result az_mqtt5_telemetry_producer_send_begin(
    az_mqtt5_telemetry_producer* client,
    az_mqtt5_telemetry_producer_send_req_event_data* data)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }

  return _az_hfsm_send_event(
      &client->_internal.telemetry_producer_hfsm,
      (az_event){ .type = AZ_MQTT5_EVENT_TELEMETRY_PRODUCER_SEND_REQ, .data = data });
}

AZ_NODISCARD az_result _az_mqtt5_telemetry_producer_hfsm_policy_init(
    _az_hfsm* hfsm,
    _az_event_client* event_client,
    az_mqtt5_connection* connection)
{
  _az_RETURN_IF_FAILED(_az_hfsm_init(hfsm, root, _get_parent, NULL, NULL));
  _az_RETURN_IF_FAILED(_az_hfsm_transition_substate(hfsm, root, ready));

  event_client->policy = (az_event_policy*)hfsm;
  _az_RETURN_IF_FAILED(_az_event_policy_collection_add_client(
      &connection->_internal.policy_collection, event_client));

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_telemetry_producer_init(
    az_mqtt5_telemetry_producer* client,
    az_mqtt5_telemetry_producer_codec* telemetry_producer_codec,
    az_mqtt5_connection* connection,
    az_mqtt5_property_bag property_bag,
    az_span client_id,
    az_span model_id,
    az_span telemetry_topic_buffer,
    int32_t publish_timeout_in_seconds,
    az_mqtt5_telemetry_producer_codec_options* options)
{
  _az_PRECONDITION_NOT_NULL(client);

  if (publish_timeout_in_seconds <= 0)
  {
    return AZ_ERROR_ARG;
  }

  client->_internal.telemetry_producer_codec = telemetry_producer_codec;

  _az_RETURN_IF_FAILED(az_mqtt5_telemetry_producer_codec_init(
      client->_internal.telemetry_producer_codec, model_id, client_id, options));
  client->_internal.property_bag = property_bag;
  client->_internal.connection = connection;
  client->_internal.telemetry_topic_buffer = telemetry_topic_buffer;
  client->_internal.publish_timeout_in_seconds = publish_timeout_in_seconds;

  // Initialize the stateful sub-client.
  if ((connection != NULL))
  {
    _az_RETURN_IF_FAILED(_az_mqtt5_telemetry_producer_hfsm_policy_init(
        (_az_hfsm*)client, &client->_internal.subclient, connection));
  }

  return AZ_OK;
}

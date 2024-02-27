// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_mqtt5_telemetry.h>
#include <azure/core/az_mqtt5_telemetry_consumer.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

static az_result root(az_event_policy* me, az_event event);
static az_result ready(az_event_policy* me, az_event event);
static az_result faulted(az_event_policy* me, az_event event);

AZ_NODISCARD az_result _az_mqtt5_telemetry_consumer_policy_init(
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
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_telemetry_consumer"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      break;

    case AZ_HFSM_EVENT_ERROR:
    {
      if (az_result_failed(az_event_policy_send_inbound_event(me, event)))
      {
        az_platform_critical_error();
      }
      break;
    }

    case AZ_HFSM_EVENT_EXIT:
    {
      if (_az_LOG_SHOULD_WRITE(AZ_HFSM_EVENT_EXIT))
      {
        _az_LOG_WRITE(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_mqtt5_telemetry_consumer: PANIC!"));
      }

      az_platform_critical_error();
      break;
    }

    case AZ_MQTT5_EVENT_PUBACK_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ:
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
    case AZ_MQTT5_EVENT_UNSUBACK_RSP:
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

/**
 * @brief start subscription timer
 */
AZ_INLINE az_result _subscribe_start_timer(az_mqtt5_telemetry_consumer* me)
{
  _az_event_pipeline* pipeline = &me->_internal.connection->_internal.event_pipeline;
  _az_event_pipeline_timer* timer = &me->_internal.telemetry_consumer_timer;

  _az_RETURN_IF_FAILED(_az_event_pipeline_timer_create(pipeline, timer));

  int32_t delay_milliseconds = (int32_t)me->_internal.subscribe_timeout_in_seconds * 1000;

  _az_RETURN_IF_FAILED(az_platform_timer_start(&timer->platform_timer, delay_milliseconds));

  return AZ_OK;
}

/**
 * @brief stop subscription timer
 */
AZ_INLINE az_result _subscribe_stop_timer(az_mqtt5_telemetry_consumer* me)
{
  _az_event_pipeline_timer* timer = &me->_internal.telemetry_consumer_timer;
  return az_platform_timer_destroy(&timer->platform_timer);
}

/**
 * @brief Handle an incoming telemetry message
 *
 * @param this_policy
 * @param data event data received from the publish
 *
 * @return az_result
 */
AZ_INLINE az_result
_handle_telemetry(az_mqtt5_telemetry_consumer* this_policy, az_mqtt5_recv_data* data)
{
  _az_PRECONDITION_NOT_NULL(data->properties);
  _az_PRECONDITION_NOT_NULL(this_policy);

  az_result ret = AZ_OK;

  // read the content type so the application can properly deserialize the request
  az_mqtt5_property_string content_type
      = az_mqtt5_property_bag_read_string(data->properties, AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE);

  az_span content_type_str = az_mqtt5_property_get_string(&content_type);

  az_mqtt5_telemetry_consumer_ind_event_data telemetry_data
      = (az_mqtt5_telemetry_consumer_ind_event_data){
          .telemetry_payload = data->payload,
          .telemetry_topic = data->topic,
          .content_type = content_type_str,
        };

  ret = az_event_policy_send_inbound_event(
      (az_event_policy*)this_policy,
      (az_event){ .type = AZ_MQTT5_EVENT_TELEMETRY_CONSUMER_IND, .data = &telemetry_data });

  az_mqtt5_property_read_free_string(&content_type);

  return ret;
}

/**
 * @brief Main state where the telemetry consumer waits for incoming telemetry
 */
static az_result ready(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_telemetry_consumer* this_policy = (az_mqtt5_telemetry_consumer*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_telemetry_consumer/ready"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op
      break;

    case AZ_MQTT5_EVENT_TELEMETRY_CONSUMER_SUB_REQ:
    {
      az_mqtt5_sub_data subscription_data
          = { .topic_filter = this_policy->_internal.subscription_topic,
              .qos = AZ_MQTT5_DEFAULT_TELEMETRY_QOS,
              .out_id = 0 };
      _subscribe_start_timer(this_policy);
      _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event(
          me, (az_event){ .type = AZ_MQTT5_EVENT_SUB_REQ, .data = &subscription_data }));
      this_policy->_internal.pending_subscription_id = subscription_data.out_id;
      break;
    }

    case AZ_MQTT5_EVENT_SUBACK_RSP:
    {
      // if get suback that matches the sub we sent, stop waiting for the suback
      az_mqtt5_suback_data* data = (az_mqtt5_suback_data*)event.data;
      if (data->id == this_policy->_internal.pending_subscription_id)
      {
        _subscribe_stop_timer(this_policy);
        this_policy->_internal.pending_subscription_id = 0;
      }
      // else, keep waiting for the proper suback
      break;
    }

    case AZ_HFSM_EVENT_TIMEOUT:
    {
      if (event.data == &this_policy->_internal.telemetry_consumer_timer)
      {
        // if subscribing times out, go to faulted state - this is not recoverable
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, ready, faulted));
      }
      break;
    }

    case AZ_MQTT5_EVENT_PUB_RECV_IND:
    {
      az_mqtt5_recv_data* recv_data = (az_mqtt5_recv_data*)event.data;
      az_mqtt5_telemetry_consumer_codec_data telemetry_event_data;

      // Check if the publish is on the telemetry topic
      if (az_result_succeeded(az_mqtt5_telemetry_consumer_codec_parse_received_topic(
              this_policy->_internal.telemetry_consumer_codec,
              recv_data->topic,
              &telemetry_event_data)))
      {
        // clear subscription timer if we get a pub on the topic, since that implies we're
        // subscribed
        if (this_policy->_internal.pending_subscription_id != 0)
        {
          _subscribe_stop_timer(this_policy);
          this_policy->_internal.pending_subscription_id = 0;
        }

        // parse the telemetry details and send it to the application
        if (az_result_failed(_handle_telemetry(this_policy, recv_data)))
        {
          if (_az_LOG_SHOULD_WRITE(AZ_HFSM_EVENT_ERROR))
          {
            _az_LOG_WRITE(
                AZ_HFSM_EVENT_ERROR,
                AZ_SPAN_FROM_STR("az_telemetry_consumer/ready Error handling incoming publish - "
                                 "missing required content type property"));
          }
        }
      }
      break;
    }

    case AZ_MQTT5_EVENT_PUBACK_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ:
    case AZ_MQTT5_EVENT_CONNECT_RSP:
      break;

    default:
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
  az_mqtt5_telemetry_consumer* this_policy = (az_mqtt5_telemetry_consumer*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_telemetry_consumer/faulted"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    {
      if (az_result_failed(az_event_policy_send_inbound_event(
              (az_event_policy*)this_policy,
              (az_event){ .type = AZ_HFSM_EVENT_ERROR, .data = NULL })))
      {
        az_platform_critical_error();
      }
      break;
    }
    default:
      ret = AZ_ERROR_HFSM_INVALID_STATE;
      break;
  }

  return ret;
}

AZ_NODISCARD az_result _az_mqtt5_telemetry_consumer_policy_init(
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

AZ_NODISCARD az_result
az_mqtt5_telemetry_consumer_subscribe_begin(az_mqtt5_telemetry_consumer* client)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }

  return _az_hfsm_send_event(
      &client->_internal.telemetry_consumer_hfsm,
      (az_event){ .type = AZ_MQTT5_EVENT_TELEMETRY_CONSUMER_SUB_REQ, .data = NULL });
}

AZ_NODISCARD az_result
az_mqtt5_telemetry_consumer_unsubscribe_begin(az_mqtt5_telemetry_consumer* client)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }

  az_mqtt5_unsub_data unsubscription_data
      = { .topic_filter = client->_internal.subscription_topic };

  return az_event_policy_send_outbound_event(
      (az_event_policy*)client,
      (az_event){ .type = AZ_MQTT5_EVENT_UNSUB_REQ, .data = &unsubscription_data });
}

AZ_NODISCARD az_result az_mqtt5_telemetry_consumer_init(
    az_mqtt5_telemetry_consumer* client,
    az_mqtt5_telemetry_consumer_codec* telemetry_consumer_codec,
    az_mqtt5_connection* connection,
    az_span subscription_topic,
    az_span model_id,
    az_span sender_id,
    int32_t subscribe_timeout_in_seconds,
    az_mqtt5_telemetry_consumer_codec_options* options)
{
  _az_PRECONDITION_NOT_NULL(client);
  client->_internal.telemetry_consumer_codec = telemetry_consumer_codec;
  _az_RETURN_IF_FAILED(az_mqtt5_telemetry_consumer_codec_init(
      client->_internal.telemetry_consumer_codec, model_id, sender_id, options));

  if (subscribe_timeout_in_seconds <= 0)
  {
    return AZ_ERROR_ARG;
  }

  size_t topic_length;
  _az_RETURN_IF_FAILED(az_mqtt5_telemetry_consumer_codec_get_subscribe_topic(
      client->_internal.telemetry_consumer_codec,
      (char*)az_span_ptr(subscription_topic),
      (size_t)az_span_size(subscription_topic),
      &topic_length));

  client->_internal.subscription_topic
      = az_span_slice(subscription_topic, 0, (int32_t)topic_length);
  client->_internal.connection = connection;
  client->_internal.subscribe_timeout_in_seconds = subscribe_timeout_in_seconds;

  // Initialize the stateful sub-client.
  if ((connection != NULL))
  {
    _az_RETURN_IF_FAILED(_az_mqtt5_telemetry_consumer_policy_init(
        (_az_hfsm*)client, &client->_internal.subclient, connection));
  }

  return AZ_OK;
}

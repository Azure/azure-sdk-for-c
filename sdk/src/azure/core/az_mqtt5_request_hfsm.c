// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_mqtt5_request.h>
#include <azure/core/az_mqtt5_rpc_client.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

static az_result root(az_event_policy* me, az_event event);
static az_result idle(az_event_policy* me, az_event event);
static az_result publishing(az_event_policy* me, az_event event);
static az_result waiting(az_event_policy* me, az_event event);
static az_result completed(az_event_policy* me, az_event event);
static az_result faulted(az_event_policy* me, az_event event);

AZ_NODISCARD az_result _az_mqtt5_request_hfsm_policy_init(
    _az_hfsm* hfsm,
    _az_event_client* event_client,
    _az_event_policy_collection* request_policy_collection);

void log_event(az_event event, az_span message, az_span correlation_id);
void log_event(az_event event, az_span message, az_span correlation_id)
{
  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    // print correlation id
    char* corr = (char*)az_span_ptr(correlation_id);
    char curr_char;
    printf("\x1b[2mcorrelation id: ");
    for (int i = 0; i < 16; i++)
    {
      curr_char = corr[i];
      printf("%d", curr_char);
    }
    printf("\x1B[0m\t");

    // log event
    _az_LOG_WRITE(event.type, message);
  }
}

static az_event_policy_handler _get_parent(az_event_policy_handler child_state)
{
  az_event_policy_handler parent_state;

  if (child_state == root)
  {
    parent_state = NULL;
  }
  else if (
      child_state == idle || child_state == publishing || child_state == waiting
      || child_state == completed || child_state == faulted)
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
  az_mqtt5_request* this_policy = (az_mqtt5_request*)me;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy"),
          this_policy->_internal.correlation_id);
      break;

    case AZ_HFSM_EVENT_ERROR:
    {
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy"),
          this_policy->_internal.correlation_id);
      if (az_result_failed(az_event_policy_send_inbound_event(me, event)))
      {
        az_platform_critical_error();
      }
      break;
    }

    case AZ_HFSM_EVENT_EXIT:
    {
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy"),
          this_policy->_internal.correlation_id);
      _az_LOG_WRITE_IF_SHOULD(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_mqtt5_request: PANIC!"));

      az_platform_critical_error();
      break;
    }

    case AZ_MQTT5_EVENT_RPC_CLIENT_REMOVE_REQ:
    {
      az_mqtt5_rpc_client_remove_req_event_data* event_data
          = (az_mqtt5_rpc_client_remove_req_event_data*)event.data;
      if (az_span_is_content_equal(
              *event_data->correlation_id, this_policy->_internal.correlation_id))
      {
        log_event(
            event,
            AZ_SPAN_FROM_STR("az_mqtt5_request_policy"),
            this_policy->_internal.correlation_id);
        *event_data->correlation_id = this_policy->_internal.correlation_id;
        *event_data->policy = this_policy;
      }
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
    case AZ_MQTT5_EVENT_REQUEST_INIT:
    case AZ_MQTT5_EVENT_REQUEST_COMPLETE:
    case AZ_MQTT5_EVENT_REQUEST_FAULTED:
    case AZ_HFSM_EVENT_TIMEOUT:
      // ignore
      break;

    default:
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy"),
          this_policy->_internal.correlation_id);
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

/**
 * @brief start timer
 */
AZ_INLINE az_result _request_start_timer(
    az_mqtt5_request* me,
    int32_t timeout_in_seconds,
    _az_event_pipeline_timer* timer)
{
  _az_event_pipeline* pipeline = &me->_internal.connection->_internal.event_pipeline;

  _az_RETURN_IF_FAILED(_az_event_pipeline_timer_create(pipeline, timer));

  int32_t delay_milliseconds = (int32_t)timeout_in_seconds * 1000;

  _az_RETURN_IF_FAILED(az_platform_timer_start(&timer->platform_timer, delay_milliseconds));

  return AZ_OK;
}

/**
 * @brief stop timer
 */
AZ_INLINE az_result _request_stop_timer(_az_event_pipeline_timer* timer)
{
  return az_platform_timer_destroy(&timer->platform_timer);
}

static az_result idle(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_request* this_policy = (az_mqtt5_request*)me;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy/idle"),
          this_policy->_internal.correlation_id);
      _request_start_timer(
          this_policy,
          this_policy->_internal.request_completion_timeout_in_seconds,
          &this_policy->_internal.request_completion_timer);
      break;

    case AZ_HFSM_EVENT_EXIT:
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy/idle"),
          this_policy->_internal.correlation_id);
      break;

    case AZ_MQTT5_EVENT_REQUEST_INIT:
    {
      init_event_data* event_data = (init_event_data*)event.data;
      if (az_span_is_content_equal(
              event_data->correlation_id, this_policy->_internal.correlation_id))
      {
        log_event(
            event,
            AZ_SPAN_FROM_STR("az_mqtt5_request_policy/idle"),
            this_policy->_internal.correlation_id);
        this_policy->_internal.pending_pub_id = event_data->pub_id;

        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, idle, publishing));
      }

      break;
    }

    case AZ_MQTT5_EVENT_REQUEST_COMPLETE:
    case AZ_MQTT5_EVENT_REQUEST_FAULTED:
      // ignore
      break;

    case AZ_HFSM_EVENT_TIMEOUT:
    {
      if (event.data == &this_policy->_internal.request_completion_timer)
      {
        log_event(
            event,
            AZ_SPAN_FROM_STR("az_mqtt5_request_policy/idle"),
            this_policy->_internal.correlation_id);
        // if execution times out, send failure to application and go to faulted
        az_mqtt5_rpc_client_rsp_event_data resp_data
            = { .response_payload = AZ_SPAN_EMPTY,
                .status = AZ_MQTT5_RPC_STATUS_TIMEOUT,
                .error_message = AZ_SPAN_FROM_STR("Execution timed out."),
                .content_type = AZ_SPAN_EMPTY,
                .correlation_id = this_policy->_internal.correlation_id };

        _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(
            this_policy->_internal.connection,
            (az_event){ .type = AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP, .data = &resp_data }));

        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, idle, faulted));
      }
      break;
    }

    default:
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy/idle"),
          this_policy->_internal.correlation_id);
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result publishing(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_request* this_policy = (az_mqtt5_request*)me;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy/publishing"),
          this_policy->_internal.correlation_id);
      _request_start_timer(
          this_policy,
          this_policy->_internal.publish_timeout_in_seconds,
          &this_policy->_internal.request_pub_timer);
      break;

    case AZ_HFSM_EVENT_EXIT:
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy/publishing"),
          this_policy->_internal.correlation_id);
      _request_stop_timer(&this_policy->_internal.request_pub_timer);
      this_policy->_internal.pending_pub_id = 0;
      break;

    case AZ_MQTT5_EVENT_PUBACK_RSP:
    {
      az_mqtt5_puback_data* puback_data = (az_mqtt5_puback_data*)event.data;
      if (puback_data->id == this_policy->_internal.pending_pub_id)
      {
        log_event(
            event,
            AZ_SPAN_FROM_STR("az_mqtt5_request_policy/publishing"),
            this_policy->_internal.correlation_id);
        if (puback_data->reason_code != 0)
        {
          az_mqtt5_rpc_client_rsp_event_data resp_data
              = { .response_payload = AZ_SPAN_EMPTY,
                  .status = puback_data->reason_code,
                  .error_message = AZ_SPAN_FROM_STR("Puback has failure code."),
                  .content_type = AZ_SPAN_EMPTY,
                  .correlation_id = this_policy->_internal.correlation_id };

          // send to application to handle
          // if ((az_event_policy*)this_policy->inbound_policy != NULL)
          // {
          // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type =
          // AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP, .data = &resp_data});
          // }
          _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(
              this_policy->_internal.connection,
              (az_event){ .type = AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP, .data = &resp_data }));

          _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, publishing, faulted));
        }
        else
        {
          _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, publishing, waiting));
        }
      }
      break;
    }

    case AZ_HFSM_EVENT_TIMEOUT:
    {
      if (event.data == &this_policy->_internal.request_pub_timer)
      {
        log_event(
            event,
            AZ_SPAN_FROM_STR("az_mqtt5_request_policy/publishing"),
            this_policy->_internal.correlation_id);
        // if publishing times out, send failure to application and go to faulted
        az_mqtt5_rpc_client_rsp_event_data resp_data
            = { .response_payload = AZ_SPAN_EMPTY,
                .status = AZ_MQTT5_RPC_STATUS_TIMEOUT,
                .error_message = AZ_SPAN_FROM_STR("Publish timed out."),
                .content_type = AZ_SPAN_EMPTY,
                .correlation_id = this_policy->_internal.correlation_id };

        // send to application to handle
        _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(
            this_policy->_internal.connection,
            (az_event){ .type = AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP, .data = &resp_data }));

        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, publishing, faulted));
      }
      else if (event.data == &this_policy->_internal.request_completion_timer)
      {
        log_event(
            event,
            AZ_SPAN_FROM_STR("az_mqtt5_request_policy/publishing"),
            this_policy->_internal.correlation_id);
        // if execution times out, send failure to application and go to faulted
        az_mqtt5_rpc_client_rsp_event_data resp_data
            = { .response_payload = AZ_SPAN_EMPTY,
                .status = AZ_MQTT5_RPC_STATUS_TIMEOUT,
                .error_message = AZ_SPAN_FROM_STR("Execution timed out."),
                .content_type = AZ_SPAN_EMPTY,
                .correlation_id = this_policy->_internal.correlation_id };

        // send to application to handle
        _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(
            this_policy->_internal.connection,
            (az_event){ .type = AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP, .data = &resp_data }));

        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, publishing, faulted));
      }
      break;
    }

    case AZ_MQTT5_EVENT_REQUEST_COMPLETE:
    {
      az_span* correlation_id = (az_span*)event.data;
      if (az_span_is_content_equal(*correlation_id, this_policy->_internal.correlation_id))
      {
        log_event(
            event,
            AZ_SPAN_FROM_STR("az_mqtt5_request_policy/publishing"),
            this_policy->_internal.correlation_id);
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, publishing, completed));
      }
      break;
    }

    case AZ_MQTT5_EVENT_REQUEST_FAULTED:
    {
      az_span* correlation_id = (az_span*)event.data;
      if (az_span_is_content_equal(*correlation_id, this_policy->_internal.correlation_id))
      {
        log_event(
            event,
            AZ_SPAN_FROM_STR("az_mqtt5_request_policy/publishing"),
            this_policy->_internal.correlation_id);
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, publishing, faulted));
      }
      break;
    }

    case AZ_MQTT5_EVENT_REQUEST_INIT:
      // ignore
      break;

    default:
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy/publishing"),
          this_policy->_internal.correlation_id);
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result waiting(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_request* this_policy = (az_mqtt5_request*)me;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy/waiting"),
          this_policy->_internal.correlation_id);
      break;

    case AZ_MQTT5_EVENT_REQUEST_COMPLETE:
    {
      az_span* correlation_id = (az_span*)event.data;
      if (az_span_is_content_equal(*correlation_id, this_policy->_internal.correlation_id))
      {
        log_event(
            event,
            AZ_SPAN_FROM_STR("az_mqtt5_request_policy/waiting"),
            this_policy->_internal.correlation_id);
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, waiting, completed));
      }
      break;
    }

    case AZ_MQTT5_EVENT_REQUEST_FAULTED:
    {
      az_span* correlation_id = (az_span*)event.data;
      if (az_span_is_content_equal(*correlation_id, this_policy->_internal.correlation_id))
      {
        log_event(
            event,
            AZ_SPAN_FROM_STR("az_mqtt5_request_policy/waiting"),
            this_policy->_internal.correlation_id);
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, waiting, faulted));
      }
      break;
    }

    case AZ_HFSM_EVENT_TIMEOUT:
    {
      if (event.data == &this_policy->_internal.request_completion_timer)
      {
        log_event(
            event,
            AZ_SPAN_FROM_STR("az_mqtt5_request_policy/waiting"),
            this_policy->_internal.correlation_id);
        // if execution times out, send failure to application and go to faulted
        az_mqtt5_rpc_client_rsp_event_data resp_data
            = { .response_payload = AZ_SPAN_EMPTY,
                .status = AZ_MQTT5_RPC_STATUS_TIMEOUT,
                .error_message = AZ_SPAN_FROM_STR("Execution timed out."),
                .content_type = AZ_SPAN_EMPTY,
                .correlation_id = this_policy->_internal.correlation_id };

        _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(
            this_policy->_internal.connection,
            (az_event){ .type = AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP, .data = &resp_data }));

        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, waiting, faulted));
      }
      break;
    }

    default:
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy/waiting"),
          this_policy->_internal.correlation_id);
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result completed(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_request* this_policy = (az_mqtt5_request*)me;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy/completed"),
          this_policy->_internal.correlation_id);
      _request_stop_timer(&this_policy->_internal.request_completion_timer);
      break;

    case AZ_HFSM_EVENT_EXIT:
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy/completed"),
          this_policy->_internal.correlation_id);
      break;

    case AZ_MQTT5_EVENT_PUBACK_RSP:
    case AZ_MQTT5_EVENT_REQUEST_INIT:
    case AZ_MQTT5_EVENT_REQUEST_COMPLETE:
    case AZ_MQTT5_EVENT_REQUEST_FAULTED:
    case AZ_HFSM_EVENT_TIMEOUT:
      // ignore
      break;

    default:
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy/completed"),
          this_policy->_internal.correlation_id);
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
  az_mqtt5_request* this_policy = (az_mqtt5_request*)me;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    {
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy/faulted"),
          this_policy->_internal.correlation_id);
      _request_stop_timer(&this_policy->_internal.request_completion_timer);
      /* we always send an error before transitioning to faulted, so no need to send another one
      here, just return AZ_OK to confirm the transition was successful. Exception for
      AZ_MQTT5_EVENT_REQUEST_FAULTED because an error is sent from the
      rpc client when this event is sent*/
      break;
    }

    case AZ_MQTT5_EVENT_PUBACK_RSP:
    case AZ_MQTT5_EVENT_SUBACK_RSP:
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
    case AZ_MQTT5_EVENT_UNSUBACK_RSP:
    case AZ_MQTT5_EVENT_PUB_RECV_IND:
    case AZ_HFSM_EVENT_TIMEOUT:
      // ignore, not from application
      break;

    case AZ_MQTT5_EVENT_REQUEST_COMPLETE:
    case AZ_MQTT5_EVENT_REQUEST_FAULTED:
    {
      az_span* correlation_id = (az_span*)event.data;
      if (az_span_is_content_equal(*correlation_id, this_policy->_internal.correlation_id))
      {
        log_event(
            event,
            AZ_SPAN_FROM_STR("az_mqtt5_request_policy/faulted"),
            this_policy->_internal.correlation_id);
        ret = AZ_ERROR_HFSM_INVALID_STATE;
      }
      break;
    }

    case AZ_MQTT5_EVENT_RPC_CLIENT_REMOVE_REQ:
    {
      az_mqtt5_rpc_client_remove_req_event_data* event_data
          = (az_mqtt5_rpc_client_remove_req_event_data*)event.data;
      // Make sure this remove request is either for this request or any faulted request
      if (az_span_is_content_equal(*event_data->correlation_id, AZ_SPAN_EMPTY)
          || az_span_is_content_equal(
              *event_data->correlation_id, this_policy->_internal.correlation_id))
      {
        log_event(
            event,
            AZ_SPAN_FROM_STR("az_mqtt5_request_policy/faulted"),
            this_policy->_internal.correlation_id);
        *event_data->correlation_id = this_policy->_internal.correlation_id;
        *event_data->policy = this_policy;
      }
      break;
    }

    case AZ_MQTT5_EVENT_REQUEST_INIT:
      // ignore
      break;

    default:
      log_event(
          event,
          AZ_SPAN_FROM_STR("az_mqtt5_request_policy/faulted"),
          this_policy->_internal.correlation_id);
      ret = AZ_ERROR_HFSM_INVALID_STATE;
      break;
  }

  return ret;
}

AZ_NODISCARD az_result _az_mqtt5_request_hfsm_policy_init(
    _az_hfsm* hfsm,
    _az_event_client* event_client,
    _az_event_policy_collection* request_policy_collection)
{
  _az_RETURN_IF_FAILED(_az_hfsm_init(hfsm, root, _get_parent, NULL, NULL));
  _az_RETURN_IF_FAILED(_az_hfsm_transition_substate(hfsm, root, idle));

  event_client->policy = (az_event_policy*)hfsm;
  _az_RETURN_IF_FAILED(
      _az_event_policy_collection_add_client(request_policy_collection, event_client));

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_request_init(
    az_mqtt5_request* request,
    az_mqtt5_connection* connection,
    _az_event_policy_collection* request_policy_collection,
    az_span correlation_id,
    int32_t publish_timeout_in_seconds,
    int32_t request_completion_timeout_in_seconds)
{
  _az_PRECONDITION_NOT_NULL(request);

  if (publish_timeout_in_seconds <= 0 || request_completion_timeout_in_seconds <= 0)
  {
    return AZ_ERROR_ARG;
  }

  request->_internal.connection = connection;
  request->_internal.correlation_id = correlation_id;
  request->_internal.publish_timeout_in_seconds = publish_timeout_in_seconds;
  request->_internal.request_completion_timeout_in_seconds = request_completion_timeout_in_seconds;
  request->_internal.request_policy_collection = request_policy_collection;
  request->_internal.pending_pub_id = 0;

  // Initialize the stateful sub-client.
  if ((connection != NULL))
  {
    _az_RETURN_IF_FAILED(_az_mqtt5_request_hfsm_policy_init(
        (_az_hfsm*)request,
        &request->_internal.subclient,
        request->_internal.request_policy_collection));
  }

  return AZ_OK;
}
/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 * @brief Mosquitto rpc server sample
 *
 */

#include <az_log_listener.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "unlock_json_parser.h"
#include <azure/az_core.h>
#include <azure/core/az_log.h>

#ifdef _WIN32
// Required for Sleep(DWORD)
#include <Windows.h>
#else
// Required for sleep(unsigned int)
#include <unistd.h>
#endif

static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to pem file>");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to key file>");
static const az_span client_id = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span username = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span password = AZ_SPAN_EMPTY;
static const az_span hostname = AZ_SPAN_LITERAL_FROM_STR("<hostname>");
static const az_span command_name = AZ_SPAN_LITERAL_FROM_STR("unlock");
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:rpc:samples:vehicle;1");
static const az_span content_type = AZ_SPAN_LITERAL_FROM_STR("application/json");

static char sub_topic_buffer[256];
static char response_payload_buffer[256];

// for pending_command
static char correlation_id_buffer[37];
static char response_topic_buffer[256];
static char command_name_buffer[256];
static char request_payload_buffer[256];
static char content_type_buffer[256];

static az_mqtt5_connection iot_connection;
static az_context connection_context;

static az_mqtt5_rpc_server rpc_server;

volatile bool connected = false;

static az_mqtt5_rpc_server_options rpc_server_options;

static az_mqtt5_rpc_server_execution_req_event_data pending_command;

#ifdef _WIN32
static timer_t timer; // placeholder
#else
static timer_t timer;
static struct sigevent sev;
static struct itimerspec trigger;
#endif

void az_platform_critical_error()
{
  printf(LOG_APP "\x1B[31mPANIC!\x1B[0m\n");

  while (1)
    ;
}

/**
 * @brief On command timeout, send an error response with timeout details to the HFSM
 * @note May need to be modified for your solution
 */
static void timer_callback(union sigval sv)
{
#ifdef _WIN32
  return; // AZ_ERROR_DEPENDENCY_NOT_PROVIDED
#else
  void* callback_context = sv.sival_ptr;
#endif

  printf(LOG_APP_ERROR "Command execution timed out.\n");
  az_mqtt5_rpc_server_execution_resp_event_data return_data
      = { .correlation_id = pending_command.correlation_id,
          .error_message = AZ_SPAN_FROM_STR("Command Server timeout"),
          .response_topic = pending_command.response_topic,
          .status = AZ_MQTT5_RPC_STATUS_TIMEOUT,
          .response = AZ_SPAN_EMPTY,
          .content_type = AZ_SPAN_EMPTY };
  if (az_result_failed(az_mqtt5_rpc_server_execution_finish(&rpc_server, &return_data)))
  {
    printf(LOG_APP_ERROR "Failed sending execution response to HFSM\n");
    return;
  }

  pending_command.content_type = AZ_SPAN_FROM_BUFFER(content_type_buffer);
  pending_command.response_topic = AZ_SPAN_FROM_BUFFER(response_topic_buffer);
  pending_command.correlation_id = AZ_SPAN_EMPTY;

#ifdef _WIN32
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
#else
  if (0 != timer_delete(timer))
  {
    printf(LOG_APP_ERROR "Failed destroying timer\n");
    return;
  }
#endif
}

/**
 * @brief Start a timer
 */
AZ_INLINE az_result start_timer(void* callback_context, int32_t delay_milliseconds)
{
#ifdef _WIN32
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
#else
  sev.sigev_notify = SIGEV_THREAD;
  sev.sigev_notify_function = &timer_callback;
  sev.sigev_value.sival_ptr = &callback_context;
  if (0 != timer_create(CLOCK_REALTIME, &sev, &timer))
  {
    // if (ENOMEM == errno)
    // {
    //   return AZ_ERROR_OUT_OF_MEMORY;
    // }
    // else
    // {
    return AZ_ERROR_ARG;
    // }
  }

  // start timer
  trigger.it_value.tv_sec = delay_milliseconds / 1000;
  trigger.it_value.tv_nsec = (delay_milliseconds % 1000) * 1000000;

  if (0 != timer_settime(timer, 0, &trigger, NULL))
  {
    return AZ_ERROR_ARG;
  }
#endif

  return AZ_OK;
}

/**
 * @brief Stop the timer
 */
AZ_INLINE az_result stop_timer()
{
#ifdef _WIN32
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
#else
  if (0 != timer_delete(timer))
  {
    return AZ_ERROR_ARG;
  }
#endif

  return AZ_OK;
}

/**
 * @brief Function that does the actual command execution
 * @note Needs to be modified for your solution
 */
az_mqtt5_rpc_status execute_command(unlock_request req)
{
  // for now, just print details from the command
  printf(
      LOG_APP "Executing command from: %s at: %ld\n",
      az_span_ptr(req.requested_from),
      req.request_timestamp);
  return AZ_MQTT5_RPC_STATUS_OK;
}

/**
 * @brief Check if there is a pending command and execute it. On completion, if the command hasn't
 * timed out, send the result back to the hfsm
 * @note Result to be sent back to the hfsm needs to be modified for your solution
 */
az_result check_for_commands()
{
  if (az_span_ptr(pending_command.correlation_id) != NULL)
  {
    // copy correlation id to a new span so we can compare it later
    char copy_buffer[az_span_size(pending_command.correlation_id)];
    az_span correlation_id_copy
        = az_span_create(copy_buffer, az_span_size(pending_command.correlation_id));
    az_span_copy(correlation_id_copy, pending_command.correlation_id);

    if (!az_span_is_content_equal(content_type, pending_command.content_type))
    {
      // TODO: should this completely fail execution? This currently matches the C# implementation.
      // I feel like it should send an error response
      printf(
          LOG_APP_ERROR "Invalid content type. Expected: {%s} Actual: {%s}\n",
          az_span_ptr(content_type),
          az_span_ptr(pending_command.content_type));
      return AZ_ERROR_NOT_SUPPORTED;
    }

    unlock_request req;
    LOG_AND_EXIT_IF_FAILED(deserialize_unlock_request(pending_command.request_data, &req));
    az_mqtt5_rpc_status rc = execute_command(req);

    // if command hasn't timed out, send result back
    if (az_span_is_content_equal(correlation_id_copy, pending_command.correlation_id))
    {
      stop_timer();

      // Serialize response
      az_span response_payload = AZ_SPAN_FROM_BUFFER(response_payload_buffer);
      LOG_AND_EXIT_IF_FAILED(serialize_response_payload(req, response_payload));

      /* Modify the response/error message/status as needed for your solution */
      az_mqtt5_rpc_server_execution_resp_event_data return_data
          = { .correlation_id = pending_command.correlation_id,
              .response = response_payload,
              .response_topic = pending_command.response_topic,
              .status = rc,
              .content_type = content_type,
              .error_message = AZ_SPAN_EMPTY };
      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_execution_finish(&rpc_server, &return_data));

      pending_command.content_type = AZ_SPAN_FROM_BUFFER(content_type_buffer);
      pending_command.response_topic = AZ_SPAN_FROM_BUFFER(response_topic_buffer);
      pending_command.correlation_id = AZ_SPAN_EMPTY;
    }
  }
  return AZ_OK;
}

az_result copy_execution_event_data(
    az_mqtt5_rpc_server_execution_req_event_data* destination,
    az_mqtt5_rpc_server_execution_req_event_data source)
{
  az_span_copy(destination->command_name, source.command_name);
  az_span_copy(destination->response_topic, source.response_topic);
  destination->response_topic
      = az_span_slice(destination->response_topic, 0, az_span_size(source.response_topic));
  az_span_copy(destination->request_data, source.request_data);
  az_span_copy(destination->content_type, source.content_type);
  destination->content_type
      = az_span_slice(destination->content_type, 0, az_span_size(source.content_type));
  destination->correlation_id = AZ_SPAN_FROM_BUFFER(correlation_id_buffer);
  az_span_copy(destination->correlation_id, source.correlation_id);
  destination->correlation_id
      = az_span_slice(destination->correlation_id, 0, az_span_size(source.correlation_id));

  return AZ_OK;
}

/**
 * @brief Callback function for all clients
 * @note If you add other clients, you can add handling for their events here
 */
az_result iot_callback(az_mqtt5_connection* client, az_event event)
{
  az_app_log_callback(event.type, AZ_SPAN_FROM_STR("APP/callback"));
  switch (event.type)
  {
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    {
      connected = true;
      az_mqtt5_connack_data* connack_data = (az_mqtt5_connack_data*)event.data;
      printf(LOG_APP "[%p] CONNACK: %d\n", client, connack_data->connack_reason);

      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_register(&rpc_server));
      break;
    }

    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
    {
      connected = false;
      printf(LOG_APP "[%p] DISCONNECTED\n", client);
      break;
    }

    case AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND_REQ:
    {
      if (az_span_ptr(pending_command.correlation_id) != NULL)
      {
        printf(LOG_APP "Received command while another command is pending. Ignoring.\n");
      }
      else
      {
        // Mark that there's a pending command to be executed
        az_mqtt5_rpc_server_execution_req_event_data data
            = *(az_mqtt5_rpc_server_execution_req_event_data*)event.data;
        LOG_AND_EXIT_IF_FAILED(copy_execution_event_data(&pending_command, data));
        start_timer(NULL, 10000);
        printf(LOG_APP "Added command to queue\n");
      }

      break;
    }

    case AZ_EVENT_RPC_SERVER_UNHANDLED_COMMAND:
    {
      az_mqtt5_recv_data* recv_data = (az_mqtt5_recv_data*)event.data;
      printf(LOG_APP "Received unhandled command.\n");
      // could put this command in a queue and trigger a AZ_MQTT5_EVENT_PUB_RECV_IND with it once
      // we're finished with the current command
      break;
    }

    default:
      // TODO:
      break;
  }

  return AZ_OK;
}

int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  /* Required before calling other mosquitto functions */
  if (mosquitto_lib_init() != MOSQ_ERR_SUCCESS)
  {
    printf(LOG_APP "Failed to initialize MosquittoLib\n");
    return -1;
  }

  printf(LOG_APP "Using MosquittoLib %d\n", mosquitto_lib_version(NULL, NULL, NULL));

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

  connection_context = az_context_create_with_expiration(
      &az_context_application, az_context_get_expiration(&az_context_application));

  az_mqtt5 mqtt5;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_init(&mqtt5, NULL));

  az_mqtt5_x509_client_certificate primary_credential = (az_mqtt5_x509_client_certificate){
    .cert = cert_path1,
    .key = key_path1,
  };

  az_mqtt5_connection_options connection_options = az_mqtt5_connection_options_default();
  connection_options.client_id_buffer = client_id;
  connection_options.username_buffer = username;
  connection_options.password_buffer = password;
  connection_options.hostname = hostname;
  connection_options.client_certificates[0] = primary_credential;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_init(
      &iot_connection, &connection_context, &mqtt5, iot_callback, &connection_options));

  pending_command.request_data = AZ_SPAN_FROM_BUFFER(request_payload_buffer);
  pending_command.content_type = AZ_SPAN_FROM_BUFFER(content_type_buffer);
  pending_command.correlation_id = AZ_SPAN_EMPTY;
  pending_command.response_topic = AZ_SPAN_FROM_BUFFER(response_topic_buffer);
  pending_command.command_name = AZ_SPAN_FROM_BUFFER(command_name_buffer);

  az_mqtt5_property_bag property_bag;
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_property_bag_init(&property_bag, &mqtt5, NULL));

  rpc_server_options
      = (az_mqtt5_rpc_server_options){ .sub_topic = AZ_SPAN_FROM_BUFFER(sub_topic_buffer),
                                       .command_name = command_name,
                                       .model_id = model_id };

  az_mqtt5_rpc_server_memory rpc_server_memory
      = (az_mqtt5_rpc_server_memory){ .property_bag = property_bag };

  LOG_AND_EXIT_IF_FAILED(
      az_rpc_server_init(&rpc_server, &iot_connection, &rpc_server_memory, &rpc_server_options));

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_open(&iot_connection));

  // infinite execution loop
  for (int i = 45; i > 0; i++)
  {
    LOG_AND_EXIT_IF_FAILED(check_for_commands());
#ifdef _WIN32
    Sleep((DWORD)1000);
#else
    sleep(1);
#endif
    printf(LOG_APP "Waiting...\r");
    fflush(stdout);
  }

  // clean-up functions shown for completeness
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_close(&iot_connection));

  if (mqtt5._internal.mosquitto_handle != NULL)
  {
    mosquitto_loop_stop(mqtt5._internal.mosquitto_handle, false);
    mosquitto_destroy(mqtt5._internal.mosquitto_handle);
  }

  // mosquitto allocates the property bag for us, but we're responsible for free'ing it
  mosquitto_property_free_all(&property_bag.properties);

  if (mosquitto_lib_cleanup() != MOSQ_ERR_SUCCESS)
  {
    printf(LOG_APP "Failed to cleanup MosquittoLib\n");
    return -1;
  }

  printf(LOG_APP "Done.                                \n");
  return 0;
}

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
#include <uuid/uuid.h>

#include "mosquitto_rpc_server_sample_json_parser.h"
#include "rpc_client_pending_commands.h"
#include <azure/az_core.h>
#include <azure/core/az_log.h>
#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_mqtt5_rpc_client.h>

#ifdef _WIN32
// Required for Sleep(DWORD)
#include <Windows.h>
#else
// Required for sleep(unsigned int)
#include <unistd.h>
#endif

static const az_span client_id = AZ_SPAN_LITERAL_FROM_STR("f9e57487-616a-4725-aa9f-ee88b611228a");
static const az_span username = AZ_SPAN_LITERAL_FROM_STR("f9e57487-616a-4725-aa9f-ee88b611228a");
static const az_span hostname = AZ_SPAN_LITERAL_FROM_STR("localhost");
static const az_span server_client_id = AZ_SPAN_LITERAL_FROM_STR("controller-id");
static const az_span content_type = AZ_SPAN_LITERAL_FROM_STR("application/json");
static const az_span command_name = AZ_SPAN_LITERAL_FROM_STR("startModule");
static const az_span command_name2 = AZ_SPAN_LITERAL_FROM_STR("stopModule");

static char server_subscription_topic_buffer[256];
// static char response_payload_buffer[256];

static char client_response_topic_buffer[256];
static char client_request_topic_buffer[256];
static char client_subscription_topic_buffer[256];

static uint8_t client_correlation_id_buffers[RPC_CLIENT_MAX_PENDING_COMMANDS]
                                     [AZ_MQTT5_RPC_CORRELATION_ID_LENGTH];
static pending_commands_array client_pending_commands;

// for pending_server_command
static char server_correlation_id_buffer[256];
static char server_response_topic_buffer[256];
static char server_request_topic_buffer[256];
static char server_request_payload_buffer[256];
static char server_content_type_buffer[256];

static az_mqtt5_connection mqtt_connection;
static az_context connection_context;

static az_mqtt5_rpc_server rpc_server;
static az_mqtt5_rpc_server_policy rpc_server_policy;

static az_mqtt5_rpc_client_policy rpc_client_policy;
static az_mqtt5_rpc_client rpc_client;

volatile bool sample_finished = false;

static az_mqtt5_rpc_server_execution_req_event_data pending_server_command;

#ifdef _WIN32
static timer_t timer; // placeholder
#else
static timer_t timer;
static struct sigevent sev;
static struct itimerspec trigger;
#endif

az_mqtt5_rpc_status execute_command();
az_result check_for_commands();
az_result copy_execution_event_data(
    az_mqtt5_rpc_server_execution_req_event_data* destination,
    az_mqtt5_rpc_server_execution_req_event_data source);
az_result mqtt_callback(az_mqtt5_connection* client, az_event event);

void az_platform_critical_error()
{
  printf(LOG_APP_ERROR "\x1B[31mPANIC!\x1B[0m\n");

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
  // void* callback_context = sv.sival_ptr;
  (void)sv;
#endif

  printf(LOG_APP_ERROR "Command execution timed out.\n");
  az_mqtt5_rpc_server_execution_rsp_event_data return_data
      = { .correlation_id = pending_server_command.correlation_id,
          .error_message = AZ_SPAN_FROM_STR("Command Server timeout"),
          .response_topic = pending_server_command.response_topic,
          .request_topic = pending_server_command.request_topic,
          .status = AZ_MQTT5_RPC_STATUS_TIMEOUT,
          .response = AZ_SPAN_EMPTY,
          .content_type = AZ_SPAN_EMPTY };
  if (az_result_failed(az_mqtt5_rpc_server_execution_finish(&rpc_server_policy, &return_data)))
  {
    printf(LOG_APP_ERROR "Failed sending execution response to HFSM\n");
    return;
  }

  pending_server_command.content_type = AZ_SPAN_FROM_BUFFER(server_content_type_buffer);
  pending_server_command.request_topic = AZ_SPAN_FROM_BUFFER(server_request_topic_buffer);
  pending_server_command.correlation_id = AZ_SPAN_EMPTY;

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
az_mqtt5_rpc_status execute_command()
{
  // for now, just print details from the command
  printf(
      LOG_APP "Executing command from topic: %s\n",
      az_span_ptr(pending_server_command.request_topic));
  return AZ_MQTT5_RPC_STATUS_OK;
}

/**
 * @brief Check if there is a pending command and execute it. On completion, if the command hasn't
 * timed out, send the result back to the hfsm
 * @note Result to be sent back to the hfsm needs to be modified for your solution
 */
az_result check_for_commands()
{
  if (az_span_ptr(pending_server_command.correlation_id) != NULL)
  {
    // copy correlation id to a new span so we can compare it later
    uint8_t copy_buffer[az_span_size(pending_server_command.correlation_id)];
    az_span correlation_id_copy
        = az_span_create(copy_buffer, az_span_size(pending_server_command.correlation_id));
    az_span_copy(correlation_id_copy, pending_server_command.correlation_id);

    if (!az_span_is_content_equal(content_type, pending_server_command.content_type))
    {
      // TODO: should this completely fail execution? This currently matches the C# implementation.
      // I feel like it should send an error response
      printf(
          LOG_APP_ERROR "Invalid content type. Expected: {%s} Actual: {%s}\n",
          az_span_ptr(content_type),
          az_span_ptr(pending_server_command.content_type));
      return AZ_ERROR_NOT_SUPPORTED;
    }

    // unlock_request req;
    az_mqtt5_rpc_status rc;
    az_span error_message = AZ_SPAN_EMPTY;

    // if (az_result_failed(deserialize_unlock_request(pending_server_command.request_data, &req)))
    // {
    //   printf(LOG_APP_ERROR "Failed to deserialize request\n");
    //   rc = AZ_MQTT5_RPC_STATUS_UNSUPPORTED_TYPE;
    //   error_message = az_span_create_from_str("Failed to deserialize unlock command request.");
    // }
    // else
    // {
      rc = execute_command();
    // }

    // if command hasn't timed out, send result back
    if (az_span_is_content_equal(correlation_id_copy, pending_server_command.correlation_id))
    {
      stop_timer();
      az_span response_payload = AZ_SPAN_EMPTY;
      // if (rc == AZ_MQTT5_RPC_STATUS_OK)
      // {
      //   // Serialize response
      //   response_payload = AZ_SPAN_FROM_BUFFER(response_payload_buffer);
      //   LOG_AND_EXIT_IF_FAILED(serialize_response_payload(req, response_payload));
      // }

      /* Modify the response/error message/status as needed for your solution */
      az_mqtt5_rpc_server_execution_rsp_event_data return_data
          = { .correlation_id = pending_server_command.correlation_id,
              .response = response_payload,
              .response_topic = pending_server_command.response_topic,
              .request_topic = pending_server_command.request_topic,
              .status = rc,
              .content_type = content_type,
              .error_message = error_message };
      LOG_AND_EXIT_IF_FAILED(
          az_mqtt5_rpc_server_execution_finish(&rpc_server_policy, &return_data));

      pending_server_command.content_type = AZ_SPAN_FROM_BUFFER(server_content_type_buffer);
      pending_server_command.request_topic = AZ_SPAN_FROM_BUFFER(server_request_topic_buffer);
      pending_server_command.correlation_id = AZ_SPAN_EMPTY;
    }
  }
  return AZ_OK;
}

az_result copy_execution_event_data(
    az_mqtt5_rpc_server_execution_req_event_data* destination,
    az_mqtt5_rpc_server_execution_req_event_data source)
{
  az_span_copy(destination->request_topic, source.request_topic);
  destination->request_topic
      = az_span_slice(destination->request_topic, 0, az_span_size(source.request_topic));
  az_span_copy(destination->response_topic, source.response_topic);
  az_span_copy(destination->request_data, source.request_data);
  az_span_copy(destination->content_type, source.content_type);
  destination->content_type
      = az_span_slice(destination->content_type, 0, az_span_size(source.content_type));
  destination->correlation_id = AZ_SPAN_FROM_BUFFER(server_correlation_id_buffer);
  az_span_copy(destination->correlation_id, source.correlation_id);
  destination->correlation_id
      = az_span_slice(destination->correlation_id, 0, az_span_size(source.correlation_id));

  return AZ_OK;
}

/**
 * @brief Callback function for all clients
 * @note If you add other clients, you can add handling for their events here
 */
az_result mqtt_callback(az_mqtt5_connection* client, az_event event)
{
  (void)client;
  az_app_log_callback(event.type, AZ_SPAN_FROM_STR("APP/callback"));
  switch (event.type)
  {
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    {
      az_mqtt5_connack_data* connack_data = (az_mqtt5_connack_data*)event.data;
      printf(LOG_APP "CONNACK: %d\n", connack_data->connack_reason);

      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_register(&rpc_server_policy));
      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_client_subscribe_begin(&rpc_client_policy));
      break;
    }

    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
    {
      printf(LOG_APP "DISCONNECTED\n");
      sample_finished = true;
      break;
    }

    case AZ_MQTT5_EVENT_RPC_SERVER_EXECUTE_COMMAND_REQ:
    {
      az_mqtt5_rpc_server_execution_req_event_data data
          = *(az_mqtt5_rpc_server_execution_req_event_data*)event.data;
      // can check here for the expected request topic to determine which command to execute
      if (az_span_ptr(pending_server_command.correlation_id) != NULL)
      {
        // can add this command to a queue to be executed if the application supports executing
        // multiple commands at once.
        printf(LOG_APP
               "Received command while another command is executing. Sending error response.\n");
        az_mqtt5_rpc_server_execution_rsp_event_data return_data
            = { .correlation_id = data.correlation_id,
                .error_message = AZ_SPAN_FROM_STR("Can't execute more than one command at a time"),
                .response_topic = data.response_topic,
                .request_topic = data.request_topic,
                .status = AZ_MQTT5_RPC_STATUS_THROTTLED,
                .response = AZ_SPAN_EMPTY,
                .content_type = AZ_SPAN_EMPTY };
        if (az_result_failed(
                az_mqtt5_rpc_server_execution_finish(&rpc_server_policy, &return_data)))
        {
          printf(LOG_APP_ERROR "Failed sending execution response to HFSM\n");
        }
      }
      else
      {
        // Mark that there's a pending command to be executed
        LOG_AND_EXIT_IF_FAILED(copy_execution_event_data(&pending_server_command, data));
        start_timer(NULL, 10000);
        printf(LOG_APP "Added command to queue\n");
      }

      break;
    }

    case AZ_MQTT5_EVENT_RPC_CLIENT_READY_IND:
    {
      az_mqtt5_rpc_client* ready_rpc_client = (az_mqtt5_rpc_client*)event.data;
      if (ready_rpc_client == &rpc_client)
      {
        printf(LOG_APP "RPC Client Ready\n");
        // invoke any queued requests that couldn't be sent earlier?
      }
      else
      {
        printf(LOG_APP_ERROR "Unknown client ready\n");
      }
      break;
    }

    case AZ_MQTT5_EVENT_RPC_CLIENT_RSP:
    {
      az_mqtt5_rpc_client_rsp_event_data* recv_data
          = (az_mqtt5_rpc_client_rsp_event_data*)event.data;
      if (is_command_pending(client_pending_commands, recv_data->correlation_id))
      {
        if (recv_data->status != AZ_MQTT5_RPC_STATUS_OK)
        {
          printf(
              LOG_APP_ERROR "Error response received. Status :%d. Message :%s\n",
              recv_data->status,
              az_span_ptr(recv_data->error_message));
        }
        else
        {
          if (!az_span_is_content_equal(content_type, recv_data->content_type))
          {
            printf(
                LOG_APP_ERROR "Invalid content type. Expected: {%s} Actual: {%s}\n",
                az_span_ptr(content_type),
                az_span_ptr(recv_data->content_type));
          }
          else
          {
            // TODO: deserialize
            printf(
                LOG_APP "Command response received: %s\n",
                az_span_ptr(recv_data->response_payload));
          }
        }
        remove_command(&client_pending_commands, recv_data->correlation_id);
      }
      else
      {
        printf(LOG_APP_ERROR "Request with ");
        print_correlation_id(recv_data->correlation_id);
        printf("not found\n");
      }
      break;
    }

    case AZ_MQTT5_EVENT_RPC_CLIENT_ERROR_RSP:
    {
      az_mqtt5_rpc_client_rsp_event_data* recv_data
          = (az_mqtt5_rpc_client_rsp_event_data*)event.data;
      printf(LOG_APP_ERROR "Broker/Client failure for command ");
      print_correlation_id(recv_data->correlation_id);
      printf(": %s Status: %d\n", az_span_ptr(recv_data->error_message), recv_data->status);
      remove_command(&client_pending_commands, recv_data->correlation_id);
      break;
    }

    case AZ_HFSM_EVENT_ERROR:
      printf(LOG_APP_ERROR "Error Event\n");
      // az_platform_critical_error();
      break;

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
  struct mosquitto* mosq = NULL;

  az_mqtt5_options mqtt5_options = az_mqtt5_options_default();
  // mqtt5_options.certificate_authority_trusted_roots = ca_file;
  mqtt5_options.disable_tls = true;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_init(&mqtt5, &mosq, &mqtt5_options));

  // az_mqtt5_x509_client_certificate primary_credential = (az_mqtt5_x509_client_certificate){
  //   .cert = cert_path1,
  //   .key = key_path1,
  // };

  az_mqtt5_connection_options connection_options = az_mqtt5_connection_options_default();
  connection_options.client_id_buffer = client_id;
  connection_options.username_buffer = username;
  connection_options.password_buffer = AZ_SPAN_EMPTY;
  connection_options.hostname = hostname;
  connection_options.port = 1883;
  // connection_options.client_certificates[0] = primary_credential;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_init(
      &mqtt_connection, &connection_context, &mqtt5, mqtt_callback, &connection_options));

  pending_server_command.request_data = AZ_SPAN_FROM_BUFFER(server_request_payload_buffer);
  pending_server_command.content_type = AZ_SPAN_FROM_BUFFER(server_content_type_buffer);
  pending_server_command.correlation_id = AZ_SPAN_EMPTY;
  pending_server_command.response_topic = AZ_SPAN_FROM_BUFFER(server_response_topic_buffer);
  pending_server_command.request_topic = AZ_SPAN_FROM_BUFFER(server_request_topic_buffer);

  az_mqtt5_property_bag server_property_bag;
  mosquitto_property* server_mosq_prop = NULL;
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_property_bag_init(&server_property_bag, &mqtt5, &server_mosq_prop));

  az_mqtt5_rpc_server_options server_options = az_mqtt5_rpc_server_options_default();
  server_options.subscription_topic_format = AZ_SPAN_FROM_STR("wasmcontroller/{executorId}/command/{name}\0");

  LOG_AND_EXIT_IF_FAILED(az_rpc_server_policy_init(
      &rpc_server_policy,
      &rpc_server,
      &mqtt_connection,
      server_property_bag,
      AZ_SPAN_FROM_BUFFER(server_subscription_topic_buffer),
      AZ_SPAN_EMPTY,
      client_id,
      AZ_SPAN_EMPTY,
      &server_options));

  az_mqtt5_property_bag client_property_bag;
  mosquitto_property* client_mosq_prop = NULL;
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_property_bag_init(&client_property_bag, &mqtt5, &client_mosq_prop));

  az_mqtt5_rpc_client_options client_options = az_mqtt5_rpc_client_options_default();
  client_options.subscription_topic_format = AZ_SPAN_FROM_STR("wasmcontroller/{executorId}/command/{name}/__for_{invokerId}\0");
  client_options.request_topic_format = AZ_SPAN_FROM_STR("wasmcontroller/{executorId}/command/{name}\0");

  LOG_AND_EXIT_IF_FAILED(az_rpc_client_policy_init(
      &rpc_client_policy,
      &rpc_client,
      &mqtt_connection,
      client_property_bag,
      client_id,
      AZ_SPAN_EMPTY,
      AZ_SPAN_EMPTY,
      AZ_SPAN_FROM_BUFFER(client_response_topic_buffer),
      AZ_SPAN_FROM_BUFFER(client_request_topic_buffer),
      AZ_SPAN_FROM_BUFFER(client_subscription_topic_buffer),
      &client_options));

  LOG_AND_EXIT_IF_FAILED(pending_commands_array_init(&client_pending_commands, client_correlation_id_buffers));
  

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_open(&mqtt_connection));

  az_result rc;

  // infinite execution loop
  for (int i = 20; !sample_finished && i > 0; i--)
  {
    // check for commands for the server
    LOG_AND_EXIT_IF_FAILED(check_for_commands());

    // remove any expired commands from the client
    pending_command* expired_command = get_first_expired_command(client_pending_commands);
    while (expired_command != NULL)
    {
      printf(LOG_APP_ERROR "command ");
      print_correlation_id(expired_command->correlation_id);
      printf(" expired\n");
      az_result ret = remove_command(&client_pending_commands, expired_command->correlation_id);
      if (ret != AZ_OK)
      {
        printf(LOG_APP_ERROR "Command not found\n");
      }
      expired_command = get_first_expired_command(client_pending_commands);
    }
#ifdef _WIN32
    Sleep((DWORD)1000);
#else
    sleep(1);
#endif
    printf(LOG_APP "Waiting...\r");
    fflush(stdout);
    // invokes a command every 15 seconds. This cadence/how it is triggered should be customized for
    // your solution.
    if (i % 3 == 0)
    {
      uuid_t new_uuid;
      uuid_generate(new_uuid);
      az_mqtt5_rpc_client_invoke_req_event_data command_data
          = { .correlation_id
              = az_span_create((uint8_t*)new_uuid, AZ_MQTT5_RPC_CORRELATION_ID_LENGTH),
              .content_type = content_type,
              .rpc_server_client_id = server_client_id,
              .command_name = command_name,
              .request_payload = AZ_SPAN_FROM_STR(
                  "{\"RequestTimestamp\":1691530585198,\"RequestedFrom\":\"mobile-app\"}") };
      LOG_AND_EXIT_IF_FAILED(
          add_command(&client_pending_commands, command_data.correlation_id, command_name, 10000));
      rc = az_mqtt5_rpc_client_invoke_begin(&rpc_client_policy, &command_data);
      if (az_result_failed(rc))
      {
        printf(LOG_APP_ERROR "Failed to invoke command with rc: %s\n", az_result_to_string(rc));
        remove_command(&client_pending_commands, command_data.correlation_id);
      }
    }
    else if (i % 4 == 0)
    {
      uuid_t new_uuid;
      uuid_generate(new_uuid);
      az_mqtt5_rpc_client_invoke_req_event_data command_data
          = { .correlation_id
              = az_span_create((uint8_t*)new_uuid, AZ_MQTT5_RPC_CORRELATION_ID_LENGTH),
              .content_type = content_type,
              .rpc_server_client_id = server_client_id,
              .command_name = command_name2,
              .request_payload = AZ_SPAN_FROM_STR(
                  "{\"RequestTimestamp\":1691530585198,\"RequestedFrom\":\"mobile-app\"}") };
      LOG_AND_EXIT_IF_FAILED(
          add_command(&client_pending_commands, command_data.correlation_id, command_name2, 10000));
      rc = az_mqtt5_rpc_client_invoke_begin(&rpc_client_policy, &command_data);
      if (az_result_failed(rc))
      {
        printf(LOG_APP_ERROR "Failed to invoke command with rc: %s\n", az_result_to_string(rc));
        remove_command(&client_pending_commands, command_data.correlation_id);
      }
    }
  }

  // clean-up functions shown for completeness
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_close(&mqtt_connection));

  if (mosq != NULL)
  {
    mosquitto_loop_stop(mosq, false);
    mosquitto_destroy(mosq);
  }

  // mosquitto allocates the property bag for us, but we're responsible for free'ing it
  mosquitto_property_free_all(&client_mosq_prop);
  mosquitto_property_free_all(&server_mosq_prop);

  if (mosquitto_lib_cleanup() != MOSQ_ERR_SUCCESS)
  {
    printf(LOG_APP "Failed to cleanup MosquittoLib\n");
    return -1;
  }

  printf(LOG_APP "Done.                                \n");
  return 0;
}

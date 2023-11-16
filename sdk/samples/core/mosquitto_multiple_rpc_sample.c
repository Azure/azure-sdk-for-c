/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 * @brief RPC Server & Client sample for Mosquitto MQTT that supports multiple commands under the
 * same subscription
 *
 */

#include <az_log_listener.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#include "rpc_client_pending_commands.h"
#include <azure/az_core.h>
#include <azure/core/az_log.h>
#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_mqtt5_rpc_client.h>

// User-defined parameters
#define SERVER_COMMAND_TIMEOUT_MS 10000
#define CLIENT_COMMAND_TIMEOUT_MS 10000
static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to cert pem file>");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to cert key file>");
static const az_span client_id = AZ_SPAN_LITERAL_FROM_STR("application-id");
static const az_span username = AZ_SPAN_LITERAL_FROM_STR("application-id");
static const az_span hostname = AZ_SPAN_LITERAL_FROM_STR("localhost");
static const az_span server_client_id = AZ_SPAN_LITERAL_FROM_STR("controller-id");
static const az_span content_type = AZ_SPAN_LITERAL_FROM_STR("application/json");
static const az_span start_module_command_name = AZ_SPAN_LITERAL_FROM_STR("startModule");
static const az_span stop_module_command_name = AZ_SPAN_LITERAL_FROM_STR("stopModule");

// Static memory allocation
static char client_response_topic_buffer[256];
static char client_request_topic_buffer[256];
static char client_subscription_topic_buffer[256];
static uint8_t client_correlation_id_buffer[AZ_MQTT5_RPC_CORRELATION_ID_LENGTH];
static uint8_t client_correlation_id_buffers[RPC_CLIENT_MAX_PENDING_COMMANDS]
                                            [AZ_MQTT5_RPC_CORRELATION_ID_LENGTH];

static char server_subscription_topic_buffer[256];

// for pending_server_command
static char server_correlation_id_buffer[256];
static char server_response_topic_buffer[256];
static char server_request_topic_buffer[256];
static char server_request_payload_buffer[256];
static char server_content_type_buffer[256];

// State variables
static az_mqtt5_connection mqtt_connection;
static az_context connection_context;

static az_mqtt5_rpc_server_codec rpc_server_codec;
static az_mqtt5_rpc_server rpc_server;
static az_mqtt5_rpc_client_codec rpc_client_codec;
static az_mqtt5_rpc_client rpc_client;

volatile bool sample_finished = false;

static pending_commands_array client_pending_commands;
static az_mqtt5_rpc_server_execution_req_event_data pending_server_command;
static az_platform_mutex pending_server_command_mutex;

static _az_platform_timer timer;

az_mqtt5_rpc_status execute_command();
az_result check_for_commands_to_execute();
az_result copy_execution_event_data(
    az_mqtt5_rpc_server_execution_req_event_data* destination,
    az_mqtt5_rpc_server_execution_req_event_data source);
az_result mqtt_callback(az_mqtt5_connection* client, az_event event, void* callback_context);
void handle_response(az_span response_payload);
az_result invoke_stop_module();
az_result invoke_start_module();
az_result invoke_begin(az_span command_name, az_span payload);
void remove_expired_commands();

void az_platform_critical_error()
{
  printf(LOG_APP_ERROR "\x1B[31mPANIC!\x1B[0m\n");

  while (1)
    ;
}

/**
 * @brief On command timeout, send an error response with timeout details to the rpc_server callback
 * @note May need to be modified for your solution
 */
static void timer_callback(void* callback_context)
{
  (void)callback_context;
  if (!az_result_succeeded(az_platform_mutex_acquire(&pending_server_command_mutex)))
  {
    printf(LOG_APP_ERROR "Failed to acquire pending command mutex.\n");
    return;
  }

  if (az_result_failed(az_platform_timer_destroy(&timer)))
  {
    printf(LOG_APP_ERROR "Failed destroying timer\n");
    return;
  }

  printf(LOG_APP_ERROR "Command execution timed out.\n");
  az_mqtt5_rpc_server_execution_rsp_event_data return_data
      = { .correlation_id = pending_server_command.correlation_id,
          .error_message = AZ_SPAN_FROM_STR("Command Server timeout"),
          .response_topic = pending_server_command.response_topic,
          .request_topic = pending_server_command.request_topic,
          .status = AZ_MQTT5_RPC_STATUS_TIMEOUT,
          .response = AZ_SPAN_EMPTY,
          .content_type = AZ_SPAN_EMPTY };
  if (az_result_failed(az_mqtt5_rpc_server_execution_finish(&rpc_server, &return_data)))
  {
    printf(LOG_APP_ERROR "Failed sending execution response to rpc_server callback\n");
    return;
  }

  pending_server_command.content_type = AZ_SPAN_FROM_BUFFER(server_content_type_buffer);
  pending_server_command.request_topic = AZ_SPAN_FROM_BUFFER(server_request_topic_buffer);
  pending_server_command.response_topic = AZ_SPAN_FROM_BUFFER(server_response_topic_buffer);
  pending_server_command.request_data = AZ_SPAN_FROM_BUFFER(server_request_payload_buffer);
  pending_server_command.correlation_id = AZ_SPAN_EMPTY;

  if (!az_result_succeeded(az_platform_mutex_release(&pending_server_command_mutex)))
  {
    printf(LOG_APP_ERROR "Failed to release pending command mutex.\n");
  }
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
 * timed out, send the result back to the rpc_server callback
 * @note Result to be sent back to the rpc_server callback needs to be modified for your solution
 */
az_result check_for_commands_to_execute()
{
  LOG_AND_EXIT_IF_FAILED(az_platform_mutex_acquire(&pending_server_command_mutex));
  if (az_span_ptr(pending_server_command.correlation_id) != NULL)
  {
    // copy correlation id to a new span so we can compare it later
    int32_t correlation_id_size = az_span_size(pending_server_command.correlation_id);
    uint8_t correlation_id_copy_buffer[correlation_id_size];
    az_span correlation_id_copy = az_span_create(correlation_id_copy_buffer, correlation_id_size);
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

    // TODO: deserialize request

    LOG_AND_EXIT_IF_FAILED(az_platform_mutex_release(&pending_server_command_mutex));
    rc = execute_command();
    LOG_AND_EXIT_IF_FAILED(az_platform_mutex_acquire(&pending_server_command_mutex));

    // if command hasn't timed out, send result back
    if (az_span_is_content_equal(correlation_id_copy, pending_server_command.correlation_id))
    {
      LOG_AND_EXIT_IF_FAILED(az_platform_timer_destroy(&timer));
      az_span response_payload = AZ_SPAN_EMPTY;
      // Serialize response here if needed

      /* Modify the response/error message/status as needed for your solution */
      az_mqtt5_rpc_server_execution_rsp_event_data return_data
          = { .correlation_id = pending_server_command.correlation_id,
              .response = response_payload,
              .response_topic = pending_server_command.response_topic,
              .request_topic = pending_server_command.request_topic,
              .status = rc,
              .content_type = content_type,
              .error_message = error_message };
      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_execution_finish(&rpc_server, &return_data));

      pending_server_command.content_type = AZ_SPAN_FROM_BUFFER(server_content_type_buffer);
      pending_server_command.request_topic = AZ_SPAN_FROM_BUFFER(server_request_topic_buffer);
      pending_server_command.response_topic = AZ_SPAN_FROM_BUFFER(server_response_topic_buffer);
      pending_server_command.request_data = AZ_SPAN_FROM_BUFFER(server_request_payload_buffer);
      pending_server_command.correlation_id = AZ_SPAN_EMPTY;
    }
  }
  LOG_AND_EXIT_IF_FAILED(az_platform_mutex_release(&pending_server_command_mutex));
  return AZ_OK;
}

az_result copy_execution_event_data(
    az_mqtt5_rpc_server_execution_req_event_data* destination,
    az_mqtt5_rpc_server_execution_req_event_data source)
{
  az_span_copy(destination->request_topic, source.request_topic);
  destination->request_topic
      = az_span_slice(destination->request_topic, 0, az_span_size(source.request_topic));
  // add null terminator to end of topic
  az_span temp_response_topic = az_span_copy(destination->response_topic, source.response_topic);
  az_span_copy_u8(temp_response_topic, '\0');
  destination->response_topic
      = az_span_slice(destination->response_topic, 0, az_span_size(source.response_topic));
  az_span_copy(destination->request_data, source.request_data);
  destination->request_data
      = az_span_slice(destination->request_data, 0, az_span_size(source.request_data));
  az_span_copy(destination->content_type, source.content_type);
  destination->content_type
      = az_span_slice(destination->content_type, 0, az_span_size(source.content_type));
  destination->correlation_id = AZ_SPAN_FROM_BUFFER(server_correlation_id_buffer);
  az_span_copy(destination->correlation_id, source.correlation_id);
  destination->correlation_id
      = az_span_slice(destination->correlation_id, 0, az_span_size(source.correlation_id));
  return AZ_OK;
}

void handle_response(az_span response_payload)
{
  printf(LOG_APP "Command response received: %s\n", az_span_ptr(response_payload));
}

/**
 * @brief MQTT client callback function for all clients
 * @note If you add other clients, you can add handling for their events here
 */
az_result mqtt_callback(az_mqtt5_connection* client, az_event event, void* callback_context)
{
  (void)client;
  (void)callback_context;
  az_app_log_callback(event.type, AZ_SPAN_FROM_STR("APP/callback"));
  switch (event.type)
  {
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    {
      az_mqtt5_connack_data* connack_data = (az_mqtt5_connack_data*)event.data;
      printf(LOG_APP "CONNACK: reason=%d\n", connack_data->connack_reason);

      if (connack_data->connack_reason == 0)
      {
        LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_register(&rpc_server));
        LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_client_subscribe_begin(&rpc_client));
      }
      else
      {
        sample_finished = true;
      }
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
        printf(
            LOG_APP
            "Received new command while another command is executing. Sending error response.\n");
        // add null terminator to end of response topic
        int32_t null_terminated_response_topic_size = az_span_size(data.response_topic) + 1;
        uint8_t temp_response_topic_buffer[null_terminated_response_topic_size];
        az_span null_terminated_response_topic
            = az_span_create(temp_response_topic_buffer, null_terminated_response_topic_size);
        az_span temp_response_topic
            = az_span_copy(null_terminated_response_topic, data.response_topic);
        az_span_copy_u8(temp_response_topic, '\0');
        az_mqtt5_rpc_server_execution_rsp_event_data return_data
            = { .correlation_id = data.correlation_id,
                .error_message = AZ_SPAN_FROM_STR("Can't execute more than one command at a time"),
                .response_topic = null_terminated_response_topic,
                .request_topic = data.request_topic,
                .status = AZ_MQTT5_RPC_STATUS_THROTTLED,
                .response = AZ_SPAN_EMPTY,
                .content_type = AZ_SPAN_EMPTY };
        if (az_result_failed(az_mqtt5_rpc_server_execution_finish(&rpc_server, &return_data)))
        {
          printf(LOG_APP_ERROR "Failed sending execution response to rpc_server callback\n");
        }
      }
      else
      {
        // Mark that there's a pending command to be executed
        LOG_AND_EXIT_IF_FAILED(az_platform_mutex_acquire(&pending_server_command_mutex));
        LOG_AND_EXIT_IF_FAILED(copy_execution_event_data(&pending_server_command, data));
        LOG_AND_EXIT_IF_FAILED(az_platform_timer_create(&timer, timer_callback, NULL));
        LOG_AND_EXIT_IF_FAILED(az_platform_timer_start(&timer, SERVER_COMMAND_TIMEOUT_MS));
        LOG_AND_EXIT_IF_FAILED(az_platform_mutex_release(&pending_server_command_mutex));
        printf(LOG_APP "Added command to queue\n");
      }

      break;
    }

    case AZ_MQTT5_EVENT_RPC_CLIENT_READY_IND:
    {
      az_mqtt5_rpc_client_codec* ready_rpc_client_codec = (az_mqtt5_rpc_client_codec*)event.data;
      if (ready_rpc_client_codec == &rpc_client_codec)
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
            // TODO: deserialize before passing to handle_response
            handle_response(recv_data->response_payload);
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
      break;

    default:
      break;
  }

  return AZ_OK;
}

/**
 * @brief Removes any expired commands from the client_pending_commands array
 * @note Even if a command has expired, if we get a response for it, we will still receive an event
 * with the results in the mqtt_callback
 */
void remove_expired_commands()
{
  pending_command* expired_command = get_first_expired_command(client_pending_commands);
  while (expired_command != NULL)
  {
    printf(LOG_APP_ERROR "command ");
    print_correlation_id(expired_command->correlation_id);
    printf(" expired\n");
    az_result ret = remove_command(&client_pending_commands, expired_command->correlation_id);
    if (ret != AZ_OK)
    {
      printf(LOG_APP_ERROR "Expired command not a pending command\n");
    }
    expired_command = get_first_expired_command(client_pending_commands);
  }
}

az_result invoke_begin(az_span command_name, az_span payload)
{
  uuid_t new_uuid;
  uuid_generate(new_uuid);
  az_mqtt5_rpc_client_invoke_req_event_data command_data
      = { .correlation_id = az_span_create((uint8_t*)new_uuid, AZ_MQTT5_RPC_CORRELATION_ID_LENGTH),
          .content_type = content_type,
          .rpc_server_client_id = server_client_id,
          .command_name = command_name,
          .request_payload = payload };
  LOG_AND_EXIT_IF_FAILED(add_command(
      &client_pending_commands,
      command_data.correlation_id,
      command_name,
      CLIENT_COMMAND_TIMEOUT_MS));
  az_result rc = az_mqtt5_rpc_client_invoke_begin(&rpc_client, &command_data);
  if (az_result_failed(rc))
  {
    printf(
        LOG_APP_ERROR "Failed to invoke command '%s' with rc: %s\n",
        az_span_ptr(command_name),
        az_result_to_string(rc));
    remove_command(&client_pending_commands, command_data.correlation_id);
  }
  return AZ_OK;
}

az_result invoke_start_module()
{
  // TODO: Payload should be generated and serialized
  return invoke_begin(
      start_module_command_name,
      AZ_SPAN_FROM_STR("{\"RequestTimestamp\":1691530585198,\"RequestedFrom\":\"mobile-app\"}"));
}

az_result invoke_stop_module()
{
  // TODO: Payload should be generated and serialized
  return invoke_begin(
      stop_module_command_name,
      AZ_SPAN_FROM_STR("{\"RequestTimestamp\":1691530585198,\"RequestedFrom\":\"mobile-app\"}"));
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

  printf(LOG_APP "Using MosquittoLib version %d\n", mosquitto_lib_version(NULL, NULL, NULL));

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

  connection_context = az_context_create_with_expiration(
      &az_context_application, az_context_get_expiration(&az_context_application));

  az_mqtt5 mqtt5;
  struct mosquitto* mosq = NULL;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_init(&mqtt5, &mosq, NULL));

  az_mqtt5_x509_client_certificate primary_credential = (az_mqtt5_x509_client_certificate){
    .cert = cert_path1,
    .key = key_path1,
  };

  az_mqtt5_connection_options connection_options = az_mqtt5_connection_options_default();
  connection_options.client_id_buffer = client_id;
  connection_options.username_buffer = username;
  connection_options.password_buffer = AZ_SPAN_EMPTY;
  connection_options.hostname = hostname;
  connection_options.client_certificates[0] = primary_credential;
  connection_options.client_certificate_count = 1;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_init(
      &mqtt_connection, &connection_context, &mqtt5, mqtt_callback, &connection_options, NULL));

  LOG_AND_EXIT_IF_FAILED(az_platform_mutex_init(&pending_server_command_mutex));

  pending_server_command.request_data = AZ_SPAN_FROM_BUFFER(server_request_payload_buffer);
  pending_server_command.content_type = AZ_SPAN_FROM_BUFFER(server_content_type_buffer);
  pending_server_command.correlation_id = AZ_SPAN_EMPTY;
  pending_server_command.response_topic = AZ_SPAN_FROM_BUFFER(server_response_topic_buffer);
  pending_server_command.request_topic = AZ_SPAN_FROM_BUFFER(server_request_topic_buffer);

  az_mqtt5_property_bag server_property_bag;
  mosquitto_property* server_mosq_prop = NULL;
  LOG_AND_EXIT_IF_FAILED(
      az_mqtt5_property_bag_init(&server_property_bag, &mqtt5, &server_mosq_prop));

  az_mqtt5_rpc_server_codec_options server_codec_options
      = az_mqtt5_rpc_server_codec_options_default();

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_init(
      &rpc_server,
      &rpc_server_codec,
      &mqtt_connection,
      server_property_bag,
      AZ_SPAN_FROM_BUFFER(server_subscription_topic_buffer),
      AZ_SPAN_EMPTY,
      client_id,
      AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS,
      &server_codec_options));

  az_mqtt5_property_bag client_property_bag;
  mosquitto_property* client_mosq_prop = NULL;
  LOG_AND_EXIT_IF_FAILED(
      az_mqtt5_property_bag_init(&client_property_bag, &mqtt5, &client_mosq_prop));

  az_mqtt5_rpc_client_codec_options client_codec_options
      = az_mqtt5_rpc_client_codec_options_default();

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_client_init(
      &rpc_client,
      &rpc_client_codec,
      &mqtt_connection,
      client_property_bag,
      client_id,
      AZ_SPAN_EMPTY,
      AZ_SPAN_FROM_BUFFER(client_response_topic_buffer),
      AZ_SPAN_FROM_BUFFER(client_request_topic_buffer),
      AZ_SPAN_FROM_BUFFER(client_subscription_topic_buffer),
      AZ_SPAN_FROM_BUFFER(client_correlation_id_buffer),
      AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS,
      AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS,
      &client_codec_options));

  LOG_AND_EXIT_IF_FAILED(
      pending_commands_array_init(&client_pending_commands, client_correlation_id_buffers));

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_open(&mqtt_connection));

  // infinite execution loop
  for (int i = 0; !sample_finished; i++)
  {
    printf(LOG_APP "Waiting...\r");
    fflush(stdout);

    // check for commands for the server
    LOG_AND_EXIT_IF_FAILED(check_for_commands_to_execute());

    // remove any expired commands from the client
    remove_expired_commands();

    // invokes a start module command every 15 seconds and a stop module command every 10 seconds.
    // This cadence/how it is triggered should be customized for your solution.
    if (i % 15 == 0)
    {
      LOG_AND_EXIT_IF_FAILED(invoke_start_module());
    }
    else if (i % 10 == 0)
    {
      LOG_AND_EXIT_IF_FAILED(invoke_stop_module());
    }
    LOG_AND_EXIT_IF_FAILED(az_platform_sleep_msec(1000));
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

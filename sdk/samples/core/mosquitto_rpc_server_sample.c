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

#include "mosquitto_rpc_server_sample_json_parser.h"
#include <azure/az_core.h>
#include <azure/core/az_log.h>

// User-defined parameters
#define SERVER_COMMAND_TIMEOUT_MS 10000
static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to cert pem file>");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to cert key file>");
static const az_span client_id = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span username = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span hostname = AZ_SPAN_LITERAL_FROM_STR("<hostname>");
static const az_span command_name = AZ_SPAN_LITERAL_FROM_STR("unlock");
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:rpc:samples:vehicle;1");
static const az_span content_type = AZ_SPAN_LITERAL_FROM_STR("application/json");
static const az_span subscription_topic_format
    = AZ_SPAN_LITERAL_FROM_STR("vehicles/{serviceId}/commands/{executorId}/{name}");

// Static memory allocation.
static char subscription_topic_buffer[256];
static char response_payload_buffer[256];

// for pending_command
static char correlation_id_buffer[256];
static char response_topic_buffer[256];
static char request_topic_buffer[256];
static char request_payload_buffer[256];
static char content_type_buffer[256];

// State variables
static az_mqtt5_connection mqtt_connection;
static az_context connection_context;

static az_mqtt5_rpc_server rpc_server;
static az_mqtt5_rpc_server_policy rpc_server_policy;

volatile bool sample_finished = false;

static az_mqtt5_rpc_server_execution_req_event_data pending_command;
static az_platform_mutex pending_command_mutex;

static _az_platform_timer timer;

az_mqtt5_rpc_status execute_command(unlock_request req);
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
 * @brief On command timeout, send an error response with timeout details to the rpc_server callback
 * @note May need to be modified for your solution
 */
static void timer_callback(void* callback_context)
{
  (void)callback_context;
  if (!az_result_succeeded(az_platform_mutex_acquire(&pending_command_mutex)))
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
      = { .correlation_id = pending_command.correlation_id,
          .error_message = AZ_SPAN_FROM_STR("Command Server timeout"),
          .response_topic = pending_command.response_topic,
          .request_topic = pending_command.request_topic,
          .status = AZ_MQTT5_RPC_STATUS_TIMEOUT,
          .response = AZ_SPAN_EMPTY,
          .content_type = AZ_SPAN_EMPTY };
  if (az_result_failed(az_mqtt5_rpc_server_execution_finish(&rpc_server_policy, &return_data)))
  {
    printf(LOG_APP_ERROR "Failed sending execution response to rpc_server callback\n");
    return;
  }

  pending_command.content_type = AZ_SPAN_FROM_BUFFER(content_type_buffer);
  pending_command.request_topic = AZ_SPAN_FROM_BUFFER(request_topic_buffer);
  pending_command.response_topic = AZ_SPAN_FROM_BUFFER(response_topic_buffer);
  pending_command.request_data = AZ_SPAN_FROM_BUFFER(request_payload_buffer);
  pending_command.correlation_id = AZ_SPAN_EMPTY;

  if (!az_result_succeeded(az_platform_mutex_release(&pending_command_mutex)))
  {
    printf(LOG_APP_ERROR "Failed to release pending command mutex.\n");
  }
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
 * timed out, send the result back to the rpc_server callback
 * @note Result to be sent back to the rpc_server callback needs to be modified for your solution
 */
az_result check_for_commands()
{
  LOG_AND_EXIT_IF_FAILED(az_platform_mutex_acquire(&pending_command_mutex));
  if (az_span_ptr(pending_command.correlation_id) != NULL)
  {
    // copy correlation id to a new span so we can compare it later
    uint8_t copy_buffer[az_span_size(pending_command.correlation_id)];
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
    az_mqtt5_rpc_status rc;
    az_span error_message = AZ_SPAN_EMPTY;

    if (az_result_failed(deserialize_unlock_request(pending_command.request_data, &req)))
    {
      printf(LOG_APP_ERROR "Failed to deserialize request\n");
      rc = AZ_MQTT5_RPC_STATUS_UNSUPPORTED_TYPE;
      error_message = az_span_create_from_str("Failed to deserialize unlock command request.");
    }
    else
    {
      LOG_AND_EXIT_IF_FAILED(az_platform_mutex_release(&pending_command_mutex));
      rc = execute_command(req);
      LOG_AND_EXIT_IF_FAILED(az_platform_mutex_acquire(&pending_command_mutex));
    }

    // if command hasn't timed out, send result back
    if (az_span_is_content_equal(correlation_id_copy, pending_command.correlation_id))
    {
      LOG_AND_EXIT_IF_FAILED(az_platform_timer_destroy(&timer));
      az_span response_payload = AZ_SPAN_EMPTY;
      if (rc == AZ_MQTT5_RPC_STATUS_OK)
      {
        // Serialize response
        response_payload = AZ_SPAN_FROM_BUFFER(response_payload_buffer);
        LOG_AND_EXIT_IF_FAILED(serialize_response_payload(req, response_payload));
      }

      /* Modify the response/error message/status as needed for your solution */
      az_mqtt5_rpc_server_execution_rsp_event_data return_data
          = { .correlation_id = pending_command.correlation_id,
              .response = response_payload,
              .response_topic = pending_command.response_topic,
              .request_topic = pending_command.request_topic,
              .status = rc,
              .content_type = content_type,
              .error_message = error_message };
      LOG_AND_EXIT_IF_FAILED(
          az_mqtt5_rpc_server_execution_finish(&rpc_server_policy, &return_data));

      pending_command.content_type = AZ_SPAN_FROM_BUFFER(content_type_buffer);
      pending_command.request_topic = AZ_SPAN_FROM_BUFFER(request_topic_buffer);
      pending_command.response_topic = AZ_SPAN_FROM_BUFFER(response_topic_buffer);
      pending_command.request_data = AZ_SPAN_FROM_BUFFER(request_payload_buffer);
      pending_command.correlation_id = AZ_SPAN_EMPTY;
    }
  }
  LOG_AND_EXIT_IF_FAILED(az_platform_mutex_release(&pending_command_mutex));
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
  destination->correlation_id = AZ_SPAN_FROM_BUFFER(correlation_id_buffer);
  az_span_copy(destination->correlation_id, source.correlation_id);
  destination->correlation_id
      = az_span_slice(destination->correlation_id, 0, az_span_size(source.correlation_id));
  return AZ_OK;
}

/**
 * @brief MQTT client callback function for all clients
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
      printf(LOG_APP "CONNACK: reason=%d\n", connack_data->connack_reason);

      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_register(&rpc_server_policy));
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
      if (az_span_ptr(pending_command.correlation_id) != NULL)
      {
        // can add this command to a queue to be executed if the application supports executing
        // multiple commands at once.
        printf(
            LOG_APP
            "Received new command while another command is executing. Sending error response.\n");
        // add null terminator to end of response topic
        uint8_t temp_response_topic_buffer[az_span_size(data.response_topic) + (int32_t)1];
        az_span null_terminated_response_topic = AZ_SPAN_FROM_BUFFER(temp_response_topic_buffer);
        az_span temp_response_topic = az_span_copy(null_terminated_response_topic, data.response_topic);
        az_span_copy_u8(temp_response_topic, '\0');
        az_mqtt5_rpc_server_execution_rsp_event_data return_data
            = { .correlation_id = data.correlation_id,
                .error_message = AZ_SPAN_FROM_STR("Can't execute more than one command at a time"),
                .response_topic = null_terminated_response_topic,
                .request_topic = data.request_topic,
                .status = AZ_MQTT5_RPC_STATUS_THROTTLED,
                .response = AZ_SPAN_EMPTY,
                .content_type = AZ_SPAN_EMPTY };
        if (az_result_failed(
                az_mqtt5_rpc_server_execution_finish(&rpc_server_policy, &return_data)))
        {
          printf(LOG_APP_ERROR "Failed sending execution response to rpc_server callback\n");
        }
      }
      else
      {
        // Mark that there's a pending command to be executed
        LOG_AND_EXIT_IF_FAILED(az_platform_mutex_acquire(&pending_command_mutex));
        LOG_AND_EXIT_IF_FAILED(copy_execution_event_data(&pending_command, data));
        LOG_AND_EXIT_IF_FAILED(az_platform_timer_create(&timer, timer_callback, NULL));
        LOG_AND_EXIT_IF_FAILED(az_platform_timer_start(&timer, SERVER_COMMAND_TIMEOUT_MS));
        LOG_AND_EXIT_IF_FAILED(az_platform_mutex_release(&pending_command_mutex));
        printf(LOG_APP "Added command to queue\n");
      }

      break;
    }

    default:
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

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_init(
      &mqtt_connection, &connection_context, &mqtt5, mqtt_callback, &connection_options));

  LOG_AND_EXIT_IF_FAILED(az_platform_mutex_init(&pending_command_mutex));

  pending_command.request_data = AZ_SPAN_FROM_BUFFER(request_payload_buffer);
  pending_command.content_type = AZ_SPAN_FROM_BUFFER(content_type_buffer);
  pending_command.correlation_id = AZ_SPAN_EMPTY;
  pending_command.response_topic = AZ_SPAN_FROM_BUFFER(response_topic_buffer);
  pending_command.request_topic = AZ_SPAN_FROM_BUFFER(request_topic_buffer);

  az_mqtt5_property_bag property_bag;
  mosquitto_property* mosq_prop = NULL;
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_property_bag_init(&property_bag, &mqtt5, &mosq_prop));

  az_mqtt5_rpc_server_options server_options = az_mqtt5_rpc_server_options_default();
  server_options.subscription_topic_format = subscription_topic_format;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_policy_init(
      &rpc_server_policy,
      &rpc_server,
      &mqtt_connection,
      property_bag,
      AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
      model_id,
      client_id,
      command_name,
      &server_options));

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_open(&mqtt_connection));

  // infinite execution loop
  while (!sample_finished)
  {
    LOG_AND_EXIT_IF_FAILED(check_for_commands());
    printf(LOG_APP "Waiting...\r");
    fflush(stdout);
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
  mosquitto_property_free_all(&mosq_prop);

  if (mosquitto_lib_cleanup() != MOSQ_ERR_SUCCESS)
  {
    printf(LOG_APP "Failed to cleanup MosquittoLib\n");
    return -1;
  }

  printf(LOG_APP "Done.                                \n");
  return 0;
}

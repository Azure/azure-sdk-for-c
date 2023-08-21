/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 * @brief Mosquitto rpc client sample
 *
 */

#include <az_log_listener.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/az_core.h>
#include <azure/core/az_log.h>
#include <azure/core/az_mqtt5_rpc_client.h>
#include <azure/core/az_mqtt5_rpc.h>
#include "rpc_client_command_hash_table.h"

#ifdef _WIN32
// Required for Sleep(DWORD)
#include <Windows.h>
#else
// Required for sleep(unsigned int)
#include <unistd.h>
#endif

static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to pem file>");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to key file>");
static const az_span client_id = AZ_SPAN_LITERAL_FROM_STR("mobile-app");
static const az_span username = AZ_SPAN_LITERAL_FROM_STR("mobile-app");
static const az_span hostname = AZ_SPAN_LITERAL_FROM_STR("<hostname>");
static const az_span command_name = AZ_SPAN_LITERAL_FROM_STR("unlock");
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:rpc:samples:vehicle;1");
static const az_span executor_client_id = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span content_type = AZ_SPAN_LITERAL_FROM_STR("application/json");

static char response_topic_buffer[256];
static char request_topic_buffer[256];

pending_command* pending_commands = NULL;

static az_mqtt5_connection mqtt_connection;
static az_context connection_context;

static az_mqtt5_rpc_client_hfsm rpc_client;
static az_mqtt5_rpc_client underlying_rpc_client;

volatile bool sample_finished = false;

az_result iot_callback(az_mqtt5_connection* client, az_event event);

void az_platform_critical_error()
{
  printf(LOG_APP_ERROR "\x1B[31mPANIC!\x1B[0m\n");

  while (1)
    ;
}

/**
 * @brief Callback function for all clients
 * @note If you add other clients, you can add handling for their events here
 */
az_result iot_callback(az_mqtt5_connection* client, az_event event)
{
  (void)client;
  az_app_log_callback(event.type, AZ_SPAN_FROM_STR("APP/callback"));
  switch (event.type)
  {
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    {
      az_mqtt5_connack_data* connack_data = (az_mqtt5_connack_data*)event.data;
      printf(LOG_APP "CONNACK: %d\n", connack_data->connack_reason);

      az_mqtt5_rpc_client_command_req_event_data command_data = {
        .correlation_id = az_rpc_client_generate_correlation_id(),
        .content_type = content_type,
        .request_payload = AZ_SPAN_FROM_STR("{\"RequestTimestamp\":1691530585198,\"RequestedFrom\":\"mobile-app\"}")
      };

      pending_commands = add_command(pending_commands, command_data.correlation_id, 10000);

      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_client_invoke_command(
        &rpc_client,
        &command_data));

      break;
    }

    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
    {
      printf(LOG_APP "DISCONNECTED\n");
      sample_finished = true;
      break;
    }

    case AZ_EVENT_RPC_CLIENT_COMMAND_RSP:
    {
      az_result ret;
      az_mqtt5_rpc_client_command_rsp_event_data* recv_data = (az_mqtt5_rpc_client_command_rsp_event_data*)event.data;
      if (recv_data->parsing_failure)
      {
        printf(LOG_APP_ERROR "Parsing failure for command %s: %s\n", az_span_ptr(recv_data->correlation_id), az_span_ptr(recv_data->error_message));
        ret = remove_command(&pending_commands, recv_data->correlation_id);

      }
      else if (is_pending_command(pending_commands, recv_data->correlation_id))
      {
        if (recv_data->status != AZ_MQTT5_RPC_STATUS_OK)
        {
          printf(LOG_APP_ERROR "Error response received. Status :%d. Message :%s\n", recv_data->status, az_span_ptr(recv_data->error_message));
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
            // deserialize
            printf(LOG_APP "Command response received: %s\n", az_span_ptr(recv_data->response_payload));
          }
        }
        ret = remove_command(&pending_commands, recv_data->correlation_id);
      }
      else
      {
        printf(LOG_APP_ERROR "Request with rid: %s not found\n", az_span_ptr(recv_data->correlation_id));
      }
      (void)ret;
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
  connection_options.password_buffer = AZ_SPAN_EMPTY;
  connection_options.hostname = hostname;
  connection_options.client_certificates[0] = primary_credential;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_init(
      &mqtt_connection, &connection_context, &mqtt5, iot_callback, &connection_options));

  az_mqtt5_property_bag property_bag;
  mosquitto_property* mosq_prop = NULL;
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_property_bag_init(&property_bag, &mqtt5, &mosq_prop));

  LOG_AND_EXIT_IF_FAILED(az_rpc_client_hfsm_init(
      &rpc_client,
      &underlying_rpc_client,
      &mqtt_connection,
      property_bag,
      client_id,
      model_id,
      executor_client_id,
      command_name,
      AZ_SPAN_FROM_BUFFER(response_topic_buffer),
      AZ_SPAN_FROM_BUFFER(request_topic_buffer),
      NULL));
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_open(&mqtt_connection));

  // infinite execution loop
  for (int i = 45; !sample_finished && i > 0; i++)
  {
    pending_command* expired_command = get_first_expired_command(pending_commands);
    while (expired_command != NULL)
    {
      printf(LOG_APP_ERROR "command %s expired\n", az_span_ptr(expired_command->correlation_id));
      az_result ret = remove_command(&pending_commands, expired_command->correlation_id);
      if (ret != AZ_OK)
      {
        printf(LOG_APP_ERROR "Command not found\n");
      }
      expired_command = get_first_expired_command(pending_commands);
    }
#ifdef _WIN32
    Sleep((DWORD)1000);
#else
    sleep(1);
#endif
    printf(LOG_APP "Waiting...\r");
    fflush(stdout);
    if (i % 15 == 0)
    {
      az_mqtt5_rpc_client_command_req_event_data command_data = {
        .correlation_id = az_rpc_client_generate_correlation_id(),
        .content_type = content_type,
        .request_payload = AZ_SPAN_FROM_STR("{\"RequestTimestamp\":1691530585198,\"RequestedFrom\":\"mobile-app\"}")
      };
      pending_commands = add_command(pending_commands, command_data.correlation_id, 10000);
      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_client_invoke_command(
        &rpc_client,
        &command_data));
    }
  }

  // clean-up functions shown for completeness
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_close(&mqtt_connection));

  if (mqtt5._internal.mosquitto_handle != NULL)
  {
    mosquitto_loop_stop(mqtt5._internal.mosquitto_handle, false);
    mosquitto_destroy(mqtt5._internal.mosquitto_handle);
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

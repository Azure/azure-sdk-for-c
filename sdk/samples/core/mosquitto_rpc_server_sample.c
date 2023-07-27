/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 * @brief Mosquitto async callback
 *
 */

#include <az_log_listener.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/az_core.h>
#include <azure/core/az_log.h>

static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to pem file>");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to pem file>");
static const az_span client_id = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span username = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span password = AZ_SPAN_EMPTY;
static const az_span hostname = AZ_SPAN_LITERAL_FROM_STR("<hostname>");
static const az_span command_name = AZ_SPAN_LITERAL_FROM_STR("unlock");
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:rpc:samples:vehicle;1");

static char correlation_id_buffer[37];
static char response_topic_buffer[256];
static char sub_topic_buffer[256];

static az_mqtt5_connection iot_connection;
static az_context connection_context;

static az_mqtt5_rpc_server rpc_server;

volatile bool connected = false;

static az_mqtt5_rpc_server_options rpc_server_options;

void az_platform_critical_error()
{
  printf(LOG_APP "\x1B[31mPANIC!\x1B[0m\n");

  while (1)
    ;
}

az_mqtt5_rpc_status execute_command(az_mqtt5_rpc_server_command_data* command_data)
{
  // for now, just print details from the command
  printf(LOG_APP "Executing command to return to: %s\n", az_span_ptr(command_data->response_topic));

  return AZ_MQTT5_RPC_STATUS_OK;
}

az_result iot_callback(az_mqtt5_connection* client, az_event event)
{
  printf(LOG_APP "[APP/callback] %d\n", event.type);
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

    case AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND:
    {
      az_mqtt5_rpc_server_command_data* command_data = (az_mqtt5_rpc_server_command_data*)event.data;
      // function to actually handle command execution
      az_mqtt5_rpc_status rc = execute_command(command_data);
      az_mqtt5_rpc_server_execution_data return_data = {
        .correlation_id = command_data->correlation_id,
        .response = AZ_SPAN_FROM_STR("{\"Succeed\":true,\"ReceivedFrom\":\"mobile-app\",\"processedMs\":5}"),
        .response_topic = command_data->response_topic,
        .status = rc
      };
      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_execution_finish(&rpc_server, &return_data));
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
  // az_mqtt5_options mqtt5_options = az_mqtt5_options_default();
  // mqtt5_options.certificate_authority_trusted_roots = NULL;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_init(&mqtt5, NULL));

  az_mqtt5_x509_client_certificate primary_credential = (az_mqtt5_x509_client_certificate){
    .cert = cert_path1,
    .key = key_path1,
    // .key_type = AZ_CREDENTIALS_X509_KEY_MEMORY,
  };

  az_mqtt5_connection_options connection_options = az_mqtt5_connection_options_default();
  // connection_options.disable_sdk_connection_management = false;
  connection_options.client_id_buffer = client_id;
  connection_options.username_buffer = username;
  connection_options.password_buffer = password;
  connection_options.hostname = hostname;
  connection_options.client_certificates[0] = primary_credential;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_init(
      &iot_connection, &connection_context, &mqtt5, iot_callback, &connection_options));

  az_mqtt5_property_bag property_bag;
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_property_bag_init(&property_bag, &mqtt5, NULL));

  rpc_server_options = (az_mqtt5_rpc_server_options){
    .sub_topic = AZ_SPAN_FROM_BUFFER(sub_topic_buffer),
    .command_name = command_name,
    .model_id = model_id
  };

  az_mqtt5_rpc_server_data rpc_server_data = (az_mqtt5_rpc_server_data){
    .property_bag = &property_bag
  };

  LOG_AND_EXIT_IF_FAILED(az_rpc_server_init(&rpc_server, &iot_connection, &rpc_server_options, &rpc_server_data));
  

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_open(&iot_connection));

  for (int i = 45; i > 0; i--)
  {
    LOG_AND_EXIT_IF_FAILED(az_platform_sleep_msec(1000));
    printf(LOG_APP "Waiting %ds        \r", i);
    fflush(stdout);
  }

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_close(&iot_connection));

  for (int i = 15; connected && i > 0; i--)
  {
    LOG_AND_EXIT_IF_FAILED(az_platform_sleep_msec(1000));
    printf(LOG_APP "Waiting for disconnect %ds        \r", i);
    fflush(stdout);
  }

  mosquitto_property_free_all(&property_bag._internal.options.properties);

  if (mosquitto_lib_cleanup() != MOSQ_ERR_SUCCESS)
  {
    printf(LOG_APP "Failed to cleanup MosquittoLib\n");
    return -1;
  }

  printf(LOG_APP "Done.                                \n");
  return 0;
}

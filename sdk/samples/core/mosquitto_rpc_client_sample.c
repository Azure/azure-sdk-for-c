/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 * @brief Mosquitto RPC client sample
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
static const az_span server_client_id = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span content_type = AZ_SPAN_LITERAL_FROM_STR("application/json");

static char response_topic_buffer[256];
static char request_topic_buffer[256];
static char subscription_topic_buffer[256];

static uint8_t correlation_id_buffers[RPC_CLIENT_MAX_PENDING_COMMANDS][AZ_MQTT5_RPC_CORRELATION_ID_LENGTH];
static pending_commands_array pending_commands;

static az_mqtt5_connection mqtt_connection;
static az_context connection_context;

static az_mqtt5_rpc_client_policy rpc_client_policy;
static az_mqtt5_rpc_client rpc_client;

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

      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_client_subscribe_begin(&rpc_client_policy));

      break;
    }

    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
    {
      printf(LOG_APP "DISCONNECTED\n");
      sample_finished = true;
      break;
    }

    case AZ_EVENT_RPC_CLIENT_READY_IND:
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
      // printf(LOG_APP "RPC Client Ready for invoke requests\n");
      break;
    }

    case AZ_MQTT5_EVENT_PUBACK_RSP:
    {
      az_mqtt5_puback_data* puback_data = (az_mqtt5_puback_data*)event.data;
      pending_command* puback_cmd = get_command_with_mid(&pending_commands, puback_data->id);
      if (puback_cmd != NULL)
      {
        printf("Pub with mid %d acknowledged\n", puback_cmd->mid);
        // TODO: handle no subscribers on pub topic scenario or other bad RCs
      }
      else
      {
        printf("Puback for unknown mid %d\n", puback_data->id);
      }
      break;
    }

    case AZ_EVENT_RPC_CLIENT_RSP:
    {
      az_mqtt5_rpc_client_rsp_event_data* recv_data
          = (az_mqtt5_rpc_client_rsp_event_data*)event.data;
      if (is_pending_command(pending_commands, recv_data->correlation_id))
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
        remove_command(&pending_commands, recv_data->correlation_id);
      }
      else
      {
        printf(LOG_APP_ERROR "Request with ");
        print_correlation_id(recv_data->correlation_id);
        printf("not found\n");
      }
      break;
    }

    case AZ_EVENT_RPC_CLIENT_PARSE_ERROR_RSP:
    {
      az_mqtt5_rpc_client_rsp_event_data* recv_data
          = (az_mqtt5_rpc_client_rsp_event_data*)event.data;
      printf(
          LOG_APP_ERROR "Parsing failure for command %s: %s\n",
          az_span_ptr(recv_data->correlation_id),
          az_span_ptr(recv_data->error_message));
      remove_command(&pending_commands, recv_data->correlation_id);
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

  LOG_AND_EXIT_IF_FAILED(az_rpc_client_policy_init(
      &rpc_client_policy,
      &rpc_client,
      &mqtt_connection,
      property_bag,
      client_id,
      model_id,
      command_name,
      AZ_SPAN_FROM_BUFFER(response_topic_buffer),
      AZ_SPAN_FROM_BUFFER(request_topic_buffer),
      AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
      NULL));

  LOG_AND_EXIT_IF_FAILED(pending_commands_array_init(&pending_commands, correlation_id_buffers));
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_open(&mqtt_connection));

  az_result rc;

  // infinite execution loop
  for (int i = 45; !sample_finished && i > 0; i++)
  {
    pending_command* expired_command = get_first_expired_command(pending_commands);
    while (expired_command != NULL)
    {
      printf(LOG_APP_ERROR "command %d expired\n", expired_command->mid);
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
      uuid_t new_uuid;
      uuid_generate(new_uuid);
      az_mqtt5_rpc_client_invoke_req_event_data command_data
          = { .correlation_id = az_span_create((uint8_t*)new_uuid, AZ_MQTT5_RPC_CORRELATION_ID_LENGTH),
              .content_type = content_type,
              .rpc_server_client_id = server_client_id,
              .request_payload = AZ_SPAN_FROM_STR(
                  "{\"RequestTimestamp\":1691530585198,\"RequestedFrom\":\"mobile-app\"}") };
      LOG_AND_EXIT_IF_FAILED(add_command(&pending_commands, command_data.correlation_id, command_name, 10000));
      rc = az_mqtt5_rpc_client_invoke_begin(&rpc_client_policy, &command_data);
      LOG_AND_EXIT_IF_FAILED(add_mid_to_command(&pending_commands, command_data.correlation_id, command_data.mid));

      if (az_result_failed(rc))
      {
        printf(LOG_APP_ERROR "Failed to invoke command with rc: %s\n", az_result_to_string(rc));
        remove_command(&pending_commands, command_data.correlation_id);
      }
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

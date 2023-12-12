/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 * @brief Telemetry Consumer sample for Mosquitto MQTT.
 *
 */

#include <az_log_listener.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/az_core.h>
#include <azure/core/az_log.h>
#include <azure/core/az_mqtt5_telemetry.h>
#include <azure/core/az_mqtt5_telemetry_consumer.h>

// User-defined parameters
static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to cert pem file>");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to cert key file>");
static const az_span client_id = AZ_SPAN_LITERAL_FROM_STR("mobile-app");
static const az_span username = AZ_SPAN_LITERAL_FROM_STR("mobile-app");
static const az_span hostname = AZ_SPAN_LITERAL_FROM_STR("<hostname>");
static const az_span sender_id = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:telemetry:samples:vehicle;1");
static const az_span content_type = AZ_SPAN_LITERAL_FROM_STR("application/json");

// Static memory allocation.
static char subscription_topic_buffer[256];

// State variables
static az_mqtt5_connection mqtt_connection;
static az_context connection_context;

static az_mqtt5_telemetry_consumer_codec telemetry_consumer_codec;
static az_mqtt5_telemetry_consumer telemetry_consumer;

volatile bool sample_finished = false;

az_result handle_telemetry(az_span payload);
az_result mqtt_callback(az_mqtt5_connection* client, az_event event, void* callback_context);

void az_platform_critical_error()
{
  printf(LOG_APP_ERROR "\x1B[31mPANIC!\x1B[0m\n");

  while (1)
    ;
}

/**
 * @brief Function that handles the telemetry data
 * @note Needs to be modified for your solution
 */
az_result handle_telemetry(az_span payload)
{
  // for now, just print details from the telemetry
  printf(LOG_APP "Received telemetry: %s\n", az_span_ptr(payload));
  return AZ_OK;
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
        LOG_AND_EXIT_IF_FAILED(az_mqtt5_telemetry_consumer_subscribe_begin(&telemetry_consumer));
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

    case AZ_MQTT5_EVENT_TELEMETRY_CONSUMER_IND:
    {
      az_mqtt5_telemetry_consumer_ind_event_data data
          = *(az_mqtt5_telemetry_consumer_ind_event_data*)event.data;
      // can check here for the expected telemetry topic to determine which type of telemetry this
      // is
      if (!az_span_is_content_equal(content_type, data.content_type))
      {
        printf(
            LOG_APP_ERROR "Invalid content type. Expected: {%s} Actual: {%s}\n",
            az_span_ptr(content_type),
            az_span_ptr(data.content_type));
        return AZ_ERROR_NOT_SUPPORTED; // TODO: should this return like this here?
      }

      // Deserialize payload here
      handle_telemetry(data.telemetry_payload);

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
  connection_options.client_certificate_count = 1;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_init(
      &mqtt_connection, &connection_context, &mqtt5, mqtt_callback, &connection_options, NULL));

  az_mqtt5_telemetry_consumer_codec_options telemetry_consumer_codec_options
      = az_mqtt5_telemetry_consumer_codec_options_default();

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_telemetry_consumer_init(
      &telemetry_consumer,
      &telemetry_consumer_codec,
      &mqtt_connection,
      AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
      model_id,
      sender_id,
      AZ_MQTT5_TELEMETRY_DEFAULT_TIMEOUT_SECONDS,
      &telemetry_consumer_codec_options));

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_open(&mqtt_connection));

  // infinite execution loop
  while (!sample_finished)
  {
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

  if (mosquitto_lib_cleanup() != MOSQ_ERR_SUCCESS)
  {
    printf(LOG_APP "Failed to cleanup MosquittoLib\n");
    return -1;
  }

  printf(LOG_APP "Done.                                \n");
  return 0;
}

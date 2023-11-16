/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 * @brief Telemetry Producer sample for Mosquitto MQTT
 *
 */

#include <az_log_listener.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/az_core.h>
#include <azure/core/az_log.h>
#include <azure/core/az_mqtt5_telemetry_producer.h>
#include <azure/core/az_mqtt5_telemetry.h>

// User-defined parameters
static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to cert pem file>");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to cert key file>");
static const az_span client_id = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span username = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span hostname = AZ_SPAN_LITERAL_FROM_STR("<hostname>");
static const az_span telemetry_name = AZ_SPAN_LITERAL_FROM_STR("fuelLevel");
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:telemetry:samples:vehicle;1");
static const az_span content_type = AZ_SPAN_LITERAL_FROM_STR("application/json");

// Static memory allocation.
static char telemetry_topic_buffer[256];

// State variables
static az_mqtt5_connection mqtt_connection;
static az_context connection_context;

static az_mqtt5_telemetry_producer telemetry_producer;
static az_mqtt5_telemetry_producer_codec telemetry_producer_codec;

volatile bool sample_finished = false;

az_result mqtt_callback(az_mqtt5_connection* client, az_event event, void* callback_context);
az_result telemetry_send_begin(az_span telemetry_name, az_span payload, int8_t qos);

void az_platform_critical_error()
{
  printf(LOG_APP_ERROR "\x1B[31mPANIC!\x1B[0m\n");

  while (1)
    ;
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
      break;
    }

    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
    {
      printf(LOG_APP "DISCONNECTED\n");
      sample_finished = true;
      break;
    }

    case AZ_MQTT5_EVENT_TELEMETRY_PRODUCER_ERROR_RSP:
    {
      az_mqtt5_telemetry_producer_error_rsp_event_data* recv_data
          = (az_mqtt5_telemetry_producer_error_rsp_event_data*)event.data;
      printf(LOG_APP_ERROR "Failure sending telemetry : %s Reason Code: %d\n", az_span_ptr(recv_data->error_message), recv_data->reason_code);
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

az_result telemetry_send_begin(az_span send_telemetry_name, az_span payload, int8_t qos)
{
  az_mqtt5_telemetry_producer_send_req_event_data telemetry_data
      = { .content_type = content_type,
          .telemetry_name = send_telemetry_name,
          .telemetry_payload = payload,
          .qos = qos };
  az_result rc = az_mqtt5_telemetry_producer_send_begin(&telemetry_producer, &telemetry_data);
  if (az_result_failed(rc))
  {
    printf(
        LOG_APP_ERROR "Failed to send telemetry '%s' with rc: %s\n",
        az_span_ptr(send_telemetry_name),
        az_result_to_string(rc));
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

  az_mqtt5_property_bag property_bag;
  mosquitto_property* mosq_prop = NULL;
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_property_bag_init(&property_bag, &mqtt5, &mosq_prop));

  az_mqtt5_telemetry_producer_codec_options telemetry_producer_codec_options
      = az_mqtt5_telemetry_producer_codec_options_default();

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_telemetry_producer_init(
      &telemetry_producer,
      &telemetry_producer_codec,
      &mqtt_connection,
      property_bag,
      client_id,
      model_id,
      AZ_SPAN_FROM_BUFFER(telemetry_topic_buffer),
      AZ_MQTT5_TELEMETRY_DEFAULT_TIMEOUT_SECONDS,
      &telemetry_producer_codec_options));

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_open(&mqtt_connection));

  // infinite execution loop
  for (int i = 0; !sample_finished; i++)
  {
    printf(LOG_APP "Waiting...\r");
    fflush(stdout);

    // sends a telemetry message every 15 seconds. This cadence/how it is triggered should be customized for
    // your solution.
    if (i % 15 == 0)
    {
      // TODO: Payload should be generated and serialized
      LOG_AND_EXIT_IF_FAILED(telemetry_send_begin(
          telemetry_name,
          AZ_SPAN_FROM_STR(
              "{\"TelemetryTimestamp\":1691530585198,\"FuelLevel\":\"50\"}"),
          AZ_MQTT5_QOS_AT_LEAST_ONCE));
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
  mosquitto_property_free_all(&mosq_prop);

  if (mosquitto_lib_cleanup() != MOSQ_ERR_SUCCESS)
  {
    printf(LOG_APP "Failed to cleanup MosquittoLib\n");
    return -1;
  }

  printf(LOG_APP "Done.                                \n");
  return 0;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
#pragma warning(push)
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
#endif
#include <paho-mqtt/MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "iot_samples_common.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#define SAMPLE_TYPE PAHO_IOT_HUB
#define SAMPLE_NAME PAHO_IOT_HUB_SAS_TELEMETRY_SAMPLE

#define TELEMETRY_SEND_INTERVAL_SEC 1
#define TELEMETRY_NUMBER_OF_MESSAGES 5
#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)

static iot_sample_environment_variables env_vars;
static az_iot_hub_client hub_client;
static MQTTClient mqtt_client;
static char mqtt_client_username_buffer[128];

// Generate SAS key variables
static char sas_signature_buffer[128];
static char sas_encoded_signed_signature_buffer[128];
static char mqtt_password_buffer[256];

// Functions
static void create_and_configure_mqtt_client(void);
static void connect_mqtt_client_to_iot_hub(void);
static void send_telemetry_messages_to_iot_hub(void);
static void disconnect_mqtt_client_from_iot_hub(void);

static void generate_sas_key(void);

/*
 * This sample sends five telemetry messages to the Azure IoT Hub.
 * SAS certification is used.
 */
int main(void)
{
  create_and_configure_mqtt_client();
  LOG_SUCCESS("Client created and configured.");

  connect_mqtt_client_to_iot_hub();
  LOG_SUCCESS("Client connected to IoT Hub.");

  send_telemetry_messages_to_iot_hub();
  LOG_SUCCESS("Client sent telemetry messages to IoT Hub.");

  disconnect_mqtt_client_from_iot_hub();
  LOG_SUCCESS("Client disconnected from IoT Hub.");

  return 0;
}

static void create_and_configure_mqtt_client(void)
{
  int rc;

  // Reads in environment variables set by user for purposes of running sample.
  if (az_failed(rc = read_environment_variables(SAMPLE_TYPE, SAMPLE_NAME, &env_vars)))
  {
    LOG_ERROR(
        "Failed to read configuration from environment variables: az_result return code 0x%08x.",
        rc);
    exit(rc);
  }

  // Build an MQTT endpoint c-string.
  char mqtt_endpoint_buffer[128];
  if (az_failed(
          rc = create_mqtt_endpoint(
              SAMPLE_TYPE, &env_vars, mqtt_endpoint_buffer, sizeof(mqtt_endpoint_buffer))))
  {
    LOG_ERROR("Failed to create MQTT endpoint: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Initialize the hub client with the default connection options.
  if (az_failed(
          rc = az_iot_hub_client_init(
              &hub_client, env_vars.hub_hostname, env_vars.hub_device_id, NULL)))
  {
    LOG_ERROR("Failed to initialize hub client: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Get the MQTT client id used for the MQTT connection.
  char mqtt_client_id_buffer[128];
  if (az_failed(
          rc = az_iot_hub_client_get_client_id(
              &hub_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT client id: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Create the Paho MQTT client.
  if ((rc = MQTTClient_create(
           &mqtt_client,
           mqtt_endpoint_buffer,
           mqtt_client_id_buffer,
           MQTTCLIENT_PERSISTENCE_NONE,
           NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to create MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }

  generate_sas_key();
  LOG_SUCCESS("Client generated SAS Key.");
}

static void connect_mqtt_client_to_iot_hub(void)
{
  int rc;

  // Get the MQTT client username.
  if (az_failed(
          rc = az_iot_hub_client_get_user_name(
              &hub_client, mqtt_client_username_buffer, sizeof(mqtt_client_username_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT client username: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Set MQTT connection options.
  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;
  mqtt_connect_options.username = mqtt_client_username_buffer;
  mqtt_connect_options.password = mqtt_password_buffer;
  mqtt_connect_options.cleansession = false; // Set to false so can receive any pending messages.
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  if (az_span_size(env_vars.x509_trust_pem_file_path) != 0) // Is only set if required by OS.
  {
    mqtt_ssl_options.trustStore = (char*)az_span_ptr(env_vars.x509_trust_pem_file_path);
  }
  mqtt_connect_options.ssl = &mqtt_ssl_options;

  // Connect MQTT client to the Azure IoT Hub.
  if ((rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR(
        "Failed to connect: MQTTClient return code %d.\n"
        "If on Windows, confirm the AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH environment variable is "
        "set correctly.",
        rc);
    exit(rc);
  }
}

static void send_telemetry_messages_to_iot_hub(void)
{
  int rc;

  // Get the Telemetry topic to publish the telemetry messages.
  char telemetry_topic_buffer[128];
  if (az_failed(
          rc = az_iot_hub_client_telemetry_get_publish_topic(
              &hub_client, NULL, telemetry_topic_buffer, sizeof(telemetry_topic_buffer), NULL)))
  {
    LOG_ERROR("Failed to get Telemetry publish topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  const char* telemetry_message_payloads[TELEMETRY_NUMBER_OF_MESSAGES] = {
    "Message One", "Message Two", "Message Three", "Message Four", "Message Five",
  };

  // Publish # of telemetry messages.
  for (uint8_t i = 0; i < TELEMETRY_NUMBER_OF_MESSAGES; ++i)
  {
    LOG("Sending message %d.", i + 1);
    if ((rc = MQTTClient_publish(
             mqtt_client,
             telemetry_topic_buffer,
             (int)strlen(telemetry_message_payloads[i]),
             telemetry_message_payloads[i],
             0,
             0,
             NULL))
        != MQTTCLIENT_SUCCESS)
    {
      LOG_ERROR("Failed to publish telemetry message %d, MQTTClient return code %d.", i + 1, rc);
      exit(rc);
    }
    sleep_for_seconds(TELEMETRY_SEND_INTERVAL_SEC);
  }
}

static void disconnect_mqtt_client_from_iot_hub(void)
{
  int rc;

  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to disconnect MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }

  MQTTClient_destroy(&mqtt_client);
}

static void generate_sas_key(void)
{
  az_result rc;

  // Create the POSIX expiration time from input minutes.
  uint64_t sas_duration = get_epoch_expiration_time_from_minutes(env_vars.sas_key_duration_minutes);

  // Get the signature that will later be signed.
  az_span sas_signature = AZ_SPAN_FROM_BUFFER(sas_signature_buffer);
  if (az_failed(
          rc = az_iot_hub_client_sas_get_signature(
              &hub_client, sas_duration, sas_signature, &sas_signature)))
  {
    LOG_ERROR("Could not get the signature for SAS key: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Generate the encoded, signed signature (b64 encoded, HMAC-SHA256 signing)
  az_span sas_encoded_signed_signature = AZ_SPAN_FROM_BUFFER(sas_encoded_signed_signature_buffer);
  sas_generate_encoded_signed_signature(
      env_vars.hub_sas_key,
      sas_signature,
      sas_encoded_signed_signature,
      &sas_encoded_signed_signature);

  // Get the resulting MQTT password, passing the base64 encoded, HMAC signed bytes
  size_t mqtt_password_length;
  if (az_failed(
          rc = az_iot_hub_client_sas_get_password(
              &hub_client,
              sas_encoded_signed_signature,
              sas_duration,
              AZ_SPAN_NULL,
              mqtt_password_buffer,
              sizeof(mqtt_password_buffer),
              &mqtt_password_length)))
  {
    LOG_ERROR("Could not get the password: az_result return code 0x%08x.", rc);
    exit(rc);
  }
}

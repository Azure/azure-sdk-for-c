// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
#endif
#include <paho-mqtt/MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(default : 4201)
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
#define SAMPLE_NAME PAHO_IOT_HUB_C2D_SAMPLE

#define MAX_C2D_MESSAGE_COUNT 5
#define TIMEOUT_MQTT_RECEIVE_MS (60 * 1000)
#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)

static sample_environment_variables env_vars;
static az_iot_hub_client hub_client;
static MQTTClient mqtt_client;
static char mqtt_client_username_buffer[128];

// Functions
void create_and_configure_mqtt_client();
void connect_mqtt_client_to_iot_hub();
void subscribe_mqtt_client_to_iot_hub_topics();
void receive_c2d_messages();
void disconnect_mqtt_client_from_iot_hub();

void parse_c2d_message(
    char* topic,
    int topic_len,
    const MQTTClient_message* message,
    az_iot_hub_client_c2d_request* c2d_request);

/*
 * This sample receives incoming cloud-to-device (C2D) messages sent from the Azure IoT Hub to
 * the device. It will successfully receive up to MAX_C2D_MESSAGE_COUNT messages sent from the
 * service. If a timeout occurs of TIMEOUT_MQTT_RECEIVE_MS while waiting for a message, the sample
 * will exit. X509 self-certification is used.
 *
 * To send a C2D message, select your device's Message to Device tab in the Azure Portal for your
 * IoT Hub. Enter a message in the Message Body and select Send Message.
 */
int main()
{
  create_and_configure_mqtt_client();
  LOG_SUCCESS("Client created and configured.");

  connect_mqtt_client_to_iot_hub();
  LOG_SUCCESS("Client connected to IoT Hub.");

  subscribe_mqtt_client_to_iot_hub_topics();
  LOG_SUCCESS("Client subscribed to IoT Hub topics and is ready to receive C2D messages.");

  receive_c2d_messages();
  LOG_SUCCESS("Client received messages.")

  disconnect_mqtt_client_from_iot_hub();
  LOG_SUCCESS("Client disconnected from IoT Hub.");

  return 0;
}

void create_and_configure_mqtt_client()
{
  int rc;

  // Reads in environment variables set by user for purposes of running sample.
  if (az_failed(rc = read_environment_variables(SAMPLE_TYPE, SAMPLE_NAME, &env_vars)))
  {
    LOG_ERROR(
        "Failed to read configuration from environment variables: az_result return code 0x%04x.",
        rc);
    exit(rc);
  }

  // Build an MQTT endpoint c-string.
  char mqtt_endpoint_buffer[128];
  if (az_failed(
          rc = create_mqtt_endpoint(
              SAMPLE_TYPE, &env_vars, mqtt_endpoint_buffer, sizeof(mqtt_endpoint_buffer))))
  {
    LOG_ERROR("Failed to create MQTT endpoint: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Initialize the hub client with the default connection options.
  if (az_failed(
          rc = az_iot_hub_client_init(
              &hub_client, env_vars.hub_hostname, env_vars.hub_device_id, NULL)))
  {
    LOG_ERROR("Failed to initialize hub client: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Get the MQTT client id used for the MQTT connection.
  char mqtt_client_id_buffer[128];
  if (az_failed(
          rc = az_iot_hub_client_get_client_id(
              &hub_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT client id: az_result return code 0x%04x.", rc);
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
}

void connect_mqtt_client_to_iot_hub()
{
  int rc;

  // Get the MQTT client username.
  if (az_failed(
          rc = az_iot_hub_client_get_user_name(
              &hub_client, mqtt_client_username_buffer, sizeof(mqtt_client_username_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT client username: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Set MQTT connection options.
  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;
  mqtt_connect_options.username = mqtt_client_username_buffer;
  mqtt_connect_options.password = NULL; // This sample uses x509 authentication.
  mqtt_connect_options.cleansession = false; // Set to false so can receive any pending messages.
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  mqtt_ssl_options.keyStore = (char*)x509_cert_pem_file_path_buffer;
  if (*az_span_ptr(env_vars.x509_trust_pem_file_path) != '\0') // Is only set if required by OS.
  {
    mqtt_ssl_options.trustStore = (char*)x509_trust_pem_file_path_buffer;
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

void subscribe_mqtt_client_to_iot_hub_topics()
{
  int rc;

  // Messages received on the C2D topic will be cloud-to-device messages.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to subscribe to the C2D topic: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

void receive_c2d_messages()
{
  int rc;
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;

  // Continue until max # messages received or timeout expires.
  for (uint8_t message_count = 0; message_count < MAX_C2D_MESSAGE_COUNT; message_count++)
  {
    LOG("Waiting for C2D message.");

    if (((rc
          = MQTTClient_receive(mqtt_client, &topic, &topic_len, &message, TIMEOUT_MQTT_RECEIVE_MS))
         != MQTTCLIENT_SUCCESS)
        && (rc != MQTTCLIENT_TOPICNAME_TRUNCATED))
    {
      LOG_ERROR("Failed to receive message: MQTTClient return code %d.", rc);
      exit(rc);
    }
    else if (message == NULL)
    {
      LOG_ERROR("Timeout expired: MQTTClient return code %d.", rc);
      exit(rc);
    }
    else if (rc == MQTTCLIENT_TOPICNAME_TRUNCATED)
    {
      topic_len = (int)strlen(topic);
    }
    LOG_SUCCESS("Message #%d: Client received message from the service.", message_count + 1);

    // Parse c2d message.
    az_iot_hub_client_c2d_request c2d_request;
    parse_c2d_message(topic, topic_len, message, &c2d_request);
    LOG_SUCCESS("Client parsed message.");

    LOG(" "); // formatting

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topic);
  }
}

void disconnect_mqtt_client_from_iot_hub()
{
  int rc;

  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to disconnect MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }

  MQTTClient_destroy(&mqtt_client);
}

void parse_c2d_message(
    char* topic,
    int topic_len,
    const MQTTClient_message* message,
    az_iot_hub_client_c2d_request* c2d_request)
{
  PRECONDITION_NOT_NULL(topic);
  PRECONDITION_NOT_NULL(message);
  PRECONDITION_NOT_NULL(c2d_request);

  int rc;
  az_span topic_span = az_span_create((uint8_t*)topic, topic_len);
  az_span message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  // Parse message and retrieve c2d_request info.
  if (az_failed(
          rc = az_iot_hub_client_c2d_parse_received_topic(&hub_client, topic_span, c2d_request)))
  {
    LOG_ERROR("Message from unknown topic: az_result return code 0x%04x.", rc);
    LOG_AZ_SPAN("Topic:", topic_span);
    exit(rc);
  }
  LOG_SUCCESS("Client received a valid topic response:");
  LOG_AZ_SPAN("Topic:", topic_span);
  LOG_AZ_SPAN("Payload:", message_span);
}

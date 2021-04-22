// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning(push)
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
#endif
#include <paho-mqtt/MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <azure/az_core.h>
#include <azure/az_iot.h>

#include "iot_sample_common.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
// warning within intel/tinycbor: conversion from 'int' to uint8_t'
#pragma GCC diagnostic ignored "-Wconversion"
#endif
#include "cbor.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#define SAMPLE_TYPE PAHO_IOT_HUB
#define SAMPLE_NAME PAHO_IOT_HUB_TWIN_SAMPLE

#define MAX_TWIN_MESSAGE_COUNT 5
#define MQTT_TIMEOUT_RECEIVE_MS (60 * 1000)
#define MQTT_TIMEOUT_DISCONNECT_MS (10 * 1000)

static az_span const twin_document_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("get_twin");
static az_span const twin_patch_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("reported_prop");

static char* const desired_device_count_property_name = "device_count";
static int64_t device_count_value = 0;

static iot_sample_environment_variables env_vars;
static az_iot_hub_client hub_client;
static MQTTClient mqtt_client;
static char mqtt_client_username_buffer[256];

// Functions
static void create_and_configure_mqtt_client(void);
static void connect_mqtt_client_to_iot_hub(void);
static void subscribe_mqtt_client_to_iot_hub_topics(void);
static void send_and_receive_device_twin_messages(void);
static void disconnect_mqtt_client_from_iot_hub(void);

static void get_device_twin_document(void);
static void send_reported_property(void);
static bool receive_device_twin_message(void);
static void parse_device_twin_message(
    char* topic,
    int topic_len,
    MQTTClient_message const* message,
    az_iot_hub_client_twin_response* out_twin_response);
static void handle_device_twin_message(
    MQTTClient_message const* message,
    az_iot_hub_client_twin_response const* twin_response);
static bool parse_desired_device_count_property(
    MQTTClient_message const* message,
    int64_t* out_parsed_device_count);
static void update_device_count_property(int64_t device_count);
static void build_reported_property(
    uint8_t* reported_property_payload,
    size_t reported_property_payload_size,
    size_t* out_reported_property_length);

/*
 * This sample utilizes the Azure IoT Hub to get the device twin document in CBOR, send a reported
 * property message in CBOR, and receive up to 5 desired property messages in CBOR. If a timeout
 * occurs while waiting for a message from the Azure IoT Hub, the sample will exit. Upon receiving a
 * desired property message, the sample will update the twin property locally and send a reported
 * property message back to the service. X509 self-certification is used.
 *
 * A property named `device_count` is supported for this sample. To send a device twin desired
 * property message, select your device's Device Twin tab in the Azure Portal of your IoT Hub. Add
 * the property `device_count` along with a corresponding value to the `desired` section of the
 * JSON. Select Save to update the twin document and send the twin message to the device.
 *
 * {
 *   "properties": {
 *     "desired": {
 *       "device_count": 42,
 *     }
 *   }
 * }
 *
 * No other property names sent in a desired property message are supported. If any are sent, the
 * IOT_SAMPLE_LOG will report there is nothing to update.
 */
int main(void)
{
  create_and_configure_mqtt_client();
  IOT_SAMPLE_LOG_SUCCESS("Client created and configured.");

  connect_mqtt_client_to_iot_hub();
  IOT_SAMPLE_LOG_SUCCESS("Client connected to IoT Hub.");

  subscribe_mqtt_client_to_iot_hub_topics();
  IOT_SAMPLE_LOG_SUCCESS("Client subscribed to IoT Hub topics.");

  send_and_receive_device_twin_messages();

  disconnect_mqtt_client_from_iot_hub();
  IOT_SAMPLE_LOG_SUCCESS("Client disconnected from IoT Hub.");

  return 0;
}

static void create_and_configure_mqtt_client(void)
{
  int rc;

  // Reads in environment variables set by user for purposes of running sample.
  iot_sample_read_environment_variables(SAMPLE_TYPE, SAMPLE_NAME, &env_vars);

  // Build an MQTT endpoint c-string.
  char mqtt_endpoint_buffer[128];
  iot_sample_create_mqtt_endpoint(
      SAMPLE_TYPE, &env_vars, mqtt_endpoint_buffer, sizeof(mqtt_endpoint_buffer));

  // Initialize the hub client with the default connection options.
  az_iot_hub_client_options options = az_iot_hub_client_options_default();

  // Set the content type to CBOR for Direct Method payloads and Twin Document.
  options.method_twin_content_type
      = AZ_SPAN_FROM_STR(AZ_IOT_HUB_CLIENT_OPTION_METHOD_TWIN_CONTENT_TYPE_CBOR);

  rc = az_iot_hub_client_init(&hub_client, env_vars.hub_hostname, env_vars.hub_device_id, &options);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to initialize hub client: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Get the MQTT client id used for the MQTT connection.
  char mqtt_client_id_buffer[128];
  rc = az_iot_hub_client_get_client_id(
      &hub_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), NULL);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get MQTT client id: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Create the Paho MQTT client.
  rc = MQTTClient_create(
      &mqtt_client, mqtt_endpoint_buffer, mqtt_client_id_buffer, MQTTCLIENT_PERSISTENCE_NONE, NULL);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to create MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

static void connect_mqtt_client_to_iot_hub(void)
{
  int rc;

  // Get the MQTT client username.
  rc = az_iot_hub_client_get_user_name(
      &hub_client, mqtt_client_username_buffer, sizeof(mqtt_client_username_buffer), NULL);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get MQTT client username: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  IOT_SAMPLE_LOG("MQTT client username: %s\n", mqtt_client_username_buffer);

  // Set MQTT connection options.
  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;
  mqtt_connect_options.username = mqtt_client_username_buffer;
  mqtt_connect_options.password = NULL; // This sample uses x509 authentication.
  mqtt_connect_options.cleansession = false; // Set to false so can receive any pending messages.
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  mqtt_ssl_options.verify = 1;
  mqtt_ssl_options.enableServerCertAuth = 1;
  mqtt_ssl_options.keyStore = (char*)az_span_ptr(env_vars.x509_cert_pem_file_path);
  if (az_span_size(env_vars.x509_trust_pem_file_path) != 0) // Is only set if required by OS.
  {
    mqtt_ssl_options.trustStore = (char*)az_span_ptr(env_vars.x509_trust_pem_file_path);
  }
  mqtt_connect_options.ssl = &mqtt_ssl_options;

  // Connect MQTT client to the Azure IoT Hub.
  rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to connect: MQTTClient return code %d.\n"
        "If on Windows, confirm the AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH environment variable is "
        "set correctly.",
        rc);
    exit(rc);
  }
}

static void subscribe_mqtt_client_to_iot_hub_topics(void)
{
  int rc;

  // Messages received on the Twin Patch topic will be updates to the desired properties.
  rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC, 1);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to subscribe to the Twin Patch topic: MQTTClient return code %d.", rc);
    exit(rc);
  }

  // Messages received on Twin Response topic will be response statuses from the server.
  rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to subscribe to the Twin Response topic: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

static void send_and_receive_device_twin_messages(void)
{
  get_device_twin_document();
  (void)receive_device_twin_message();
  IOT_SAMPLE_LOG(" "); // Formatting

  send_reported_property();
  (void)receive_device_twin_message();

  // Continue until max # messages received or timeout occurs.
  for (uint8_t message_count = 0; message_count < MAX_TWIN_MESSAGE_COUNT; message_count++)
  {
    if (!receive_device_twin_message())
    {
      return;
    }
  }

  IOT_SAMPLE_LOG_SUCCESS("Client received messages.");
}

static void disconnect_mqtt_client_from_iot_hub(void)
{
  int rc = MQTTClient_disconnect(mqtt_client, MQTT_TIMEOUT_DISCONNECT_MS);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to disconnect MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }

  MQTTClient_destroy(&mqtt_client);
}

static void get_device_twin_document(void)
{
  int rc;

  IOT_SAMPLE_LOG("Client requesting device twin document from service.");

  // Get the Twin Document topic to publish the twin document request.
  char twin_document_topic_buffer[128];
  rc = az_iot_hub_client_twin_document_get_publish_topic(
      &hub_client,
      twin_document_topic_request_id,
      twin_document_topic_buffer,
      sizeof(twin_document_topic_buffer),
      NULL);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to get the Twin Document topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Publish the twin document request.
  rc = MQTTClient_publish(
      mqtt_client, twin_document_topic_buffer, 0, NULL, IOT_SAMPLE_MQTT_PUBLISH_QOS, 0, NULL);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to publish the Twin Document request: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

static void send_reported_property(void)
{
  int rc;

  IOT_SAMPLE_LOG("Client sending reported property to service.");

  // Get the Twin Patch topic to publish a reported property update.
  char twin_patch_topic_buffer[128];
  rc = az_iot_hub_client_twin_patch_get_publish_topic(
      &hub_client,
      twin_patch_topic_request_id,
      twin_patch_topic_buffer,
      sizeof(twin_patch_topic_buffer),
      NULL);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get the Twin Patch topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Build the updated reported property message.
  uint8_t reported_property_payload_buffer[128];
  size_t reported_property_payload_length;
  build_reported_property(
      reported_property_payload_buffer,
      sizeof(reported_property_payload_buffer),
      &reported_property_payload_length);

  // Publish the reported property update.
  rc = MQTTClient_publish(
      mqtt_client,
      twin_patch_topic_buffer,
      (int)reported_property_payload_length,
      reported_property_payload_buffer,
      IOT_SAMPLE_MQTT_PUBLISH_QOS,
      0,
      NULL);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to publish the Twin Patch reported property update: MQTTClient return code %d.",
        rc);
    exit(rc);
  }
  IOT_SAMPLE_LOG_SUCCESS("Client published the Twin Patch reported property message.");
  IOT_SAMPLE_LOG("Payload as ASCII: %s", reported_property_payload_buffer);
  IOT_SAMPLE_LOG_HEX(
      "Payload as Hex Value:",
      reported_property_payload_buffer,
      (int)reported_property_payload_length);
}

static bool receive_device_twin_message(void)
{
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;
  bool message_received = true;

  IOT_SAMPLE_LOG(" "); // Formatting
  IOT_SAMPLE_LOG("Waiting for device twin message.\n");

  // MQTTCLIENT_SUCCESS or MQTTCLIENT_TOPICNAME_TRUNCATED if a message is received.
  // MQTTCLIENT_SUCCESS can also indicate that the timeout expired, in which case message is NULL.
  // MQTTCLIENT_TOPICNAME_TRUNCATED if the topic contains embedded NULL characters.
  // An error code is returned if there was a problem trying to receive a message.
  int rc = MQTTClient_receive(mqtt_client, &topic, &topic_len, &message, MQTT_TIMEOUT_RECEIVE_MS);
  if ((rc != MQTTCLIENT_SUCCESS) && (rc != MQTTCLIENT_TOPICNAME_TRUNCATED))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to receive message: MQTTClient return code %d.", rc);
    exit(rc);
  }
  else if (message == NULL)
  {
    IOT_SAMPLE_LOG("Receive message timeout expired.");
    return !message_received;
  }
  else if (rc == MQTTCLIENT_TOPICNAME_TRUNCATED)
  {
    topic_len = (int)strlen(topic);
  }
  IOT_SAMPLE_LOG_SUCCESS("Client received a device twin message from the service.");

  // Parse device twin message.
  az_iot_hub_client_twin_response twin_response;
  parse_device_twin_message(topic, topic_len, message, &twin_response);
  IOT_SAMPLE_LOG_SUCCESS("Client parsed device twin message.");

  handle_device_twin_message(message, &twin_response);

  MQTTClient_freeMessage(&message);
  MQTTClient_free(topic);

  return message_received;
}

static void parse_device_twin_message(
    char* topic,
    int topic_len,
    MQTTClient_message const* message,
    az_iot_hub_client_twin_response* out_twin_response)
{
  az_span const topic_span = az_span_create((uint8_t*)topic, topic_len);
  az_span const message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  // Parse message and retrieve twin_response info.
  az_result rc
      = az_iot_hub_client_twin_parse_received_topic(&hub_client, topic_span, out_twin_response);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR("Message from unknown topic: az_result return code 0x%08x.", rc);
    IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
    exit(rc);
  }
  IOT_SAMPLE_LOG_SUCCESS("Client received a valid topic response.");
  IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
  IOT_SAMPLE_LOG_AZ_SPAN("Payload as ASCII:", message_span);
  IOT_SAMPLE_LOG_HEX(
      "Payload as Hex Value:", az_span_ptr(message_span), az_span_size(message_span));
  IOT_SAMPLE_LOG("Status: %d", out_twin_response->status);
}

static void handle_device_twin_message(
    MQTTClient_message const* message,
    az_iot_hub_client_twin_response const* twin_response)
{
  // Invoke appropriate action per response type (3 types only).
  switch (twin_response->response_type)
  {
    case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_GET:
      IOT_SAMPLE_LOG("Message Type: GET");
      break;

    case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
      IOT_SAMPLE_LOG("Message Type: Reported Properties");
      break;

    case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
      IOT_SAMPLE_LOG("Message Type: Desired Properties");

      // Parse for the device count property.
      int64_t desired_device_count;
      if (parse_desired_device_count_property(message, &desired_device_count))
      {
        IOT_SAMPLE_LOG(" "); // Formatting

        // Update device count locally and report update to server.
        update_device_count_property(desired_device_count);
        send_reported_property();
        (void)receive_device_twin_message();
      }
      break;
  }
}

static bool parse_desired_device_count_property(
    MQTTClient_message const* message,
    int64_t* out_parsed_device_count)
{
  *out_parsed_device_count = 0;

  CborError rc; // CborNoError == 0
  bool result;

  // Parse message_span.
  CborParser parser;
  CborValue root;
  CborValue desired_device_count;

  rc = cbor_parser_init((uint8_t*)message->payload, (size_t)message->payloadlen, 0, &parser, &root);
  if (rc)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to initiate parser: CborError %d.", rc);
    exit(rc);
  }

  rc = cbor_value_map_find_value(&root, desired_device_count_property_name, &desired_device_count);
  if (rc)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Error when searching for %s: CborError %d.", desired_device_count_property_name, rc);
    exit(rc);
  }

  if (cbor_value_is_valid(&desired_device_count))
  {
    if (cbor_value_is_integer(&desired_device_count))
    {
      rc = cbor_value_get_int64(&desired_device_count, out_parsed_device_count);
      if (rc)
      {
        IOT_SAMPLE_LOG_ERROR(
            "Failed to get int64 value for %s: CborError %d.",
            desired_device_count_property_name,
            rc);
        exit(rc);
      }
      else
      {
        IOT_SAMPLE_LOG(
            "Parsed desired `%s`: %" PRIi64,
            desired_device_count_property_name,
            *out_parsed_device_count);
        result = true;
      }
    }
    else
    {
      IOT_SAMPLE_LOG("`%s` property was not an integer.", desired_device_count_property_name);
      result = false;
    }
  }
  else
  {
    IOT_SAMPLE_LOG(
        "`%s` property was not found in desired property message.",
        desired_device_count_property_name);
    result = false;
  }

  return result;
}

static void update_device_count_property(int64_t device_count)
{
  device_count_value = device_count;
  IOT_SAMPLE_LOG_SUCCESS(
      "Client updated `%s` locally to %." PRIi64,
      desired_device_count_property_name,
      device_count_value);
}

static void build_reported_property(
    uint8_t* reported_property_payload,
    size_t reported_property_payload_size,
    size_t* out_reported_property_length)
{
  CborError rc; // CborNoError == 0

  CborEncoder encoder;
  CborEncoder encoder_map;

  cbor_encoder_init(&encoder, reported_property_payload, reported_property_payload_size, 0);

  rc = cbor_encoder_create_map(&encoder, &encoder_map, 1);
  if (rc)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to create map: CborError %d.", rc);
    exit(rc);
  }

  rc = cbor_encode_text_string(
      &encoder_map, desired_device_count_property_name, strlen(desired_device_count_property_name));
  if (rc)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to encode text string: CborError %d.", rc);
    exit(rc);
  }

  rc = cbor_encode_int(&encoder_map, device_count_value);
  if (rc)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to encode int: CborError %d.", rc);
    exit(rc);
  }

  rc = cbor_encoder_close_container(&encoder, &encoder_map);
  if (rc)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to close container: CborError %d.", rc);
    exit(rc);
  }

  *out_reported_property_length = cbor_encoder_get_buffer_size(&encoder, reported_property_payload);
}

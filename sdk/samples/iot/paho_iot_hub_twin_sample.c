// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "sample.h"

#define SAMPLE_TYPE PAHO_IOT_HUB
#define SAMPLE_NAME PAHO_IOT_HUB_TWIN_SAMPLE

#define MAX_TWIN_MESSAGE_COUNT 5
#define TIMEOUT_MQTT_RECEIVE_MS (60 * 1000)

static const az_span twin_document_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("get_twin");
static const az_span twin_patch_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("reported_prop");
static const az_span version_name = AZ_SPAN_LITERAL_FROM_STR("$version");
static az_span reported_property_name = AZ_SPAN_LITERAL_FROM_STR("device_count");
static int32_t reported_property_value = 0;
static char reported_property_buffer[64];

static sample_environment_variables env_vars;
static az_iot_hub_client hub_client;
static MQTTClient mqtt_client;
static char mqtt_client_username_buffer[128];

// Functions
void create_and_configure_client();
void connect_client_to_iot_hub();
void subscribe_client_to_iot_hub_topics();
void get_twin_document();
void send_reported_property();
void receive_desired_property();
void disconnect_client_from_iot_hub();

az_result build_reported_property(az_span* reported_property_payload);
az_result update_property(const az_span* desired_payload);
void receive_message();
void parse_message(
    char* topic,
    int topic_len,
    const MQTTClient_message* message,
    az_iot_hub_client_twin_response* twin_response);

/*
 * This sample utilizes the Azure IoT Hub to get the twin document, send a reported property
 * message, and receive up to 5 desired property messages. If a timeout occurs while waiting for a
 * message from the Azure IoT Hub, the sample will exit. Upon receiving a desired property message,
 * the sample will update the property locally and send a reported property message back to the
 * service. X509 self-certification is used.
 *
 * A property named `device_count` is supported for this sample. To send a device twin desired
 * property message, select your device's Device Twin tab in your Azure IoT Hub. Add the property
 * `device_count` along with a corresponding value to the `desired` section of the JSON. Select Save
 * to send the message.
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
 * log will report there is nothing to update.
 */
int main()
{
  create_and_configure_client();
  LOG_SUCCESS("Client created and configured.");

  connect_client_to_iot_hub();
  LOG_SUCCESS("Client connected to IoT Hub.");

  subscribe_client_to_iot_hub_topics();
  LOG_SUCCESS("Client subscribed to IoT Hub topics.");

  get_twin_document();
  LOG_SUCCESS("Client got twin document.");

  send_reported_property();
  LOG_SUCCESS("Client sent reported property.");

  receive_desired_property();
  LOG_SUCCESS("Client received desired property.");

  disconnect_client_from_iot_hub();
  LOG_SUCCESS("Client disconnected from IoT Hub.");

  return 0;
}

void create_and_configure_client()
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
          rc
          = create_mqtt_endpoint(SAMPLE_TYPE, &env_vars, mqtt_endpoint_buffer, sizeof(mqtt_endpoint_buffer))))
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

  return;
}

void connect_client_to_iot_hub()
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
  if (*az_span_ptr(env_vars.x509_trust_pem_file_path)
      != '\0') // Should only be set if required by OS.
  {
    mqtt_ssl_options.trustStore = (char*)x509_trust_pem_file_path_buffer;
  }
  mqtt_connect_options.ssl = &mqtt_ssl_options;

  // Connect MQTT client to the Azure IoT Hub.
  if ((rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR(
        "Failed to connect: MQTTClient return code %d.\n"
        "If on Windows, confirm the AZ_IOT_DEVICE_X509_TRUST_PEM_FILE environment variable is set "
        "correctly.",
        rc);
    exit(rc);
  }

  return;
}

void subscribe_client_to_iot_hub_topics()
{
  int rc;

  // Messages received on the Twin Patch topic will be updates to the desired properties.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to subscribe to the Twin Patch topic: MQTTClient return code %d.", rc);
    exit(rc);
  }

  // Messages received on Response topic will be response statuses from the server.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to subscribe to the Twin Response topic: MQTTClient return code %d.", rc);
    exit(rc);
  }

  return;
}

void get_twin_document()
{
  int rc;

  LOG("Client requesting twin document from service.");

  // Get the Twin Document topic to publish the twin document request.
  char twin_document_topic[128];
  if (az_failed(
          rc = az_iot_hub_client_twin_document_get_publish_topic(
              &hub_client,
              twin_document_topic_request_id,
              twin_document_topic,
              sizeof(twin_document_topic),
              NULL)))
  {
    LOG_ERROR("Failed to get Twin Document publish topic: az_result return code %04x", rc);
    exit(rc);
  }

  // Publish the twin document request.
  if ((rc = MQTTClient_publish(mqtt_client, twin_document_topic, 0, NULL, 0, 0, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to publish twin document request: MQTTClient return code %d", rc);
    exit(rc);
  }

  // Receive the twin document message from the server.
  receive_message();

  return;
}

void send_reported_property()
{
  int rc;

  LOG("Client sending reported property to service.");

  // Get the Twin Patch topic to send a reported property update.
  char twin_patch_topic[128];
  if (az_failed(
          rc = az_iot_hub_client_twin_patch_get_publish_topic(
              &hub_client,
              twin_patch_topic_request_id,
              twin_patch_topic,
              sizeof(twin_patch_topic),
              NULL)))
  {
    LOG_ERROR("Failed to get Twin Patch publish topic: az_result return code %04x", rc);
    exit(rc);
  }

  // Build the updated reported property message.
  az_span reported_property_payload;
  if (az_failed(rc = build_reported_property(&reported_property_payload)))
  {
    LOG_ERROR("Failed to build reported property payload to send: az_result return code %04x", rc);
    exit(rc);
  }

  // Publish the reported property update.
  if ((rc = MQTTClient_publish(
           mqtt_client,
           twin_patch_topic,
           az_span_size(reported_property_payload),
           az_span_ptr(reported_property_payload),
           0,
           0,
           NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to publish reported property update: MQTTClient return code %d", rc);
    exit(rc);
  }
  LOG_SUCCESS("Client sent reported property message:");
  LOG_AZ_SPAN("Payload:", reported_property_payload);

  // Receive the server resonse.
  receive_message();

  return;
}

void receive_desired_property()
{
  // Continue until max # messages received
  for (uint8_t message_count = 0; message_count < MAX_TWIN_MESSAGE_COUNT; message_count++)
  {
    receive_message();
    send_reported_property();
  }

  return;
}

void disconnect_client_from_iot_hub()
{
  int rc;

  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to disconnect MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }

  MQTTClient_destroy(&mqtt_client);

  return;
}

void receive_message()
{
  int rc;
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;

  LOG("Waiting for message.");

  if (((rc = MQTTClient_receive(mqtt_client, &topic, &topic_len, &message, TIMEOUT_MQTT_RECEIVE_MS))
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
  LOG_SUCCESS("Client received message from the service.");

  // Parse message.
  az_iot_hub_client_twin_response twin_response;
  parse_message(topic, topic_len, message, &twin_response);
  LOG_SUCCESS("Client parsed message.");

  LOG(" "); // formatting

  MQTTClient_freeMessage(&message);
  MQTTClient_free(topic);
  return;
}

void parse_message(
    char* topic,
    int topic_len,
    const MQTTClient_message* message,
    az_iot_hub_client_twin_response* twin_response)
{
  _az_PRECONDITION_NOT_NULL(topic);
  _az_PRECONDITION_NOT_NULL(message);
  _az_PRECONDITION_NOT_NULL(twin_response);

  int rc;
  az_span topic_span = az_span_create((uint8_t*)topic, topic_len);
  az_span message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  // Parse message and retrieve twin_response info.
  if (az_failed(
          rc = az_iot_hub_client_twin_parse_received_topic(&hub_client, topic_span, twin_response)))
  {
    LOG_ERROR("Message from unknown topic: az_result return code 0x%04x.", rc);
    LOG_AZ_SPAN("Topic:", topic_span);
    exit(rc);
  }
  LOG_SUCCESS("Client received a valid topic response:");
  LOG_AZ_SPAN("Topic:", topic_span);
  LOG_AZ_SPAN("Payload:", message_span);
  LOG("Status: %d", twin_response->status);

  // Invoke appropriate action per response type (3 Types only).
  switch (twin_response->response_type)
  {
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET:
      LOG("Type: GET");
      break;

    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
      LOG("Type: Reported Properties");
      break;

    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
      LOG("Type: Desired Properties");

      if (az_failed(rc = update_property(&message_span)))
      {
        LOG_ERROR("Failed to update property locally: az_result return code 0x%04x.", rc);
        exit(rc);
      }
      break;
  }

  return;
}

az_result build_reported_property(az_span* reported_property_payload)
{
  _az_PRECONDITION_NOT_NULL(reported_property_payload);

  az_json_writer json_writer;

  az_span temp = AZ_SPAN_FROM_BUFFER(reported_property_buffer);
  printf("size of az_span: %d\n", az_span_size(temp));
  AZ_RETURN_IF_FAILED(az_json_writer_init(&json_writer, temp, NULL));
  printf("1\n");
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&json_writer));
  printf("2\n");
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&json_writer, reported_property_name));
  printf("3\n");
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(&json_writer, reported_property_value));
  printf("4\n");
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&json_writer));
  printf("5\n");
  *reported_property_payload = az_json_writer_get_bytes_used_in_destination(&json_writer);

  return AZ_OK;
}

az_result update_property(const az_span* desired_payload)
{
  _az_PRECONDITION_NOT_NULL(desired_payload);

  // Parse desired property payload.
  az_json_reader json_reader;
  AZ_RETURN_IF_FAILED(az_json_reader_init(&json_reader, *desired_payload, NULL));

  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&json_reader));
  if (json_reader.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&json_reader));

  // Update property locally if found.
  while (json_reader.token.kind != AZ_JSON_TOKEN_END_OBJECT)
  {
    if (az_json_token_is_text_equal(&json_reader.token, version_name))
    {
      break; // Desired property not found in payload.
    }
    else if (az_json_token_is_text_equal(&json_reader.token, reported_property_name))
    {
      // Move to the value token and store value.
      AZ_RETURN_IF_FAILED(az_json_reader_next_token(&json_reader));
      AZ_RETURN_IF_FAILED(az_json_token_get_int32(&json_reader.token, &reported_property_value));
      LOG_SUCCESS(
          "Client updated \"%.*s\" locally to %d.",
          az_span_size(reported_property_name),
          az_span_ptr(reported_property_name),
          reported_property_value);

      return AZ_OK;
    }
    else
    {
      AZ_RETURN_IF_FAILED(az_json_reader_skip_children(&json_reader)); // Ignore children tokens.
    }

    AZ_RETURN_IF_FAILED(az_json_reader_next_token(&json_reader)); // Check next sibling token.
  }

  LOG("Did not find \"%.*s\" in desired property payload. Nothing to update.",
      az_span_size(reported_property_name),
      az_span_ptr(reported_property_name));

  return AZ_OK;
}
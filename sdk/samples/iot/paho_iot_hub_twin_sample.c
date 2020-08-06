// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include <paho-mqtt/MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#define TIMEOUT_MQTT_RECEIVE_MS (60 * 1000)
#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)
#define MAX_MESSAGE_COUNT 5

#ifdef _MSC_VER
// "'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead."
#pragma warning(disable : 4996)
#endif

// DO NOT MODIFY: Device ID Environment Variable Name
#define ENV_DEVICE_ID "AZ_IOT_DEVICE_ID"

// DO NOT MODIFY: IoT Hub Hostname Environment Variable Name
#define ENV_IOT_HUB_HOSTNAME "AZ_IOT_HUB_HOSTNAME"

// DO NOT MODIFY: The path to a PEM file containing the device certificate and
// key as well as any intermediate certificates chaining to an uploaded group certificate.
#define ENV_DEVICE_X509_CERT_PEM_FILE "AZ_IOT_DEVICE_X509_CERT_PEM_FILE"

// DO NOT MODIFY: the path to a PEM file containing the server trusted CA
// This is usually not needed on Linux or Mac but needs to be set on Windows.
#define ENV_DEVICE_X509_TRUST_PEM_FILE "AZ_IOT_DEVICE_X509_TRUST_PEM_FILE"

// Logging with formatting
#define LOG_ERROR(...) \
  { \
    (void)fprintf(stderr, "ERROR:\t\t%s:%s():%d: ", __FILE__, __func__, __LINE__); \
    (void)fprintf(stderr, __VA_ARGS__); \
    (void)fprintf(stderr, "\n"); \
    fflush(stdout); \
    fflush(stderr); \
  }
#define LOG_SUCCESS(...) \
  { \
    (void)printf("SUCCESS:\t"); \
    (void)printf(__VA_ARGS__); \
    (void)printf("\n"); \
  }
#define LOG(...) \
  { \
    (void)printf("\t\t"); \
    (void)printf(__VA_ARGS__); \
    (void)printf("\n"); \
  }
#define LOG_AZ_SPAN(span_description, span) \
  { \
    (void)printf("\t\t%s ", span_description); \
    char* buffer = (char*)az_span_ptr(span); \
    for (int32_t i = 0; i < az_span_size(span); i++) \
    { \
      putchar(*buffer++); \
    } \
    (void)printf("\n"); \
  }

static const uint8_t null_terminator = '\0';
static char device_id[64];
static char iot_hub_hostname[128];
static char x509_cert_pem_file[512];
static char x509_trust_pem_file[256];

static char mqtt_client_id[128];
static char mqtt_username[128];
static char mqtt_endpoint[128];
static az_span mqtt_url_prefix = AZ_SPAN_LITERAL_FROM_STR("ssl://");
static az_span mqtt_url_suffix = AZ_SPAN_LITERAL_FROM_STR(":8883");

static char get_twin_topic[128];
static az_span get_twin_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("get_twin");
static char reported_property_topic[128];
static az_span reported_property_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("reported_prop");
static az_span reported_property_name = AZ_SPAN_LITERAL_FROM_STR("device_count");
static int32_t reported_property_value = 0;
static char reported_property_buffer[64];
static az_span version_name = AZ_SPAN_LITERAL_FROM_STR("$version");

static az_iot_hub_client client;
static MQTTClient mqtt_client;

//
// Configuration and connection functions
//
static az_result read_configuration_and_init_client();
static az_result read_configuration_entry(
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span buffer,
    az_span* out_value);
static az_result create_mqtt_endpoint(char* destination, int32_t destination_size, az_span iot_hub);
static int connect_device();
static int subscribe();

//
// Messaging functions
//
static void get_twin_document();
static void send_reported_property();
static az_result build_reported_property(az_span* reported_property_payload);
static void receive_desired_property();
static az_result update_property(az_span desired_payload);
static void receive_message();
static void parse_message(
    char* topic,
    int topic_len,
    const MQTTClient_message* message,
    az_iot_hub_client_twin_response* twin_response);

int main()
{
  int rc;

  // Read in the necessary environment variables and initialize the az_iot_hub_client
  if (az_failed(rc = read_configuration_and_init_client()))
  {
    printf(
        "Failed to read configuration from environment variables, az_result return code %04x\n",
        rc);
    return rc;
  }

  // Get the MQTT client id used for the MQTT connection
  if (az_failed(
          rc
          = az_iot_hub_client_get_client_id(&client, mqtt_client_id, sizeof(mqtt_client_id), NULL)))
  {
    printf("Failed to get MQTT clientId, az_result return code %04x\n", rc);
    return rc;
  }

  // Create the Paho MQTT client
  if ((rc = MQTTClient_create(
           &mqtt_client, mqtt_endpoint, mqtt_client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to create MQTT client, MQTTClient return code %d\n", rc);
    return rc;
  }

  // Connect to IoT Hub
  if ((rc = connect_device()) != MQTTCLIENT_SUCCESS)
  {
    return rc;
  }

  // Subscribe to the necessary twin topics to receive twin updates and responses
  if ((rc = subscribe()) != MQTTCLIENT_SUCCESS)
  {
    return rc;
  }

  get_twin_document();
  send_reported_property();
  receive_desired_property();

  // Gracefully disconnect: send the disconnect packet and close the socket
  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to disconnect MQTT client, MQTTClient return code %d\n", rc);
    return rc;
  }
  printf("Disconnected\n");

  // Clean up and release resources allocated by the mqtt client
  MQTTClient_destroy(&mqtt_client);

  return 0;
}

// Read the user environment variables used to connect to IoT Hub and initialize the IoT Hub client
static az_result read_configuration_and_init_client()
{
  az_span cert = AZ_SPAN_FROM_BUFFER(x509_cert_pem_file);
  AZ_RETURN_IF_FAILED(
      read_configuration_entry(ENV_DEVICE_X509_CERT_PEM_FILE, NULL, false, cert, &cert));

  az_span trusted = AZ_SPAN_FROM_BUFFER(x509_trust_pem_file);
  AZ_RETURN_IF_FAILED(
      read_configuration_entry(ENV_DEVICE_X509_TRUST_PEM_FILE, "", false, trusted, &trusted));

  az_span device_id_span = AZ_SPAN_FROM_BUFFER(device_id);
  AZ_RETURN_IF_FAILED(
      read_configuration_entry(ENV_DEVICE_ID, NULL, false, device_id_span, &device_id_span));

  az_span iot_hub_hostname_span = AZ_SPAN_FROM_BUFFER(iot_hub_hostname);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_IOT_HUB_HOSTNAME, NULL, false, iot_hub_hostname_span, &iot_hub_hostname_span));

  // Paho requires that the MQTT endpoint be of the form ssl://<HUB ENDPOINT>:8883
  AZ_RETURN_IF_FAILED(
      create_mqtt_endpoint(mqtt_endpoint, (int32_t)sizeof(mqtt_endpoint), iot_hub_hostname_span));

  // Initialize the hub client with the hub host endpoint and the default connection options
  AZ_RETURN_IF_FAILED(az_iot_hub_client_init(
      &client,
      az_span_slice(iot_hub_hostname_span, 0, (int32_t)strlen(iot_hub_hostname)),
      az_span_slice(device_id_span, 0, (int32_t)strlen(device_id)),
      NULL));

  return AZ_OK;
}

// Read OS environment variables using stdlib function
static az_result read_configuration_entry(
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span buffer,
    az_span* out_value)
{
  printf("%s = ", env_name);
  char* env_value = getenv(env_name);

  if (env_value == NULL && default_value != NULL)
  {
    env_value = default_value;
  }

  if (env_value != NULL)
  {
    printf("%s\n", hide_value ? "***" : env_value);
    az_span env_span = az_span_create_from_str(env_value);
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(buffer, az_span_size(env_span));
    az_span_copy(buffer, env_span);
    *out_value = az_span_slice(buffer, 0, az_span_size(env_span));
  }
  else
  {
    printf("(missing) Please set the %s environment variable.\n", env_name);
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

// Create mqtt endpoint e.g: ssl//contoso.azure-devices.net:8883
static az_result create_mqtt_endpoint(char* destination, int32_t destination_size, az_span iot_hub)
{
  int32_t iot_hub_length = (int32_t)strlen(iot_hub_hostname);
  int32_t required_size = az_span_size(mqtt_url_prefix) + iot_hub_length
      + az_span_size(mqtt_url_suffix) + (int32_t)sizeof(null_terminator);

  if (required_size > destination_size)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }

  az_span destination_span = az_span_create((uint8_t*)destination, destination_size);
  az_span remainder = az_span_copy(destination_span, mqtt_url_prefix);
  remainder = az_span_copy(remainder, az_span_slice(iot_hub, 0, iot_hub_length));
  remainder = az_span_copy(remainder, mqtt_url_suffix);
  az_span_copy_u8(remainder, null_terminator);

  return AZ_OK;
}

static int connect_device()
{
  int rc;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;

  // NOTE: We recommend setting clean session to false in order to receive any pending messages
  mqtt_connect_options.cleansession = false;
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  // Get the MQTT username used to connect to IoT Hub
  if (az_failed(
          rc
          = az_iot_hub_client_get_user_name(&client, mqtt_username, sizeof(mqtt_username), NULL)))
  {
    printf("Failed to get MQTT username, az_result return code %04x\n", rc);
    return rc;
  }

  // This sample uses X509 authentication so the password field is set to NULL
  mqtt_connect_options.username = mqtt_username;
  mqtt_connect_options.password = NULL;

  // Set the device cert for tls mutual authentication
  mqtt_ssl_options.keyStore = (char*)x509_cert_pem_file;
  if (*x509_trust_pem_file != '\0')
  {
    mqtt_ssl_options.trustStore = (char*)x509_trust_pem_file;
  }

  mqtt_connect_options.ssl = &mqtt_ssl_options;

  // Connect to IoT Hub
  if ((rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to connect, MQTTClient return code %d\n", rc);
    return rc;
  }

  return MQTTCLIENT_SUCCESS;
}

static int subscribe()
{
  int rc;

  // Subscribe to the desired properties PATCH topic. Messages received on this topic will be
  // updates to the desired properties.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to subscribe to the twin patch topic filter, MQTTClient return code %d\n", rc);
    return rc;
  }

  // Subscribe to the twin response topic. Messages received on this topic will be response statuses
  // from published reported properties or the requested twin document from twin GET publish
  // messages
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    printf(
        "Failed to subscribe to the twin response topic filter, MQTTClient return code %d\n", rc);
    return rc;
  }

  return MQTTCLIENT_SUCCESS;
}

static void get_twin_document()
{
  int rc;
  LOG("Device requesting twin document from service.");

  // Get the topic to send a twin GET publish message to service.
  if (az_failed(
          rc = az_iot_hub_client_twin_document_get_publish_topic(
              &client, get_twin_topic_request_id, get_twin_topic, sizeof(get_twin_topic), NULL)))
  {
    LOG_ERROR("Unable to get twin document publish topic, az_result return code %04x", rc);
    exit(rc);
  }

  // Publish the twin document request. This will trigger the service to send back the twin document
  // for this device. The response is handled in the receive_message function.
  if ((rc = MQTTClient_publish(mqtt_client, get_twin_topic, 0, NULL, 0, 0, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to publish twin document request, MQTTClient return code %d", rc);
    exit(rc);
  }

  receive_message();

  return;
}

static void send_reported_property()
{
  int rc;
  az_span reported_property_payload;
  LOG("Device sending reported property to service.");

  // Get the topic used to send a reported property update
  if (az_failed(
          rc = az_iot_hub_client_twin_patch_get_publish_topic(
              &client,
              reported_property_topic_request_id,
              reported_property_topic,
              sizeof(reported_property_topic),
              NULL)))
  {
    LOG_ERROR("Unable to get reported property publish topic, az_result return code %04x", rc);
    exit(rc);
  }

  if (az_failed(rc = build_reported_property(&reported_property_payload)))
  {
    LOG_ERROR("Unable to build reported property payload to send, az_result return code %04x", rc);
    exit(rc);
  }
  LOG_AZ_SPAN("Sending payload:", reported_property_payload);

  // Publish the reported property payload to IoT Hub. This will trigger the service to send back a
  // response to this device. The response is handled in the receive_message function.
  if ((rc = MQTTClient_publish(
           mqtt_client,
           reported_property_topic,
           az_span_size(reported_property_payload),
           az_span_ptr(reported_property_payload),
           0,
           0,
           NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to publish reported property, MQTTClient return code %d", rc);
    exit(rc);
  }

  receive_message();

  return;
}

static az_result build_reported_property(az_span* reported_property_payload)
{
  az_json_writer json_writer;

  AZ_RETURN_IF_FAILED(
      az_json_writer_init(&json_writer, AZ_SPAN_FROM_BUFFER(reported_property_buffer), NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&json_writer));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&json_writer, reported_property_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(&json_writer, reported_property_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&json_writer));

  *reported_property_payload = az_json_writer_get_bytes_used_in_destination(&json_writer);

  return AZ_OK;
}

static void receive_desired_property()
{
  // Wait until max # messages received
  for (uint8_t message_count = 0; message_count < MAX_MESSAGE_COUNT; message_count++)
  {
    LOG("Device waiting for desired property twin message #%d from service.", message_count + 1);
    receive_message();
    send_reported_property();
  }

  return;
}

static void receive_message()
{
  int rc;
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;
  az_iot_hub_client_twin_response twin_response;

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

  parse_message(topic, topic_len, message, &twin_response);
  LOG_SUCCESS("Client parsed message.");

  LOG(" "); // formatting

  MQTTClient_freeMessage(&message);
  MQTTClient_free(topic);
  return;
}

static void parse_message(
    char* topic,
    int topic_len,
    const MQTTClient_message* message,
    az_iot_hub_client_twin_response* twin_response)
{
  int rc;
  az_span topic_span = az_span_create((uint8_t*)topic, topic_len);
  az_span message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  if (az_failed(
          rc = az_iot_hub_client_twin_parse_received_topic(&client, topic_span, twin_response)))
  {
    LOG_ERROR("Message from unknown topic: az_result return code 0x%04x.", rc);
    LOG_AZ_SPAN("Topic:", topic_span);
    exit(rc);
  }
  LOG_SUCCESS("Client received a valid topic response:");
  LOG_AZ_SPAN("Topic:", topic_span);
  LOG_AZ_SPAN("Payload:", message_span);
  LOG("Status: %d", twin_response->status);

  switch (twin_response->response_type) // There are only 3 enum values
  {
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET:
      LOG("Type: GET");
      break;

    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
      LOG("Type: Reported Properties");
      break;

    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
      LOG("Type: Desired Properties");

      if (az_failed(rc = update_property(message_span)))
      {
        LOG_ERROR("Failed to update property locally, az_result return code %04x\n", rc);
        exit(rc);
      }
      break;
  }

  return;
}

static az_result update_property(az_span desired_payload)
{
  // Parse desired property payload
  az_json_reader json_reader;
  AZ_RETURN_IF_FAILED(az_json_reader_init(&json_reader, desired_payload, NULL));

  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&json_reader));
  if (json_reader.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&json_reader));

  // Update property locally if found
  while (json_reader.token.kind != AZ_JSON_TOKEN_END_OBJECT)
  {
    if (az_json_token_is_text_equal(&json_reader.token, version_name))
    {
      break;
    }
    else if (az_json_token_is_text_equal(&json_reader.token, reported_property_name))
    {
      // Move to the value token
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
      // ignore other tokens
      AZ_RETURN_IF_FAILED(az_json_reader_skip_children(&json_reader));
    }

    AZ_RETURN_IF_FAILED(az_json_reader_next_token(&json_reader));
  }

  LOG("Did not find \"%.*s\" in desired property payload. Nothing to update.",
      az_span_size(reported_property_name),
      az_span_ptr(reported_property_name));

  return AZ_OK;
}

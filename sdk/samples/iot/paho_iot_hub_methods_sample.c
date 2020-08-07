// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "sample.h"

#define SAMPLE_TYPE PAHO_IOT_HUB
#define SAMPLE_NAME PAHO_IOT_HUB_METHODS_SAMPLE

#define MAX_METHOD_MESSAGE_COUNT 5
#define TIMEOUT_MQTT_RECEIVE_MS (60 * 1000)

static const az_span ping_method_name = AZ_SPAN_LITERAL_FROM_STR("ping");
static const az_span method_fail_response = AZ_SPAN_LITERAL_FROM_STR("{}");

static sample_environment_variables env_vars;
static az_iot_hub_client hub_client;
static MQTTClient mqtt_client;
static char mqtt_client_username_buffer[128];

// Functions
void create_and_configure_client();
void connect_client_to_iot_hub();
void subscribe_client_to_iot_hub_topics();
void receive_messages();
void disconnect_client_from_iot_hub();

void parse_message(
    char* topic,
    int topic_len,
    const MQTTClient_message* message,
    az_iot_hub_client_method_request* method_request);
void invoke_method(const az_iot_hub_client_method_request* method_request);
az_span ping_method(void);
void send_method_response(
    const az_iot_hub_client_method_request* request,
    uint16_t status,
    az_span response);

// This sample receives incoming method commands invoked from the the Azure IoT Hub.
// It will successfully receive up to MAX_METHOD_MESSAGE_COUNT method commands sent from the service.
// If a timeout occurs of TIMEOUT_MQTT_RECEIVE_MS while waiting for a message, the sample will exit.
// X509 self-certification is used.

  To send a method command, select your device's Direct Method tab in your Azure IoT Hub. Enter a method name and select Invoke Method. A method named `ping` is supported, which if successful will return a json payload of the following:

  ```json
  {"response": "pong"}
  ```

  No other method commands are supported. If any are attempted to be invoked, the log will report the method is not found.
int main()
{
  create_and_configure_client();
  LOG_SUCCESS("Client created and configured.");

  connect_client_to_iot_hub();
  LOG_SUCCESS("Client connected to IoT Hub.");

  subscribe_client_to_iot_hub_topics();
  LOG_SUCCESS("Client subscribed to IoT Hub topics.");

  receive_messages();
  LOG_SUCCESS("Client received messages.")

  disconnect_client_from_iot_hub();
  LOG_SUCCESS("Client disconnected from IoT Hub.");

  return 0;
}

void create_and_configure_client()
{
  int rc;

  // Reads in environment variables set by user for purposes of running sample
  if (az_failed(rc = read_environment_variables(SAMPLE_TYPE, SAMPLE_NAME, &env_vars)))
  {
    LOG_ERROR(
        "Failed to read configuration from environment variables: az_result return code 0x%04x.",
        rc);
    exit(rc);
  }

  // Set mqtt endpoint as null terminated in buffer
  char hub_mqtt_endpoint_buffer[128];
  if (az_failed(
          rc = create_mqtt_endpoint(
              SAMPLE_TYPE, hub_mqtt_endpoint_buffer, sizeof(hub_mqtt_endpoint_buffer))))
  {
    LOG_ERROR("Failed to create MQTT endpoint: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Initialize the hub client with the default connection options
  if (az_failed(
          rc = az_iot_hub_client_init(
              &hub_client, env_vars.hub_hostname, env_vars.hub_device_id, NULL)))
  {
    LOG_ERROR("Failed to initialize hub client: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Get the MQTT client id used for the MQTT connection
  char mqtt_client_id_buffer[128];
  if (az_failed(
          rc = az_iot_hub_client_get_client_id(
              &hub_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT client id: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Create the Paho MQTT client
  if ((rc = MQTTClient_create(
           &mqtt_client,
           hub_mqtt_endpoint_buffer,
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

  if (az_failed(
          rc = az_iot_hub_client_get_user_name(
              &hub_client, mqtt_client_username_buffer, sizeof(mqtt_client_username_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT username: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;
  mqtt_connect_options.username = mqtt_client_username_buffer;
  mqtt_connect_options.password = NULL; // This sample uses x509 authentication.
  mqtt_connect_options.cleansession = false; // Set to false so can receive any pending messages.
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  mqtt_ssl_options.keyStore = (char*)x509_cert_pem_file_path_buffer;
  if (*az_span_ptr(env_vars.x509_trust_pem_file_path) != '\0')
  {
    mqtt_ssl_options.trustStore = (char*)x509_trust_pem_file_path_buffer;
  }
  mqtt_connect_options.ssl = &mqtt_ssl_options;

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

  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to subscribe to the Methods topic: MQTTClient return code %d.", rc);
    exit(rc);
  }

  return;
}

void receive_messages()
{
  int rc;
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;
  az_iot_hub_client_method_request method_request;

  LOG("Waiting for messages.");

  // Wait until max # messages received or timeout to receive a single message expires.
  for(uint8_t message_count = 0; message_count < MAX_MESSAGE_COUNT; message_count++)
  {
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

    parse_message(topic, topic_len, message, &method_request);
    LOG_SUCCESS("Client parsed message.");

    invoke_method(&method_request);
    LOG(" "); //formatting
  }

  MQTTClient_freeMessage(&message);
  MQTTClient_free(topic);
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

void parse_message(
    char* topic,
    int topic_len,
    const MQTTClient_message* message,
    az_iot_hub_client_method_request* method_request)
{
  int rc;
  az_span topic_span = az_span_init((uint8_t*)topic, topic_len);
  az_span message_span = az_span_init((uint8_t*)message->payload, message->payloadlen);

  if (az_failed(rc = az_iot_hub_client_methods_parse_received_topic(&hub_client, topic_span, method_request)))
  {
    LOG_ERROR("Message from unknown topic: az_result return code 0x%04x.", rc);
    LOG_AZ_SPAN("Topic:", topic_span);
    exit(rc);
  }
  LOG_SUCCESS("Client received a valid topic response:");
  LOG_AZ_SPAN("Topic:", topic_span);
  LOG_AZ_SPAN("Payload:", message_span);

  return;
}

void invoke_method(const az_iot_hub_client_method_request* method_request)
{
  if (az_span_is_content_equal(ping_method_name, method_request->name))
  {
    az_span response = ping_method();
    LOG_SUCCESS("Client invoked ping method.");

    send_method_response(method_request, AZ_IOT_STATUS_OK, response);
  }
  else
  {
    LOG_AZ_SPAN("Method not found:", method_request->name);
    send_method_response(method_request, AZ_IOT_STATUS_NOT_FOUND, method_fail_response);
  }

  return;
}

az_span ping_method(void)
{
  LOG("PING!");
  az_span response = AZ_SPAN_LITERAL_FROM_STR("{\"response\": \"pong\"}");

  return response;
}

void send_method_response(
    const az_iot_hub_client_method_request* request,
    uint16_t status,
    az_span response)
{
  int rc;

  // Get the response topic to publish the method response
  char methods_response_topic[128];
  if (az_failed(
          rc = az_iot_hub_client_methods_response_get_publish_topic(
              &hub_client,
              request->request_id,
              status,
              methods_response_topic,
              sizeof(methods_response_topic),
              NULL)))
  {
    LOG_ERROR("Unable to get method response publish topic, az_result return code %04x", rc);
    exit(rc);
  }

  // Send the methods response
  if ((rc = MQTTClient_publish(
           mqtt_client,
           methods_response_topic,
           az_span_size(response),
           az_span_ptr(response),
           0,
           0,
           NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to publish method response, MQTTClient return code %d\n", rc);
    exit(rc);
  }
  LOG_SUCCESS("Client published method response:");
  LOG("Status: %u", status);
  LOG_AZ_SPAN("Payload:", response);

  return;
}

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

#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)

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

static char methods_response_topic[128];
static const az_span ping_method_name_span = AZ_SPAN_LITERAL_FROM_STR("ping");
static az_span ping_method_success_response = AZ_SPAN_LITERAL_FROM_STR("{\"response\": \"pong\"}");
static az_span ping_method_fail_response = AZ_SPAN_LITERAL_FROM_STR("{}");

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
static int on_received(void* context, char* topicName, int topicLen, MQTTClient_message* message);
static az_span ping_method(void);
static int send_method_response(
    az_iot_hub_client_method_request* request,
    uint16_t status,
    az_span response);

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
  size_t client_id_length;
  if (az_failed(
          rc = az_iot_hub_client_get_client_id(
              &client, mqtt_client_id, sizeof(mqtt_client_id), &client_id_length)))
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

  // Set the callback for incoming MQTT messages
  if ((rc = MQTTClient_setCallbacks(mqtt_client, NULL, NULL, on_received, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to set MQTT callbacks, MQTTClient return code %d\n", rc);
    return rc;
  }

  // Connect to IoT Hub
  if ((rc = connect_device()) != MQTTCLIENT_SUCCESS)
  {
    return rc;
  }

  // Subscribe to the methods topic to receive method invocations
  if ((rc = subscribe()) != MQTTCLIENT_SUCCESS)
  {
    return rc;
  }

  printf("Subscribed to topics.\n");

  // Wait for any incoming method invocations
  printf("Waiting for activity. [Press any key to abort]\n");
  (void)getchar();

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

// Read the user environment variables used to connect to IoT Hub
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
    az_span env_span = az_span_from_str(env_value);
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

  az_span destination_span = az_span_init((uint8_t*)destination, destination_size);
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

  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to subscribe to the methods topic filter, MQTTClient return code %d\n", rc);
    return rc;
  }

  return MQTTCLIENT_SUCCESS;
}

static int on_received(void* context, char* topicName, int topicLen, MQTTClient_message* message)
{
  (void)context;
  int rc;

  printf("Received a message from service.\n");
  printf("Topic: %s\n", topicName);

  if (topicLen == 0)
  {
    // The length of the topic if there are one or more NULL characters embedded in topicName,
    // otherwise topicLen is 0.
    topicLen = (int)strlen(topicName);
  }
  az_span topic_span = az_span_init((uint8_t*)topicName, topicLen);

  // Parse the incoming message topic and check to make sure it is a methods message
  az_iot_hub_client_method_request method_request;
  if (az_failed(
          rc
          = az_iot_hub_client_methods_parse_received_topic(&client, topic_span, &method_request)))
  {
    printf("Topic is not a methods message, az_result return code %04x\n", rc);
  }
  else
  {
    if (az_span_is_content_equal(ping_method_name_span, method_request.name))
    {
      // Invoke Method
      az_span response = ping_method();

      // Send a response
      if ((send_method_response(&method_request, AZ_IOT_STATUS_OK, response)) != MQTTCLIENT_SUCCESS)
      {
        printf("Unable to send %d response.\n", AZ_IOT_STATUS_OK);
      }
    }
    else
    {
      // Unsupported Method
      printf(
          "Method %.*s not found\n",
          az_span_size(method_request.name),
          az_span_ptr(method_request.name));
      if ((send_method_response(
              &method_request, AZ_IOT_STATUS_NOT_FOUND, ping_method_fail_response))
          != MQTTCLIENT_SUCCESS)
      {
        printf("Unable to send %d response.\n", AZ_IOT_STATUS_NOT_FOUND);
      }
    }
  }

  putchar('\n');
  MQTTClient_freeMessage(&message);
  MQTTClient_free(topicName);

  return 1;
}

static az_span ping_method(void)
{
  printf("PING!\n");
  return ping_method_success_response;
}

static int send_method_response(
    az_iot_hub_client_method_request* request,
    uint16_t status,
    az_span response)
{
  int rc;

  // Get the response topic to publish the method response
  if (az_failed(
          rc = az_iot_hub_client_methods_response_get_publish_topic(
              &client,
              request->request_id,
              status,
              methods_response_topic,
              sizeof(methods_response_topic),
              NULL)))
  {
    printf("Unable to get method response publish topic, az_result return code %04x", rc);
    return rc;
  }

  printf("Status: %u\tPayload: ", status);
  char* payload_char = (char*)az_span_ptr(response);
  for (int32_t i = 0; i < az_span_size(response); i++)
  {
    putchar(*payload_char++);
  }
  putchar('\n');

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
    printf("Failed to publish method response, MQTTClient return code %d\n", rc);
    return rc;
  }

  printf("Sent response\n");

  return MQTTCLIENT_SUCCESS;
}

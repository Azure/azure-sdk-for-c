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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <az_iot_hub_client.h>
#include <az_json.h>
#include <az_result.h>
#include <az_span.h>

// TODO: #564 - Remove the use of the _az_cfh.h header in samples.
//              Note: this is required to work-around MQTTClient.h as well as az_span init issues.
#include <_az_cfg.h>

// Device ID
#define DEVICE_ID "AZ_IOT_DEVICE_ID"

// IoT Hub Hostname
#define IOT_HUB_HOSTNAME "AZ_IOT_HUB_HOSTNAME"

// AZ_IOT_DEVICE_X509_CERT_PEM_FILE is the path to a PEM file containing the device certificate and
// key as well as any intermediate certificates chaining to an uploaded group certificate.
#define DEVICE_X509_CERT_PEM_FILE "AZ_IOT_DEVICE_X509_CERT_PEM_FILE"

// AZ_IOT_DEVICE_X509_TRUST_PEM_FILE is the path to a PEM file containing the server trusted CA
// This is usually not needed on Linux or Mac but needs to be set on Windows.
#define DEVICE_X509_TRUST_PEM_FILE "AZ_IOT_DEVICE_X509_TRUST_PEM_FILE"

#define TIMEOUT_MQTT_DISCONNECT_MS 10 * 1000

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

static char methods_subscribe_topic[128];
static char methods_response_topic[128];
static const az_span ping_method_name_span = AZ_SPAN_LITERAL_FROM_STR("ping");
static az_span ping_method_success_response = AZ_SPAN_LITERAL_FROM_STR("{\"response\": \"pong\"}");
static az_span ping_method_fail_response = AZ_SPAN_LITERAL_FROM_STR("{}");

static az_iot_hub_client client;
static MQTTClient mqtt_client;

static az_result read_configuration_entry(
    const char* name,
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span buffer,
    az_span* out_value)
{
  printf("%s = ", name);
  char* env = getenv(env_name);

  if (env != NULL)
  {
    printf("%s\n", hide_value ? "***" : env);
    az_span env_span = az_span_from_str(env);
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(buffer, az_span_size(env_span));
    az_span_copy(buffer, env_span);
    *out_value = az_span_slice(buffer, 0, az_span_size(env_span));
  }
  else if (default_value != NULL)
  {
    printf("%s\n", default_value);
    az_span default_span = az_span_from_str(default_value);
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(buffer, az_span_size(default_span));
    az_span_copy(buffer, default_span);
    *out_value = az_span_slice(buffer, 0, az_span_size(default_span));
  }
  else
  {
    printf("(missing) Please set the %s environment variable.\n", env_name);
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

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

static az_result read_configuration_and_init_client()
{
  az_span cert = AZ_SPAN_FROM_BUFFER(x509_cert_pem_file);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      "X509 Certificate PEM Store File", DEVICE_X509_CERT_PEM_FILE, NULL, false, cert, &cert));

  az_span trusted = AZ_SPAN_FROM_BUFFER(x509_trust_pem_file);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      "X509 Trusted PEM Store File", DEVICE_X509_TRUST_PEM_FILE, "", false, trusted, &trusted));

  az_span device_id_span = AZ_SPAN_FROM_BUFFER(device_id);
  AZ_RETURN_IF_FAILED(
      read_configuration_entry("Device ID", DEVICE_ID, "", false, device_id_span, &device_id_span));

  az_span iot_hub_hostname_span = AZ_SPAN_FROM_BUFFER(iot_hub_hostname);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      "IoT Hub Hostname", IOT_HUB_HOSTNAME, "", false, iot_hub_hostname_span, &iot_hub_hostname_span));

  AZ_RETURN_IF_FAILED(
      create_mqtt_endpoint(mqtt_endpoint, (int32_t)sizeof(mqtt_endpoint), iot_hub_hostname_span));

  AZ_RETURN_IF_FAILED(az_iot_hub_client_init(
      &client,
      az_span_slice(iot_hub_hostname_span, 0, (int32_t)strlen(iot_hub_hostname)),
      az_span_slice(device_id_span, 0, (int32_t)strlen(device_id)),
      NULL));

  return AZ_OK;
}

static az_span ping_method(void)
{
  // Nothing to do. Good to go.
  return ping_method_success_response;
}

static int send_method_response(
    az_iot_hub_client_method_request* request,
    uint16_t status,
    az_span response)
{
  // Get the response topic
  if (az_iot_hub_client_methods_response_get_publish_topic(
           &client,
           request->request_id,
           status,
           methods_response_topic,
           sizeof(methods_response_topic),
           NULL)
      != AZ_OK)
  {
    printf("Unable to get twin document publish topic");
    return -1;
  }

  printf("Status: %u\tPayload:", status);
  char* payload_char = (char*)az_span_ptr(response);
  for (int32_t i = 0; i < az_span_size(response); i++)
  {
    putchar(*(payload_char + i));
  }
  putchar('\n');

  // Send the response
  int rc;
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
    printf("Failed to send method response, return code %d\n", rc);
    return rc;
  }

  printf("Sent response\n");

  return 0;
}

static int on_received(void* context, char* topicName, int topicLen, MQTTClient_message* message)
{
  (void)context;

  if (topicLen == 0)
  {
    // The length of the topic if there are one or more NULL characters embedded in topicName,
    // otherwise topicLen is 0.
    topicLen = (int)strlen(topicName);
  }

  az_iot_hub_client_method_request method_request;
  if (az_iot_hub_client_methods_parse_received_topic(
          &client, az_span_init((uint8_t*)topicName, topicLen), &method_request)
      == AZ_OK)
  {
    printf("Direct Method arrived\n");
    if (az_span_is_content_equal(ping_method_name_span, method_request.name))
    {
      // Invoke Method
      az_span response = ping_method();

      // Send a response
      int rc;
      if ((rc = send_method_response(&method_request, 200, response)) != 0)
      {
        printf("Unable to send %d response, status %d\n", 200, rc);
      }
    }
    else
    {
      // Unsupported Method
      int rc;
      if ((rc = send_method_response(&method_request, 404, ping_method_fail_response)) != 0)
      {
        printf("Unable to send %d response, status %d\n", 404, rc);
      }
    }
  }

  MQTTClient_freeMessage(&message);
  MQTTClient_free(topicName);

  return 1;
}

static int connect_device()
{
  int rc;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;
  mqtt_connect_options.cleansession = false;
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  size_t username_length;
  if ((rc = az_iot_hub_client_get_user_name(
           &client, mqtt_username, sizeof(mqtt_username), &username_length))
      != AZ_OK)

  {
    printf("Failed to get MQTT clientId, return code %d\n", rc);
    return rc;
  }

  mqtt_connect_options.username = mqtt_username;
  mqtt_connect_options.password = NULL;

  mqtt_ssl_options.keyStore = (char*)x509_cert_pem_file;
  if (*x509_trust_pem_file != '\0')
  {
    mqtt_ssl_options.trustStore = (char*)x509_trust_pem_file;
  }

  mqtt_connect_options.ssl = &mqtt_ssl_options;

  if ((rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to connect, return code %d\n", rc);
    return rc;
  }

  return 0;
}

static int subscribe()
{
  int rc;

  size_t methods_subscribe_topic_length;
  if ((rc = az_iot_hub_client_methods_get_subscribe_topic_filter(
           &client,
           methods_subscribe_topic,
           sizeof(methods_subscribe_topic),
           &methods_subscribe_topic_length))
      != AZ_OK)

  {
    printf("Failed to get method topic filter, return code %d\n", rc);
    return rc;
  }

  if ((rc = MQTTClient_subscribe(mqtt_client, methods_subscribe_topic, 1)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to subscribe to method topic filter, return code %d\n", rc);
    return rc;
  }

  return 0;
}

int main()
{
  int rc;

  if ((rc = read_configuration_and_init_client()) != AZ_OK)
  {
    printf("Failed to read configuration from environment variables, return code %d\n", rc);
    return rc;
  }

  size_t client_id_length;
  if ((rc
       = az_iot_hub_client_get_client_id(&client, mqtt_client_id, sizeof(mqtt_client_id), &client_id_length))
      != AZ_OK)
  {
    printf("Failed to get MQTT clientId, return code %d\n", rc);
    return rc;
  }

  if ((rc = MQTTClient_create(
           &mqtt_client, mqtt_endpoint, mqtt_client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to create MQTT client, return code %d\n", rc);
    return rc;
  }

  if ((rc = MQTTClient_setCallbacks(mqtt_client, NULL, NULL, on_received, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to set MQTT callbacks, return code %d\n", rc);
    return rc;
  }

  if ((rc = connect_device()) != 0)
  {
    return rc;
  }

  if ((rc = subscribe()) != 0)
  {
    return rc;
  }

  printf("Subscribed to topics.\n");

  printf("Waiting for activity. [Press ENTER to abort]\n");
  (void)getchar();

  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to disconnect MQTT client, return code %d\n", rc);
    return rc;
  }

  printf("Disconnected.\n");
  MQTTClient_destroy(&mqtt_client);

  return 0;
}

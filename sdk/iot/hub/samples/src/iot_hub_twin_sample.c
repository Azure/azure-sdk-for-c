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

static char twin_response_topic[128];
static char twin_desired_topic[128];
static char get_twin_topic[128];
static az_span get_twin_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("get_twin");
static char reported_property_topic[128];
static az_span reported_property_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("reported_prop");
static az_span reported_property_name = AZ_SPAN_LITERAL_FROM_STR("foo");
static int32_t reported_property_value = 0;
static char reported_property_payload[64];

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
      "IoT Hub Hostname",
      IOT_HUB_HOSTNAME,
      "",
      false,
      iot_hub_hostname_span,
      &iot_hub_hostname_span));

  AZ_RETURN_IF_FAILED(
      create_mqtt_endpoint(mqtt_endpoint, (int32_t)sizeof(mqtt_endpoint), iot_hub_hostname_span));

  AZ_RETURN_IF_FAILED(az_iot_hub_client_init(
      &client,
      az_span_slice(iot_hub_hostname_span, 0, (int32_t)strlen(iot_hub_hostname)),
      az_span_slice(device_id_span, 0, (int32_t)strlen(device_id)),
      NULL));

  return AZ_OK;
}

static void print_twin_response_type(
    az_iot_hub_client_twin_response_type type,
    MQTTClient_message* message)
{
  switch (type)
  {
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET:
      printf("A twin GET response was received\n");
      if (message->payloadlen)
      {
        printf("Payload:\n%.*s\n", message->payloadlen, (char*)message->payload);
      }
      break;
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
      printf("A twin desired properties message was received\n");
      printf("Payload:\n%.*s\n", message->payloadlen, (char*)message->payload);
      break;
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
      printf("A twin reported properties message was received\n");
      break;
  }
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

  az_span topic_span = az_span_init((uint8_t*)topicName, topicLen);

  az_iot_hub_client_twin_response twin_response;
  if (az_iot_hub_client_twin_parse_received_topic(&client, topic_span, &twin_response) == AZ_OK)
  {
    printf("Twin Message Arrived\n");
    print_twin_response_type(twin_response.response_type, message);
    printf("Response status was %d\n", twin_response.status);
  }

  putchar('\n');
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

  if ((rc = az_iot_hub_client_twin_patch_get_subscribe_topic_filter(
           &client, twin_desired_topic, sizeof(twin_desired_topic), NULL))
      != AZ_OK)
  {
    printf("Failed to get twin patch topic filter, return code %d\n", rc);
    return rc;
  }

  if ((rc = az_iot_hub_client_twin_response_get_subscribe_topic_filter(
           &client, twin_response_topic, sizeof(twin_response_topic), NULL))
      != AZ_OK)
  {
    printf("Failed to get twin response topic filter, return code %d\n", rc);
    return rc;
  }

  if ((rc = MQTTClient_subscribe(mqtt_client, twin_desired_topic, 1)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to subscribe to twin patch topic filter, return code %d\n", rc);
    return rc;
  }

  if ((rc = MQTTClient_subscribe(mqtt_client, twin_response_topic, 1)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to subscribe to twin response topic filter, return code %d\n", rc);
    return rc;
  }

  return 0;
}

static int send_get_twin()
{
  int rc;
  printf("Requesting twin document\n");

  if ((rc = az_iot_hub_client_twin_document_get_publish_topic(
           &client, get_twin_topic_request_id, get_twin_topic, sizeof(get_twin_topic), NULL))
      != AZ_OK)
  {
    printf("Unable to get twin document publish topic, return code %d\n", rc);
    return rc;
  }

  if ((rc = MQTTClient_publish(mqtt_client, get_twin_topic, 0, NULL, 0, 0, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to publish twin document request, return code %d\n", rc);
    return rc;
  }
  return rc;
}

static int build_reported_property(az_json_builder* json_builder)
{
  az_result result;
  result = az_json_builder_init(json_builder, AZ_SPAN_FROM_BUFFER(reported_property_payload));
  result = az_json_builder_append_token(json_builder, az_json_token_object_start());
  az_json_token reported_property_value_token
      = az_json_token_number((double)reported_property_value++);
  result = az_json_builder_append_object(
      json_builder, reported_property_name, reported_property_value_token);
  result = az_json_builder_append_token(json_builder, az_json_token_object_end());

  return result;
}

static int send_reported_property()
{
  int rc;
  printf("Sending reported property\n");

  if ((rc = az_iot_hub_client_twin_patch_get_publish_topic(
           &client,
           reported_property_topic_request_id,
           reported_property_topic,
           sizeof(reported_property_topic),
           NULL))
      != AZ_OK)
  {
    printf("Unable to get twin document publish topic, return code %d\n", rc);
    return rc;
  }

  az_json_builder json_builder;
  if ((rc = build_reported_property(&json_builder)) != AZ_OK)
  {
    return rc;
  }
  az_span json_payload = az_json_builder_span_get(&json_builder);

  printf("Payload: %.*s\n", az_span_size(json_payload), (char*)az_span_ptr(json_payload));

  if ((rc = MQTTClient_publish(
           mqtt_client,
           reported_property_topic,
           az_span_size(json_payload),
           az_span_ptr(json_payload),
           0,
           0,
           NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to publish reported property, return code %d\n", rc);
    return rc;
  }
  return rc;
}

int main()
{
  int rc;

  if ((rc = read_configuration_and_init_client()) != AZ_OK)
  {
    printf("Failed to read configuration from environment variables, return code %d\n", rc);
    return rc;
  }

  if ((rc = az_iot_hub_client_get_client_id(&client, mqtt_client_id, sizeof(mqtt_client_id), NULL))
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

  printf(
      "\nWaiting for activity:\nPress 'g' to get the twin document\nPress 'r' to send a reported "
      "property\n[Press 'q' to quit]\n");

  int input;
  while (1)
  {
    input = getchar();
    if (input != '\n')
    {
      switch (input)
      {
        case 'g':
          send_get_twin();
          break;
        case 'r':
          send_reported_property();
          break;
        default:
          break;
      }
      if (input == 'q')
      {
        break;
      }
    }
  }

  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to disconnect MQTT client, return code %d\n", rc);
    return rc;
  }

  printf("Disconnected.\n");
  MQTTClient_destroy(&mqtt_client);

  return 0;
}

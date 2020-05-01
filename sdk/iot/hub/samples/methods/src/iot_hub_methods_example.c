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

// IoT Hub FQDN
#define IOT_HUB_FQDN "AZ_IOT_HUB_FQDN"

// AZ_IOT_DEVICE_X509_CERT_PEM_FILE is the path to a PEM file containing the device certificate and
// key as well as any intermediate certificates chaining to an uploaded group certificate.
#define DEVICE_X509_CERT_PEM_FILE "AZ_IOT_DEVICE_X509_CERT_PEM_FILE"

// AZ_IOT_DEVICE_X509_TRUST_PEM_FILE is the path to a PEM file containing the server trusted CA
// This is usually not needed on Linux or Mac but needs to be set on Windows.
#define DEVICE_X509_TRUST_PEM_FILE "AZ_IOT_DEVICE_X509_TRUST_PEM_FILE"

static char device_id[64];
static char iot_hub_fqdn[128];
static char x509_cert_pem_file[512];
static char x509_trust_pem_file[256];

static char mqtt_endpoint[128];
static az_span mqtt_url_prefix = AZ_SPAN_LITERAL_FROM_STR("ssl://");
static az_span mqtt_url_suffix = AZ_SPAN_LITERAL_FROM_STR(":8883");

static char method_reponse_payload[64];
static az_span method_response_name = AZ_SPAN_LITERAL_FROM_STR("doubled value");

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

static az_result create_mqtt_endpoint(char* destination, int32_t size, az_span iot_hub)
{
  int32_t iot_hub_length = (int32_t)strlen(iot_hub_fqdn);
  int32_t required_size
      = az_span_size(mqtt_url_prefix) + iot_hub_length + az_span_size(mqtt_url_suffix);
  if (required_size > size)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }
  az_span destination_span = az_span_init((uint8_t*)destination, size);
  az_span remainder = az_span_copy(destination_span, mqtt_url_prefix);
  remainder = az_span_copy(remainder, az_span_slice(iot_hub, 0, iot_hub_length));
  az_span_copy(remainder, mqtt_url_suffix);

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
      read_configuration_entry("Device ID", DEVICE_ID, "", false, device_id_span, &trusted));

  az_span iot_hub_fqdn_span = AZ_SPAN_FROM_BUFFER(iot_hub_fqdn);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      "IoT Hub FQDN", IOT_HUB_FQDN, "", false, iot_hub_fqdn_span, &trusted));

  AZ_RETURN_IF_FAILED(
      create_mqtt_endpoint(mqtt_endpoint, (int32_t)sizeof(mqtt_endpoint), iot_hub_fqdn_span));

  AZ_RETURN_IF_FAILED(az_iot_hub_client_init(
      &client,
      az_span_slice(iot_hub_fqdn_span, 0, (int32_t)strlen(iot_hub_fqdn)),
      az_span_slice(device_id_span, 0, (int32_t)strlen(device_id)),
      NULL));

  return AZ_OK;
}

static az_result parse_value_to_double(az_span payload, int64_t* value)
{
  az_result result;
  az_json_parser json_parser;
  AZ_RETURN_IF_FAILED(az_json_parser_init(&json_parser, payload));

  az_json_token json_token;
  az_json_token_member json_token_member;
  double double_value;
  do
  {
    result = az_json_parser_parse_token(&json_parser, &json_token);

    if (json_token.kind == AZ_JSON_TOKEN_OBJECT_START)
    {
      AZ_RETURN_IF_FAILED(az_json_parser_parse_token_member(&json_parser, &json_token_member));
      if (az_span_is_content_equal(AZ_SPAN_FROM_STR("value"), json_token_member.name))
      {
        AZ_RETURN_IF_FAILED(az_json_token_get_number(&json_token_member.token, &double_value));
        *value = (int64_t)double_value;
        break;
      }
    }
  } while (result != AZ_OK);
  return result;
}

static const az_span double_method_span = AZ_SPAN_LITERAL_FROM_STR("double");
static az_result double_method(int64_t in, int64_t* out)
{
  printf("Invoking double method\n");
  if ((in > INT64_MAX / 2) || in < INT64_MIN / 2)
  {
    return AZ_ERROR_ARG;
  }
  *out = in * 2;
  return AZ_OK;
}

static az_result send_method_response(
    az_iot_hub_client_method_request* request,
    uint16_t status,
    int64_t return_value)
{
  az_result result;

  // Get the response topic
  char methods_response_topic[128];
  if ((result = az_iot_hub_client_methods_response_get_publish_topic(
           &client,
           request->request_id,
           status,
           methods_response_topic,
           sizeof(methods_response_topic),
           NULL))
      != AZ_OK)
  {
    printf("Unable to get twin document publish topic, return code %d\n", result);
    return result;
  }

  // Build the response payload
  az_span response_span;
  if (status == 200)
  {
    az_json_builder json_builder;
    result = az_json_builder_init(&json_builder, AZ_SPAN_FROM_BUFFER(method_reponse_payload));
    result = az_json_builder_append_token(&json_builder, az_json_token_object_start());
    az_json_token reported_property_value_token = az_json_token_number((double)return_value);
    result = az_json_builder_append_object(
        &json_builder, method_response_name, reported_property_value_token);
    result = az_json_builder_append_token(&json_builder, az_json_token_object_end());

    response_span = az_json_builder_span_get(&json_builder);
  }
  else
  {
    response_span = AZ_SPAN_FROM_STR("{}");
  }

  printf("Status: %d\tPayload:", status);
  char* payload_char = (char*)az_span_ptr(response_span);
  for (int32_t i = 0; i < az_span_size(response_span); i++)
  {
    putchar(*(payload_char + i));
  }
  putchar('\n');

  // Send the response
  if ((result = MQTTClient_publish(
           mqtt_client,
           methods_response_topic,
           az_span_size(response_span),
           az_span_ptr(response_span),
           0,
           0,
           NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to publish twin document request, return code %d\n", result);
    return result;
  }

  printf("Sent response\n");

  return result;
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
    az_result result;

    printf("Direct Method arrived\n");
    if (az_span_is_content_equal(double_method_span, method_request.name))
    {
      // Get payload value
      az_span payload_span = az_span_init((uint8_t*)message->payload, message->payloadlen);
      int64_t value_to_double;
      if ((result = parse_value_to_double(payload_span, &value_to_double)) != AZ_OK)
      {
        printf("Unable to parse input value, status %d", result);
        return result;
      }

      // Invoke Method
      int64_t return_value;
      if ((result = double_method(value_to_double, &return_value)) == AZ_ERROR_ARG)
      {
        printf("Unable to invoke double_method, status %d", result);
        return result;
      }

      // Build a response
      if ((result = send_method_response(&method_request, 200, return_value)) != AZ_OK)
      {
        printf("Unable to send %d response, status %d", 200, result);
        return result;
      }
    }
    else
    {
      // Unsupported Method
      if ((result = send_method_response(&method_request, 404, 0xFFFF)) != AZ_OK)
      {
        printf("Unable to send %d response, status %d", 404, result);
        return result;
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

  char username[128];
  size_t username_length;
  if ((rc = az_iot_hub_client_get_user_name(&client, username, sizeof(username), &username_length))
      != AZ_OK)

  {
    printf("Failed to get MQTT clientId, return code %d\n", rc);
    return rc;
  }

  mqtt_connect_options.username = username;
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

  char c2d_topic[128];
  size_t c2d_topic_length;
  if ((rc = az_iot_hub_client_methods_get_subscribe_topic_filter(
           &client, c2d_topic, sizeof(c2d_topic), &c2d_topic_length))
      != AZ_OK)

  {
    printf("Failed to get C2D MQTT SUB topic filter, return code %d\n", rc);
    return rc;
  }

  if ((rc = MQTTClient_subscribe(mqtt_client, c2d_topic, 1)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to subscribe, return code %d\n", rc);
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

  char client_id[128];
  size_t client_id_length;
  if ((rc
       = az_iot_hub_client_get_client_id(&client, client_id, sizeof(client_id), &client_id_length))
      != AZ_OK)
  {
    printf("Failed to get MQTT clientId, return code %d\n", rc);
    return rc;
  }

  if ((rc = MQTTClient_create(
           &mqtt_client, mqtt_endpoint, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL))
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

  if ((rc = MQTTClient_disconnect(mqtt_client, 10000)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to disconnect MQTT client, return code %d\n", rc);
    return rc;
  }

  printf("Disconnected.\n");
  MQTTClient_destroy(&mqtt_client);

  return 0;
}

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
static int on_received(void* context, char* topicName, int topicLen, MQTTClient_message* message);
static int send_get_twin();
static int report_property();
static az_result build_reported_property(az_span* reported_property_payload);
static int send_reported_property(az_span reported_property_payload);
static az_result update_property(az_span desired_payload);
static void increment_property_count();

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

  // Subscribe to the necessary twin topics to receive twin updates and responses
  if ((rc = subscribe()) != MQTTCLIENT_SUCCESS)
  {
    return rc;
  }

  printf("\nSubscribed to topics.\n");
  printf("\nWaiting for activity:\n"
         "Press 'g' for device to request twin document from service.\n"
         "Press 'r' for device to send \"device_count\" reported property to service.\n"
         "\t\t\t\"device_count\" will then locally increment.\n"
         "[Press 'q' to quit]\n\n");

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
          report_property();
          increment_property_count();
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

  // Parse the incoming message topic and check to make sure it is a twin message
  az_iot_hub_client_twin_response twin_response;
  if (az_failed(
          rc = az_iot_hub_client_twin_parse_received_topic(&client, topic_span, &twin_response)))
  {
    printf("Topic is not a twin message, az_result return code %04x\n", rc);
  }
  else
  {
    // Determine type of incoming twin message. Print relevant data for the message.
    switch (twin_response.response_type)
    {
      // Type: A service response to a twin GET publish message.
      // Payload: The JSON twin document.
      case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET:
        printf("A twin GET response was received.\n");
        if (message->payloadlen)
        {
          printf("Received payload:\n%.*s\n", message->payloadlen, (char*)message->payload);
        }
        printf("Response status is %d.\n", twin_response.status);
        break;

      // Type: A service response to a twin reported properties publish message.
      // Payload: Empty if reported properties were updated successfully on service.
      case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
        printf("A twin reported properties service response was received.\n");
        if (message->payloadlen)
        {
          printf("Error. Received payload:\n%.*s\n", message->payloadlen, (char*)message->payload);
        }
        else
        {
          // Status will be 204 upon success.
          printf("Success. No payload.\n");
        }
        printf("Response status is %d.\n", twin_response.status);
        break;

      // Type: A service update to twin desired properties.
      // Payload: The desired properties JSON.
      case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
        printf("A twin desired properties message was received.\n");
        if (message->payloadlen)
        {
          printf("Received payload:\n%.*s\n", message->payloadlen, (char*)message->payload);
        }
        printf("Response status is %d.\n", twin_response.status);

        // Device will update property locally and report that property to service.
        az_span payload_span = az_span_init((uint8_t*)message->payload, message->payloadlen);
        if (az_failed(rc = update_property(payload_span)))
        {
          printf("Failed to update property locally, az_result return code %04x\n", rc);
          break;
        }
        if ((rc = report_property()) != MQTTCLIENT_SUCCESS)
        {
          break;
        }
        increment_property_count();
    }
  }

  putchar('\n');
  MQTTClient_freeMessage(&message);
  MQTTClient_free(topicName);

  return 1;
}

static int send_get_twin()
{
  int rc;
  printf("Device requesting twin document from service.\n");

  // Get the topic to send a twin GET publish message to service.
  if (az_failed(
          rc = az_iot_hub_client_twin_document_get_publish_topic(
              &client, get_twin_topic_request_id, get_twin_topic, sizeof(get_twin_topic), NULL)))
  {
    printf("Unable to get twin document publish topic, az_result return code %04x\n", rc);
    return rc;
  }

  // Publish the twin document request. This will trigger the service to send back the twin document
  // for this device. The response is handled in the on_received function.
  if ((rc = MQTTClient_publish(mqtt_client, get_twin_topic, 0, NULL, 0, 0, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to publish twin document request, MQTTClient return code %d\n", rc);
    return rc;
  }
  return MQTTCLIENT_SUCCESS;
}

static int report_property()
{
  int rc;
  az_span reported_property_payload;
  printf("Device building reported property payload and sending to service.\n");

  if (az_failed(rc = build_reported_property(&reported_property_payload)))
  {
    printf("Unable to build reported property payload to send, az_result return code %04x\n", rc);
    return rc;
  }
  if ((rc = send_reported_property(reported_property_payload)) != MQTTCLIENT_SUCCESS)
  {
    return rc;
  }

  return MQTTCLIENT_SUCCESS;
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

  *reported_property_payload = az_json_writer_get_json(&json_writer);

  return AZ_OK;
}

static int send_reported_property(az_span reported_property_payload)
{
  int rc;
  printf("Device sending reported property to service.\n");
  printf(
      "Sending payload:\n%.*s\n",
      az_span_size(reported_property_payload),
      az_span_ptr(reported_property_payload));

  // Get the topic used to send a reported property update
  if (az_failed(
          rc = az_iot_hub_client_twin_patch_get_publish_topic(
              &client,
              reported_property_topic_request_id,
              reported_property_topic,
              sizeof(reported_property_topic),
              NULL)))
  {
    printf("Unable to get reported property publish topic, az_result return code %04x\n", rc);
    return rc;
  }

  // Publish the reported property payload to IoT Hub. This will trigger the service to send back a
  // response to this device. The response is handled in the on_received function.
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
    printf("Failed to publish reported property, MQTTClient return code %d\n", rc);
    return rc;
  }

  return MQTTCLIENT_SUCCESS;
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

      printf(
          "Updating \"%.*s\" locally.\n",
          az_span_size(reported_property_name),
          az_span_ptr(reported_property_name));

      return AZ_OK;
    }
    else
    {
      // ignore other tokens
      AZ_RETURN_IF_FAILED(az_json_reader_skip_children(&json_reader));
    }

    AZ_RETURN_IF_FAILED(az_json_reader_next_token(&json_reader));
  }

  printf(
      "Did not find \"%.*s\" in desired property payload.\n",
      az_span_size(reported_property_name),
      az_span_ptr(reported_property_name));
  return AZ_ERROR_ITEM_NOT_FOUND;
}

static void increment_property_count()
{
  printf(
      "Incrementing \"%.*s\" locally.\n",
      az_span_size(reported_property_name),
      az_span_ptr(reported_property_name));
  reported_property_value++;
  return;
}

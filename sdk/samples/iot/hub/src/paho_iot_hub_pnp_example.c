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
#include <time.h>
#ifdef _WIN32
// Required for Sleep(DWORD)
#include <windows.h>
#else
// Required for sleep(unsigned int)
#include <unistd.h>
#endif

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

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

#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)
#define TELEMETRY_SEND_INTERVAL 1
#define NUMBER_OF_MESSAGES 5
#define DEAFULT_START_TEMP_CELSIUS 22

static const uint8_t null_terminator = '\0';
static char boot_time_str[32];
static az_span boot_time_span;
static const char iso_spec_time_format[] = "%Y-%m-%dT%H:%M:%S%z";

// IoT Hub Connection Values
static az_iot_hub_client client;
static char device_id[64];
static char iot_hub_hostname[128];
static char x509_cert_pem_file[512];
static char x509_trust_pem_file[256];
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:com:example:Thermostat;1");

// MQTT Client Values
static MQTTClient mqtt_client;
static char mqtt_client_id[128];
static char mqtt_username[256];
static char mqtt_endpoint[128];
static az_span mqtt_url_prefix = AZ_SPAN_LITERAL_FROM_STR("ssl://");
static az_span mqtt_url_suffix = AZ_SPAN_LITERAL_FROM_STR(":8883");

// IoT Hub Telemetry Values
char telemetry_topic[128];
static const az_span telemetry_name = AZ_SPAN_LITERAL_FROM_STR("temperature");
static char telemetry_payload[32];

// IoT Hub Commands Values
static char commands_response_topic[128];
static const az_span report_command_payload_value_span = AZ_SPAN_LITERAL_FROM_STR("since");
static const az_span report_command_name_span = AZ_SPAN_LITERAL_FROM_STR("getMaxMinReport");
static const az_span report_max_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("maxTemp");
static const az_span report_min_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("minTemp");
static const az_span report_avg_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("avgTemp");
static const az_span report_start_time_name_span = AZ_SPAN_LITERAL_FROM_STR("startTime");
static const az_span report_end_time_name_span = AZ_SPAN_LITERAL_FROM_STR("endTime");
static char end_time_buffer[32];
static char commands_response_payload[256];

// IoT Hub Twin Values
static char reported_property_topic[128];
static const az_span desired_temp_property_name = AZ_SPAN_LITERAL_FROM_STR("targetTemperature");
static const az_span max_temp = AZ_SPAN_LITERAL_FROM_STR("maxTempSinceLastReboot");
static az_span reported_property_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("reported_prop");
static char reported_property_payload[32];

// PnP Device Values
static int32_t current_device_temp = DEAFULT_START_TEMP_CELSIUS;
static int32_t device_temperature_avg_total = DEAFULT_START_TEMP_CELSIUS;
static uint32_t device_temperature_avg_count = 1;
static int32_t device_max_temp = DEAFULT_START_TEMP_CELSIUS;
static int32_t device_min_temp = DEAFULT_START_TEMP_CELSIUS;
static int32_t device_avg_temp = DEAFULT_START_TEMP_CELSIUS;

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
static int send_telemetry_messages();
static int send_command_response(
    az_iot_hub_client_method_request* request,
    uint16_t status,
    az_span response);
static int send_reported_temperature_property(double desired_temp);
static az_result parse_desired_temperature_property(az_span twin_span, double* parsed_value);
static az_result invoke_getMaxMinReport(az_span payload, az_span response, az_span* out_response);

int main()
{
  int rc;

  // Get the boot time for command response
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  size_t len = strftime(boot_time_str, sizeof(boot_time_str), iso_spec_time_format, timeinfo);
  boot_time_span = az_span_init((uint8_t*)boot_time_str, (int32_t)len);

  // Read in the necessary environment variables and initialize the az_iot_hub_client
  if (az_failed(rc = read_configuration_and_init_client()))
  {
    printf("Failed to read configuration from environment variables, return code %d\n", rc);
    return rc;
  }

  // Get the MQTT client id used for the MQTT connection
  size_t client_id_length;
  if (az_failed(
          rc = az_iot_hub_client_get_client_id(
              &client, mqtt_client_id, sizeof(mqtt_client_id), &client_id_length)))
  {
    printf("Failed to get MQTT client id, return code %d\n", rc);
    return rc;
  }

  // Create the Paho MQTT client
  if ((rc = MQTTClient_create(
           &mqtt_client, mqtt_endpoint, mqtt_client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to create MQTT client, return code %d\n", rc);
    return rc;
  }

  // Set the callback for incoming MQTT messages
  if ((rc = MQTTClient_setCallbacks(mqtt_client, NULL, NULL, on_received, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to set MQTT callbacks, return code %d\n", rc);
    return rc;
  }

  // Connect to IoT Hub
  if ((rc = connect_device()) != 0)
  {
    return rc;
  }

  // Subscribe to the necessary twin and commands topics to receive twin updates and responses
  if ((rc = subscribe()) != 0)
  {
    return rc;
  }

  // Loop and send 5 messages
  if ((rc = send_telemetry_messages()) != 0)
  {
    return rc;
  }

  printf("Telemetry Sent | Waiting for messages from the Azure IoT Hub\n[Press ENTER to shut "
         "down]\n\n");
  (void)getchar();

  // Gracefully disconnect: send the disconnect packet and close the socket
  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to disconnect MQTT client, return code %d\n", rc);
    return rc;
  }
  printf("Disconnected.\n");

  // Clean up and release resources allocated by the mqtt client
  MQTTClient_destroy(&mqtt_client);

  return 0;
}

static void sleep_for_seconds(uint32_t seconds)
{
#ifdef _WIN32
  Sleep((DWORD)seconds * 1000);
#else
  sleep(seconds);
#endif
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

  if (env_value != NULL && default_value != NULL)
  {
    env_value = default_value;
  }

  if (env_value != NULL)
  {
    printf("%s", hide_value ? "***" : env_value);
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

// Invoke the command requested from the service. Here, it generates a report for max, min, and avg
// temperatures.
static az_result invoke_getMaxMinReport(az_span payload, az_span response, az_span* out_response)
{
  // Parse the "since" field in the payload
  az_json_parser jp;
  az_json_token_member tm;
  AZ_RETURN_IF_FAILED(az_json_parser_init(&jp, payload));
  AZ_RETURN_IF_FAILED(az_json_parser_parse_token(&jp, &tm.token));
  if (tm.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }

  az_span start_time_span = AZ_SPAN_NULL;
  while (az_succeeded(az_json_parser_parse_token_member(&jp, &tm)))
  {
    if (az_span_is_content_equal(report_command_payload_value_span, tm.name))
    {
      AZ_RETURN_IF_FAILED(az_json_token_get_string(&tm.token, &start_time_span));
      break;
    }

    // else ignore token.
  }

  // Get the current time as a string
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  size_t len = strftime(end_time_buffer, sizeof(end_time_buffer), iso_spec_time_format, timeinfo);
  az_span time_span = az_span_init((uint8_t*)end_time_buffer, (int32_t)len);

  // Build the command response payload
  az_json_builder json_builder;
  AZ_RETURN_IF_FAILED(az_json_builder_init(&json_builder, response, NULL));
  AZ_RETURN_IF_FAILED(az_json_builder_append_begin_object(&json_builder));
  AZ_RETURN_IF_FAILED(
      az_json_builder_append_property_name(&json_builder, report_max_temp_name_span));
  AZ_RETURN_IF_FAILED(az_json_builder_append_int32_number(&json_builder, (int32_t)device_max_temp));
  AZ_RETURN_IF_FAILED(
      az_json_builder_append_property_name(&json_builder, report_min_temp_name_span));
  AZ_RETURN_IF_FAILED(az_json_builder_append_int32_number(&json_builder, (int32_t)device_min_temp));
  AZ_RETURN_IF_FAILED(
      az_json_builder_append_property_name(&json_builder, report_avg_temp_name_span));
  AZ_RETURN_IF_FAILED(az_json_builder_append_int32_number(&json_builder, (int32_t)device_avg_temp));
  AZ_RETURN_IF_FAILED(
      az_json_builder_append_property_name(&json_builder, report_start_time_name_span));

  // If the user specified a time, use that as the start | Otherwise use boot time
  if (az_span_size(start_time_span) > 0)
  {
    AZ_RETURN_IF_FAILED(az_json_builder_append_string(&json_builder, start_time_span));
  }
  else
  {
    AZ_RETURN_IF_FAILED(az_json_builder_append_string(&json_builder, boot_time_span));
  }

  AZ_RETURN_IF_FAILED(
      az_json_builder_append_property_name(&json_builder, report_end_time_name_span));
  AZ_RETURN_IF_FAILED(az_json_builder_append_string(&json_builder, time_span));
  AZ_RETURN_IF_FAILED(az_json_builder_append_end_object(&json_builder));

  *out_response = az_json_builder_get_json(&json_builder);

  return AZ_OK;
}

// Send the response of the command invocation
static int send_command_response(
    az_iot_hub_client_method_request* request,
    uint16_t status,
    az_span response)
{
  // Get the response topic to publish the command response
  if (az_failed(az_iot_hub_client_methods_response_get_publish_topic(
          &client,
          request->request_id,
          status,
          commands_response_topic,
          sizeof(commands_response_topic),
          NULL)))
  {
    printf("Unable to get twin document publish topic");
    return -1;
  }

  printf("Status: %u\tPayload:", status);
  char* payload_char = (char*)az_span_ptr(response);
  if (payload_char != NULL)
  {
    for (int32_t i = 0; i < az_span_size(response); i++)
    {
      putchar(*(payload_char + i));
    }
    putchar('\n');
  }

  // Send the commands response
  int rc;
  if ((rc = MQTTClient_publish(
           mqtt_client,
           commands_response_topic,
           az_span_size(response),
           az_span_ptr(response),
           0,
           0,
           NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to send command response, return code %d\n", rc);
    return rc;
  }

  printf("Sent response\n");

  return 0;
}

// Build the JSON payload for the reported property
static az_result build_reported_property(az_json_builder* json_builder, double property_val)
{
  az_result result;
  result = az_json_builder_init(json_builder, AZ_SPAN_FROM_BUFFER(reported_property_payload), NULL);
  result = az_json_builder_append_begin_object(json_builder);
  result = az_json_builder_append_property_name(json_builder, max_temp);
  result = az_json_builder_append_int32_number(json_builder, (int32_t)property_val);
  result = az_json_builder_append_end_object(json_builder);

  return result;
}

// Send the twin reported property to the service
static int send_reported_temperature_property(double desired_temp)
{
  int rc;
  printf("Sending reported property\n");

  // Get the topic used to send a reported property update
  if (az_failed(
          rc = az_iot_hub_client_twin_patch_get_publish_topic(
              &client,
              reported_property_topic_request_id,
              reported_property_topic,
              sizeof(reported_property_topic),
              NULL)))
  {
    printf("Unable to get twin document publish topic, return code %d\n", rc);
    return rc;
  }

  // Twin reported properties must be in JSON format. The payload is constructed here.
  az_json_builder json_builder;
  if (az_failed(rc = build_reported_property(&json_builder, desired_temp)))
  {
    return rc;
  }
  az_span json_payload = az_json_builder_get_json(&json_builder);

  printf("Payload: %.*s\n", az_span_size(json_payload), (char*)az_span_ptr(json_payload));

  // Publish the reported property payload to IoT Hub
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

// Parse the desired temperature property from the incoming JSON
static az_result parse_desired_temperature_property(az_span twin_span, double* parsed_value)
{
  az_json_parser jp;
  az_json_token_member tm;
  AZ_RETURN_IF_FAILED(az_json_parser_init(&jp, twin_span));
  AZ_RETURN_IF_FAILED(az_json_parser_parse_token(&jp, &tm.token));
  if (tm.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }

  while (az_succeeded(az_json_parser_parse_token_member(&jp, &tm)))
  {
    if (az_span_is_content_equal(desired_temp_property_name, tm.name))
    {
      AZ_RETURN_IF_FAILED(az_json_token_get_number(&tm.token, parsed_value));
    }

    // else ignore token.
  }

  return AZ_OK;
}

// Callback for incoming MQTT messages
static int on_received(void* context, char* topicName, int topicLen, MQTTClient_message* message)
{
  (void)context;

  if (topicLen == 0)
  {
    // The length of the topic if there are one or more NULL characters embedded in topicName,
    // otherwise topicLen is 0.
    topicLen = (int)strlen(topicName);
  }

  printf("Topic: %s\n", topicName);

  az_span topic_span = az_span_init((uint8_t*)topicName, topicLen);

  // Parse the incoming message topic and check which feature it is for
  az_iot_hub_client_twin_response twin_response;
  az_iot_hub_client_method_request command_request;

  if (az_succeeded(
          az_iot_hub_client_twin_parse_received_topic(&client, topic_span, &twin_response)))
  {
    printf("Twin Message Arrived\n");

    // Determine what type of incoming twin message this is. Print relevant data for the message.
    switch (twin_response.response_type)
    {
      // A response from a twin GET publish message with the twin document as a payload.
      case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET:
        printf("A twin GET response was received\n");
        if (message->payloadlen)
        {
          printf("Payload:\n%.*s\n", message->payloadlen, (char*)message->payload);
        }
        break;
      // An update to the desired properties with the properties as a JSON payload.
      case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
        printf("A twin desired properties message was received\n");
        printf("Payload:\n%.*s\n", message->payloadlen, (char*)message->payload);
        az_span twin_span = az_span_init((uint8_t*)message->payload, (int32_t)message->payloadlen);

        // Get the new temperature
        double desired_temp;
        if (az_failed(parse_desired_temperature_property(twin_span, &desired_temp)))
        {
          printf("Could not parse desired temperature property\n");
          break;
        }
        current_device_temp = (int32_t)desired_temp;

        // Set the max/min temps if apply
        if (current_device_temp > device_max_temp)
        {
          device_max_temp = current_device_temp;
        }
        if (current_device_temp < device_min_temp)
        {
          device_min_temp = current_device_temp;
        }

        // Increment the avg count, add the new temp to the total, and calculate the new avg
        device_temperature_avg_count++;
        device_temperature_avg_total += current_device_temp;
        device_avg_temp = device_temperature_avg_total / (int32_t)device_temperature_avg_count;

        // Send back the max temp
        send_reported_temperature_property(device_max_temp);

        break;
      // A response from a twin reported properties publish message. With a successfull update of
      // the reported properties, the payload will be empty and the status will be 204.
      case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
        printf("A twin reported properties response message was received\n");
        break;
    }
    printf("Response status was %d\n", twin_response.status);
  }
  if (az_succeeded(az_iot_hub_client_methods_parse_received_topic(
          &client, az_span_init((uint8_t*)topicName, topicLen), &command_request)))
  {
    printf("command arrived\n");
    if (az_span_is_content_equal(report_command_name_span, command_request.name))
    {
      az_span command_response_span = AZ_SPAN_FROM_BUFFER(commands_response_payload);
      az_span command_payload_span
          = az_span_init((uint8_t*)message->payload, (int32_t)message->payloadlen);

      // Invoke command
      az_result response = invoke_getMaxMinReport(
          command_payload_span, command_response_span, &command_response_span);
      (void)response;

      // Send command response with report as JSON payload
      int rc;
      if ((rc = send_command_response(&command_request, 200, command_response_span)) != 0)
      {
        printf("Unable to send %d response, status %d\n", 200, rc);
      }
    }
    else
    {
      // Unsupported command
      printf(
          "Unsupported command received: %.*s.\n",
          az_span_size(command_request.name),
          az_span_ptr(command_request.name));

      int rc;
      if ((rc = send_command_response(&command_request, 404, AZ_SPAN_NULL)) != 0)
      {
        printf("Unable to send %d response, status %d\n", 404, rc);
      }
    }
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

  // NOTE: We recommend setting clean session to false in order to receive any pending messages
  mqtt_connect_options.cleansession = false;
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  // Get the MQTT username used to connect to IoT Hub
  if (az_failed(
          rc = az_iot_hub_client_get_user_name_with_model_id(
              &client, model_id, mqtt_username, sizeof(mqtt_username), NULL)))

  {
    printf("Failed to get MQTT username, return code %d\n", rc);
    return rc;
  }

  printf("%s\r\n", mqtt_username);

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
    printf("Failed to connect, return code %d\n", rc);
    return rc;
  }

  return 0;
}

static int subscribe()
{
  int rc;

  // Subscribe to the commands topic. Messages received on this topic are commands to be invoked
  // on the device.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to subscribe to the commands subscribe topic filter, return code %d\n", rc);
    return rc;
  }

  // Subscribe to the desired properties PATCH topic. Messages received on this topic will be
  // updates to the desired properties.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to subscribe to the twin patch topic filter, return code %d\n", rc);
    return rc;
  }

  // Subscribe to the twin response topic. Messages received on this topic will be response statuses
  // from published reported properties or the requested twin document from twin GET publish
  // messages
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to subscribe to twin response topic filter, return code %d\n", rc);
    return rc;
  }

  return 0;
}

// Send JSON formatted telemetry messages
static int send_telemetry_messages()
{
  int rc;

  if (az_failed(
          rc = az_iot_hub_client_telemetry_get_publish_topic(
              &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    return rc;
  }

  az_json_builder json_builder;
  az_result result;
  result = az_json_builder_init(&json_builder, AZ_SPAN_FROM_BUFFER(telemetry_payload), NULL);
  result = az_json_builder_append_begin_object(&json_builder);
  result = az_json_builder_append_property_name(&json_builder, telemetry_name);
  result = az_json_builder_append_int32_number(&json_builder, (int32_t)current_device_temp);
  result = az_json_builder_append_end_object(&json_builder);
  az_span json_payload = az_json_builder_get_json(&json_builder);
  (void)result;

  // New line to separate messages on the console
  putchar('\n');
  for (int i = 0; i < NUMBER_OF_MESSAGES; ++i)
  {
    printf("Sending Message %d\n", i + 1);
    if ((rc = MQTTClient_publish(
             mqtt_client,
             telemetry_topic,
             (int)az_span_size(json_payload),
             az_span_ptr(json_payload),
             0,
             0,
             NULL))
        != MQTTCLIENT_SUCCESS)
    {
      printf("Failed to publish telemetry message %d, return code %d\n", i + 1, rc);
      return rc;
    }
    sleep_for_seconds(TELEMETRY_SEND_INTERVAL);
  }
  return rc;
}


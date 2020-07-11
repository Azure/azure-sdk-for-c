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

#define TIMEOUT_WAIT_FOR_RECEIVE_MESSAGE_MS (8 * 1000)
#define TIMEOUT_WAIT_FOR_COMPLETION_MS 1000
#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)
#define DEVICE_DO_WORK_SLEEP_MS 2
#define TELEMETRY_SEND_INTERVAL 1
#define DEFAULT_START_TEMP_CELSIUS 22.0
#define DOUBLE_DECIMAL_PLACE_DIGITS 2

bool device_operational = true;
static const uint8_t null_terminator = '\0';
static char boot_time_str[32];
static az_span boot_time_span;

// * PnP Values *
// The model id is the JSON document (also called the Digital Twins Model Identifier or DTMI)
// which defines the capability of your device. The functionality of the device should match what
// is described in the coresponding DTMI. Should you choose to program your own PnP capable device,
// the functionality would need to match the DTMI and you would need to update the below 'model_id'.
// Please see the sample README for more information on this DTMI.
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:com:example:Thermostat;1");
// ISO8601 Time Format
static const char iso_spec_time_format[] = "%Y-%m-%dT%H:%M:%S%z";

// IoT Hub Connection Values
static az_iot_hub_client client;
static char device_id[64];
static char iot_hub_hostname[128];
static char x509_cert_pem_file[512];
static char x509_trust_pem_file[256];
static int32_t request_id_int;
static char request_id_buf[8];

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
static const az_span report_command_name_span = AZ_SPAN_LITERAL_FROM_STR("getMaxMinReport");
static const az_span report_max_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("maxTemp");
static const az_span report_min_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("minTemp");
static const az_span report_avg_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("avgTemp");
static const az_span report_start_time_name_span = AZ_SPAN_LITERAL_FROM_STR("startTime");
static const az_span report_end_time_name_span = AZ_SPAN_LITERAL_FROM_STR("endTime");
static const az_span report_error_payload = AZ_SPAN_LITERAL_FROM_STR("{}");
static char end_time_buffer[32];
static char commands_response_payload[256];
static char incoming_since_value[32];

// IoT Hub Twin Values
static char twin_get_topic[128];
static char reported_property_topic[128];
static const az_span desired_property_name = AZ_SPAN_LITERAL_FROM_STR("desired");
static const az_span desired_property_version_name = AZ_SPAN_LITERAL_FROM_STR("$version");
static const az_span desired_temp_property_name = AZ_SPAN_LITERAL_FROM_STR("targetTemperature");
static const az_span desired_temp_response_value_name = AZ_SPAN_LITERAL_FROM_STR("value");
static const az_span desired_temp_ack_code_name = AZ_SPAN_LITERAL_FROM_STR("ac");
static const az_span desired_temp_ack_version_name = AZ_SPAN_LITERAL_FROM_STR("av");
static const az_span desired_temp_ack_description_name = AZ_SPAN_LITERAL_FROM_STR("ad");
static const az_span max_temp_reported_property_name
    = AZ_SPAN_LITERAL_FROM_STR("maxTempSinceLastReboot");
static char reported_property_payload[128];

// PnP Device Values
static double current_device_temp = DEFAULT_START_TEMP_CELSIUS;
static double device_temperature_avg_total = DEFAULT_START_TEMP_CELSIUS;
static uint32_t device_temperature_avg_count = 1;
static double device_max_temp = DEFAULT_START_TEMP_CELSIUS;
static double device_min_temp = DEFAULT_START_TEMP_CELSIUS;
static double device_avg_temp = DEFAULT_START_TEMP_CELSIUS;

//
// Configuration and connection functions
//
static az_result read_configuration_and_init_client(void);
static az_result read_configuration_entry(
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span buffer,
    az_span* out_value);
static az_result create_mqtt_endpoint(char* destination, int32_t destination_size, az_span iot_hub);
static int connect_device(void);
static int subscribe(void);
static void sleep_for_seconds(uint32_t seconds);

//
// Messaging functions
//
static int mqtt_publish_message(char* topic, az_span payload, int qos);
static int on_received(char* topicName, int topicLen, MQTTClient_message* message);
static int send_telemetry_message(void);
static int send_twin_get_message(void);
static int send_command_response(
    az_iot_hub_client_method_request* request,
    uint16_t status,
    az_span response);
static int send_reported_temperature_property(
    double temp_value,
    int32_t version,
    bool is_max_reported_prop);
static void handle_twin_message(
    MQTTClient_message* message,
    az_iot_hub_client_twin_response* twin_response);
static void handle_command_message(
    MQTTClient_message* message,
    az_iot_hub_client_method_request* command_request);
static az_result parse_twin_desired_temperature_property(
    az_span twin_payload_span,
    bool is_twin_get,
    double* parsed_value,
    int32_t* version_number);
static az_result invoke_getMaxMinReport(az_span payload, az_span response, az_span* out_response);
static az_span get_request_id(void);

int main(void)
{
  int rc;

  // Get the program start time for command response
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  size_t len = strftime(boot_time_str, sizeof(boot_time_str), iso_spec_time_format, timeinfo);
  if (len == 0)
  {
    printf("Insufficient buffer size for program start time.\n");
    return -1;
  }
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

  char* incoming_message_topic;
  int incoming_message_topic_len;
  MQTTClient_message* message;

  // First get the twin document to check for updated desired properties. Will then parse desired
  // property and update accordingly.
  send_twin_get_message();

  while (device_operational)
  {
    // Receive any incoming messages from twin or commands
    if (MQTTClient_receive(
            mqtt_client,
            &incoming_message_topic,
            &incoming_message_topic_len,
            &message,
            TIMEOUT_WAIT_FOR_RECEIVE_MESSAGE_MS)
            == MQTTCLIENT_SUCCESS
        && incoming_message_topic != NULL)
    {
      on_received(incoming_message_topic, incoming_message_topic_len, message);

      MQTTClient_freeMessage(&message);
      MQTTClient_free(incoming_message_topic);
    }

    // Send a telemetry message
    send_telemetry_message();

    sleep_for_seconds(DEVICE_DO_WORK_SLEEP_MS);
  }

  // Gracefully disconnect: send the disconnect packet and close the socket
  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to disconnect MQTT client, return code %d\n", rc);
    return rc;
  }
  else
  {
    printf("Disconnected.\n");
  }

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

static int mqtt_publish_message(char* topic, az_span payload, int qos)
{
  int rc;
  MQTTClient_deliveryToken token;
  if ((rc = MQTTClient_publish(
           mqtt_client, topic, az_span_size(payload), az_span_ptr(payload), qos, 0, &token))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Unable to publish message, return code %d\n", rc);
    return rc;
  }
  if (qos > 0)
  {
    if ((rc = MQTTClient_waitForCompletion(mqtt_client, token, TIMEOUT_WAIT_FOR_COMPLETION_MS))
        != MQTTCLIENT_SUCCESS)
    {
      printf("Wait for message completion timed out, return code %d\n", rc);
      return rc;
    }
  }
  return 0;
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
static az_result read_configuration_and_init_client(void)
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
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.model_id = model_id;
  AZ_RETURN_IF_FAILED(az_iot_hub_client_init(
      &client,
      az_span_slice(iot_hub_hostname_span, 0, (int32_t)strlen(iot_hub_hostname)),
      az_span_slice(device_id_span, 0, (int32_t)strlen(device_id)),
      &options));

  return AZ_OK;
}

static az_result build_command_response_payload(
    az_json_writer* json_builder,
    az_span start_time_span,
    az_span end_time_span,
    az_span* response_payload)
{
  // Build the command response payload
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_builder));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_max_temp_name_span));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_double(json_builder, device_max_temp, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_min_temp_name_span));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_double(json_builder, device_min_temp, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_avg_temp_name_span));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_double(json_builder, device_avg_temp, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_builder, report_start_time_name_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(json_builder, start_time_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_end_time_name_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(json_builder, end_time_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(json_builder));

  *response_payload = az_json_writer_get_json(json_builder);

  return AZ_OK;
}

// Invoke the command requested from the service. Here, it generates a report for max, min, and avg
// temperatures.
static az_result invoke_getMaxMinReport(az_span payload, az_span response, az_span* out_response)
{
  // Parse the "since" field in the payload.
  az_span start_time_span = AZ_SPAN_NULL;
  az_json_reader jp;
  AZ_RETURN_IF_FAILED(az_json_reader_init(&jp, payload, NULL));
  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jp));
  int32_t incoming_since_value_len;
  AZ_RETURN_IF_FAILED(az_json_token_get_string(
      &jp.token, incoming_since_value, sizeof(incoming_since_value), &incoming_since_value_len));
  start_time_span = az_span_init((uint8_t*)incoming_since_value, incoming_since_value_len);

  // Set the response payload to error if the "since" field was not sent
  if (az_span_ptr(start_time_span) == NULL)
  {
    response = report_error_payload;
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  // Get the current time as a string
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  size_t len = strftime(end_time_buffer, sizeof(end_time_buffer), iso_spec_time_format, timeinfo);
  az_span end_time_span = az_span_init((uint8_t*)end_time_buffer, (int32_t)len);

  az_json_writer json_builder;
  AZ_RETURN_IF_FAILED(az_json_writer_init(&json_builder, response, NULL));
  AZ_RETURN_IF_FAILED(
      build_command_response_payload(&json_builder, start_time_span, end_time_span, out_response));

  return AZ_OK;
}

// Send the response of the command invocation
static int send_command_response(
    az_iot_hub_client_method_request* request,
    uint16_t status,
    az_span response)
{
  int rc;
  // Get the response topic to publish the command response
  if (az_failed(
          rc = az_iot_hub_client_methods_response_get_publish_topic(
              &client,
              request->request_id,
              status,
              commands_response_topic,
              sizeof(commands_response_topic),
              NULL)))
  {
    printf("Unable to get twin document publish topic\n");
    return rc;
  }

  printf("Status: %u\tPayload:", status);
  char* payload_char = (char*)az_span_ptr(response);
  if (payload_char != NULL)
  {
    for (int32_t i = 0; i < az_span_size(response); i++)
    {
      putchar(*(payload_char + i));
    }
  }
  putchar('\n');

  // Send the commands response
  if ((rc = mqtt_publish_message(commands_response_topic, response, 0)) == 0)
  {
    printf("Sent response\n");
  }

  return rc;
}

// Build the JSON payload for the reported property
static az_result build_confirmed_reported_property(
    az_json_writer* json_builder,
    az_span property_name,
    double property_val,
    int32_t ack_code_value,
    int32_t ack_version_value,
    az_span ack_description_value)
{
  AZ_RETURN_IF_FAILED(
      az_json_writer_init(json_builder, AZ_SPAN_FROM_BUFFER(reported_property_payload), NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_builder));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, property_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_builder));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_builder, desired_temp_response_value_name));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_double(json_builder, property_val, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_builder, desired_temp_ack_code_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(json_builder, ack_code_value));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_builder, desired_temp_ack_version_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(json_builder, ack_version_value));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_builder, desired_temp_ack_description_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(json_builder, ack_description_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(json_builder));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(json_builder));

  return AZ_OK;
}

static az_result build_reported_property(
    az_json_writer* json_builder,
    az_span property_name,
    double property_val)
{
  AZ_RETURN_IF_FAILED(
      az_json_writer_init(json_builder, AZ_SPAN_FROM_BUFFER(reported_property_payload), NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_builder));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, property_name));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_double(json_builder, property_val, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(json_builder));

  return AZ_OK;
}
// Send the twin reported property to the service
static int send_reported_temperature_property(
    double temp_value,
    int32_t version,
    bool is_max_reported_prop)
{
  int rc;
  printf("Sending reported property\n");

  // Get the topic used to send a reported property update
  az_span request_id_span = get_request_id();
  if (az_failed(
          rc = az_iot_hub_client_twin_patch_get_publish_topic(
              &client,
              request_id_span,
              reported_property_topic,
              sizeof(reported_property_topic),
              NULL)))
  {
    printf("Unable to get twin document publish topic, return code %d\n", rc);
    return rc;
  }

  // Twin reported properties must be in JSON format. The payload is constructed here.
  az_json_writer json_builder;
  if (is_max_reported_prop)
  {
    if (az_failed(
            rc
            = build_reported_property(&json_builder, max_temp_reported_property_name, temp_value)))
    {
      return rc;
    }
  }
  else
  {
    if (az_failed(
            rc = build_confirmed_reported_property(
                &json_builder,
                desired_temp_property_name,
                temp_value,
                200,
                version,
                AZ_SPAN_FROM_STR("success"))))
    {
      return rc;
    }
  }
  az_span json_payload = az_json_writer_get_json(&json_builder);

  printf("Payload: %.*s\n", az_span_size(json_payload), (char*)az_span_ptr(json_payload));

  // Publish the reported property payload to IoT Hub
  rc = mqtt_publish_message(reported_property_topic, json_payload, 0);

  return rc;
}

// Parse the desired temperature property from the incoming JSON
static az_result parse_twin_desired_temperature_property(
    az_span twin_payload_span,
    bool is_twin_get,
    double* parsed_value,
    int32_t* version_number)
{
  az_json_reader jp;
  bool desired_found = false;
  AZ_RETURN_IF_FAILED(az_json_reader_init(&jp, twin_payload_span, NULL));
  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jp));
  if (jp.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }

  if (is_twin_get)
  {
    // If is twin get payload, we have to parse one level deeper for "desired" wrapper
    AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jp));
    while (jp.token.kind != AZ_JSON_TOKEN_END_OBJECT)
    {
      if (az_json_token_is_text_equal(&jp.token, desired_property_name))
      {
        desired_found = true;
        AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jp));
        break;
      }
      else
      {
        // else ignore token.
        AZ_RETURN_IF_FAILED(az_json_reader_skip_children(&jp));
      }

      AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jp));
    }
  }
  else
  {
    desired_found = true;
  }

  if (!desired_found)
  {
    printf("Desired property object not found in twin");
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  if (jp.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jp));

  bool temp_found = false;
  bool version_found = false;
  while (!(temp_found && version_found) || (jp.token.kind != AZ_JSON_TOKEN_END_OBJECT))
  {
    if (az_json_token_is_text_equal(&jp.token, desired_temp_property_name))
    {
      AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jp));
      AZ_RETURN_IF_FAILED(az_json_token_get_double(&jp.token, parsed_value));
      temp_found = true;
    }
    else if (az_json_token_is_text_equal(&jp.token, desired_property_version_name))
    {
      AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jp));
      AZ_RETURN_IF_FAILED(az_json_token_get_uint32(&jp.token, (uint32_t*)version_number));
      version_found = true;
    }
    else
    {
      // else ignore token.
      AZ_RETURN_IF_FAILED(az_json_reader_skip_children(&jp));
    }
    AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jp));
  }

  if (temp_found && version_found)
  {
    printf("Desired temperature: %2f\tVersion number: %d\n", *parsed_value, *version_number);
    return AZ_OK;
  }

  return AZ_ERROR_ITEM_NOT_FOUND;
}

static void update_device_temp(double temp, bool* max_temp_changed)
{
  current_device_temp = temp;

  bool ret = false;
  if (current_device_temp > device_max_temp)
  {
    device_max_temp = current_device_temp;
    ret = true;
  }
  if (current_device_temp < device_min_temp)
  {
    device_min_temp = current_device_temp;
  }

  // Increment the avg count, add the new temp to the total, and calculate the new avg
  device_temperature_avg_count++;
  device_temperature_avg_total += current_device_temp;
  device_avg_temp = device_temperature_avg_total / device_temperature_avg_count;

  *max_temp_changed = ret;
}

// Switch on the type of twin message and handle accordingly | On desired prop, respond with max
// temp reported prop.
static void handle_twin_message(
    MQTTClient_message* message,
    az_iot_hub_client_twin_response* twin_response)
{
  az_result result;

  if (message->payloadlen)
  {
    printf("Payload:\n%.*s\n", message->payloadlen, (char*)message->payload);
  }

  bool max_temp_changed;
  double desired_temp;
  int32_t version_num;
  az_span twin_payload_span
      = az_span_init((uint8_t*)message->payload, (int32_t)message->payloadlen);
  // Determine what type of incoming twin message this is. Print relevant data for the message.
  switch (twin_response->response_type)
  {
    // A response from a twin GET publish message with the twin document as a payload.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET:
      printf("A twin GET response was received\n");
      if (az_failed(
              result = parse_twin_desired_temperature_property(
                  twin_payload_span, true, &desired_temp, &version_num)))
      {
        // If the item can't be found, the desired temp might not be set so take no action
        break;
      }
      else
      {
        send_reported_temperature_property(desired_temp, version_num, false);
        update_device_temp(desired_temp, &max_temp_changed);

        if (max_temp_changed)
        {
          send_reported_temperature_property(device_max_temp, -1, true);
        }
      }
      break;
    // An update to the desired properties with the properties as a JSON payload.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
      printf("A twin desired properties message was received\n");

      // Get the new temperature
      if (az_failed(
              result = parse_twin_desired_temperature_property(
                  twin_payload_span, false, &desired_temp, &version_num)))
      {
        printf("Could not parse desired temperature property, az_result %04x\n", result);
        break;
      }
      send_reported_temperature_property(desired_temp, version_num, false);
      update_device_temp(desired_temp, &max_temp_changed);

      if (max_temp_changed)
      {
        send_reported_temperature_property(device_max_temp, -1, true);
      }
      break;

    // A response from a twin reported properties publish message. With a successfull update of
    // the reported properties, the payload will be empty and the status will be 204.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
      printf("A twin reported properties response message was received\n");
      break;
  }
}

// Invoke the requested command if supported and return status | Return error otherwise
static void handle_command_message(
    MQTTClient_message* message,
    az_iot_hub_client_method_request* command_request)
{

  if (az_span_is_content_equal(report_command_name_span, command_request->name))
  {
    az_span command_response_span = AZ_SPAN_FROM_BUFFER(commands_response_payload);
    az_span command_response_payload_span
        = az_span_init((uint8_t*)message->payload, (int32_t)message->payloadlen);

    // Invoke command
    uint16_t return_code;
    az_result response = invoke_getMaxMinReport(
        command_response_payload_span, command_response_span, &command_response_span);
    if (response != AZ_OK)
    {
      return_code = 400;
    }
    else
    {
      return_code = 200;
    }

    // Send command response with report as JSON payload
    int rc;
    if ((rc = send_command_response(command_request, return_code, command_response_span)) != 0)
    {
      printf("Unable to send %u response, status %d\n", return_code, rc);
    }
  }
  else
  {
    // Unsupported command
    printf(
        "Unsupported command received: %.*s.\n",
        az_span_size(command_request->name),
        az_span_ptr(command_request->name));

    int rc;
    if ((rc = send_command_response(command_request, 404, report_error_payload)) != 0)
    {
      printf("Unable to send %d response, status %d\n", 404, rc);
    }
  }
}

// Callback for incoming MQTT messages
static int on_received(char* topicName, int topicLen, MQTTClient_message* message)
{
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
    printf("Twin Message Arrived: status %d\n", twin_response.status);

    // Determine what kind of twin message it is and take appropriate actions
    handle_twin_message(message, &twin_response);
  }
  else if (az_succeeded(az_iot_hub_client_methods_parse_received_topic(
               &client, az_span_init((uint8_t*)topicName, topicLen), &command_request)))
  {
    printf("Command arrived\n");

    // Determine if the command is supported and take appropriate actions
    handle_command_message(message, &command_request);
  }

  putchar('\n');

  return 1;
}

static int connect_device(void)
{
  int rc;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;

  // NOTE: We recommend setting clean session to false in order to receive any pending messages
  mqtt_connect_options.cleansession = false;
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  // Get the MQTT username used to connect to IoT Hub
  if (az_failed(
          rc = az_iot_hub_client_get_user_name(
              &client, mqtt_username, sizeof(mqtt_username), NULL)))

  {
    printf("Failed to get MQTT username, return code %d\n", rc);
    return rc;
  }

  printf("MQTT username: %s\r\n", mqtt_username);

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

static int subscribe(void)
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
  // messages.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to subscribe to twin response topic filter, return code %d\n", rc);
    return rc;
  }

  return 0;
}

static int send_twin_get_message(void)
{
  int rc;

  az_span request_id_span = get_request_id();
  if (az_failed(
          rc = az_iot_hub_client_twin_document_get_publish_topic(
              &client, request_id_span, twin_get_topic, sizeof(twin_get_topic), NULL)))
  {
    printf("Could not get twin get publish topic, az_result %d\n", rc);
    return rc;
  }

  printf("Sending twin get request\n");
  rc = mqtt_publish_message(twin_get_topic, AZ_SPAN_NULL, 0);

  return rc;
}

static az_result build_telemetry_message(az_span* out_payload)
{
  az_json_writer json_builder;
  AZ_RETURN_IF_FAILED(
      az_json_writer_init(&json_builder, AZ_SPAN_FROM_BUFFER(telemetry_payload), NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&json_builder));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&json_builder, telemetry_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_double(
      &json_builder, current_device_temp, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&json_builder));
  *out_payload = az_json_writer_get_json(&json_builder);

  return AZ_OK;
}

// Send JSON formatted telemetry messages
static int send_telemetry_message(void)
{
  int rc;

  if (az_failed(
          rc = az_iot_hub_client_telemetry_get_publish_topic(
              &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    return rc;
  }

  az_span telemetry_payload_span;
  if (az_failed(rc = build_telemetry_message(&telemetry_payload_span)))
  {
    printf("Could not build telemetry payload, az_result %d\n", rc);
    return rc;
  }

  printf("Sending Telemetry Message\n");
  rc = mqtt_publish_message(telemetry_topic, telemetry_payload_span, 0);

  // New line to separate messages on the console
  putchar('\n');
  return rc;
}

// Create request id span which increments source int each call. Capable of holding 8 digit number.
static az_span get_request_id(void)
{
  az_span remainder;
  az_span out_span = az_span_init((uint8_t*)request_id_buf, sizeof(request_id_buf));
  az_result result = az_span_i32toa(out_span, request_id_int++, &remainder);
  (void)remainder;
  (void)result;
  return out_span;
}


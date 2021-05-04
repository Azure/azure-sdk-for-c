// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4204: nonstandard extension used: non-constant aggregate initializer
#pragma warning(disable : 4204)
// warning C4996: 'localtime': This function or variable may be unsafe.  Consider using localtime_s
// instead.
#pragma warning(disable : 4996)
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _MSC_VER
#pragma warning(push)
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
#endif
#include <paho-mqtt/MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <azure/az_core.h>
#include <azure/az_iot.h>

#include "iot_sample_common.h"

#define SAMPLE_TYPE PAHO_IOT_HUB
#define SAMPLE_NAME PAHO_IOT_HUB_PNP_SAMPLE

#define MQTT_TIMEOUT_RECEIVE_MAX_MESSAGE_COUNT 3
#define MQTT_TIMEOUT_RECEIVE_MS (8 * 1000)
#define MQTT_TIMEOUT_DISCONNECT_MS (10 * 1000)

#define DEFAULT_START_TEMP_COUNT 1
#define DEFAULT_START_TEMP_CELSIUS 22.0
#define DOUBLE_DECIMAL_PLACE_DIGITS 2

bool is_device_operational = true;
static char const iso_spec_time_format[] = "%Y-%m-%dT%H:%M:%S%z"; // ISO8601 Time Format

// * PnP Values *
// The model id is the JSON document (also called the Digital Twins Model Identifier or DTMI) which
// defines the capability of your device. The functionality of the device should match what is
// described in the corresponding DTMI. Should you choose to program your own PnP capable device,
// the functionality would need to match the DTMI and you would need to update the below 'model_id'.
// Please see the sample README for more information on this DTMI.
static az_span const model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:com:example:Thermostat;1");

// IoT Hub Connection Values
static uint32_t connection_request_id_int = 0;
static char connection_request_id_buffer[16];

// IoT Hub Device Twin Values
static az_span const twin_desired_name = AZ_SPAN_LITERAL_FROM_STR("desired");
static az_span const twin_version_name = AZ_SPAN_LITERAL_FROM_STR("$version");
static az_span const twin_success_name = AZ_SPAN_LITERAL_FROM_STR("success");
static az_span const twin_value_name = AZ_SPAN_LITERAL_FROM_STR("value");
static az_span const twin_ack_code_name = AZ_SPAN_LITERAL_FROM_STR("ac");
static az_span const twin_ack_version_name = AZ_SPAN_LITERAL_FROM_STR("av");
static az_span const twin_ack_description_name = AZ_SPAN_LITERAL_FROM_STR("ad");
static az_span const twin_desired_temperature_property_name
    = AZ_SPAN_LITERAL_FROM_STR("targetTemperature");
static az_span const twin_reported_maximum_temperature_property_name
    = AZ_SPAN_LITERAL_FROM_STR("maxTempSinceLastReboot");

// IoT Hub Method (Command) Values
static az_span const command_getMaxMinReport_name = AZ_SPAN_LITERAL_FROM_STR("getMaxMinReport");
static az_span const command_max_temp_name = AZ_SPAN_LITERAL_FROM_STR("maxTemp");
static az_span const command_min_temp_name = AZ_SPAN_LITERAL_FROM_STR("minTemp");
static az_span const command_avg_temp_name = AZ_SPAN_LITERAL_FROM_STR("avgTemp");
static az_span const command_start_time_name = AZ_SPAN_LITERAL_FROM_STR("startTime");
static az_span const command_end_time_name = AZ_SPAN_LITERAL_FROM_STR("endTime");
static az_span const command_empty_response_payload = AZ_SPAN_LITERAL_FROM_STR("{}");
static char command_start_time_value_buffer[32];
static char command_end_time_value_buffer[32];
static char command_response_payload_buffer[256];

// IoT Hub Telemetry Values
static az_span const telemetry_temperature_name = AZ_SPAN_LITERAL_FROM_STR("temperature");

// PnP Device Values
static double device_current_temperature = DEFAULT_START_TEMP_CELSIUS;
static double device_maximum_temperature = DEFAULT_START_TEMP_CELSIUS;
static double device_minimum_temperature = DEFAULT_START_TEMP_CELSIUS;
static double device_temperature_summation = DEFAULT_START_TEMP_CELSIUS;
static uint32_t device_temperature_count = DEFAULT_START_TEMP_COUNT;
static double device_average_temperature = DEFAULT_START_TEMP_CELSIUS;

static iot_sample_environment_variables env_vars;
static az_iot_hub_client hub_client;
static MQTTClient mqtt_client;
static char mqtt_client_username_buffer[256];

//
// Functions
//
static void create_and_configure_mqtt_client(void);
static void connect_mqtt_client_to_iot_hub(void);
static void subscribe_mqtt_client_to_iot_hub_topics(void);
static void request_device_twin_document(void);
static void receive_messages(void);
static void disconnect_mqtt_client_from_iot_hub(void);

static az_span get_request_id(void);
static void publish_mqtt_message(char const* topic, az_span payload, int qos);
static void on_message_received(char* topic, int topic_len, MQTTClient_message const* message);

// Device Twin functions
static void handle_device_twin_message(
    MQTTClient_message const* message,
    az_iot_hub_client_twin_response const* twin_response);
static void process_device_twin_message(az_span message_span, bool is_twin_get);
static bool parse_desired_temperature_property(
    az_span message_span,
    bool is_twin_get,
    double* out_parsed_temperature,
    int32_t* out_parsed_version_number);
static void update_device_temperature_property(double temperature, bool* out_is_max_temp_changed);
static void send_reported_property(az_span name, double value, int32_t version, bool confirm);

// Command functions
static void handle_command_request(
    MQTTClient_message const* message,
    az_iot_hub_client_method_request const* command_request);
static void send_command_response(
    az_iot_hub_client_method_request const* command_request,
    az_iot_status status,
    az_span response);
static bool invoke_getMaxMinReport(az_span payload, az_span response, az_span* out_response);

// Telemetry functions
static void send_telemetry_message(void);

// JSON build functions
static void build_property_payload(
    uint8_t property_count,
    az_span const names[],
    double const values[],
    az_span const times[],
    az_span property_payload,
    az_span* out_property_payload);
static void build_property_payload_with_status(
    az_span name,
    double value,
    int32_t ack_code_value,
    int32_t ack_version_value,
    az_span ack_description_value,
    az_span property_payload,
    az_span* out_property_payload);

/*
 * This sample connects an IoT Plug and Play enabled device with the Digital Twin Model ID (DTMI).
 * If a timeout occurs while waiting for a message from the Azure IoT Explorer, the sample will
 * continue. If MQTT_TIMEOUT_RECEIVE_MAX_MESSAGE_COUNT timeouts occur consecutively, the sample will
 * disconnect. X509 self-certification is used.
 *
 * To interact with this sample, you must use the Azure IoT Explorer. The capabilities are Device
 * Twin, Direct Method (Command), and Telemetry:
 *
 * Device Twin: Two device twin properties are supported in this sample.
 *   - A desired property named `targetTemperature` with a `double` value for the desired
 * temperature.
 *   - A reported property named `maxTempSinceLastReboot` with a `double` value for the highest
 * temperature reached since device boot.
 *
 * To send a device twin desired property message, select your device's Device Twin tab in the Azure
 * IoT Explorer. Add the property targetTemperature along with a corresponding value to the desired
 * section of the JSON. Select Save to update the twin document and send the twin message to the
 * device.
 *   {
 *     "properties": {
 *       "desired": {
 *         "targetTemperature": 68.5,
 *       }
 *     }
 *   }
 *
 * Upon receiving a desired property message, the sample will update the twin property locally and
 * send a reported property of the same name back to the service. This message will include a set of
 * "ack" values: `ac` for the HTTP-like ack code, `av` for ack version of the property, and an
 * optional `ad` for an ack description.
 *   {
 *     "properties": {
 *       "reported": {
 *         "targetTemperature": {
 *           "value": 68.5,
 *           "ac": 200,
 *           "av": 14,
 *           "ad": "success"
 *         },
 *         "maxTempSinceLastReboot": 74.3,
 *       }
 *     }
 *   }
 *
 * Direct Method (Command): One device command is supported in this sample: `getMaxMinReport`. If
 * any other commands are attempted to be invoked, the log will report the command is not found. To
 * invoke a command, select your device's Direct Method tab in the Azure IoT Explorer. Enter the
 * command name `getMaxMinReport` along with a payload using an ISO8061 time format and select
 * Invoke method.
 *
 *   "2020-08-18T17:09:29-0700"
 *
 * The command will send back to the service a response containing the following JSON payload with
 * updated values in each field:
 *   {
 *     "maxTemp": 74.3,
 *     "minTemp": 65.2,
 *     "avgTemp": 68.79,
 *     "startTime": "2020-08-18T17:09:29-0700",
 *     "endTime": "2020-08-18T17:24:32-0700"
 *   }
 *
 * Telemetry: Device sends a JSON message with the field name `temperature` and the `double` value
 * of the current temperature.
 */
int main(void)
{
  create_and_configure_mqtt_client();
  IOT_SAMPLE_LOG_SUCCESS("Client created and configured.");

  connect_mqtt_client_to_iot_hub();
  IOT_SAMPLE_LOG_SUCCESS("Client connected to IoT Hub.");

  subscribe_mqtt_client_to_iot_hub_topics();
  IOT_SAMPLE_LOG_SUCCESS("Client subscribed to IoT Hub topics.");

  request_device_twin_document();
  receive_messages();

  disconnect_mqtt_client_from_iot_hub();
  IOT_SAMPLE_LOG_SUCCESS("Client disconnected from IoT Hub.");

  return 0;
}

static void create_and_configure_mqtt_client(void)
{
  int rc;

  // Reads in environment variables set by user for purposes of running sample.
  iot_sample_read_environment_variables(SAMPLE_TYPE, SAMPLE_NAME, &env_vars);

  // Build an MQTT endpoint c-string.
  char mqtt_endpoint_buffer[128];
  iot_sample_create_mqtt_endpoint(
      SAMPLE_TYPE, &env_vars, mqtt_endpoint_buffer, sizeof(mqtt_endpoint_buffer));

  // Initialize the hub client with the connection options.
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.model_id = model_id;

  rc = az_iot_hub_client_init(&hub_client, env_vars.hub_hostname, env_vars.hub_device_id, &options);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to initialize hub client: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Get the MQTT client id used for the MQTT connection.
  char mqtt_client_id_buffer[128];
  rc = az_iot_hub_client_get_client_id(
      &hub_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), NULL);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get MQTT client id: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Create the Paho MQTT client.
  rc = MQTTClient_create(
      &mqtt_client, mqtt_endpoint_buffer, mqtt_client_id_buffer, MQTTCLIENT_PERSISTENCE_NONE, NULL);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to create MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

static void connect_mqtt_client_to_iot_hub(void)
{
  int rc;

  // Get the MQTT client username.
  rc = az_iot_hub_client_get_user_name(
      &hub_client, mqtt_client_username_buffer, sizeof(mqtt_client_username_buffer), NULL);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get MQTT client username: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  IOT_SAMPLE_LOG("MQTT client username: %s\n", mqtt_client_username_buffer);

  // Set MQTT connection options.
  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;
  mqtt_connect_options.username = mqtt_client_username_buffer;
  mqtt_connect_options.password = NULL; // This sample uses x509 authentication.
  mqtt_connect_options.cleansession = false; // Set to false so can receive any pending messages.
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  mqtt_ssl_options.verify = 1;
  mqtt_ssl_options.enableServerCertAuth = 1;
  mqtt_ssl_options.keyStore = (char*)az_span_ptr(env_vars.x509_cert_pem_file_path);
  if (az_span_size(env_vars.x509_trust_pem_file_path) != 0) // Is only set if required by OS.
  {
    mqtt_ssl_options.trustStore = (char*)az_span_ptr(env_vars.x509_trust_pem_file_path);
  }
  mqtt_connect_options.ssl = &mqtt_ssl_options;

  // Connect MQTT client to the Azure IoT Hub.
  rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to connect: MQTTClient return code %d.\n"
        "If on Windows, confirm the AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH environment variable is "
        "set correctly.",
        rc);
    exit(rc);
  }
}

static void subscribe_mqtt_client_to_iot_hub_topics(void)
{
  int rc;

  // Messages received on the Methods topic will be commands to be invoked.
  rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC, 1);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to subscribe to the Methods topic: MQTTClient return code %d.", rc);
    exit(rc);
  }

  // Messages received on the Twin Patch topic will be updates to the desired properties.
  rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC, 1);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to subscribe to the Twin Patch topic: MQTTClient return code %d.", rc);
    exit(rc);
  }

  // Messages received on Twin Response topic will be response statuses from the server.
  rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to subscribe to the Twin Response topic: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

static void request_device_twin_document(void)
{
  az_result rc;

  IOT_SAMPLE_LOG("Client requesting device twin document from service.");

  // Get the Twin Document topic to publish the twin document request.
  char twin_document_topic_buffer[128];
  rc = az_iot_hub_client_twin_document_get_publish_topic(
      &hub_client,
      get_request_id(),
      twin_document_topic_buffer,
      sizeof(twin_document_topic_buffer),
      NULL);

  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get the Twin Document topic: az_result return code %04x", rc);
    exit(rc);
  }

  // Publish the twin document request.
  publish_mqtt_message(twin_document_topic_buffer, AZ_SPAN_EMPTY, IOT_SAMPLE_MQTT_PUBLISH_QOS);
}

static void receive_messages(void)
{
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;
  uint8_t timeout_counter = 0;

  // Continue to receive commands or device twin messages while device is operational.
  while (is_device_operational)
  {
    IOT_SAMPLE_LOG(" "); // Formatting
    IOT_SAMPLE_LOG("Waiting for command request or device twin message.\n");

    // MQTTCLIENT_SUCCESS or MQTTCLIENT_TOPICNAME_TRUNCATED if a message is received.
    // MQTTCLIENT_SUCCESS can also indicate that the timeout expired, in which case message is NULL.
    // MQTTCLIENT_TOPICNAME_TRUNCATED if the topic contains embedded NULL characters.
    // An error code is returned if there was a problem trying to receive a message.
    int rc = MQTTClient_receive(mqtt_client, &topic, &topic_len, &message, MQTT_TIMEOUT_RECEIVE_MS);
    if ((rc != MQTTCLIENT_SUCCESS) && (rc != MQTTCLIENT_TOPICNAME_TRUNCATED))
    {
      IOT_SAMPLE_LOG_ERROR("Failed to receive message: MQTTClient return code %d.", rc);
      exit(rc);
    }
    else if (message == NULL)
    {
      // Allow up to MQTT_TIMEOUT_RECEIVE_MAX_COUNT before disconnecting.
      if (++timeout_counter >= MQTT_TIMEOUT_RECEIVE_MAX_MESSAGE_COUNT)
      {
        IOT_SAMPLE_LOG(
            "Receive message timeout expiration count of %d reached.",
            MQTT_TIMEOUT_RECEIVE_MAX_MESSAGE_COUNT);
        return;
      }
    }
    else
    {
      IOT_SAMPLE_LOG_SUCCESS("Client received a message from the service.");
      timeout_counter = 0; // Reset

      if (rc == MQTTCLIENT_TOPICNAME_TRUNCATED)
      {
        topic_len = (int)strlen(topic);
      }

      on_message_received(topic, topic_len, message);
      IOT_SAMPLE_LOG(" "); // Formatting

      MQTTClient_freeMessage(&message);
      MQTTClient_free(topic);
    }

    send_telemetry_message();
  }
}

static void disconnect_mqtt_client_from_iot_hub(void)
{
  int rc = MQTTClient_disconnect(mqtt_client, MQTT_TIMEOUT_DISCONNECT_MS);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to disconnect MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }

  MQTTClient_destroy(&mqtt_client);
}

static az_span get_request_id(void)
{
  az_span remainder;
  az_span out_span = az_span_create(
      (uint8_t*)connection_request_id_buffer, sizeof(connection_request_id_buffer));

  az_result rc = az_span_u32toa(out_span, connection_request_id_int++, &remainder);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get request id: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  return az_span_slice(out_span, 0, az_span_size(out_span) - az_span_size(remainder));
}

static void publish_mqtt_message(const char* topic, az_span payload, int qos)
{
  int rc = MQTTClient_publish(
      mqtt_client, topic, az_span_size(payload), az_span_ptr(payload), qos, 0, NULL);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to publish message: MQTTClient return code %d", rc);
    exit(rc);
  }
}

static void on_message_received(char* topic, int topic_len, MQTTClient_message const* message)
{
  az_result rc;

  az_span const topic_span = az_span_create((uint8_t*)topic, topic_len);
  az_span const message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  az_iot_hub_client_twin_response twin_response;
  az_iot_hub_client_method_request command_request;

  // Parse the incoming message topic and handle appropriately.
  rc = az_iot_hub_client_twin_parse_received_topic(&hub_client, topic_span, &twin_response);
  if (az_result_succeeded(rc))
  {
    IOT_SAMPLE_LOG_SUCCESS("Client received a valid topic response.");
    IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
    IOT_SAMPLE_LOG_AZ_SPAN("Payload:", message_span);
    IOT_SAMPLE_LOG("Status: %d", twin_response.status);

    handle_device_twin_message(message, &twin_response);
  }
  else
  {
    rc = az_iot_hub_client_methods_parse_received_topic(&hub_client, topic_span, &command_request);
    if (az_result_succeeded(rc))
    {
      IOT_SAMPLE_LOG_SUCCESS("Client received a valid topic response.");
      IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
      IOT_SAMPLE_LOG_AZ_SPAN("Payload:", message_span);

      handle_command_request(message, &command_request);
    }
    else
    {
      IOT_SAMPLE_LOG_ERROR("Message from unknown topic: az_result return code 0x%08x.", rc);
      IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
      exit(rc);
    }
  }
}

static void handle_device_twin_message(
    MQTTClient_message const* message,
    az_iot_hub_client_twin_response const* twin_response)
{
  bool is_twin_get = false;
  az_span const message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  // Invoke appropriate action per response type (3 types only).
  switch (twin_response->response_type)
  {
    // A response from a twin GET publish message with the twin document as a payload.
    case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_GET:
      IOT_SAMPLE_LOG("Message Type: GET");
      is_twin_get = true;
      process_device_twin_message(message_span, is_twin_get);
      break;

    // An update to the desired properties with the properties as a payload.
    case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
      IOT_SAMPLE_LOG("Message Type: Desired Properties");
      process_device_twin_message(message_span, is_twin_get);
      break;

    // A response from a twin reported properties publish message.
    case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
      IOT_SAMPLE_LOG("Message Type: Reported Properties");
      break;
  }
}

static void process_device_twin_message(az_span message_span, bool is_twin_get)
{
  double desired_temperature;
  int32_t version_number;

  // Parse for the desired temperature property.
  if (parse_desired_temperature_property(
          message_span, is_twin_get, &desired_temperature, &version_number))
  {
    IOT_SAMPLE_LOG(" "); // Formatting

    bool confirm = true;
    bool is_max_temp_changed;

    // Update device temperature locally and report update to server.
    update_device_temperature_property(desired_temperature, &is_max_temp_changed);
    send_reported_property(
        twin_desired_temperature_property_name, desired_temperature, version_number, confirm);

    if (is_max_temp_changed)
    {
      confirm = false;
      send_reported_property(
          twin_reported_maximum_temperature_property_name, device_maximum_temperature, -1, confirm);
    }
  }
}

static bool parse_desired_temperature_property(
    az_span message_span,
    bool is_twin_get,
    double* out_parsed_temperature,
    int32_t* out_parsed_version_number)
{
  char const* const log = "Failed to parse for `%.*s` property";
  az_span property = twin_desired_temperature_property_name;

  *out_parsed_temperature = 0.0;
  *out_parsed_version_number = 0;

  // Parse message_span.
  az_json_reader jr;
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_init(&jr, message_span, NULL), log, property);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_next_token(&jr), log, property);

  if (jr.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    IOT_SAMPLE_LOG(
        "`%.*s` property object not found in device twin GET response.",
        az_span_size(twin_desired_name),
        az_span_ptr(twin_desired_name));
    return false;
  }

  // Device twin GET response: Parse to the "desired" wrapper if it exists.
  bool desired_found = false;
  if (is_twin_get)
  {
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_next_token(&jr), log, property);

    while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
    {
      if (az_json_token_is_text_equal(&jr.token, twin_desired_name))
      {
        IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_next_token(&jr), log, property);
        desired_found = true;
        break;
      }
      else
      {
        IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_skip_children(&jr), log, property);
      }
      IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_next_token(&jr), log, property);
    }

    if (!desired_found)
    {
      IOT_SAMPLE_LOG(
          "`%.*s` property object not found in device twin GET response.",
          az_span_size(twin_desired_name),
          az_span_ptr(twin_desired_name));
      return false;
    }
  }

  // Device twin get response OR desired property response:
  // Parse for the desired temperature property
  bool temp_found = false;
  bool version_found = false;

  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_next_token(&jr), log, property);
  while (!(temp_found && version_found) && (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT))
  {
    if (az_json_token_is_text_equal(&jr.token, twin_desired_temperature_property_name))
    {
      IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_next_token(&jr), log, property);
      IOT_SAMPLE_EXIT_IF_AZ_FAILED(
          az_json_token_get_double(&jr.token, out_parsed_temperature), log, property);
      temp_found = true;
    }
    else if (az_json_token_is_text_equal(&jr.token, twin_version_name))
    {
      IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_next_token(&jr), log, property);
      IOT_SAMPLE_EXIT_IF_AZ_FAILED(
          az_json_token_get_int32(&jr.token, out_parsed_version_number), log, property);
      version_found = true;
    }
    else
    {
      IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_skip_children(&jr), log, property);
    }
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_next_token(&jr), log, property);
  }

  if (temp_found && version_found)
  {
    IOT_SAMPLE_LOG(
        "Parsed desired `%.*s`: %2f",
        az_span_size(property),
        az_span_ptr(property),
        *out_parsed_temperature);
    IOT_SAMPLE_LOG(
        "Parsed `%.*s` number: %d",
        az_span_size(twin_version_name),
        az_span_ptr(twin_version_name),
        *out_parsed_version_number);
  }
  else
  {
    IOT_SAMPLE_LOG(
        "Either `%.*s` or `%.*s` were not found in desired property response.",
        az_span_size(property),
        az_span_ptr(property),
        az_span_size(twin_version_name),
        az_span_ptr(twin_version_name));
    return false;
  }

  return true;
}

static void update_device_temperature_property(double temperature, bool* out_is_max_temp_changed)
{
  if (device_maximum_temperature < device_minimum_temperature)
  {
    exit(1);
  }

  *out_is_max_temp_changed = false;
  device_current_temperature = temperature;

  // Update maximum or minimum temperatures.
  if (device_current_temperature > device_maximum_temperature)
  {
    device_maximum_temperature = device_current_temperature;
    *out_is_max_temp_changed = true;
  }
  else if (device_current_temperature < device_minimum_temperature)
  {
    device_minimum_temperature = device_current_temperature;
  }

  // Calculate the new average temperature.
  device_temperature_count++;
  device_temperature_summation += device_current_temperature;
  device_average_temperature = device_temperature_summation / device_temperature_count;

  IOT_SAMPLE_LOG_SUCCESS("Client updated desired temperature variables locally.");
  IOT_SAMPLE_LOG("Current Temperature: %2f", device_current_temperature);
  IOT_SAMPLE_LOG("Maximum Temperature: %2f", device_maximum_temperature);
  IOT_SAMPLE_LOG("Minimum Temperature: %2f", device_minimum_temperature);
  IOT_SAMPLE_LOG("Average Temperature: %2f", device_average_temperature);
}

static void send_reported_property(az_span name, double value, int32_t version, bool confirm)
{
  az_result rc;

  // Get the Twin Patch topic to send a reported property update.
  char twin_patch_topic_buffer[128];
  rc = az_iot_hub_client_twin_patch_get_publish_topic(
      &hub_client,
      get_request_id(),
      twin_patch_topic_buffer,
      sizeof(twin_patch_topic_buffer),
      NULL);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get the Twin Patch topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Build the updated reported property message.
  char reported_property_payload_buffer[128];
  az_span reported_property_payload = AZ_SPAN_FROM_BUFFER(reported_property_payload_buffer);

  if (confirm)
  {
    build_property_payload_with_status(
        name,
        value,
        AZ_IOT_STATUS_OK,
        version,
        twin_success_name,
        reported_property_payload,
        &reported_property_payload);
  }
  else
  {
    uint8_t count = 1;
    az_span const names[1] = { name };
    double const values[1] = { value };

    build_property_payload(
        count, names, values, NULL, reported_property_payload, &reported_property_payload);
  }

  // Publish the reported property update.
  publish_mqtt_message(
      twin_patch_topic_buffer, reported_property_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);
  IOT_SAMPLE_LOG_SUCCESS("Client published the Twin Patch reported property message.");
  IOT_SAMPLE_LOG_AZ_SPAN("Payload:", reported_property_payload);
}

static void handle_command_request(
    MQTTClient_message const* message,
    az_iot_hub_client_method_request const* command_request)
{
  az_span const message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  if (az_span_is_content_equal(command_getMaxMinReport_name, command_request->name))
  {
    az_iot_status status;
    az_span command_response_payload = AZ_SPAN_FROM_BUFFER(command_response_payload_buffer);

    // Invoke command.
    if (invoke_getMaxMinReport(message_span, command_response_payload, &command_response_payload))
    {
      status = AZ_IOT_STATUS_BAD_REQUEST;
    }
    else
    {
      status = AZ_IOT_STATUS_OK;
    }
    IOT_SAMPLE_LOG_SUCCESS("Client invoked command 'getMaxMinReport'.");

    send_command_response(command_request, status, command_response_payload);
  }
  else
  {
    IOT_SAMPLE_LOG_AZ_SPAN("Command not supported:", command_request->name);
    send_command_response(command_request, AZ_IOT_STATUS_NOT_FOUND, command_empty_response_payload);
  }
}

static void send_command_response(
    az_iot_hub_client_method_request const* command_request,
    az_iot_status status,
    az_span response)
{
  az_result rc;

  // Get the Methods response topic to publish the command response.
  char methods_response_topic_buffer[128];
  rc = az_iot_hub_client_methods_response_get_publish_topic(
      &hub_client,
      command_request->request_id,
      (uint16_t)status,
      methods_response_topic_buffer,
      sizeof(methods_response_topic_buffer),
      NULL);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to get the Methods Response topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Publish the command response.
  publish_mqtt_message(methods_response_topic_buffer, response, IOT_SAMPLE_MQTT_PUBLISH_QOS);
  IOT_SAMPLE_LOG_SUCCESS("Client published the Command response.");
  IOT_SAMPLE_LOG("Status: %d", status);
  IOT_SAMPLE_LOG_AZ_SPAN("Payload:", response);
}

static bool invoke_getMaxMinReport(az_span payload, az_span response, az_span* out_response)
{
  int32_t incoming_since_value_len = 0;

  // Parse the `since` field in the payload.
  char const* const log = "Failed to parse for `since` field in payload";

  az_json_reader jr;
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_init(&jr, payload, NULL), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_next_token(&jr), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_token_get_string(
          &jr.token,
          command_start_time_value_buffer,
          sizeof(command_start_time_value_buffer),
          &incoming_since_value_len),
      log);

  // Set the response payload to error if the `since` value was empty.
  if (incoming_since_value_len == 0)
  {
    *out_response = command_empty_response_payload;
    return false;
  }

  az_span start_time_span
      = az_span_create((uint8_t*)command_start_time_value_buffer, incoming_since_value_len);

  IOT_SAMPLE_LOG_AZ_SPAN("Start time:", start_time_span);

  // Get the current time as a string.
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  size_t length = strftime(
      command_end_time_value_buffer,
      sizeof(command_end_time_value_buffer),
      iso_spec_time_format,
      timeinfo);
  az_span end_time_span = az_span_create((uint8_t*)command_end_time_value_buffer, (int32_t)length);

  IOT_SAMPLE_LOG_AZ_SPAN("End Time:", end_time_span);

  // Build command response message.
  uint8_t count = 3;
  az_span const names[3] = { command_max_temp_name, command_min_temp_name, command_avg_temp_name };
  double const values[3]
      = { device_maximum_temperature, device_minimum_temperature, device_average_temperature };
  az_span const times[2] = { start_time_span, end_time_span };

  build_property_payload(count, names, values, times, response, out_response);

  return true;
}

static void send_telemetry_message(void)
{
  az_result rc;

  // Get the Telemetry topic to publish the telemetry message.
  char telemetry_topic_buffer[128];
  rc = az_iot_hub_client_telemetry_get_publish_topic(
      &hub_client, NULL, telemetry_topic_buffer, sizeof(telemetry_topic_buffer), NULL);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get the Telemetry topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Build the telemetry message.
  uint8_t count = 1;
  az_span const names[1] = { telemetry_temperature_name };
  double const values[1] = { device_current_temperature };

  char telemetry_payload_buffer[128];
  az_span telemetry_payload = AZ_SPAN_FROM_BUFFER(telemetry_payload_buffer);
  build_property_payload(count, names, values, NULL, telemetry_payload, &telemetry_payload);

  // Publish the telemetry message.
  publish_mqtt_message(telemetry_topic_buffer, telemetry_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);
  IOT_SAMPLE_LOG_SUCCESS("Client published the Telemetry message.");
  IOT_SAMPLE_LOG_AZ_SPAN("Payload:", telemetry_payload);
}

static void build_property_payload(
    uint8_t property_count,
    az_span const names[],
    double const values[],
    az_span const times[],
    az_span property_payload,
    az_span* out_property_payload)
{
  char const* const log = "Failed to build property payload";

  az_json_writer jw;
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_init(&jw, property_payload, NULL), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), log);

  for (uint8_t i = 0; i < property_count; i++)
  {
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_property_name(&jw, names[i]), log);
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(
        az_json_writer_append_double(&jw, values[i], DOUBLE_DECIMAL_PLACE_DIGITS), log);
  }

  if (times != NULL)
  {
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(
        az_json_writer_append_property_name(&jw, command_start_time_name), log);
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_string(&jw, times[0]), log);
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(
        az_json_writer_append_property_name(&jw, command_end_time_name), log);
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_string(&jw, times[1]), log);
  }

  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), log);
  *out_property_payload = az_json_writer_get_bytes_used_in_destination(&jw);
}

static void build_property_payload_with_status(
    az_span name,
    double value,
    int32_t ack_code_value,
    int32_t ack_version_value,
    az_span ack_description_value,
    az_span property_payload,
    az_span* out_property_payload)
{
  char const* const log = "Failed to build property payload with status";

  az_json_writer jw;
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_init(&jw, property_payload, NULL), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_property_name(&jw, name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_property_name(&jw, twin_value_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_double(&jw, value, DOUBLE_DECIMAL_PLACE_DIGITS), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_property_name(&jw, twin_ack_code_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_int32(&jw, ack_code_value), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, twin_ack_version_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_int32(&jw, ack_version_value), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, twin_ack_description_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_string(&jw, ack_description_value), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), log);

  *out_property_payload = az_json_writer_get_bytes_used_in_destination(&jw);
}

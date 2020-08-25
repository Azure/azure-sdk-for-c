// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4204: nonstandard extension used: non-constant aggregate initializer
#pragma warning(disable : 4204)
// warning C4996: 'localtime': This function or variable may be unsafe.  Consider using localtime_s
// instead.
#pragma warning(disable : 4996)
#pragma warning(push)
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
#endif
#include <paho-mqtt/MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "iot_sample_common.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#define SAMPLE_TYPE PAHO_IOT_HUB
#define SAMPLE_NAME PAHO_IOT_HUB_PNP_SAMPLE

#define TELEMETRY_SEND_INTERVAL 1
#define TIMEOUT_MQTT_RECEIVE_MAX_MESSAGE_COUNT 3
#define TIMEOUT_MQTT_RECEIVE_MS (8 * 1000)
#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)
#define TIMEOUT_MQTT_WAIT_FOR_COMPLETION_MS 1000

#define DEFAULT_START_TEMP_AVG_COUNT 1
#define DEFAULT_START_TEMP_CELSIUS 22.0
#define DOUBLE_DECIMAL_PLACE_DIGITS 2
#define SAMPLE_PUBLISH_QOS 0

static bool is_device_operational = true;
static const char iso_spec_time_format[] = "%Y-%m-%dT%H:%M:%S%z"; // ISO8601 Time Format

// * PnP Values *
// The model id is the JSON document (also called the Digital Twins Model Identifier or DTMI)
// which defines the capability of your device. The functionality of the device should match what
// is described in the coresponding DTMI. Should you choose to program your own PnP capable device,
// the functionality would need to match the DTMI and you would need to update the below 'model_id'.
// Please see the sample README for more information on this DTMI.
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:com:example:Thermostat;1");

// IoT Hub Connection Values
static int32_t connection_request_id_int = 0;
static char connection_request_id_buffer[16];

// IoT Hub Device Twin Values
static const az_span twin_desired_name = AZ_SPAN_LITERAL_FROM_STR("desired");
static const az_span twin_version_name = AZ_SPAN_LITERAL_FROM_STR("$version");
static const az_span twin_success_name = AZ_SPAN_LITERAL_FROM_STR("success");
static const az_span twin_value_name = AZ_SPAN_LITERAL_FROM_STR("value");
static const az_span twin_ack_code_name = AZ_SPAN_LITERAL_FROM_STR("ac");
static const az_span twin_ack_version_name = AZ_SPAN_LITERAL_FROM_STR("av");
static const az_span twin_ack_description_name = AZ_SPAN_LITERAL_FROM_STR("ad");
static const az_span twin_desired_temp_property_name
    = AZ_SPAN_LITERAL_FROM_STR("targetTemperature");
static const az_span twin_reported_max_temp_property_name
    = AZ_SPAN_LITERAL_FROM_STR("maxTempSinceLastReboot");

// IoT Hub Method (Command) Values
static const az_span command_name = AZ_SPAN_LITERAL_FROM_STR("getMaxMinReport");
static const az_span command_max_temp_name = AZ_SPAN_LITERAL_FROM_STR("maxTemp");
static const az_span command_min_temp_name = AZ_SPAN_LITERAL_FROM_STR("minTemp");
static const az_span command_avg_temp_name = AZ_SPAN_LITERAL_FROM_STR("avgTemp");
static const az_span command_start_time_name = AZ_SPAN_LITERAL_FROM_STR("startTime");
static const az_span command_end_time_name = AZ_SPAN_LITERAL_FROM_STR("endTime");
static const az_span command_empty_response_payload = AZ_SPAN_LITERAL_FROM_STR("{}");
static char command_start_time_value_buffer[32];
static char command_end_time_value_buffer[32];
static char command_response_payload_buffer[256];

// IoT Hub Telemetry Values
static const az_span telemetry_temperature_name = AZ_SPAN_LITERAL_FROM_STR("temperature");

// PnP Device Values
static double device_current_temp = DEFAULT_START_TEMP_CELSIUS;
static double device_temp_avg_total = DEFAULT_START_TEMP_CELSIUS;
static uint32_t device_temp_avg_count = DEFAULT_START_TEMP_AVG_COUNT;
static double device_max_temp = DEFAULT_START_TEMP_CELSIUS;
static double device_min_temp = DEFAULT_START_TEMP_CELSIUS;
static double device_avg_temp = DEFAULT_START_TEMP_CELSIUS;

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
static void mqtt_publish_message(const char* topic, az_span payload, int qos);
static void on_message_received(char* topic, int topic_len, const MQTTClient_message* message);

// Device Twin functions
static void handle_device_twin_message(
    az_span twin_message_span,
    const az_iot_hub_client_twin_response* twin_response);
static void process_device_twin_message(az_span twin_message_span, bool is_twin_get);
static az_result parse_device_twin_desired_temperature_property(
    az_span twin_message_span,
    bool is_twin_get,
    double* parsed_temp,
    int32_t* version_number);
static void update_device_temp(double temp, bool* is_max_temp_changed);
static void send_reported_property(az_span name, double value, int32_t version, bool confirm);

// Command functions
static void handle_command_message(
    az_span command_message_span,
    const az_iot_hub_client_method_request* command_request);
static void send_command_response(
    const az_iot_hub_client_method_request* command_request,
    az_iot_status status,
    az_span response_payload);
static az_result invoke_getMaxMinReport(
    az_span payload,
    az_span response_destination,
    az_span* out_response);

// Telemetry functions
static void send_telemetry_message(void);

// JSON build functions
static az_result build_property_payload(
    uint8_t property_count,
    const az_span names[],
    const double values[],
    const az_span times[],
    az_span payload_destination,
    az_span* out_payload);
static az_result build_property_payload_with_status(
    az_span name,
    double value,
    int32_t ack_code_value,
    int32_t ack_version_value,
    az_span ack_description_value,
    az_span payload_destination,
    az_span* out_payload);

/*
 * This sample connects an IoT Plug and Play enabled device with the Digital Twin Model ID (DTMI).
 * If a timeout occurs while waiting for a message from the Azure IoT Explorer, the sample will
 * continue. If TIMEOUT_MQTT_RECEIVE_MAX_COUNT timeouts occur consecutively, the sample will
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
 * To send a device twin desired property message, select your device's Device Twin tab
 * in the Azure IoT Explorer. Add the property targetTemperature along with a corresponding value to
 * the desired section of the JSON. Select Save to update the twin document and send the twin
 * message to the device.
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
 * Telemetry: Device sends a JSON message with the field name `temperature` and the `double`
 * value of the current temperature.
 */
int main(void)
{
  create_and_configure_mqtt_client();
  LOG_SUCCESS("Client created and configured.");

  connect_mqtt_client_to_iot_hub();
  LOG_SUCCESS("Client connected to IoT Hub.");

  subscribe_mqtt_client_to_iot_hub_topics();
  LOG_SUCCESS("Client subscribed to IoT Hub topics.");

  request_device_twin_document();
  LOG_SUCCESS("Client requested device twin document.")

  receive_messages();
  LOG_SUCCESS("Client received messages.")

  disconnect_mqtt_client_from_iot_hub();
  LOG_SUCCESS("Client disconnected from IoT Hub.");

  return 0;
}

static void create_and_configure_mqtt_client(void)
{
  int rc;

  // Reads in environment variables set by user for purposes of running sample.
  if (az_failed(rc = iot_sample_read_environment_variables(SAMPLE_TYPE, SAMPLE_NAME, &env_vars)))
  {
    LOG_ERROR(
        "Failed to read configuration from environment variables: az_result return code 0x%08x.",
        rc);
    exit(rc);
  }

  // Build an MQTT endpoint c-string.
  char mqtt_endpoint_buffer[128];
  if (az_failed(
          rc = iot_sample_create_mqtt_endpoint(
              SAMPLE_TYPE, &env_vars, mqtt_endpoint_buffer, sizeof(mqtt_endpoint_buffer))))
  {
    LOG_ERROR("Failed to create MQTT endpoint: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Initialize the hub client with the connection options.
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.model_id = model_id;
  if (az_failed(
          rc = az_iot_hub_client_init(
              &hub_client, env_vars.hub_hostname, env_vars.hub_device_id, &options)))
  {
    LOG_ERROR("Failed to initialize hub client: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Get the MQTT client id used for the MQTT connection.
  char mqtt_client_id_buffer[128];
  if (az_failed(
          rc = az_iot_hub_client_get_client_id(
              &hub_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT client id: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Create the Paho MQTT client.
  if ((rc = MQTTClient_create(
           &mqtt_client,
           mqtt_endpoint_buffer,
           mqtt_client_id_buffer,
           MQTTCLIENT_PERSISTENCE_NONE,
           NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to create MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

static void connect_mqtt_client_to_iot_hub(void)
{
  int rc;

  // Get the MQTT client username.
  if (az_failed(
          rc = az_iot_hub_client_get_user_name(
              &hub_client, mqtt_client_username_buffer, sizeof(mqtt_client_username_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT client username: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Set MQTT connection options.
  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;
  mqtt_connect_options.username = mqtt_client_username_buffer;
  mqtt_connect_options.password = NULL; // This sample uses x509 authentication.
  mqtt_connect_options.cleansession = false; // Set to false so can receive any pending messages.
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  mqtt_ssl_options.keyStore = (char*)az_span_ptr(env_vars.x509_cert_pem_file_path);
  if (az_span_size(env_vars.x509_trust_pem_file_path) != 0) // Is only set if required by OS.
  {
    mqtt_ssl_options.trustStore = (char*)az_span_ptr(env_vars.x509_trust_pem_file_path);
  }
  mqtt_connect_options.ssl = &mqtt_ssl_options;

  // Connect MQTT client to the Azure IoT Hub.
  if ((rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR(
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
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to subscribe to the Methods topic: MQTTClient return code %d.", rc);
    exit(rc);
  }

  // Messages received on the Twin Patch topic will be updates to the desired properties.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to subscribe to the Twin Patch topic: MQTTClient return code %d.", rc);
    exit(rc);
  }

  // Messages received on Twin Response topic will be response statuses from the server.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to subscribe to the Twin Response topic: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

static void request_device_twin_document(void)
{
  az_result rc;

  // Get the Twin Document topic to publish the twin document request.
  char twin_document_topic_buffer[128];
  if (az_failed(
          rc = az_iot_hub_client_twin_document_get_publish_topic(
              &hub_client,
              get_request_id(),
              twin_document_topic_buffer,
              sizeof(twin_document_topic_buffer),
              NULL)))
  {
    LOG_ERROR("Failed to get Twin Document publish topic: az_result return code %04x", rc);
    exit(rc);
  }

  // Publish the twin document request.
  mqtt_publish_message(twin_document_topic_buffer, AZ_SPAN_NULL, SAMPLE_PUBLISH_QOS);
}

static void receive_messages(void)
{
  int rc;
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;
  uint8_t timeout_counter = 0;

  // Continue to receive commands or device twin messages while device is operational.
  while (is_device_operational)
  {
    LOG(" "); // Formatting.
    LOG("Waiting for Command or Device Twin message.\n");

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
      // Allow up to TIMEOUT_MQTT_RECEIVE_MAX_COUNT before disconnecting.
      if (++timeout_counter >= TIMEOUT_MQTT_RECEIVE_MAX_MESSAGE_COUNT)
      {
        LOG("Receive message timeout count of %d reached.", TIMEOUT_MQTT_RECEIVE_MAX_MESSAGE_COUNT);
        return;
      }
    }
    else
    {
      LOG_SUCCESS("Client received a message from the service.");

      if (rc == MQTTCLIENT_TOPICNAME_TRUNCATED)
      {
        topic_len = (int)strlen(topic);
      }

      timeout_counter = 0; // Reset.

      on_message_received(topic, topic_len, message);
      LOG(" "); // Formatting.

      MQTTClient_freeMessage(&message);
      MQTTClient_free(topic);
    }

    // Send a telemetry message.
    send_telemetry_message();
  }
}

static void disconnect_mqtt_client_from_iot_hub(void)
{
  int rc;

  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to disconnect MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }

  MQTTClient_destroy(&mqtt_client);
}

static az_span get_request_id(void)
{
  az_result rc;
  az_span out_span;
  az_span destination = az_span_create(
      (uint8_t*)connection_request_id_buffer, sizeof(connection_request_id_buffer));

  if (az_failed(rc = az_span_i32toa(destination, connection_request_id_int++, &out_span)))
  {
    LOG_ERROR("Failed to get request id: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  return az_span_slice(destination, 0, az_span_size(destination) - az_span_size(out_span));
}

static void mqtt_publish_message(const char* topic, az_span payload, int qos)
{
  int rc;
  MQTTClient_deliveryToken token;

  if ((rc = MQTTClient_publish(
           mqtt_client, topic, az_span_size(payload), az_span_ptr(payload), qos, 0, &token))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to publish message: MQTTClient return code %d", rc);
    exit(rc);
  }

  if (qos > 0)
  {
    if ((rc = MQTTClient_waitForCompletion(mqtt_client, token, TIMEOUT_MQTT_WAIT_FOR_COMPLETION_MS))
        != MQTTCLIENT_SUCCESS)
    {
      LOG_ERROR("Wait for message completion time out expired: MQTTClient return code %d", rc);
      exit(rc);
    }
  }
}

static void on_message_received(char* topic, int topic_len, const MQTTClient_message* message)
{
  az_result rc;

  az_span topic_span = az_span_create((uint8_t*)topic, topic_len);
  az_span message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  az_iot_hub_client_twin_response twin_response;
  az_iot_hub_client_method_request command_request;

  // Parse the incoming message topic and check which feature it is for.
  if (az_succeeded(
          rc
          = az_iot_hub_client_twin_parse_received_topic(&hub_client, topic_span, &twin_response)))
  {
    LOG_SUCCESS("Client received a valid topic response.");
    LOG_AZ_SPAN("Topic:", topic_span);
    LOG_AZ_SPAN("Payload:", message_span);
    LOG("Status: %d", twin_response.status);

    handle_device_twin_message(message_span, &twin_response);
  }
  else if (az_succeeded(
               rc = az_iot_hub_client_methods_parse_received_topic(
                   &hub_client, topic_span, &command_request)))
  {
    LOG_SUCCESS("Client received a valid topic response.");
    LOG_AZ_SPAN("Topic:", topic_span);
    LOG_AZ_SPAN("Payload:", message_span);

    handle_command_message(message_span, &command_request);
  }
  else
  {
    LOG_ERROR("Message from unknown topic: az_result return code 0x%08x.", rc);
    LOG_AZ_SPAN("Topic:", topic_span);
    exit(rc);
  }
}

static void handle_device_twin_message(
    az_span twin_message_span,
    const az_iot_hub_client_twin_response* twin_response)
{
  bool is_twin_get = false;

  // Invoke appropriate action per response type (3 types only).
  switch (twin_response->response_type)
  {
    // A response from a twin GET publish message with the twin document as a payload.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET:
      LOG("Message Type: GET");
      is_twin_get = true;
      process_device_twin_message(twin_message_span, is_twin_get);
      break;

    // An update to the desired properties with the properties as a payload.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
      LOG("Message Type: Desired Properties");
      process_device_twin_message(twin_message_span, is_twin_get);
      break;

    // A response from a twin reported properties publish message.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
      LOG("Message Type: Reported Properties");
      break;
  }
}

static void process_device_twin_message(az_span twin_message_span, bool is_twin_get)
{
  double desired_temp;
  int32_t version_num;

  // Parse for the desired temperature
  if (az_succeeded(parse_device_twin_desired_temperature_property(
          twin_message_span, is_twin_get, &desired_temp, &version_num)))
  {
    bool confirm = true;
    bool is_max_temp_changed = false;

    // Update device temperature locally and report update to server
    update_device_temp(desired_temp, &is_max_temp_changed);
    send_reported_property(twin_desired_temp_property_name, desired_temp, version_num, confirm);

    if (is_max_temp_changed)
    {
      confirm = false;
      send_reported_property(twin_reported_max_temp_property_name, device_max_temp, -1, confirm);
    }
  }
  // Else desired property not found in payload. Do nothing.
}

static az_result parse_device_twin_desired_temperature_property(
    az_span twin_message_span,
    bool is_twin_get,
    double* parsed_temp,
    int32_t* version_number)
{
  az_json_reader jr;

  AZ_RETURN_IF_FAILED(az_json_reader_init(&jr, twin_message_span, NULL));
  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
  if (jr.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  // Device twin GET response: Parse to the "desired" wrapper if it exists.
  bool desired_found = false;
  if (is_twin_get)
  {
    AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
    while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
    {
      if (az_json_token_is_text_equal(&jr.token, twin_desired_name))
      {
        desired_found = true;
        AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
        break;
      }
      else
      {
        AZ_RETURN_IF_FAILED(az_json_reader_skip_children(&jr)); // Ignore children tokens.
      }

      AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr)); // Check next sibling token.
    }

    if (!desired_found)
    {
      LOG("Desired property object not found in device twin GET response.");
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
  }

  // Device twin get response OR desired property response:
  // Parse for the desired temperature property
  bool temp_found = false;
  bool version_found = false;

  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
  while (!(temp_found && version_found) && (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT))
  {
    if (az_json_token_is_text_equal(&jr.token, twin_desired_temp_property_name))
    {
      AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      AZ_RETURN_IF_FAILED(az_json_token_get_double(&jr.token, parsed_temp));
      temp_found = true;
    }
    else if (az_json_token_is_text_equal(&jr.token, twin_version_name))
    {
      AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      AZ_RETURN_IF_FAILED(az_json_token_get_uint32(&jr.token, (uint32_t*)version_number));
      version_found = true;
    }
    else
    {
      AZ_RETURN_IF_FAILED(az_json_reader_skip_children(&jr)); // Ignore children tokens.
    }
    AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr)); // Check next sibling token.
  }

  if (!(temp_found && version_found))
  {
    LOG("Either targetTemperature property or the $version property were not found in desired "
        "property response.");
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  LOG("Parsed desired targetTemperature: %2f", *parsed_temp);
  LOG("Parsed version number: %d", *version_number);
  return AZ_OK;
}

static void update_device_temp(double temp, bool* is_max_temp_changed)
{
  device_current_temp = temp;

  bool ret = false;
  if (device_current_temp > device_max_temp)
  {
    device_max_temp = device_current_temp;
    ret = true;
  }
  if (device_current_temp < device_min_temp)
  {
    device_min_temp = device_current_temp;
  }

  // Increment the avg count, add the new temp to the total, and calculate the new avg.
  device_temp_avg_count++;
  device_temp_avg_total += device_current_temp;
  device_avg_temp = device_temp_avg_total / device_temp_avg_count;

  *is_max_temp_changed = ret;

  LOG_SUCCESS("Client updated desired temperature locally.");
}

static void send_reported_property(az_span name, double value, int32_t version, bool confirm)
{
  az_result rc;

  // Get the Twin Patch topic to send a reported property update.
  char twin_patch_topic_buffer[128];
  if (az_failed(
          rc = az_iot_hub_client_twin_patch_get_publish_topic(
              &hub_client,
              get_request_id(),
              twin_patch_topic_buffer,
              sizeof(twin_patch_topic_buffer),
              NULL)))
  {
    LOG_ERROR("Failed to get Twin Patch publish topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Build the updated reported property message.
  char reported_property_payload_buffer[128];
  az_span reported_property_payload = AZ_SPAN_FROM_BUFFER(reported_property_payload_buffer);
  if (confirm)
  {
    if (az_failed(
            rc = build_property_payload_with_status(
                name,
                value,
                AZ_IOT_STATUS_OK,
                version,
                twin_success_name,
                reported_property_payload,
                &reported_property_payload)))
    {
      LOG_ERROR(
          "Failed to build reported property confirmed payload : az_result return code 0x%08x.",
          rc);
      exit(rc);
    }
  }
  else
  {
    const uint8_t count = 1;
    const az_span names[1] = { name };
    const double values[1] = { value };

    if (az_failed(
            rc = build_property_payload(
                count, names, values, NULL, reported_property_payload, &reported_property_payload)))
    {
      LOG_ERROR("Failed to build reported property payload: az_result return code 0x%08x.", rc);
      exit(rc);
    }
  }

  // Publish the reported property update.
  mqtt_publish_message(twin_patch_topic_buffer, reported_property_payload, SAMPLE_PUBLISH_QOS);
  LOG_SUCCESS("Client sent reported property message.");
  LOG_AZ_SPAN("Payload:", reported_property_payload);
}

static void handle_command_message(
    const az_span command_message_span,
    const az_iot_hub_client_method_request* command_request)
{
  if (az_span_is_content_equal(command_name, command_request->name))
  {
    az_iot_status status;
    az_span command_response_payload = AZ_SPAN_FROM_BUFFER(command_response_payload_buffer);

    // Invoke command.
    if (az_failed(invoke_getMaxMinReport(
            command_message_span, command_response_payload, &command_response_payload)))
    {
      status = AZ_IOT_STATUS_BAD_REQUEST;
    }
    else
    {
      status = AZ_IOT_STATUS_OK;
    }
    LOG_SUCCESS("Client invoked 'invoke_getMaxMinReport'.");

    send_command_response(command_request, status, command_response_payload);
  }
  else
  {
    LOG_AZ_SPAN("Command not supported:", command_request->name);
    send_command_response(command_request, AZ_IOT_STATUS_NOT_FOUND, command_empty_response_payload);
  }
}

static void send_command_response(
    const az_iot_hub_client_method_request* command_request,
    az_iot_status status,
    az_span response_payload)
{
  az_result rc;

  // Get the Methods response topic to publish the command response.
  char methods_response_topic_buffer[128];
  if (az_failed(
          rc = az_iot_hub_client_methods_response_get_publish_topic(
              &hub_client,
              command_request->request_id,
              (uint16_t)status,
              methods_response_topic_buffer,
              sizeof(methods_response_topic_buffer),
              NULL)))
  {
    LOG_ERROR("Failed to get Methods response publish topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Publish the command response.
  mqtt_publish_message(methods_response_topic_buffer, response_payload, SAMPLE_PUBLISH_QOS);
  LOG_SUCCESS("Client published command response:");
  LOG("Status: %d", status);
  LOG_AZ_SPAN("Payload:", response_payload);
}

static az_result invoke_getMaxMinReport(
    az_span payload,
    az_span response_destination,
    az_span* out_response)
{
  int32_t incoming_since_value_len = 0;
  az_json_reader jr;

  // Parse the "since" field in the payload.
  AZ_RETURN_IF_FAILED(az_json_reader_init(&jr, payload, NULL));
  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
  AZ_RETURN_IF_FAILED(az_json_token_get_string(
      &jr.token,
      command_start_time_value_buffer,
      sizeof(command_start_time_value_buffer),
      &incoming_since_value_len));
  az_span start_time_span
      = az_span_create((uint8_t*)command_start_time_value_buffer, incoming_since_value_len);

  LOG_AZ_SPAN("start time:", start_time_span);

  // Set the response payload to error if the "since" value was empty.
  if (az_span_ptr(start_time_span) == NULL)
  {
    *out_response = command_empty_response_payload;
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

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

  LOG_AZ_SPAN("end time:", end_time_span);

  // Build command response message.
  const uint8_t count = 3;
  const az_span names[3] = { command_max_temp_name, command_min_temp_name, command_avg_temp_name };
  const double values[3] = { device_max_temp, device_min_temp, device_avg_temp };
  const az_span times[2] = { start_time_span, end_time_span };

  az_result rc;
  if (az_failed(
          rc = build_property_payload(
              count, names, values, times, response_destination, out_response)))
  {
    LOG_ERROR("Failed to build command response payload: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  return AZ_OK;
}

static void send_telemetry_message(void)
{
  az_result rc;

  // Get the Telemetry topic to publish the telemetry message.
  char telemetry_topic_buffer[128];
  if (az_failed(
          rc = az_iot_hub_client_telemetry_get_publish_topic(
              &hub_client, NULL, telemetry_topic_buffer, sizeof(telemetry_topic_buffer), NULL)))
  {
    LOG_ERROR("Failed to get Telemetry publish topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Build the telemetry message.
  const uint8_t count = 1;
  const az_span names[1] = { telemetry_temperature_name };
  const double values[1] = { device_current_temp };

  char telemetry_payload_buffer[128];
  az_span telemetry_payload = AZ_SPAN_FROM_BUFFER(telemetry_payload_buffer);
  if (az_failed(
          rc = build_property_payload(
              count, names, values, NULL, telemetry_payload, &telemetry_payload)))
  {
    LOG_ERROR("Failed to build telemetry payload: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Publish the telemetry message.
  mqtt_publish_message(telemetry_topic_buffer, telemetry_payload, SAMPLE_PUBLISH_QOS);
  LOG_SUCCESS("Client sent telemetry message to the service:");
  LOG_AZ_SPAN("Payload:", telemetry_payload);
}

static az_result build_property_payload(
    uint8_t property_count,
    const az_span names[],
    const double values[],
    const az_span times[],
    az_span payload_destination,
    az_span* payload_out)
{
  az_json_writer jw;

  AZ_RETURN_IF_FAILED(az_json_writer_init(&jw, payload_destination, NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));

  for (uint8_t i = 0; i < property_count; i++)
  {
    AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, names[i]));
    AZ_RETURN_IF_FAILED(az_json_writer_append_double(&jw, values[i], DOUBLE_DECIMAL_PLACE_DIGITS));
  }

  if (times != NULL)
  {
    AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, command_start_time_name));
    AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, times[0]));
    AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, command_end_time_name));
    AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, times[1]));
  }

  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));
  *payload_out = az_json_writer_get_bytes_used_in_destination(&jw);

  return AZ_OK;
}

static az_result build_property_payload_with_status(
    az_span name,
    double value,
    int32_t ack_code_value,
    int32_t ack_version_value,
    az_span ack_description_value,
    az_span payload_destination,
    az_span* payload_out)
{
  az_json_writer jw;

  AZ_RETURN_IF_FAILED(az_json_writer_init(&jw, payload_destination, NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, twin_value_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_double(&jw, value, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, twin_ack_code_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(&jw, ack_code_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, twin_ack_version_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(&jw, ack_version_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, twin_ack_description_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, ack_description_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));

  *payload_out = az_json_writer_get_bytes_used_in_destination(&jw);

  return AZ_OK;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

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
#include <azure/iot/az_iot_pnp_client.h>

#include "iot_sample_common.h"
#include "pnp/pnp_device_info_component.h"
#include "pnp/pnp_mqtt_message.h"
#include "pnp/pnp_thermostat_component.h"

#define SAMPLE_TYPE PAHO_IOT_HUB
#define SAMPLE_NAME PAHO_IOT_PNP_COMPONENT_SAMPLE

#define DEFAULT_START_TEMP_CELSIUS 22.0
#define DOUBLE_DECIMAL_PLACE_DIGITS 2

bool is_device_operational = true;

// * PnP Values *
// The model id is the JSON document (also called the Digital Twins Model Identifier or DTMI) which
// defines the capability of your device. The functionality of the device should match what is
// described in the corresponding DTMI. Should you choose to program your own PnP capable device,
// the functionality would need to match the DTMI and you would need to update the below 'model_id'.
// Please see the sample README for more information on this DTMI.
static az_span const model_id
    = AZ_SPAN_LITERAL_FROM_STR("dtmi:com:example:TemperatureController;1");

// Components
static pnp_thermostat_component thermostat_1;
static pnp_thermostat_component thermostat_2;
static az_span thermostat_1_name = AZ_SPAN_LITERAL_FROM_STR("thermostat1");
static az_span thermostat_2_name = AZ_SPAN_LITERAL_FROM_STR("thermostat2");
static az_span pnp_device_components[] = { AZ_SPAN_LITERAL_FROM_STR("thermostat1"),
                                           AZ_SPAN_LITERAL_FROM_STR("thermostat2"),
                                           AZ_SPAN_LITERAL_FROM_STR("deviceInformation") };
static int32_t const pnp_components_length
    = sizeof(pnp_device_components) / sizeof(pnp_device_components[0]);

// Plug and Play property values
static az_span const reported_property_serial_number_name
    = AZ_SPAN_LITERAL_FROM_STR("serialNumber");
static az_span property_reported_serial_number_property_value = AZ_SPAN_LITERAL_FROM_STR("ABCDEFG");
static az_span const property_response_failed = AZ_SPAN_LITERAL_FROM_STR("failed");

// Plug and Play command values
static az_span const command_reboot_name = AZ_SPAN_LITERAL_FROM_STR("reboot");
static az_span const command_empty_response_payload = AZ_SPAN_LITERAL_FROM_STR("{}");

// Plug and Play telemetry values
static az_span const telemetry_working_set_name = AZ_SPAN_LITERAL_FROM_STR("workingSet");

static iot_sample_environment_variables env_vars;
static az_iot_pnp_client pnp_client;
static MQTTClient mqtt_client;
static char mqtt_client_username_buffer[512];
static pnp_mqtt_message publish_message;

//
// Functions
//
static void create_and_configure_mqtt_client(void);
static void connect_mqtt_client_to_iot_hub(void);
static void subscribe_mqtt_client_to_iot_hub_topics(void);
static void initialize_components(void);
static void send_device_info(void);
static void send_serial_number(void);
static void request_all_properties(void);
static void receive_messages(void);
static void disconnect_mqtt_client_from_iot_hub(void);

// General message sending and receiving functions
static void publish_mqtt_message(char const* topic, az_span payload, int qos);
static void receive_mqtt_message(void);
static void on_message_received(
    char* topic,
    int topic_len,
    MQTTClient_message const* receive_message);

// Device property, command request, telemetry functions
static void handle_device_property_message(
    MQTTClient_message const* receive_message,
    az_iot_pnp_client_property_response const* property_response);
static void handle_command_request(
    MQTTClient_message const* receive_message,
    az_iot_pnp_client_command_request const* command_request);
static void send_telemetry_messages(void);

// Temperature controller functions
static void temp_controller_build_telemetry_message(az_span payload, az_span* out_payload);
static void temp_controller_build_serial_number_reported_property(
    az_span payload,
    az_span* out_payload);
static bool temp_controller_process_command_request(
    az_span command_name,
    az_span command_payload,
    az_span payload,
    az_span* out_payload,
    az_iot_status* out_status);
static void temp_controller_invoke_reboot(void);

static az_result append_simple_json_token(az_json_writer* jw, az_json_token* json_token);

/*
 * This sample extends the IoT Plug and Play Sample above to mimic a Temperature Controller
 * and connects the IoT Plug and Play enabled device (the Temperature Controller) with the Digital
 * Twin Model ID (DTMI). If a timeout occurs while waiting for a message from the Azure IoT
 * Explorer, the sample will continue. If PNP_MQTT_TIMEOUT_RECEIVE_MAX_COUNT timeouts occur
 * consecutively, the sample will disconnect. X509 self-certification is used.
 *
 * This Temperature Controller is made up of the following components:
 * - Device Info
 * - Temperature Sensor 1
 * - Temperature Sensor 2
 *
 * To interact with this sample, you must use the Azure IoT Explorer. The capabilities are
 * properties, commands, and telemetry:
 *
 * Properties: The following properties are supported in this sample:
 *
 * Temperature Controller:
 * - A reported property named `serialNumber` with a `string` value for the device serial number.
 *
 * Device Info:
 * - A reported property named `manufacturer` with a `string` value for the name of the device
 * manufacturer.
 * - A reported property named `model` with a `string` value for the name of the device model.
 * - A reported property named `swVersion` with a `string` value for the software version running on
 * the device.
 * - A reported property named `osName` with a `string` value for the name of the operating system
 * running on the device.
 * - A reported property named `processorArchitecture` with a `string` value for the name of the
 * device architecture.
 * - A reported property named `processorManufacturer` with a `string` value for the name of the
 * device's processor manufacturer.
 * - A reported property named `totalStorage` with a `double` value for the total storage in KiB on
 * the device.
 * - A reported property named `totalMemory` with a `double` value for the total memory in KiB on
 * the device.
 *
 * Temperature Sensor:
 * - A desired property named `targetTemperature` with a `double` value for the desired temperature.
 * - A reported property named `maxTempSinceLastReboot` with a `double` value for the highest
 * temperature reached since boot.
 *
 * On initial bootup of the device, the sample will send the Temperature Controller reported
 * properties to the service. You will see the following in the device property JSON.
 *   {
 *     "properties": {
 *       "reported": {
 *         "manufacturer": "Sample-Manufacturer",
 *         "model": "pnp-sample-Model-123",
 *         "swVersion": "1.0.0.0",
 *         "osName": "Contoso",
 *         "processorArchitecture": "Contoso-Arch-64bit",
 *         "processorManufacturer": "Processor Manufacturer(TM)",
 *         "totalStorage": 1024,
 *         "totalMemory": 128,
 *         "serialNumber": "ABCDEFG",
 *       }
 *     }
 *   }
 *
 * To send a Temperature Sensor device desired property message, select your device's Device
 * Twin tab in the Azure IoT Explorer. Add the property targetTemperature along with a corresponding
 * value to the corresponding thermostat in the desired section of the JSON. Select Save to update
 * the property document and send the property message to the device.
 *   {
 *     "properties": {
 *       "desired": {
 *         "thermostat1": {
 *           "targetTemperature": 34.8
 *         },
 *         "thermostat2": {
 *           "targetTemperature": 68.5
 *         }
 *       }
 *     }
 *   }
 *
 * Upon receiving a desired property message, the sample will update the property locally and
 * send a reported property of the same name back to the service. This message will include a set of
 * "ack" values: `ac` for the HTTP-like ack code, `av` for ack version of the property, and an
 * optional `ad` for an ack description.
 *   {
 *     "properties": {
 *       "reported": {
 *         "thermostat1": {
 *           "__t": "c",
 *           "maxTempSinceLastReboot": 38.2,
 *           "targetTemperature": {
 *             "value": 34.8,
 *             "ac": 200,
 *             "av": 27,
 *             "ad": "success"
 *           }
 *         },
 *         "thermostat2": {
 *           "__t": "c",
 *           "maxTempSinceLastReboot": 69.1,
 *           "targetTemperature": {
 *             "value": 68.5,
 *             "ac": 200,
 *             "av": 28,
 *             "ad": "success"
 *           }
 *         }
 *       }
 *     }
 *   }
 *
 * Command: Two device commands are supported in this sample: `reboot` and
 * `getMaxMinReport`. If any other commands are attempted to be invoked, the log will report the
 * command is not found. To invoke a command, select your device's Direct Method tab in the Azure
 * IoT Explorer.
 *
 * - To invoke `reboot` on the Temperature Controller, enter the command name `reboot`. Select
 * Invoke method.
 * - To invoke `getMaxMinReport` on Temperature Sensor 1, enter the command name
 * `thermostat1/getMaxMinReport` along with a payload using an ISO8601 time format. Select Invoke
 * method.
 * - To invoke `getMaxMinReport` on Temperature Sensor 2, enter the command name
 * `thermostat2/getMaxMinReport` along with a payload using an ISO8601 time format. Select Invoke
 * method.
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
 * Telemetry: The Temperature Controller sends a JSON message with the property name `workingSet`
 * and a `double` value for the current working set of the device memory in KiB. Also, each
 * Temperature Sensor sends a JSON message with the property name `temperature` and a `double`
 * value for the current temperature.
 */
int main(void)
{
  create_and_configure_mqtt_client();
  IOT_SAMPLE_LOG_SUCCESS("Client created and configured.");

  connect_mqtt_client_to_iot_hub();
  IOT_SAMPLE_LOG_SUCCESS("Client connected to IoT Hub.");

  subscribe_mqtt_client_to_iot_hub_topics();
  IOT_SAMPLE_LOG_SUCCESS("Client subscribed to IoT Hub topics.");

  // Initializations
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      pnp_mqtt_message_init(&publish_message), "Failed to initialize pnp_mqtt_message");

  initialize_components();
  IOT_SAMPLE_LOG_SUCCESS("Client initialized all components.");

  // Messaging
  send_device_info();
  send_serial_number();
  request_all_properties();
  receive_messages();

  disconnect_mqtt_client_from_iot_hub();
  IOT_SAMPLE_LOG_SUCCESS("Client disconnected from IoT Hub.");

  return 0;
}

static void create_and_configure_mqtt_client(void)
{
  // Reads in environment variables set by user for purposes of running sample.
  iot_sample_read_environment_variables(SAMPLE_TYPE, SAMPLE_NAME, &env_vars);

  // Build an MQTT endpoint c-string.
  char mqtt_endpoint_buffer[128];
  iot_sample_create_mqtt_endpoint(
      SAMPLE_TYPE, env_vars.hub_hostname, mqtt_endpoint_buffer, sizeof(mqtt_endpoint_buffer));

  // Initialize the pnp client with the connection options.
  az_iot_pnp_client_options options = az_iot_pnp_client_options_default();
  options.component_names = pnp_device_components;
  options.component_names_length = pnp_components_length;

  int rc = az_iot_pnp_client_init(
      &pnp_client, env_vars.hub_hostname, env_vars.hub_device_id, model_id, &options);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to initialize pnp client");

  // Get the MQTT client id used for the MQTT connection.
  char mqtt_client_id_buffer[128];

  rc = az_iot_pnp_client_get_client_id(
      &pnp_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get MQTT client id");

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
  // Get the MQTT client username.
  az_result rc = az_iot_pnp_client_get_user_name(
      &pnp_client, mqtt_client_username_buffer, sizeof(mqtt_client_username_buffer), NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get MQTT client username");

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
  // Messages received on the command topic will be commands to be invoked.
  int rc = MQTTClient_subscribe(
      mqtt_client, AZ_IOT_PNP_CLIENT_COMMANDS_SUBSCRIBE_TOPIC, IOT_SAMPLE_MQTT_SUBSCRIBE_QOS);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to subscribe to the command topic: MQTTClient return code %d.", rc);
    exit(rc);
  }

  // Messages received on the property PATCH topic will be updates to the desired properties.
  rc = MQTTClient_subscribe(
      mqtt_client, AZ_IOT_PNP_CLIENT_PROPERTY_PATCH_SUBSCRIBE_TOPIC, IOT_SAMPLE_MQTT_SUBSCRIBE_QOS);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to subscribe to the property PATCH topic: MQTTClient return code %d.", rc);
    exit(rc);
  }

  // Messages received on property response topic will be response statuses from the server.
  rc = MQTTClient_subscribe(
      mqtt_client,
      AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_SUBSCRIBE_TOPIC,
      IOT_SAMPLE_MQTT_SUBSCRIBE_QOS);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to subscribe to the property response topic: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

static void initialize_components(void)
{
  // Initialize thermostats 1 and 2.
  az_result rc = pnp_thermostat_init(&thermostat_1, thermostat_1_name, DEFAULT_START_TEMP_CELSIUS);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to initialize Temperature Sensor 1");

  rc = pnp_thermostat_init(&thermostat_2, thermostat_2_name, DEFAULT_START_TEMP_CELSIUS);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to initialize Temperature Sensor 2");
}

static void send_device_info(void)
{
  // Get the property PATCH topic to send a reported property update.
  az_result rc = az_iot_pnp_client_property_patch_get_publish_topic(
      &pnp_client,
      pnp_mqtt_get_request_id(),
      publish_message.topic,
      publish_message.topic_length,
      NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the property PATCH topic");

  // Build the device info reported property message.
  pnp_device_info_build_reported_property(publish_message.payload, &publish_message.out_payload);

  // Publish the device info reported property update.
  publish_mqtt_message(
      publish_message.topic, publish_message.out_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);
  IOT_SAMPLE_LOG_SUCCESS("Client sent reported property message for device info.");
  IOT_SAMPLE_LOG_AZ_SPAN("Payload:", publish_message.out_payload);
  IOT_SAMPLE_LOG(" "); // Formatting
}

static void send_serial_number(void)
{
  // Get the property PATCH topic to send a reported property update.
  az_result rc = az_iot_pnp_client_property_patch_get_publish_topic(
      &pnp_client,
      pnp_mqtt_get_request_id(),
      publish_message.topic,
      publish_message.topic_length,
      NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the property PATCH topic");

  // Build the serial number reported property message.
  temp_controller_build_serial_number_reported_property(
      publish_message.payload, &publish_message.out_payload);

  // Publish the serial number reported property update.
  publish_mqtt_message(
      publish_message.topic, publish_message.out_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);
  IOT_SAMPLE_LOG_SUCCESS(
      "Client sent `%.*s` reported property message.",
      az_span_size(reported_property_serial_number_name),
      az_span_ptr(reported_property_serial_number_name));
  IOT_SAMPLE_LOG_AZ_SPAN("Payload:", publish_message.out_payload);
  IOT_SAMPLE_LOG(" "); // Formatting
}

static void request_all_properties(void)
{
  IOT_SAMPLE_LOG("Client requesting device property document from service.");

  // Get the property document topic to publish the property document request.
  az_result rc = az_iot_pnp_client_property_document_get_publish_topic(
      &pnp_client,
      pnp_mqtt_get_request_id(),
      publish_message.topic,
      publish_message.topic_length,
      NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the property document topic");

  // Publish the property document request.
  publish_mqtt_message(publish_message.topic, AZ_SPAN_EMPTY, IOT_SAMPLE_MQTT_PUBLISH_QOS);
  IOT_SAMPLE_LOG(" "); // Formatting
}

static void receive_messages(void)
{
  az_result rc;
  // Continue to receive commands or device property messages while device is operational.
  while (is_device_operational)
  {
    // Send max temp for each thermostat since boot, if needed.
    if (thermostat_1.send_maximum_temperature_property)
    {
      // Get the property PATCH topic to send a reported property update.
      rc = az_iot_pnp_client_property_patch_get_publish_topic(
          &pnp_client,
          pnp_mqtt_get_request_id(),
          publish_message.topic,
          publish_message.topic_length,
          NULL);
      IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the property PATCH topic");

      // Build the maximum temperature reported property message.
      az_span property_name;
      pnp_thermostat_build_maximum_temperature_reported_property(
          &pnp_client,
          &thermostat_1,
          publish_message.payload,
          &publish_message.out_payload,
          &property_name);

      // Publish the maximum temperature reported property update.
      publish_mqtt_message(
          publish_message.topic, publish_message.out_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);
      IOT_SAMPLE_LOG_SUCCESS(
          "Client sent Temperature Sensor 1 the `%.*s` reported property message.",
          az_span_size(property_name),
          az_span_ptr(property_name));
      IOT_SAMPLE_LOG_AZ_SPAN("Payload:", publish_message.out_payload);
      IOT_SAMPLE_LOG(" "); // Formatting

      thermostat_1.send_maximum_temperature_property = false; // Only send again if new max set.
    }

    if (thermostat_2.send_maximum_temperature_property)
    {
      // Get the property PATCH topic to send a reported property update.
      rc = az_iot_pnp_client_property_patch_get_publish_topic(
          &pnp_client,
          pnp_mqtt_get_request_id(),
          publish_message.topic,
          publish_message.topic_length,
          NULL);
      IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the property PATCH topic");

      // Build the maximum temperature reported property message.
      az_span property_name;
      pnp_thermostat_build_maximum_temperature_reported_property(
          &pnp_client,
          &thermostat_2,
          publish_message.payload,
          &publish_message.out_payload,
          &property_name);

      // Publish the maximum temperature reported property update.
      publish_mqtt_message(
          publish_message.topic, publish_message.out_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);

      IOT_SAMPLE_LOG_SUCCESS(
          "Client sent Temperature Sensor 2 the `%.*s` reported property message.",
          az_span_size(property_name),
          az_span_ptr(property_name));
      IOT_SAMPLE_LOG_AZ_SPAN("Payload:", publish_message.out_payload);
      IOT_SAMPLE_LOG(" "); // Formatting

      thermostat_2.send_maximum_temperature_property = false; // Only send again if new max set.
    }

    // Send telemetry messages. No response requested from server.
    send_telemetry_messages();
    IOT_SAMPLE_LOG(" "); // Formatting.

    // Wait for any server-initiated messages.
    receive_mqtt_message();
  }
}

static void disconnect_mqtt_client_from_iot_hub(void)
{
  int rc = MQTTClient_disconnect(mqtt_client, PNP_MQTT_TIMEOUT_DISCONNECT_MS);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to disconnect MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }

  MQTTClient_destroy(&mqtt_client);
}

static void publish_mqtt_message(char const* topic, az_span payload, int qos)
{
  int rc = MQTTClient_publish(
      mqtt_client, topic, az_span_size(payload), az_span_ptr(payload), qos, 0, NULL);

  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to publish message: MQTTClient return code %d", rc);
    exit(rc);
  }
}

static void receive_mqtt_message(void)
{
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* receive_message = NULL;
  static int8_t timeout_counter;

  IOT_SAMPLE_LOG("Waiting for command request or device property message.\n");

  // MQTTCLIENT_SUCCESS or MQTTCLIENT_TOPICNAME_TRUNCATED if a message is received.
  // MQTTCLIENT_SUCCESS can also indicate that the timeout expired, in which case message is NULL.
  // MQTTCLIENT_TOPICNAME_TRUNCATED if the topic contains embedded NULL characters.
  // An error code is returned if there was a problem trying to receive a message.
  int rc = MQTTClient_receive(
      mqtt_client, &topic, &topic_len, &receive_message, PNP_MQTT_TIMEOUT_RECEIVE_MS);
  if ((rc != MQTTCLIENT_SUCCESS) && (rc != MQTTCLIENT_TOPICNAME_TRUNCATED))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to receive message: MQTTClient return code %d.", rc);
    exit(rc);
  }
  else if (receive_message == NULL)
  {
    // Allow up to TIMEOUT_MQTT_RECEIVE_MAX_COUNT before disconnecting.
    if (++timeout_counter >= PNP_MQTT_TIMEOUT_RECEIVE_MAX_MESSAGE_COUNT)
    {
      IOT_SAMPLE_LOG(
          "Receive message timeout expiration count of %d reached.",
          PNP_MQTT_TIMEOUT_RECEIVE_MAX_MESSAGE_COUNT);
      is_device_operational = false;
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

    on_message_received(topic, topic_len, receive_message);
    IOT_SAMPLE_LOG(" "); // Formatting

    MQTTClient_freeMessage(&receive_message);
    MQTTClient_free(topic);
  }
}

static void on_message_received(
    char* topic,
    int topic_len,
    MQTTClient_message const* receive_message)
{
  az_result rc;

  az_span const topic_span = az_span_create((uint8_t*)topic, topic_len);
  az_span const message_span
      = az_span_create((uint8_t*)receive_message->payload, receive_message->payloadlen);

  az_iot_pnp_client_property_response property_response;
  az_iot_pnp_client_command_request command_request;

  // Parse the incoming message topic and handle appropriately.
  rc = az_iot_pnp_client_property_parse_received_topic(&pnp_client, topic_span, &property_response);
  if (az_result_succeeded(rc))
  {
    IOT_SAMPLE_LOG_SUCCESS("Client received a valid property topic response.");
    IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
    IOT_SAMPLE_LOG_AZ_SPAN("Payload:", message_span);
    IOT_SAMPLE_LOG("Status: %d", property_response.status);

    handle_device_property_message(receive_message, &property_response);
  }
  else
  {
    rc = az_iot_pnp_client_commands_parse_received_topic(&pnp_client, topic_span, &command_request);
    if (az_result_succeeded(rc))
    {
      IOT_SAMPLE_LOG_SUCCESS("Client received a valid command topic response.");
      IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
      IOT_SAMPLE_LOG_AZ_SPAN("Payload:", message_span);

      handle_command_request(receive_message, &command_request);
    }
    else
    {
      IOT_SAMPLE_LOG_ERROR("Message from unknown topic: az_result return code 0x%08x.", rc);
      IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
      exit(rc);
    }
  }
}

static void process_property_message(
    az_span property_message_span,
    az_iot_pnp_client_property_response_type response_type)
{
  az_result rc = az_iot_pnp_client_property_patch_get_publish_topic(
      &pnp_client,
      pnp_mqtt_get_request_id(),
      publish_message.topic,
      publish_message.topic_length,
      NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the property PATCH topic");

  az_json_reader jr;
  az_span component_name;
  int32_t version = 0;
  rc = az_json_reader_init(&jr, property_message_span, NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Could not initialize the json reader");

  rc = az_iot_pnp_client_property_get_property_version(&pnp_client, &jr, response_type, &version);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Could not get the property version");

  rc = az_json_reader_init(&jr, property_message_span, NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Could not initialize the json reader");

  while (az_result_succeeded(
      rc = az_iot_pnp_client_property_get_next_component_property(
          &pnp_client, &jr, response_type, &component_name)))
  {
    if (rc == AZ_OK)
    {
      if (az_span_is_content_equal(component_name, thermostat_1_name))
      {
        rc = pnp_thermostat_process_property_update(
            &pnp_client,
            &thermostat_1,
            &jr,
            version,
            publish_message.payload,
            &publish_message.out_payload);
        if (az_result_succeeded(rc))
        {
          // Send response to the updated property.
          publish_mqtt_message(
              publish_message.topic, publish_message.out_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);
          IOT_SAMPLE_LOG_SUCCESS("Client sent Temperature Sensor 1 reported property message:");
          IOT_SAMPLE_LOG_AZ_SPAN("Payload:", publish_message.out_payload);
        }
      }
      else if (az_span_is_content_equal(component_name, thermostat_2_name))
      {
        rc = pnp_thermostat_process_property_update(
            &pnp_client,
            &thermostat_2,
            &jr,
            version,
            publish_message.payload,
            &publish_message.out_payload);
        if (az_result_succeeded(rc))
        {
          // Send response to the updated property.
          publish_mqtt_message(
              publish_message.topic, publish_message.out_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);
          IOT_SAMPLE_LOG_SUCCESS("Client sent Temperature Sensor 2 reported property message:");
          IOT_SAMPLE_LOG_AZ_SPAN("Payload:", publish_message.out_payload);
        }
      }
      else
      {
        IOT_SAMPLE_LOG_ERROR(
            "Temperature Controller does not support writable property \"%.*s\". All writeable "
            "properties are on sub-components.",
            az_span_size(jr.token.slice),
            az_span_ptr(jr.token.slice));

        // Get the property PATCH topic to send a reported property update.
        rc = az_iot_pnp_client_property_patch_get_publish_topic(
            &pnp_client,
            pnp_mqtt_get_request_id(),
            publish_message.topic,
            publish_message.topic_length,
            NULL);
        IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get property PATCH publish topic");

        // Build the root component error reported property message.
        az_json_writer jw;
        IOT_SAMPLE_EXIT_IF_AZ_FAILED(
            az_json_writer_init(&jw, publish_message.payload, NULL),
            "Could not initialize the json writer");

        IOT_SAMPLE_EXIT_IF_AZ_FAILED(
            az_json_writer_append_begin_object(&jw), "Could not append the begin object");

        if (az_span_size(component_name) > 0)
        {
          IOT_SAMPLE_EXIT_IF_AZ_FAILED(
              az_iot_pnp_client_property_builder_begin_component(&pnp_client, &jw, component_name),
              "Could not begin the property component");
        }

        IOT_SAMPLE_EXIT_IF_AZ_FAILED(
            az_iot_pnp_client_property_builder_begin_reported_status(
                &pnp_client,
                &jw,
                jr.token.slice,
                AZ_IOT_STATUS_NOT_FOUND,
                version,
                property_response_failed),
            "Could not begin the property with status");

        IOT_SAMPLE_EXIT_IF_AZ_FAILED(
            az_json_reader_next_token(&jr), "Could not advance to property value");

        IOT_SAMPLE_EXIT_IF_AZ_FAILED(
            append_simple_json_token(&jw, &jr.token), "Could not append the property");

        IOT_SAMPLE_EXIT_IF_AZ_FAILED(
            az_iot_pnp_client_property_builder_end_reported_status(&pnp_client, &jw),
            "Could not end the property with status");

        if (az_span_size(component_name) > 0)
        {
          IOT_SAMPLE_EXIT_IF_AZ_FAILED(
              az_iot_pnp_client_property_builder_end_component(&pnp_client, &jw),
              "Could not end the property component");
        }

        IOT_SAMPLE_EXIT_IF_AZ_FAILED(
            az_json_writer_append_end_object(&jw), "Could not append end the object");

        publish_message.out_payload = az_json_writer_get_bytes_used_in_destination(&jw);

        // Send error response to the updated property.
        publish_mqtt_message(
            publish_message.topic, publish_message.out_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);
        IOT_SAMPLE_LOG_SUCCESS(
            "Client sent Temperature Controller error status reported property message:");
        IOT_SAMPLE_LOG_AZ_SPAN("Payload:", publish_message.out_payload);

        // Advance to next property name
        IOT_SAMPLE_EXIT_IF_AZ_FAILED(
            az_json_reader_next_token(&jr), "Could not move to next property name");
      }
    }
    else
    {
      IOT_SAMPLE_LOG_ERROR("Failed to update a property: az_result return code 0x%08x.", rc);
      exit(rc);
    }
  }
}

static void handle_device_property_message(
    MQTTClient_message const* receive_message,
    az_iot_pnp_client_property_response const* property_response)
{
  az_span const message_span
      = az_span_create((uint8_t*)receive_message->payload, receive_message->payloadlen);

  // Invoke appropriate action per response type (3 types only).
  switch (property_response->response_type)
  {
    // A response from a property GET publish message with the property document as a payload.
    case AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET:
      IOT_SAMPLE_LOG("Message Type: GET");
      (void)process_property_message(message_span, property_response->response_type);
      break;

    // An update to the desired properties with the properties as a payload.
    case AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES:
      IOT_SAMPLE_LOG("Message Type: Desired Properties");
      (void)process_property_message(message_span, property_response->response_type);

      break;

    // A response from a property reported properties publish message.
    case AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_REPORTED_PROPERTIES:
      IOT_SAMPLE_LOG("Message Type: Reported Properties");
      break;
  }
}

static void handle_command_request(
    MQTTClient_message const* receive_message,
    az_iot_pnp_client_command_request const* command_request)
{
  az_span const message_span
      = az_span_create((uint8_t*)receive_message->payload, receive_message->payloadlen);
  az_iot_status status = AZ_IOT_STATUS_UNKNOWN;

  // Invoke command and retrieve status and response payload to send to server.
  if (az_span_size(command_request->component_name) == 0)
  {
    if (az_result_succeeded(temp_controller_process_command_request(
            command_request->command_name,
            message_span,
            publish_message.payload,
            &publish_message.out_payload,
            &status)))
    {
      IOT_SAMPLE_LOG_AZ_SPAN(
          "Client invoked command on Temperature Controller:", command_request->command_name);
    }
  }
  else if (az_span_is_content_equal(thermostat_1.component_name, command_request->component_name))
  {
    if (pnp_thermostat_process_command_request(
            &thermostat_1,
            command_request->command_name,
            message_span,
            publish_message.payload,
            &publish_message.out_payload,
            &status))
    {
      IOT_SAMPLE_LOG_AZ_SPAN(
          "Client invoked command on Temperature Sensor 1:", command_request->command_name);
    }
  }
  else if (az_span_is_content_equal(thermostat_2.component_name, command_request->component_name))
  {
    if (pnp_thermostat_process_command_request(
            &thermostat_2,
            command_request->command_name,
            message_span,
            publish_message.payload,
            &publish_message.out_payload,
            &status))
    {
      IOT_SAMPLE_LOG_AZ_SPAN(
          "Client invoked command on Temperature Sensor 2:", command_request->command_name);
    }
  }
  else
  {
    IOT_SAMPLE_LOG_AZ_SPAN("Command not supported:", command_request->command_name);
    publish_message.out_payload = command_empty_response_payload;
    status = AZ_IOT_STATUS_NOT_FOUND;
  }

  // Get the commands response topic to publish the command response.
  az_result rc = az_iot_pnp_client_commands_response_get_publish_topic(
      &pnp_client,
      command_request->request_id,
      (uint16_t)status,
      publish_message.topic,
      publish_message.topic_length,
      NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the commands response topic");

  // Publish the command response.
  publish_mqtt_message(
      publish_message.topic, publish_message.out_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);
  IOT_SAMPLE_LOG_SUCCESS("Client published command response.");
  IOT_SAMPLE_LOG("Status: %d", status);
  IOT_SAMPLE_LOG_AZ_SPAN("Payload:", publish_message.out_payload);
}

static void send_telemetry_messages(void)
{
  // Temperature Sensor 1
  // Get the telemetry topic to publish the telemetry message.
  az_result rc = az_iot_pnp_client_telemetry_get_publish_topic(
      &pnp_client,
      thermostat_1.component_name,
      NULL,
      publish_message.topic,
      publish_message.topic_length,
      NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Unable to get the telemetry topic");

  // Build the telemetry message.
  pnp_thermostat_build_telemetry_message(
      &thermostat_1, publish_message.payload, &publish_message.out_payload);

  // Publish the telemetry message.
  publish_mqtt_message(
      publish_message.topic, publish_message.out_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);
  IOT_SAMPLE_LOG_SUCCESS("Client published the telemetry message for Temperature Sensor 1.");
  IOT_SAMPLE_LOG_AZ_SPAN("Payload:", publish_message.out_payload);

  // Temperature Sensor 2
  // Get the telemetry topic to publish the telemetry message.
  rc = az_iot_pnp_client_telemetry_get_publish_topic(
      &pnp_client,
      thermostat_2.component_name,
      NULL,
      publish_message.topic,
      publish_message.topic_length,
      NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Unable to get the telemetry topic");

  // Build the telemetry message.
  pnp_thermostat_build_telemetry_message(
      &thermostat_2, publish_message.payload, &publish_message.out_payload);

  // Publish the telemetry message.
  publish_mqtt_message(
      publish_message.topic, publish_message.out_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);
  IOT_SAMPLE_LOG_SUCCESS("Client published the telemetry message for Temperature Sensor 2.");
  IOT_SAMPLE_LOG_AZ_SPAN("Payload:", publish_message.out_payload);

  // Temperature Controller
  // Get the telemetry topic to publish the telemetry message.
  rc = az_iot_pnp_client_telemetry_get_publish_topic(
      &pnp_client, AZ_SPAN_EMPTY, NULL, publish_message.topic, publish_message.topic_length, NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the telemetry topic");

  // Build the telemetry message.
  temp_controller_build_telemetry_message(publish_message.payload, &publish_message.out_payload);

  // Publish the telemetry message.
  publish_mqtt_message(
      publish_message.topic, publish_message.out_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);
  IOT_SAMPLE_LOG_SUCCESS("Client published the telemetry message for the Temperature Controller.");
  IOT_SAMPLE_LOG_AZ_SPAN("Payload:", publish_message.out_payload);
}

static void temp_controller_build_telemetry_message(az_span payload, az_span* out_payload)
{
  int32_t working_set_ram_in_kibibytes = rand() % 128;

  az_json_writer jr;

  const char* const log = "Failed to build telemetry payload";

  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_init(&jr, payload, NULL), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jr), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jr, telemetry_working_set_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_int32(&jr, working_set_ram_in_kibibytes), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jr), log);

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jr);
}

static void temp_controller_build_serial_number_reported_property(
    az_span payload,
    az_span* out_payload)
{
  az_json_writer jw;

  const char* const log = "Failed to build reported property payload";

  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_init(&jw, payload, NULL), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, reported_property_serial_number_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_string(&jw, property_reported_serial_number_property_value), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), log);

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);
}

static bool temp_controller_process_command_request(
    az_span command_name,
    az_span command_received_payload,
    az_span payload,
    az_span* out_payload,
    az_iot_status* out_status)
{
  (void)command_received_payload; // Not used
  (void)payload; // Not used

  if (az_span_is_content_equal(command_reboot_name, command_name))
  {
    // Invoke command.
    temp_controller_invoke_reboot();
    *out_payload = command_empty_response_payload;
    *out_status = AZ_IOT_STATUS_OK;
  }
  else // Unsupported command
  {
    *out_payload = command_empty_response_payload;
    *out_status = AZ_IOT_STATUS_NOT_FOUND;
    IOT_SAMPLE_LOG_AZ_SPAN("Command not supported on Temperature Controller:", command_name);
    return false;
  }

  return true;
}

static void temp_controller_invoke_reboot(void)
{
  IOT_SAMPLE_LOG("Client invoking reboot command on Temperature Controller.");
}

static az_result append_simple_json_token(az_json_writer* jw, az_json_token* value)
{
  char const* const log = "Failed to append json token";

  az_json_token value_token = *(az_json_token*)value;

  switch (value_token.kind)
  {
    case AZ_JSON_TOKEN_STRING:
      IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_string(jw, value->slice), log);
      break;
    default:
      IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_json_text(jw, value->slice), log);
      break;
  }

  return AZ_OK;
}

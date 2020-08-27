// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

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
#include <azure/iot/az_iot_hub_client.h>

#include "sample_pnp.h"
#include "sample_pnp_device_info_component.h"
#include "sample_pnp_mqtt_component.h"
#include "sample_pnp_thermostat_component.h"

#define SAMPLE_TYPE PAHO_IOT_HUB
#define SAMPLE_NAME PAHO_IOT_HUB_PNP_COMPONENT_SAMPLE

#define TELEMETRY_SEND_INTERVAL 1
#define TIMEOUT_MQTT_RECEIVE_MAX_MESSAGE_COUNT 3
#define TIMEOUT_MQTT_RECEIVE_MS (8 * 1000)
#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)
#define TIMEOUT_MQTT_WAIT_FOR_COMPLETION_MS 1000

#define DEFAULT_START_TEMP_AVG_COUNT 1
#define DEFAULT_START_TEMP_CELSIUS 22.0
#define DOUBLE_DECIMAL_PLACE_DIGITS 2
#define MQTT_PUBLISH_QOS 0

bool is_device_operational = true;

// * PnP Values *
// The model id is the JSON document (also called the Digital Twins Model Identifier or DTMI)
// which defines the capability of your device. The functionality of the device should match what
// is described in the corresponding DTMI. Should you choose to program your own PnP capable device,
// the functionality would need to match the DTMI and you would need to update the below 'model_id'.
// Please see the sample README for more information on this DTMI.
static const az_span model_id
    = AZ_SPAN_LITERAL_FROM_STR("dtmi:com:example:TemperatureController;1");

// Components
static pnp_thermostat_component thermostat_1;
static pnp_thermostat_component thermostat_2;
static const az_span thermostat_1_name = AZ_SPAN_LITERAL_FROM_STR("thermostat1");
static const az_span thermostat_2_name = AZ_SPAN_LITERAL_FROM_STR("thermostat2");
static const az_span device_info_name = AZ_SPAN_LITERAL_FROM_STR("deviceInformation");
static const az_span* pnp_components[]
    = { &thermostat_1_name, &thermostat_2_name, &device_info_name };
static const int32_t pnp_components_num = sizeof(pnp_components) / sizeof(pnp_components[0]);

// IoT Hub Device Twin Values
static const az_span reported_serial_num_property_name = AZ_SPAN_LITERAL_FROM_STR("serialNumber");
static az_span reported_serial_num_property_value = AZ_SPAN_LITERAL_FROM_STR("ABCDEFG");
static const az_span property_response_description_failed = AZ_SPAN_LITERAL_FROM_STR("failed");

// IoT Hub Method (Command) Values
static const az_span reboot_command_name = AZ_SPAN_LITERAL_FROM_STR("reboot");
static const az_span empty_response_payload = AZ_SPAN_LITERAL_FROM_STR("{}");
static char property_scratch_buffer[64];

// IoT Hub Telemetry Values
static const az_span working_set_name = AZ_SPAN_LITERAL_FROM_STR("workingSet");

static iot_sample_environment_variables env_vars;
static az_iot_hub_client hub_client;
static MQTTClient mqtt_client;
static char mqtt_client_username_buffer[512];
static pnp_mqtt_message mqtt_message;

//
// Functions
//
static void create_and_configure_mqtt_client(void);
static void connect_mqtt_client_to_iot_hub(void);
static void subscribe_mqtt_client_to_iot_hub_topics(void);
static void initialize_components(void);
static void send_device_info(void);
static void send_device_serial_number(void);
static void request_device_twin_document(void);
static void receive_messages(void);
static void disconnect_mqtt_client_from_iot_hub(void);

static void mqtt_publish_message(const char* topic, az_span payload, int qos);
static void mqtt_receive_message(void);
static void on_message_received(char* topic, int topic_len, const MQTTClient_message* message);

// Device Twin functions
static void handle_device_twin_message(
    const az_span twin_message_span,
    const az_iot_hub_client_twin_response* twin_response);

// Command functions
static void handle_command_message(
    const az_span command_message_span,
    const az_iot_hub_client_method_request* command_request);
static az_result temp_controller_process_command(
    const az_iot_hub_client_method_request* command_request,
    az_span component_name,
    az_span command_name,
    az_span command_payload,
    pnp_mqtt_message* mqtt_message,
    az_iot_status* status);

// Telemetry functions
static void send_telemetry_messages(void);
static az_result temp_controller_get_telemetry_message(pnp_mqtt_message* message);

// Callbacks
static az_result append_string_callback(az_json_writer* jw, void* value);
static az_result append_json_token_callback(az_json_writer* jw, void* value);
static void property_callback(
    az_span component_name,
    const az_json_token* property_name,
    az_json_reader property_value,
    int32_t version,
    void* user_context_callback);

/*
 * This sample extends the IoT Hub Plug and Play Sample above to mimic a Temperature Controller
 * and connects the IoT Plug and Play enabled device (the Temperature Controller) with the Digital
 * Twin Model ID (DTMI). If a timeout occurs while waiting for a message from the Azure IoT
 * Explorer, the sample will continue. If TIMEOUT_MQTT_RECEIVE_MAX_COUNT timeouts occur
 * consecutively, the sample will disconnect. X509 self-certification is used.
 *
 * This Temperature Controller is made up of the following components:
 * - Device Info
 * - Temperature Sensor 1
 * - Temperature Sensor 2
 *
 * To interact with this sample, you must use the Azure IoT Explorer. The capabilities are Device
 * Twin, Direct Method (Command), and Telemetry:
 *
 * Device Twin: The following device twin properties are supported in this sample:
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
 * properties to the service.  You will see the following in the device twin JSON.
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
 * To send a Temperature Sensor device twin desired property message, select your device's Device
 * Twin tab in the Azure IoT Explorer. Add the property targetTemperature along with a corresponding
 * value to the corresponding thermostat in the desired section of the JSON. Select Save to update
 * the twin document and send the twin message to the device.
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
 * Upon receiving a desired property message, the sample will update the twin property locally and
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
 * Direct Method (Command): Two device commnds are supported in this sample: `reboot` and
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
 * Telemetry: The Temperature Controller sends a JSON message with the property name
 * `workingSet` and a `double` value for the current working set of the device memory in KiB.  Also,
 * each Temperature Sensor sends a JSON message with the property name `temperature` and a `double`
 * value for the current temperature.
 */
int main(void)
{
  create_and_configure_mqtt_client();
  LOG_SUCCESS("Client created and configured.");

  connect_mqtt_client_to_iot_hub();
  LOG_SUCCESS("Client connected to IoT Hub.");

  subscribe_mqtt_client_to_iot_hub_topics();
  LOG_SUCCESS("Client subscribed to IoT Hub topics.");

  pnp_mqtt_message_init(&mqtt_message);

  initialize_components();
  LOG_SUCCESS("Client initialized components.");

  send_device_info();
  send_device_serial_number();
  request_device_twin_document();

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

static void initialize_components(void)
{
  az_result rc;

  // Initialize thermostats 1 and 2.
  if (az_failed(
          rc = pnp_thermostat_init(&thermostat_1, thermostat_1_name, DEFAULT_START_TEMP_CELSIUS)))
  {
    LOG_ERROR("Could not initialize Temperature Sensor 1: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  if (az_failed(
          rc = pnp_thermostat_init(&thermostat_2, thermostat_2_name, DEFAULT_START_TEMP_CELSIUS)))
  {
    LOG_ERROR("Could not initialize Temperature Sensor 2: az_result return code 0x%08x.", rc);
    exit(rc);
  }
}

static void send_device_info(void)
{
  // Get the Twin Patch topic to send a reported property update and build the device info
  // reported property message.
  az_result rc;
  if (az_failed(rc = pnp_device_info_get_report_data(&hub_client, &mqtt_message)))
  {
    LOG_ERROR(
        "Failed to get Twin Patch publish topic or build `device info` reported property payload: "
        "az_result return code 0x%08x.",
        rc);
    exit(rc);
  }

  // Publish the device info reported property update.
  mqtt_publish_message(mqtt_message.topic, mqtt_message.out_payload_span, MQTT_PUBLISH_QOS);
  LOG_SUCCESS("Client sent `device info` reported property message.");
  LOG_AZ_SPAN("Payload:", mqtt_message.out_payload_span);
  LOG(" "); // Formatting.

  // Receive the response from the server.
  mqtt_receive_message();
}

static void send_device_serial_number(void)
{
  az_result rc;

  // Get the Twin Patch topic to send a reported property update.
  if (az_failed(
          rc = az_iot_hub_client_twin_patch_get_publish_topic(
              &hub_client, get_request_id(), mqtt_message.topic, mqtt_message.topic_length, NULL)))
  {
    LOG_ERROR("Failed to get Twin Patch publish topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Build the serial number reported property message.
  if (az_failed(
          rc = pnp_create_reported_property(
              mqtt_message.payload_span,
              AZ_SPAN_NULL,
              reported_serial_num_property_name,
              append_string_callback,
              (void*)&reported_serial_num_property_value,
              &mqtt_message.out_payload_span)))
  {
    LOG_ERROR(
        "Failed to build `serial number` reported property payload: az_result return code 0x%08x.",
        rc);
    exit(rc);
  }

  // Publish the serial number reported property update.
  mqtt_publish_message(mqtt_message.topic, mqtt_message.out_payload_span, MQTT_PUBLISH_QOS);
  LOG_SUCCESS("Client sent `serial number` reported property message.");
  LOG_AZ_SPAN("Payload:", mqtt_message.out_payload_span);
  LOG(" "); // Formatting.

  // Receive the response from the server.
  mqtt_receive_message();
}

static void request_device_twin_document(void)
{
  az_result rc;

  // Get the Twin Document topic to publish the twin document request.
  if (az_failed(
          rc = az_iot_hub_client_twin_document_get_publish_topic(
              &hub_client, get_request_id(), mqtt_message.topic, mqtt_message.topic_length, NULL)))
  {
    LOG_ERROR("Failed to get Twin Document publish topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Publish the twin document request.
  mqtt_publish_message(mqtt_message.topic, AZ_SPAN_NULL, MQTT_PUBLISH_QOS);
  LOG_SUCCESS("Client requested twin document.");
  LOG(" "); // Formatting.

  // Receive the response from the server.
  mqtt_receive_message();
}

static void receive_messages(void)
{
  // Continue to receive commands or device twin messages while device is operational.
  while (is_device_operational)
  {
    // Send max temp for each thermostat since boot if needed.
    if (pnp_thermostat_get_max_temp_report(&hub_client, &thermostat_1, &mqtt_message))
    {
      mqtt_publish_message(mqtt_message.topic, mqtt_message.out_payload_span, MQTT_PUBLISH_QOS);
      LOG_SUCCESS("Client sent Temperature Sensor 1 the `maxTempSinceLastReboot` reported property "
                  "message.");
      LOG_AZ_SPAN("Payload:", mqtt_message.out_payload_span);

      // Receive the response from the server.
      mqtt_receive_message();
    }

    if (pnp_thermostat_get_max_temp_report(&hub_client, &thermostat_2, &mqtt_message))
    {
      mqtt_publish_message(mqtt_message.topic, mqtt_message.out_payload_span, MQTT_PUBLISH_QOS);
      LOG_SUCCESS("Client sent Temperature Sensor 2 the `maxTempSinceLastReboot` reported property "
                  "message.");
      LOG_AZ_SPAN("Payload:", mqtt_message.out_payload_span);

      // Receive the response from the server.
      mqtt_receive_message();
    }

    // Send telemetry messages.  No response requested from server.
    send_telemetry_messages();

    // Wait for any server-initiated messages.
    mqtt_receive_message();
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
}

static void mqtt_receive_message(void)
{
  int rc;
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;
  static int8_t timeout_counter;

  LOG("Waiting for Command or Device Twin message.\n");

  if (((rc = MQTTClient_receive(mqtt_client, &topic, &topic_len, &message, TIMEOUT_MQTT_RECEIVE_MS))
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
      is_device_operational = false;
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
  // Invoke appropriate action per response type (3 types only).
  switch (twin_response->response_type)
  {
    // A response from a twin GET publish message with the twin document as a payload.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET:
      LOG("Message Type: GET");
      (void)pnp_process_device_twin_message(
          twin_message_span, false, pnp_components, pnp_components_num, property_callback, NULL);
      break;

    // An update to the desired properties with the properties as a payload.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
      LOG("Message Type: Desired Properties");
      (void)pnp_process_device_twin_message(
          twin_message_span, true, pnp_components, pnp_components_num, property_callback, NULL);
      break;

    // A response from a twin reported properties publish message.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
      LOG("Message Type: Reported Properties");
      break;
  }
}

static void handle_command_message(
    az_span command_message_span,
    const az_iot_hub_client_method_request* command_request)
{
  az_result rc;
  az_span command_name;
  az_span component_name;
  az_span command_response_payload;
  az_iot_status status = AZ_IOT_STATUS_UNKNOWN;

  pnp_parse_command_name(command_request->name, &component_name, &command_name);

  // Invoke command and retrieve response to send to server.
  if (az_succeeded(pnp_thermostat_process_command(
          &hub_client,
          &thermostat_1,
          command_request,
          component_name,
          command_name,
          command_message_span,
          &mqtt_message,
          &status)))
  {
    LOG_AZ_SPAN("Client invoked command on Temperature Sensor 1:", command_name);
    command_response_payload = mqtt_message.out_payload_span;
  }
  else if (az_succeeded(pnp_thermostat_process_command(
               &hub_client,
               &thermostat_2,
               command_request,
               component_name,
               command_name,
               command_message_span,
               &mqtt_message,
               &status)))
  {
    LOG_AZ_SPAN("Client invoked command on Temperature Sensor 2:", command_name);
    command_response_payload = mqtt_message.out_payload_span;
  }
  else if (az_succeeded(temp_controller_process_command(
               command_request,
               component_name,
               command_name,
               command_message_span,
               &mqtt_message,
               &status)))
  {
    LOG_AZ_SPAN("Client invoked command on temperature controller:", command_name);
    command_response_payload = mqtt_message.out_payload_span;
  }
  else
  {
    LOG_AZ_SPAN("Command not supported:", command_request->name);
    status = AZ_IOT_STATUS_NOT_FOUND;

    // Get the Methods response topic to publish the command response.
    if (az_failed(
            rc = az_iot_hub_client_methods_response_get_publish_topic(
                &hub_client,
                command_request->request_id,
                (uint16_t)status,
                mqtt_message.topic,
                mqtt_message.topic_length,
                NULL)))
    {
      LOG_ERROR("Failed to get Methods response publish topic: az_result return code 0x%08x.", rc);
      exit(rc);
    }
    command_response_payload = empty_response_payload;
  }

  // Publish the command response
  mqtt_publish_message(mqtt_message.topic, command_response_payload, MQTT_PUBLISH_QOS);
  LOG_SUCCESS("Client published command response.");
  LOG("Status: %d", status);
  LOG_AZ_SPAN("Payload:", command_response_payload);
}

static az_result temp_controller_process_command(
    const az_iot_hub_client_method_request* command_request,
    az_span component_name,
    az_span command_name,
    az_span command_payload,
    pnp_mqtt_message* message,
    az_iot_status* status)
{
  az_result rc;

  (void)command_payload;

  if (az_span_size(component_name) == 0
      && az_span_is_content_equal(reboot_command_name, command_name))
  {
    *status = AZ_IOT_STATUS_OK;

    // Get the Methods response topic to publish the command response.
    if (az_failed(
            rc = az_iot_hub_client_methods_response_get_publish_topic(
                &hub_client,
                command_request->request_id,
                (uint16_t)*status,
                message->topic,
                message->topic_length,
                NULL)))
    {
      LOG_ERROR("Failed to get Methods response publish topic: az_result return code 0x%08x.", rc);
      return rc;
    }

    message->out_payload_span = empty_response_payload;
  }
  else
  {
    rc = AZ_ERROR_ITEM_NOT_FOUND;
  }

  return rc;
}

static void send_telemetry_messages(void)
{
  az_result rc;

  // Get the Telemetry topic to publish the telemetry message and build the telemetry message.
  if (az_failed(
          rc = pnp_thermostat_get_telemetry_message(&hub_client, &thermostat_1, &mqtt_message)))
  {
    LOG_ERROR(
        "Failed to get Telemetry publish topic or build telemetry message: az_result return code "
        "0x%08x.",
        rc);
    exit(rc);
  }

  // Publish the telemetry message.
  mqtt_publish_message(mqtt_message.topic, mqtt_message.out_payload_span, MQTT_PUBLISH_QOS);
  LOG_SUCCESS("Client sent telemetry message for Temperature Sensor 1:");
  LOG_AZ_SPAN("Payload:", mqtt_message.out_payload_span);

  // Get the Telemetry topic to publish the telemetry message and build the telemetry message.
  if (az_failed(
          rc = pnp_thermostat_get_telemetry_message(&hub_client, &thermostat_2, &mqtt_message)))
  {
    LOG_ERROR(
        "Failed to get Telemetry publish topic or build telemetry message: az_result return code "
        "0x%08x.",
        rc);
    exit(rc);
  }

  // Publish the telemetry message.
  mqtt_publish_message(mqtt_message.topic, mqtt_message.out_payload_span, MQTT_PUBLISH_QOS);
  LOG_SUCCESS("Client sent telemetry message for Temperature Sensor 2:");
  LOG_AZ_SPAN("Payload:", mqtt_message.out_payload_span);

  // Get the Telemetry topic to publish the telemetry message and build the telemetry message.
  if (az_failed(rc = temp_controller_get_telemetry_message(&mqtt_message)))
  {
    LOG_ERROR(
        "Failed to get Telemetry publish topic or build telemetry message: az_result return code "
        "0x%08x.",
        rc);
    exit(rc);
  }

  // Publish the telemetry message.
  mqtt_publish_message(mqtt_message.topic, mqtt_message.out_payload_span, MQTT_PUBLISH_QOS);
  LOG_SUCCESS("Client sent telemetry message for Temperature Controller:");
  LOG_AZ_SPAN("Payload:", mqtt_message.out_payload_span);
}

static az_result temp_controller_get_telemetry_message(pnp_mqtt_message* message)
{
  az_result rc;

  // Get the Telemetry topic to publish the telemetry messages.
  if (az_failed(
          rc = pnp_get_telemetry_topic(
              &hub_client, NULL, AZ_SPAN_NULL, message->topic, message->topic_length, NULL)))
  {
    LOG_ERROR("Failed to get pnp Telemetry publish topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  int32_t working_set_ram_in_kibibytes = rand() % 128;

  // Build the telemetry message.
  az_json_writer jw;
  AZ_RETURN_IF_FAILED(az_json_writer_init(&jw, message->payload_span, NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, working_set_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(&jw, working_set_ram_in_kibibytes));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));
  message->out_payload_span = az_json_writer_get_bytes_used_in_destination(&jw);

  return rc;
}

static az_result append_string_callback(az_json_writer* jw, void* value)
{
  return az_json_writer_append_string(jw, *(az_span*)value);
}

static az_result append_json_token_callback(az_json_writer* jw, void* value)
{
  az_json_token value_token = *(az_json_token*)value;

  double value_as_double;
  int32_t string_length;

  switch (value_token.kind)
  {
    case AZ_JSON_TOKEN_NUMBER:
      AZ_RETURN_IF_FAILED(az_json_token_get_double(&value_token, &value_as_double));
      AZ_RETURN_IF_FAILED(
          az_json_writer_append_double(jw, value_as_double, DOUBLE_DECIMAL_PLACE_DIGITS));
      break;
    case AZ_JSON_TOKEN_STRING:
      AZ_RETURN_IF_FAILED(az_json_token_get_string(
          &value_token, property_scratch_buffer, sizeof(property_scratch_buffer), &string_length));
      AZ_RETURN_IF_FAILED(az_json_writer_append_string(
          jw, az_span_create((uint8_t*)property_scratch_buffer, string_length)));
      break;
    default:
      return AZ_ERROR_ITEM_NOT_FOUND;
  }

  return AZ_OK;
}

static void property_callback(
    az_span component_name,
    const az_json_token* property_name,
    az_json_reader property_value,
    int32_t version,
    void* user_context_callback)
{
  az_result rc;
  (void)user_context_callback;

  if (az_span_ptr(component_name) == NULL || az_span_size(component_name) == 0)
  {
    LOG_ERROR(
        "Temperature Controller does not support writable property \"%.*s\". All writeable "
        "properties are on sub-components.",
        az_span_size(property_name->slice),
        az_span_ptr(property_name->slice));

    // Get the Twin Patch topic to send a reported property update.
    if (az_failed(
            rc = az_iot_hub_client_twin_patch_get_publish_topic(
                &hub_client,
                get_request_id(),
                mqtt_message.topic,
                mqtt_message.topic_length,
                NULL)))
    {
      LOG_ERROR("Failed to get Twin Patch publish topic: az_result return code 0x%08x.", rc);
      exit(rc);
    }

    // Build the root component error reported property message.
    if (az_failed(
            rc = pnp_create_reported_property_with_status(
                mqtt_message.payload_span,
                component_name,
                property_name->slice,
                append_json_token_callback,
                (void*)&property_value,
                AZ_IOT_STATUS_NOT_FOUND,
                version,
                property_response_description_failed,
                &mqtt_message.out_payload_span)))
    {
      LOG_ERROR(
          "Failed to build Temperature Controller property error payload: az_result return code "
          "0x%08x.",
          rc)
      exit(rc);
    }

    // Send error response to the updated property.
    mqtt_publish_message(mqtt_message.topic, mqtt_message.out_payload_span, MQTT_PUBLISH_QOS);
    LOG_SUCCESS("Client sent Temperature Controller error status reported property message:");
    LOG_AZ_SPAN("Payload:", mqtt_message.out_payload_span);

    // Receive the response from the server.
    mqtt_receive_message();
  }
  else if (az_succeeded(
               rc = pnp_thermostat_process_property_update(
                   &hub_client,
                   &thermostat_1,
                   component_name,
                   property_name,
                   &property_value,
                   version,
                   &mqtt_message)))
  {
    // Send response to the updated property.
    mqtt_publish_message(mqtt_message.topic, mqtt_message.out_payload_span, MQTT_PUBLISH_QOS);
    LOG_SUCCESS("Client sent Temperature Sensor 1 reported property message:");
    LOG_AZ_SPAN("Payload:", mqtt_message.out_payload_span);

    // Receive the response from the server.
    mqtt_receive_message();
  }
  else if (az_succeeded(
               rc = pnp_thermostat_process_property_update(
                   &hub_client,
                   &thermostat_2,
                   component_name,
                   property_name,
                   &property_value,
                   version,
                   &mqtt_message)))
  {
    // Send response to the updated property
    mqtt_publish_message(mqtt_message.topic, mqtt_message.out_payload_span, MQTT_PUBLISH_QOS);
    LOG_SUCCESS("Client sent Temperature Sensor 2 reported property message.");
    LOG_AZ_SPAN("Payload:", mqtt_message.out_payload_span);

    // Receive the response from the server.
    mqtt_receive_message();
  }
  else
  {
    LOG_ERROR("Failed to update a property: az_result return code 0x%08x.", rc);
    exit(rc);
  }
}

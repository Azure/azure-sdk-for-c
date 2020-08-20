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

#include "iot_samples_common.h"

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
#include "sample_pnp_component_mqtt.h"
#include "sample_pnp_device_info_component.h"
#include "sample_pnp_thermostat_component.h"

#define SAMPLE_TYPE PAHO_IOT_HUB
#define SAMPLE_NAME PAHO_IOT_HUB_PNP_COMPONENT_SAMPLE

#define TELEMETRY_SEND_INTERVAL 1
#define TIMEOUT_MQTT_RECEIVE_MAX_COUNT 3
#define TIMEOUT_MQTT_RECEIVE_MS (8 * 1000)
#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)
#define TIMEOUT_MQTT_WAIT_FOR_COMPLETION_MS 1000

#define DEFAULT_START_TEMP_AVG_COUNT 1
#define DEFAULT_START_TEMP_CELSIUS 22.0
#define DOUBLE_DECIMAL_PLACE_DIGITS 2
#define SAMPLE_PUBLISH_QOS 0

bool is_device_operational = true;
static const char iso_spec_time_format[] = "%Y-%m-%dT%H:%M:%S%z"; // ISO8601 Time Format

// * PnP Values *
// The model id is the JSON document (also called the Digital Twins Model Identifier or DTMI)
// which defines the capability of your device. The functionality of the device should match what
// is described in the corresponding DTMI. Should you choose to program your own PnP capable device,
// the functionality would need to match the DTMI and you would need to update the below 'model_id'.
// Please see the sample README for more information on this DTMI.
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:com:example:TemperatureController;1");

static sample_pnp_thermostat_component sample_thermostat_1;
static const az_span sample_thermostat_1_component_name = AZ_SPAN_LITERAL_FROM_STR("thermostat1");
static sample_pnp_thermostat_component sample_thermostat_2;
static const az_span sample_thermostat_2_component_name = AZ_SPAN_LITERAL_FROM_STR("thermostat2");
static const az_span sample_device_info_component = AZ_SPAN_LITERAL_FROM_STR("deviceInformation");
static const az_span* sample_pnp_components[] = { &sample_thermostat_1_component_name,
                                                  &sample_thermostat_2_component_name,
                                                  &sample_device_info_component };
static const int32_t sample_pnp_components_num
    = sizeof(sample_pnp_components) / sizeof(sample_pnp_components[0]);

// Root Component Values
static const az_span working_set_name = AZ_SPAN_LITERAL_FROM_STR("workingSet");
static int32_t working_set_ram_in_kibibytes;
static const az_span serial_number_name = AZ_SPAN_LITERAL_FROM_STR("serialNumber");
static az_span serial_number_value = AZ_SPAN_LITERAL_FROM_STR("ABCDEFG");
static const az_span property_response_description_failed = AZ_SPAN_LITERAL_FROM_STR("failed");

// IoT Hub Command
static const az_span reboot_command_name = AZ_SPAN_LITERAL_FROM_STR("reboot");
static const az_span empty_json_object = AZ_SPAN_LITERAL_FROM_STR("{}");
static char property_scratch_buffer[64];

static sample_pnp_mqtt_message publish_message;

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
static void initialize_components(void);
static void send_device_info(void);
static void send_device_serial_number(void);
static void request_device_twin_document(void);
static void receive_messages(void);
static void disconnect_mqtt_client_from_iot_hub(void);

static void mqtt_publish_message(char* topic, az_span payload, int qos);
static void mqtt_receive_message(void);
static int on_received(char* topicName, int topicLen, MQTTClient_message* message);

// Device Twin functions
static void handle_device_twin_message(
    const az_span twin_message_span,
    const az_iot_hub_client_twin_response* twin_response);

// Command functions
static void handle_command_message(
    const az_span command_message_span,
    const az_iot_hub_client_method_request* command_request);
static az_result temp_controller_process_command(
    az_iot_hub_client_method_request* command_request,
    az_span component_name,
    az_span command_name,
    az_span command_payload,
    sample_pnp_mqtt_message* mqtt_message);

// Telemetry functions
static void send_telemetry_messages(void);

// Callbacks
static az_result append_string(az_json_writer* json_writer, void* value);
static az_result append_json_token(az_json_writer* json_writer, void* value);
static void sample_property_callback(
    az_span component_name,
    az_json_token* property_name,
    az_json_reader property_value,
    int32_t version,
    void* user_context_callback);


int main(void)
{
  create_and_configure_mqtt_client();
  LOG_SUCCESS("Client created and configured.");

  connect_mqtt_client_to_iot_hub();
  LOG_SUCCESS("Client connected to IoT Hub.");

  subscribe_mqtt_client_to_iot_hub_topics();
  LOG_SUCCESS("Client subscribed to IoT Hub topics.");

  initialize_components();
  LOG_SUCCESS("Client initialized components.");

  send_device_info();
  LOG_SUCCESS("Client sent device info to IoT Hub.")

  send_device_serial_number();
  LOG_SUCCESS("Client sent device serial number to IoT Hub.")

  request_device_twin_document();
  LOG_SUCCESS("Client requested twin document.")

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
  if (az_failed(rc = read_environment_variables(SAMPLE_TYPE, SAMPLE_NAME, &env_vars)))
  {
    LOG_ERROR(
        "Failed to read configuration from environment variables: az_result return code 0x%04x.",
        rc);
    exit(rc);
  }

  // Build an MQTT endpoint c-string.
  char mqtt_endpoint_buffer[128];
  if (az_failed(
          rc = create_mqtt_endpoint(
              SAMPLE_TYPE, &env_vars, mqtt_endpoint_buffer, sizeof(mqtt_endpoint_buffer))))
  {
    LOG_ERROR("Failed to create MQTT endpoint: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Initialize the hub client with the connection options.
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.model_id = model_id;
  if (az_failed(
          rc = az_iot_hub_client_init(
              &hub_client, env_vars.hub_hostname, env_vars.hub_device_id, &options)))
  {
    LOG_ERROR("Failed to initialize hub client: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Get the MQTT client id used for the MQTT connection.
  char mqtt_client_id_buffer[128];
  if (az_failed(
          rc = az_iot_hub_client_get_client_id(
              &hub_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT client id: az_result return code 0x%04x.", rc);
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
    LOG_ERROR("Failed to get MQTT client username: az_result return code 0x%04x.", rc);
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

  // Initialize the sample publish message.
  if (az_failed(rc = sample_pnp_mqtt_message_init(&publish_message)))
  {
    LOG_ERROR("Could not initialize publish_message: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Initialize the thermostats 1 and 2.
  if (az_failed(
          rc = sample_pnp_thermostat_init(
              &sample_thermostat_1,
              sample_thermostat_1_component_name,
              DEFAULT_START_TEMP_CELSIUS)))
  {
    LOG_ERROR("Could not initialize thermostat 1: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  if (az_failed(
               rc = sample_pnp_thermostat_init(
                   &sample_thermostat_2,
                   sample_thermostat_2_component_name,
                   DEFAULT_START_TEMP_CELSIUS)))
  {
    LOG_ERROR("Could not initialize thermostat 2: az_result return code 0x%08x.", rc);
    exit(rc);
  }
}

static void send_device_info(void)
{
  // Get the Twin Patch topic to send a reported property update and build the device info
  // reported property message.
  az_result rc;
  if (az_failed(rc = sample_pnp_device_info_get_report_data(&hub_client, &publish_message)))
  {
    LOG_ERROR("Failed to get Twin Patch publish topic or build device info reported property payload: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Publish the device info reported property update.
  mqtt_publish_message(publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);
  LOG_SUCCESS("Client sent device info reported property message:");
  LOG_AZ_SPAN("Payload:", publish_message.out_payload_span);

  // Receive the response from the server.
  mqtt_receive_message();
}

static void send_device_serial_number(void)
{
  az_result rc;

  // Get the Twin Patch topic to send a reported property update.
  if (az_failed(
               rc = az_iot_hub_client_twin_patch_get_publish_topic(
                   &hub_client,
                   get_request_id(),
                   publish_message.topic,
                   publish_message.topic_length,
                   NULL)))
  {
    LOG_ERROR("Failed to get Twin Patch publish topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Build the serial number reported property message.
  if (az_failed(
          rc = sample_pnp_create_reported_property(
              publish_message.payload_span,
              AZ_SPAN_NULL,
              serial_number_name,
              append_string,
              (void*)&serial_number_value,
              &publish_message.out_payload_span)))
  {
    LOG_ERROR("Failed to build serial number reported property payload: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Publish the serial number reported property update.
  mqtt_publish_message(publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);
  LOG_SUCCESS("Client sent serial number reported property message:");
  LOG_AZ_SPAN("Payload:", publish_message.out_payload_span);
}

static void request_device_twin_document(void)
{
  az_result rc;

  LOG("Client requesting device twin document from service.");

  // Get the Twin Document topic to publish the twin document request.
  if (az_failed(
          rc = az_iot_hub_client_twin_document_get_publish_topic(
              &hub_client, get_request_id(), publish_message.topic, publish_message.topic_length, NULL)))
  {
    LOG_ERROR("Failed to get Twin Document publish topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Publish the twin document request.
  mqtt_publish_message(publish_message.topic, AZ_SPAN_NULL, SAMPLE_PUBLISH_QOS);
}

static void receive_messages(void)
{
  // Continue to receive commands or device twin messages while device is operational.
  while (is_device_operational)
  {
    mqtt_receive_message();

    // Send max temp for each component since boot if needed.
    if (sample_pnp_thermostat_get_max_temp_report(&hub_client, &sample_thermostat_1, &publish_message))
    {
      mqtt_publish_message(
          publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);
    }

    if (sample_pnp_thermostat_get_max_temp_report(&hub_client, &sample_thermostat_2, &publish_message))
    {
      mqtt_publish_message(
          publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);
    }

    // Send telemetry messages.
    send_telemetry_messages();
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

static void mqtt_publish_message(char* topic, az_span payload, int qos)
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

static void mqtt_receive_message(void)
{
  int rc;
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;
  uint8_t timeoutCounter = 0;

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
    if (++timeoutCounter >= TIMEOUT_MQTT_RECEIVE_MAX_COUNT)
    {
      LOG("Receive message timeout count of %d reached.", TIMEOUT_MQTT_RECEIVE_MAX_COUNT);
      return;
    }
  }
  else
  {
    LOG_SUCCESS("Client received message from the service.");

    if (rc == MQTTCLIENT_TOPICNAME_TRUNCATED)
    {
      topic_len = (int)strlen(topic);
    }

    timeoutCounter = 0; // Reset.

    on_message_received(topic, topic_len, message);
    LOG(" "); // Formatting.

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topic);
  }
}

static int on_message_received(char* topic, int topic_len, MQTTClient_message* message)
{
  az_result rc;

  az_span topic_span = az_span_create((uint8_t*)topic, topic_len);
  az_span message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  az_iot_hub_client_twin_response twin_response;
  az_iot_hub_client_method_request command_request;

  // Parse the incoming message topic and check which feature it is for.
  if (az_succeeded(rc = az_iot_hub_client_twin_parse_received_topic(&hub_client, topic_span, &twin_response)))
  {
    LOG_SUCCESS("Client received a valid topic response:");
    LOG_AZ_SPAN("Topic:", topic_span);
    LOG_AZ_SPAN("Payload:", message_span);
    LOG("Status: %d", twin_response.status);

    handle_device_twin_message(message_span, &twin_response);
  }
  else if (az_succeeded(rc = az_iot_hub_client_methods_parse_received_topic(
               &hub_client, topic_span, &command_request)))
  {
    LOG_SUCCESS("Client received a valid topic response:");
    LOG_AZ_SPAN("Topic:", topic_span);
    LOG_AZ_SPAN("Payload:", message_span);

    handle_command_message(message_span, &command_request);
  }
  else
  {
    LOG_ERROR("Message from unknown topic: az_result return code 0x%04x.", rc);
    LOG_AZ_SPAN("Topic:", topic_span);
    exit(rc);
  }
}

static void handle_device_twin_message(
    const az_span twin_message_span,
    az_iot_hub_client_twin_response* twin_response)
{
  az_result rc;

  az_json_reader json_reader;
  az_span twin_payload_span;

  if (az_span_size(twin_message_span))
  {
    if (az_failed(rc = az_json_reader_init(&json_reader, twin_message_span, NULL)))
    {
      LOG_ERROR("Could not initialize JSON reader: az_result return code 0x%04x.", rc);
      exit(rc);
    }
  }

  // Invoke appropriate action per response type (3 Types only).
  switch (twin_response->response_type)
  {
    // A response from a twin GET publish message with the twin document as a payload.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET:
      LOG("Type: GET");
      sample_pnp_process_twin_data(
          &json_reader,
          false,
          sample_pnp_components,
          sample_pnp_components_num,
          sample_property_callback,
          NULL);
      break;
    // An update to the desired properties with the properties as a payload.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
      LOG("Type: Desired Properties");
      sample_pnp_process_twin_data(
          &json_reader,
          true,
          sample_pnp_components,
          sample_pnp_components_num,
          sample_property_callback,
          NULL);
      break;

    // A response from a twin reported properties publish message.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
      LOG("Type: Reported Properties");
      break;
  }
}

static void handle_command_message(
    const az_span command_message_span,
    const az_iot_hub_client_method_request* command_request)
{
  az_result rc;
  az_span command_name;
  az_span component_name;
  az_span command_response_payload;
  az_iot_status status = AZ_IOT_STATUS_UNKNOWN;

  if (az_failed(
          rc = pnp_parse_command_name(command_request->name, &component_name, &command_name)))
  {
    LOG_ERROR("Failed to parse command name: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Invoke command and retrieve response to send to server.
  if (az_succeeded(sample_pnp_thermostat_process_command(
                   &hub_client,
                   &sample_thermostat_1,
                   command_request,
                   component_name,
                   command_name,
                   command_message_span,
                   &publish_message,
                   &status)))
  {
    LOG_AZ_SPAN("Client invoked command on thermostat 1:", command_name);
    command_response_payload = publish_message.out_payload_span;
  }
  else if (az_succeeded(sample_pnp_thermostat_process_command(
                   &client,
                   &sample_thermostat_2,
                   command_request,
                   component_name,
                   command_name,
                   command_message_span,
                   &publish_message,
                   &status)))
  {
    LOG_AZ_SPAN("Client invoked command on thermostat 2:", command_name);
    command_response_payload = publish_message.out_payload_span;
  }
  else if (az_succeeded(temp_controller_process_command(
                   command_request,
                   component_name,
                   command_name,
                   command_message_span,
                   &publish_message,
                   &status)))
  {
    LOG_AZ_SPAN("Client invoked command on controller:", command_name);
    command_response_payload = publish_message.out_payload_span;
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
                status,
                publish_message.topic,
                publish_message.topic_length,
                NULL)))
    {
      LOG_ERROR("Failed to get Methods response publish topic: az_result return code 0x%04x.", rc);
      exit(rc);
    }
    command_response_payload = empty_json_object;
  }

  // Publish the command response
  mqtt_publish_message(
        publish_message.topic, command_response_payload, SAMPLE_PUBLISH_QOS);
  LOG_SUCCESS("Client published command response:");
  LOG("Status: %d", status);
  LOG_AZ_SPAN("Payload:", response_payload);

}

static az_result temp_controller_process_command(
    az_iot_hub_client_method_request* command_request,
    az_span component_name,
    az_span command_name,
    az_span command_payload,
    sample_pnp_mqtt_message* mqtt_message,
    az_iot_status* status)
{
  az_result rc;

  (void)command_payload;

  if (az_span_size(component_name) == 0 && az_span_is_content_equal(reboot_command_name, command_name))
  {
    *status = AZ_IOT_STATUS_OK;

    // Get the Methods response topic to publish the command response.
    if (az_failed(
            rc = az_iot_hub_client_methods_response_get_publish_topic(
                &hub_client,
                command_request->request_id,
                status,
                mqtt_message->topic,
                mqtt_message->topic_length,
                NULL)))
    {
      LOG_ERROR("Failed to get Methods response publish topic: az_result return code 0x%04x.", rc);
      return rc;
    }

    mqtt_message->out_payload_span = empty_json_object;
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
          rc = sample_pnp_thermostat_get_telemetry_message(
              &hub_client, &sample_thermostat_1, &publish_message)))
  {
    LOG_ERROR("Failed to get Telemetry publish topic or build telemetry message: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Publish the telemetry message.
  mqtt_publish_message(publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);
  LOG_SUCCESS("Client sent telemetry message for thermostat 1:");
  LOG_AZ_SPAN("Payload:", publish_message.out_payload_span);

  // Get the Telemetry topic to publish the telemetry message and build the telemetry message.
  if (az_failed(
          rc = sample_pnp_thermostat_get_telemetry_message(
              &hub_client, &sample_thermostat_2, &publish_message)))
  {
    LOG_ERROR("Failed to get Telemetry publish topic or build telemetry message: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Publish the telemetry message.
  mqtt_publish_message(publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);
  LOG_SUCCESS("Client sent telemetry message for thermostat 2:");
  LOG_AZ_SPAN("Payload:", publish_message.out_payload_span);

  // Get the Telemetry topic to publish the telemetry message and build the telemetry message.
  if (az_failed(rc = temperature_controller_get_telemetry_message(&publish_message)))
  {
    LOG_ERROR("Failed to get Telemetry publish topic or build telemetry message: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Publish the telemetry message.
  mqtt_publish_message(publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);
  LOG_SUCCESS("Client sent telemetry message for temperature controller:");
  LOG_AZ_SPAN("Payload:", publish_message.out_payload_span);
}

static az_result append_string(az_json_writer* json_writer, void* value)
{
  return az_json_writer_append_string(json_writer, *(az_span*)value);
}

static az_result append_json_token(az_json_writer* json_writer, void* value)
{
  az_json_token value_token = *(az_json_token*)value;

  double value_as_double;
  int32_t string_length;

  switch (value_token.kind)
  {
    case AZ_JSON_TOKEN_NUMBER:
      AZ_RETURN_IF_FAILED(az_json_token_get_double(&value_token, &value_as_double));
      AZ_RETURN_IF_FAILED(
          az_json_writer_append_double(json_writer, value_as_double, DOUBLE_DECIMAL_PLACE_DIGITS));
      break;
    case AZ_JSON_TOKEN_STRING:
      AZ_RETURN_IF_FAILED(az_json_token_get_string(
          &value_token, property_scratch_buffer, sizeof(property_scratch_buffer), &string_length));
      AZ_RETURN_IF_FAILED(az_json_writer_append_string(
          json_writer, az_span_create((uint8_t*)property_scratch_buffer, string_length)));
      break;
    default:
      return AZ_ERROR_ITEM_NOT_FOUND;
  }

  return AZ_OK;
}

static void sample_property_callback(
    az_span component_name,
    az_json_token* property_name,
    az_json_reader property_value,
    int32_t version,
    void* user_context_callback)
{
  (void)user_context_callback;
  if (az_span_ptr(component_name) == NULL || az_span_size(component_name) == 0)
  {
    LOG("Property=%.*s arrived for Control component itself. This does not support writable "
        "properties on it(all properties are on sub-components) ",
        az_span_size(property_name->slice),
        az_span_ptr(property_name->slice));

    az_result err_result;
    if (az_failed(
            err_result = pnp_create_reported_property_with_status(
                publish_message.payload_span,
                component_name,
                property_name->slice,
                append_json_token,
                (void*)&property_value,
                AZ_IOT_STATUS_NOT_FOUND,
                version,
                property_response_description_failed,
                &publish_message.out_payload_span)))
    {
      LOG_ERROR(
          "Could not create root component property error payload: error code = 0x%08x", err_result)
    }
    else
    {
      LOG_SUCCESS("Sending error status for root component property");

      // Send error response to the updated property
      mqtt_publish_message(
          publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);
    }
  }
  else if (az_succeeded(sample_pnp_thermostat_process_property_update(
               &client,
               &sample_thermostat_1,
               component_name,
               property_name,
               &property_value,
               version,
               &publish_message)))
  {
    LOG_SUCCESS("Updated property on thermostat 1");

    // Send response to the updated property
    mqtt_publish_message(
        publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);
  }
  else if (az_succeeded(sample_pnp_thermostat_process_property_update(
               &client,
               &sample_thermostat_2,
               component_name,
               property_name,
               &property_value,
               version,
               &publish_message)))
  {
    LOG_SUCCESS("Updated property on thermostat 2");

    // Send response to the updated property
    mqtt_publish_message(
        publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);
  }
  else
  {
    LOG_ERROR("There was an error updating a property");
  }
}
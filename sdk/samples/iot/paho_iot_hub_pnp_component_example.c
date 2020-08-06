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
#include <time.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#include "sample_pnp.h"
#include "sample_pnp_component_mqtt.h"
#include "sample_pnp_device_info_component.h"
#include "sample_pnp_thermostat_component.h"

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

// Logging with formatting
#define LOG_ERROR(...) \
  { \
    (void)fprintf(stderr, "ERROR:\t\t%s:%s():%d: ", __FILE__, __func__, __LINE__); \
    (void)fprintf(stderr, __VA_ARGS__); \
    (void)fprintf(stderr, "\n"); \
    fflush(stdout); \
    fflush(stderr); \
  }
#define LOG_SUCCESS(...) \
  { \
    (void)printf("SUCCESS:\t"); \
    (void)printf(__VA_ARGS__); \
    (void)printf("\n"); \
  }
#define LOG(...) \
  { \
    (void)printf("\t\t"); \
    (void)printf(__VA_ARGS__); \
    (void)printf("\n"); \
  }
#define LOG_AZ_SPAN(span_description, span) \
  { \
    (void)printf("\t\t%s ", span_description); \
    char* buffer = (char*)az_span_ptr(span); \
    for (int32_t i = 0; i < az_span_size(span); i++) \
    { \
      putchar(*buffer++); \
    } \
    (void)printf("\n"); \
  }

#define TIMEOUT_WAIT_FOR_RECEIVE_MESSAGE_MS (8 * 1000)
#define TIMEOUT_WAIT_FOR_COMPLETION_MS 1000
#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)
#define DEVICE_DO_WORK_SLEEP_MS 2
#define TELEMETRY_SEND_INTERVAL 1
#define DEFAULT_START_TEMP_CELSIUS 22.0
#define DOUBLE_DECIMAL_PLACE_DIGITS 2
#define SAMPLE_PUBLISH_QOS 0

bool is_device_operational = true;
static const uint8_t null_terminator = '\0';
static char start_time_str[32];
static az_span start_time_span;

// * PnP Values *
// The model id is the JSON document (also called the Digital Twins Model Identifier or DTMI)
// which defines the capability of your device. The functionality of the device should match what
// is described in the corresponding DTMI. Should you choose to program your own PnP capable device,
// the functionality would need to match the DTMI and you would need to update the below 'model_id'.
// Please see the sample README for more information on this DTMI.
static const az_span model_id
    = AZ_SPAN_LITERAL_FROM_STR("dtmi:com:example:TemperatureController;1");
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

// ISO8601 Time Format
static const char iso_spec_time_format[] = "%Y-%m-%dT%H:%M:%S%z";

// IoT Hub Connection Values
static az_iot_hub_client client;
static char device_id[64];
static char iot_hub_hostname[128];
static char x509_cert_pem_file[512];
static char x509_trust_pem_file[256];

// MQTT Client Values
static MQTTClient mqtt_client;
static char mqtt_client_id[128];
static char mqtt_username[256];
static char mqtt_endpoint[128];
static az_span mqtt_url_prefix = AZ_SPAN_LITERAL_FROM_STR("ssl://");
static az_span mqtt_url_suffix = AZ_SPAN_LITERAL_FROM_STR(":8883");

// Reuse topic and payload buffers since API's are synchronous
static char publish_topic[128];
static char publish_payload[512];
static sample_pnp_mqtt_message publish_message;

// IoT Hub Command
static const az_span reboot_command_name = AZ_SPAN_LITERAL_FROM_STR("reboot");
static const az_span empty_json_object = AZ_SPAN_LITERAL_FROM_STR("{}");

//
// Configuration and connection functions
//
static void components_init(void);
static az_result read_configuration_and_init_client(void);
static az_result read_configuration_entry(
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span buffer,
    az_span* out_value);
static az_result create_mqtt_endpoint(char* destination, int32_t destination_size, az_span iot_hub);
static void connect_device(void);
static void subscribe(void);

//
// Messaging functions
//
static void mqtt_publish_message(char* topic, az_span payload, int qos);
static void mqtt_receive_message(void);
static int on_received(char* topicName, int topicLen, MQTTClient_message* message);
static void send_device_serial_number(void);
static void send_device_info(void);
static void send_telemetry_messages(void);
static void send_twin_get_message(void);
static void handle_twin_message(
    MQTTClient_message* message,
    az_iot_hub_client_twin_response* twin_response);
static void handle_command_message(
    MQTTClient_message* message,
    az_iot_hub_client_method_request* command_request);

int main(void)
{
  int rc;

  // Get the program start time for command response
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  size_t len = strftime(start_time_str, sizeof(start_time_str), iso_spec_time_format, timeinfo);
  if (len == 0)
  {
    LOG_ERROR("Insufficient buffer size for program start time.");
    exit(-1);
  }
  start_time_span = az_span_init((uint8_t*)start_time_str, (int32_t)len);

  // Read in the necessary environment variables and initialize the az_iot_hub_client
  if (az_failed(rc = read_configuration_and_init_client()))
  {
    LOG_ERROR("Failed to read configuration from environment variables, return code %d", rc);
    exit(rc);
  }

  // Get the MQTT client id used for the MQTT connection
  size_t client_id_length;
  if (az_failed(
          rc = az_iot_hub_client_get_client_id(
              &client, mqtt_client_id, sizeof(mqtt_client_id), &client_id_length)))
  {
    LOG_ERROR("Failed to get MQTT client id, return code %d", rc);
    exit(rc);
  }

  // Create the Paho MQTT client
  if ((rc = MQTTClient_create(
           &mqtt_client, mqtt_endpoint, mqtt_client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to create MQTT client, return code %d", rc);
    exit(rc);
  }

  // Setup MQTT Message Struct
  publish_message.topic = publish_topic;
  publish_message.topic_length = sizeof(publish_topic);
  publish_message.out_topic_length = 0;
  publish_message.payload_span = AZ_SPAN_FROM_BUFFER(publish_payload);
  publish_message.out_payload_span = publish_message.payload_span;

  // Connect to IoT Hub
  connect_device();

  // Subscribe to the necessary twin and commands topics to receive twin updates and responses
  subscribe();

  // Initialize PnP Components
  components_init();

  // On device start up, send device info
  send_device_info();

  // On device start up, send device serial number
  send_device_serial_number();

  // Get the twin document to check for updated desired properties. Will then parse desired
  // property and update accordingly.
  send_twin_get_message();

  while (is_device_operational)
  {
    // Receive any incoming messages from twin or commands
    mqtt_receive_message();

    // Send max temp for each component since boot if needed
    if (sample_pnp_thermostat_get_max_temp_report(&client, &sample_thermostat_1, &publish_message))
    {
      mqtt_publish_message(
          publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);
    }
    if (sample_pnp_thermostat_get_max_temp_report(&client, &sample_thermostat_2, &publish_message))
    {
      mqtt_publish_message(
          publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);
    }

    // Send a telemetry message
    send_telemetry_messages();
  }

  // Gracefully disconnect: send the disconnect packet and close the socket
  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to disconnect MQTT client, return code %d", rc);
    exit(rc);
  }
  else
  {
    LOG_SUCCESS("Disconnected.");
  }

  // Clean up and release resources allocated by the mqtt client
  MQTTClient_destroy(&mqtt_client);

  return 0;
}

static void components_init()
{
  az_result result;

  if (az_failed(
          result = sample_pnp_thermostat_init(
              &sample_thermostat_1,
              sample_thermostat_1_component_name,
              DEFAULT_START_TEMP_CELSIUS)))
  {
    LOG_ERROR("Could not initialize thermostat 1: error code = 0x%08x", result);
    exit(result);
  }

  else if (az_failed(
               result = sample_pnp_thermostat_init(
                   &sample_thermostat_2,
                   sample_thermostat_2_component_name,
                   DEFAULT_START_TEMP_CELSIUS)))
  {
    LOG_ERROR("Could not initialize thermostat 2: error code = 0x%08x", result);
    exit(result);
  }

  LOG_SUCCESS("Initialized PnP components");

  // Formatting for log
  putchar('\n');
}

// Read OS environment variables using stdlib function
static az_result read_configuration_entry(
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span buffer,
    az_span* out_value)
{
  char* env_value = getenv(env_name);

  if (env_value == NULL && default_value != NULL)
  {
    env_value = default_value;
  }

  if (env_value != NULL)
  {
    LOG_SUCCESS("%s = %s", env_name, hide_value ? "***" : env_value);
    az_span env_span = az_span_from_str(env_value);
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(buffer, az_span_size(env_span));
    az_span_copy(buffer, env_span);
    *out_value = az_span_slice(buffer, 0, az_span_size(env_span));
  }
  else
  {
    LOG_ERROR("(missing) Please set the %s environment variable.", env_name);
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

static void mqtt_receive_message(void)
{
  char* incoming_message_topic;
  int incoming_message_topic_len;
  MQTTClient_message* paho_message;
  // Receive any incoming messages from twin or commands
  if (MQTTClient_receive(
          mqtt_client,
          &incoming_message_topic,
          &incoming_message_topic_len,
          &paho_message,
          TIMEOUT_WAIT_FOR_RECEIVE_MESSAGE_MS)
          == MQTTCLIENT_SUCCESS
      && incoming_message_topic != NULL)
  {
    on_received(incoming_message_topic, incoming_message_topic_len, paho_message);

    MQTTClient_freeMessage(&paho_message);
    MQTTClient_free(incoming_message_topic);
  }
}

static void mqtt_publish_message(char* topic, az_span payload, int qos)
{
  int rc;
  MQTTClient_deliveryToken token;
  if ((rc = MQTTClient_publish(
           mqtt_client, topic, az_span_size(payload), az_span_ptr(payload), qos, 0, &token))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Unable to publish message, return code %d", rc);
    exit(rc);
  }
  if (qos > 0)
  {
    if ((rc = MQTTClient_waitForCompletion(mqtt_client, token, TIMEOUT_WAIT_FOR_COMPLETION_MS))
        != MQTTCLIENT_SUCCESS)
    {
      LOG("Wait for message completion timed out, return code %d", rc);
      exit(rc);
    }
  }
}

// Create mqtt endpoint e.g: ssl//contoso.azure-devices.net:8883
static az_result create_mqtt_endpoint(char* destination, int32_t destination_size, az_span iot_hub)
{
  int32_t required_size = az_span_size(mqtt_url_prefix) + az_span_size(iot_hub)
      + az_span_size(mqtt_url_suffix) + (int32_t)sizeof(null_terminator);

  if (required_size > destination_size)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }

  az_span destination_span = az_span_init((uint8_t*)destination, destination_size);
  az_span remainder = az_span_copy(destination_span, mqtt_url_prefix);
  remainder = az_span_copy(remainder, az_span_slice(iot_hub, 0, az_span_size(iot_hub)));
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

static az_result append_string(az_json_writer* json_writer, void* value)
{
  return az_json_writer_append_string(json_writer, *(az_span*)value);
}

static az_result append_json_token(az_json_writer* json_writer, void* value)
{
  az_json_token value_token = *(az_json_token*)value;

  double value_as_double;

  switch (value_token.kind)
  {
    case AZ_JSON_TOKEN_NUMBER:
      AZ_RETURN_IF_FAILED(az_json_token_get_double(&value_token, &value_as_double));
      AZ_RETURN_IF_FAILED(
          az_json_writer_append_double(json_writer, value_as_double, DOUBLE_DECIMAL_PLACE_DIGITS));
      break;
    case AZ_JSON_TOKEN_STRING:
      AZ_RETURN_IF_FAILED(az_json_writer_append_string(json_writer, value_token.slice));
      break;
    default:
      return AZ_ERROR_ITEM_NOT_FOUND;
  }

  return AZ_OK;
}

static void send_device_serial_number(void)
{
  az_result result;

  if (az_failed(
          result = pnp_create_reported_property(
              publish_message.payload_span,
              AZ_SPAN_NULL,
              serial_number_name,
              append_string,
              (void*)&serial_number_value,
              &publish_message.out_payload_span)))
  {
    LOG_ERROR("Could not get serial number property payload");
    exit(result);
  }
  else if (az_failed(
               result = az_iot_hub_client_twin_patch_get_publish_topic(
                   &client,
                   get_request_id(),
                   publish_message.topic,
                   publish_message.topic_length,
                   NULL)))
  {
    LOG_ERROR("Error to get reported property topic with status: error code = 0x%08x", result);
    exit(result);
  }

  LOG_SUCCESS("Sending device serial number property");

  mqtt_publish_message(publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);

  // Formatting for log
  putchar('\n');
}

static void send_device_info(void)
{
  // Get the device info in a JSON payload and the topic to which to send it
  az_result result;
  if (az_failed(result = sample_pnp_device_info_get_report_data(&client, &publish_message)))
  {
    LOG_ERROR("Could not get the device info data: error code = 0x%08x", result);
    exit(result);
  }

  // Send the MQTT message to the endpoint
  mqtt_publish_message(publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);

  // Receive response for device info publish
  mqtt_receive_message();
}

// Callback invoked by pnp functions each time it finds a property in the twin document
static void sample_property_callback(
    az_span component_name,
    az_json_token* property_name,
    az_json_reader* property_value,
    int32_t version,
    void* user_context_callback)
{
  (void)user_context_callback;
  if (az_span_ptr(component_name) == NULL || az_span_size(component_name) == 0)
  {
    LOG("Property=%.*s arrived for Control component itself. This does not support writable "
        "properties on it(all properties are on sub - components) ",
        az_span_size(property_name->slice),
        az_span_ptr(property_name->slice));

    az_result err_result;
    if (az_failed(
            err_result = pnp_create_reported_property_with_status(
                publish_message.payload_span,
                component_name,
                property_name->slice,
                append_json_token,
                (void*)property_value,
                404,
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
               property_value,
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
               property_value,
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

// Switch on the type of twin message and handle accordingly
static void handle_twin_message(
    MQTTClient_message* message,
    az_iot_hub_client_twin_response* twin_response)
{
  az_result result;

  az_json_reader json_reader;
  az_span twin_payload_span;

  if (message->payloadlen)
  {
    LOG_SUCCESS("Payload: %.*s", message->payloadlen, (char*)message->payload);
    twin_payload_span = az_span_init((uint8_t*)message->payload, (int32_t)message->payloadlen);
    if (az_failed(result = az_json_reader_init(&json_reader, twin_payload_span, NULL)))
    {
      LOG_ERROR("Could not initialize JSON reader");
      return;
    }
  }
  // Determine what type of incoming twin message this is. Print relevant data for the message.
  switch (twin_response->response_type)
  {
    // A response from a twin GET publish message with the twin document as a payload.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET:
      LOG_SUCCESS("A twin GET response was received");
      pnp_process_twin_data(
          &json_reader,
          false,
          sample_pnp_components,
          sample_pnp_components_num,
          sample_property_callback,
          NULL);
      break;
    // An update to the desired properties with the properties as a JSON payload.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
      LOG_SUCCESS("A twin desired properties message was received");
      pnp_process_twin_data(
          &json_reader,
          true,
          sample_pnp_components,
          sample_pnp_components_num,
          sample_property_callback,
          NULL);
      break;

    // A response from a twin reported properties publish message. With a successful update of
    // the reported properties, the payload will be empty and the status will be 204.
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
      LOG_SUCCESS("A twin reported properties response message was received");
      break;
  }
}

static az_result sample_pnp_temp_controller_process_command(
    az_iot_hub_client_method_request* command_request,
    az_span component_name,
    az_span command_name,
    az_span command_payload,
    sample_pnp_mqtt_message* mqtt_message)
{
  az_result result;

  (void)command_payload;

  if (az_span_ptr(component_name) == NULL
      && az_span_is_content_equal(reboot_command_name, command_name))
  {
    // This is a command for the device
    if (az_failed(
            result = az_iot_hub_client_methods_response_get_publish_topic(
                &client,
                command_request->request_id,
                200,
                mqtt_message->topic,
                mqtt_message->topic_length,
                NULL)))
    {
      LOG_ERROR("Unable to get methods response publish topic");
      return result;
    }

    mqtt_message->out_payload_span = empty_json_object;
  }
  else
  {
    result = AZ_ERROR_ITEM_NOT_FOUND;
  }

  return result;
}

// Invoke the requested command if supported and return status | Return error otherwise
static void handle_command_message(
    MQTTClient_message* message,
    az_iot_hub_client_method_request* command_request)
{
  az_result result;

  (void)message;

  az_span command_payload = az_span_init(message->payload, message->payloadlen);
  az_span component_name;
  az_span command_name;
  if (az_failed(
          result = pnp_parse_command_name(command_request->name, &component_name, &command_name)))
  {
    LOG_ERROR("Failed to parse command name: error code = 0x%08x", result);
  }
  else if (az_succeeded(
               result = sample_pnp_thermostat_process_command(
                   &client,
                   &sample_thermostat_1,
                   command_request,
                   component_name,
                   command_name,
                   command_payload,
                   &publish_message)))
  {
    LOG_SUCCESS(
        "Successfully executed command %.*s on thermostat 1",
        az_span_size(command_name),
        az_span_ptr(command_name));

    mqtt_publish_message(
        publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);

    LOG_SUCCESS("Sent response");
  }
  else if (az_succeeded(
               result = sample_pnp_thermostat_process_command(
                   &client,
                   &sample_thermostat_2,
                   command_request,
                   component_name,
                   command_name,
                   command_payload,
                   &publish_message)))
  {
    LOG_SUCCESS(
        "Successfully executed command %.*s on thermostat 2",
        az_span_size(command_name),
        az_span_ptr(command_name));

    mqtt_publish_message(
        publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);

    LOG_SUCCESS("Sent command response");
  }
  else if (az_succeeded(
               result = sample_pnp_temp_controller_process_command(
                   command_request,
                   component_name,
                   command_name,
                   command_payload,
                   &publish_message)))
  {
    LOG_SUCCESS(
        "Successfully executed command %.*s on controller",
        az_span_size(command_name),
        az_span_ptr(command_name));
    mqtt_publish_message(
        publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);

    LOG_SUCCESS("Sent command response");
  }
  else
  {
    LOG("Command not supported. Sending 404 response");
    // The command is not for this device
    if (az_failed(
            result = az_iot_hub_client_methods_response_get_publish_topic(
                &client,
                command_request->request_id,
                404,
                publish_message.topic,
                publish_message.topic_length,
                NULL)))
    {
      LOG_ERROR("Unable to get twin document publish topic");
    }
    else
    {
      publish_message.out_payload_span = empty_json_object;

      mqtt_publish_message(
          publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);

      LOG_SUCCESS("Sent command response");
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

  LOG_SUCCESS("Topic: %s", topicName);

  az_span topic_span = az_span_init((uint8_t*)topicName, topicLen);

  // Parse the incoming message topic and check which feature it is for
  az_iot_hub_client_twin_response twin_response;
  az_iot_hub_client_method_request command_request;

  if (az_succeeded(
          az_iot_hub_client_twin_parse_received_topic(&client, topic_span, &twin_response)))
  {
    LOG_SUCCESS("Twin Message Arrived: status %d", twin_response.status);

    // Determine what kind of twin message it is and take appropriate actions
    handle_twin_message(message, &twin_response);
  }
  else if (az_succeeded(az_iot_hub_client_methods_parse_received_topic(
               &client, az_span_init((uint8_t*)topicName, topicLen), &command_request)))
  {
    LOG_SUCCESS("Command arrived");

    // Determine if the command is supported and take appropriate actions
    handle_command_message(message, &command_request);
  }

  putchar('\n');

  return 1;
}

static void connect_device(void)
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
    LOG_ERROR("Failed to get MQTT username, return code %d", rc);
    exit(rc);
  }

  LOG_SUCCESS("MQTT username: %s", mqtt_username);

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
    LOG_ERROR("Failed to connect, return code %d", rc);
    exit(rc);
  }

  LOG_SUCCESS("Connected to IoT Hub");
}

static void subscribe(void)
{
  int rc;

  // Subscribe to the commands topic. Messages received on this topic are commands to be invoked
  // on the device.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to subscribe to the commands subscribe topic filter, return code %d", rc);
    exit(rc);
  }

  // Subscribe to the desired properties PATCH topic. Messages received on this topic will be
  // updates to the desired properties.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to subscribe to the twin patch topic filter, return code %d", rc);
    exit(rc);
  }

  // Subscribe to the twin response topic. Messages received on this topic will be response statuses
  // from published reported properties or the requested twin document from twin GET publish
  // messages.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to subscribe to twin response topic filter, return code %d", rc);
    exit(rc);
  }

  LOG_SUCCESS("Subscribed to IoT Hub topics");
}

static void send_twin_get_message(void)
{
  int rc;

  az_span request_id_span = get_request_id();
  if (az_failed(
          rc = az_iot_hub_client_twin_document_get_publish_topic(
              &client, request_id_span, publish_message.topic, publish_message.topic_length, NULL)))
  {
    LOG_ERROR("Could not get twin get publish topic, az_result %d", rc);
    exit(rc);
  }

  LOG_SUCCESS("Sending twin get request");
  mqtt_publish_message(publish_message.topic, AZ_SPAN_NULL, SAMPLE_PUBLISH_QOS);

  // Formatting for log
  putchar('\n');
}

static az_result temperature_controller_get_telemetry_message(sample_pnp_mqtt_message* message)
{
  az_result result;
  if (az_failed(
          result = pnp_get_telemetry_topic(
              &client, NULL, AZ_SPAN_NULL, message->topic, message->topic_length, NULL)))
  {
    printf("Could not get pnp telemetry topic: error code = 0x%08x\n", result);
    return result;
  }

  working_set_ram_in_kibibytes = rand() % 128;

  az_json_writer json_builder;
  AZ_RETURN_IF_FAILED(az_json_writer_init(&json_builder, message->payload_span, NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&json_builder));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&json_builder, working_set_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(&json_builder, working_set_ram_in_kibibytes));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&json_builder));
  message->out_payload_span = az_json_writer_get_bytes_used_in_destination(&json_builder);

  return result;
}

// Send JSON formatted telemetry messages
static void send_telemetry_messages(void)
{
  az_result result;

  if (az_failed(
          result = sample_pnp_thermostat_get_telemetry_message(
              &client, &sample_thermostat_1, &publish_message)))
  {
    LOG_ERROR("Error getting message and topic for thermostat 1");
    exit(result);
  }

  LOG_SUCCESS("Sending Telemetry Message for thermostat 1");

  mqtt_publish_message(publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);

  if (az_failed(
          result = sample_pnp_thermostat_get_telemetry_message(
              &client, &sample_thermostat_2, &publish_message)))
  {
    LOG_ERROR("Error getting message and topic for thermostat 2");
    exit(result);
  }

  LOG_SUCCESS("Sending Telemetry Message for thermostat 2");

  mqtt_publish_message(publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);

  if (az_failed(result = temperature_controller_get_telemetry_message(&publish_message)))
  {
    LOG_ERROR("Error getting message and topic for root component");
    exit(result);
  }

  LOG_SUCCESS("Sending Telemetry Message for temperature controller");

  mqtt_publish_message(publish_message.topic, publish_message.out_payload_span, SAMPLE_PUBLISH_QOS);

  // New line to separate messages on the console
  putchar('\n');
}

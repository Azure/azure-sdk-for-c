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

// Reuse topic and payload buffers since API's are synchronous
static char publish_topic[128];
static char publish_payload[512];
static sample_pnp_mqtt_message publish_message;

// IoT Hub Command
static const az_span reboot_command_name = AZ_SPAN_LITERAL_FROM_STR("reboot");
static const az_span empty_json_object = AZ_SPAN_LITERAL_FROM_STR("{}");
static char property_scratch_buffer[64];


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



static void components_init(void);


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
  create_and_configure_mqtt_client();
  LOG_SUCCESS("Client created and configured.");

  connect_mqtt_client_to_iot_hub();
  LOG_SUCCESS("Client connected to IoT Hub.");

  subscribe_mqtt_client_to_iot_hub_topics();
  LOG_SUCCESS("Client subscribed to IoT Hub topics.");






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

    // Setup MQTT Message Struct
  publish_message.topic = publish_topic;
  publish_message.topic_length = sizeof(publish_topic);
  publish_message.out_topic_length = 0;
  publish_message.payload_span = AZ_SPAN_FROM_BUFFER(publish_payload);
  publish_message.out_payload_span = publish_message.payload_span;
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



static void components_init(void)
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
    twin_payload_span = az_span_create((uint8_t*)message->payload, (int32_t)message->payloadlen);
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

  az_span command_payload = az_span_create(message->payload, message->payloadlen);
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

  az_span topic_span = az_span_create((uint8_t*)topicName, topicLen);

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
               &client, az_span_create((uint8_t*)topicName, topicLen), &command_request)))
  {
    LOG_SUCCESS("Command arrived");

    // Determine if the command is supported and take appropriate actions
    handle_command_message(message, &command_request);
  }

  putchar('\n');

  return 1;
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

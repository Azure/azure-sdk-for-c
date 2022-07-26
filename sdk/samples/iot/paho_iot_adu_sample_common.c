// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/*
 * Common implementation for *a device* that implements the Model Id
 * "dtmi:com:example:Thermostat;1".  The model JSON is available in
 * https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json.
 *
 * This code assumes that an MQTT connection to Azure IoT hub and that the underlying
 * az_iot_hub_client have already been initialized in the variables mqtt_client and hub_client.  The
 * sample callers paho_iot_pnp_sample.c and paho_iot_pnp_sample_with_provisioning.c do this before
 * invoking paho_iot_adu_sample_device_implement().
 *
 * This should not be confused with ./pnp/pnp_thermostat_component.c.  Both C files implement
 * The Thermostat Model Id.  In this file, the Thermostat is the only Model that the device
 * implements.  In ./pnp/pnp_thermostat_component.c, the Thermostat is a subcomponent of a more
 * complex device and hence the logic is more complex.
 */

#ifdef _MSC_VER
// warning C4204: nonstandard extension used: non-constant aggregate initializer
#pragma warning(disable : 4204)
// warning C4996: 'localtime': This function or variable may be unsafe.  Consider using localtime_s
// instead.
#pragma warning(disable : 4996)
#endif

#ifdef _MSC_VER
#pragma warning(push)
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
#endif
#include <paho-mqtt/MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <stdlib.h>
#include <time.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_adu.h>
#include <azure/iot/az_iot_hub_client_properties.h>

#include "iot_sample_common.h"
#include "paho_iot_adu_sample_common.h"

MQTTClient mqtt_client;
az_iot_hub_client hub_client;

#define DOUBLE_DECIMAL_PLACE_DIGITS 2

#define SAMPLE_MQTT_TOPIC_LENGTH 128
#define SAMPLE_MQTT_PAYLOAD_LENGTH 1024

bool is_device_operational = true;

// MQTT Connection Values
static uint16_t connection_request_id = 0;
static char connection_request_id_buffer[16];

// Telemetry Values
static az_span const telemetry_name = AZ_SPAN_LITERAL_FROM_STR("count");
static uint32_t telemetry_count;

// Property Values
static az_span const property_success_name = AZ_SPAN_LITERAL_FROM_STR("success");

// ADU Feature Values
static az_iot_adu_update_request xBaseUpdateRequest;
static az_iot_adu_update_manifest xBaseUpdateManifest;
static char adu_new_version[16];
static bool did_parse_update = false;
static bool did_update = false;
static char adu_scratch_buffer[10000];

#define AZ_IOT_ADU_AGENT_VERSION "DU;agent/0.8.0-rc1-public-preview"
az_iot_adu_device_properties adu_device_properties
    = { .manufacturer = AZ_SPAN_LITERAL_FROM_STR(ADU_DEVICE_MANUFACTURER),
        .model = AZ_SPAN_LITERAL_FROM_STR(ADU_DEVICE_MODEL),
        .adu_version = AZ_SPAN_LITERAL_FROM_STR(AZ_IOT_ADU_AGENT_VERSION),
        .delivery_optimization_agent_version = AZ_SPAN_EMPTY,
        .update_id = { .name = AZ_SPAN_LITERAL_FROM_STR(ADU_DEVICE_MODEL),
                       .provider = AZ_SPAN_LITERAL_FROM_STR(ADU_DEVICE_MANUFACTURER),
                       .version = AZ_SPAN_LITERAL_FROM_STR(ADU_DEVICE_VERSION) } };

//
// Functions
//
static void subscribe_mqtt_client_to_iot_hub_topics(void);
static void request_all_properties(void);
static void receive_messages_and_send_telemetry_loop(void);

static az_span get_request_id(void);
static void publish_mqtt_message(char const* topic, az_span payload, int qos);
static void on_message_received(char* topic, int topic_len, MQTTClient_message const* message);

// Device Property functions
static void handle_device_property_message(
    MQTTClient_message const* message,
    az_iot_hub_client_properties_message const* property_message);
static void process_device_property_message(
    az_span message_span,
    az_iot_hub_client_properties_message_type message_type);
static void download_and_write_to_flash(az_span url);
static void verify_image_and_reboot(void);
static void spoof_new_image(void);
static void send_adu_device_properties_property(void);
static void send_adu_accept_manifest_property(int32_t version_number);
static void send_adu_in_progress_property(void);
static void send_adu_completed_property(void);

// Telemetry functions
static void send_telemetry_message(void);

// JSON write functions
static void write_json_payload(
    uint8_t property_count,
    az_span const names[],
    double const values[],
    az_span json_payload,
    az_span* out_json_payload);

// thermostat_device_implement is invoked by the caller to simulate the thermostat device.
// It assumes that the underlying MQTT connection to Azure IoT Hub has already been established.
void paho_iot_adu_sample_device_implement(void)
{
  subscribe_mqtt_client_to_iot_hub_topics();
  IOT_SAMPLE_LOG_SUCCESS("Client subscribed to IoT Hub topics.");

  send_adu_device_properties_property();
  IOT_SAMPLE_LOG_SUCCESS("Publishing device information for ADU. Response will be "
                         "received asynchronously.");

  request_all_properties();
  IOT_SAMPLE_LOG_SUCCESS(
      "Request sent for device's properties.  Response will be received asynchronously.");

  receive_messages_and_send_telemetry_loop();
  IOT_SAMPLE_LOG_SUCCESS("Exited receive and send loop.");
}

// subscribe_mqtt_client_to_iot_hub_topics subscribes to well-known MQTT topics that Azure IoT Hub
// uses to signal incoming commands to the device and notify device of properties.
static void subscribe_mqtt_client_to_iot_hub_topics(void)
{
  int rc;

  // Subscribe to property update notifications.  Messages will be sent to this topic when
  // writable properties are updated by the service.
  rc = MQTTClient_subscribe(
      mqtt_client, AZ_IOT_HUB_CLIENT_PROPERTIES_WRITABLE_UPDATES_SUBSCRIBE_TOPIC, 1);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to subscribe to the property writable updates topic: MQTTClient return code %d.",
        rc);
    exit(rc);
  }

  // Subscribe to the properties message topic.  When the device invokes a PUBLISH to get
  // all properties (both reported from device and reported - see request_all_properties() below)
  // the property payload will be sent to this topic.
  rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_SUBSCRIBE_TOPIC, 1);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to subscribe to the property message topic: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

// request_all_properties sends a request to Azure IoT Hub to request all properties for
// the device.  This call does not block.  Properties will be received on
// a topic previously subscribed to (AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_SUBSCRIBE_TOPIC.)
static void request_all_properties(void)
{
  az_result rc;

  IOT_SAMPLE_LOG("Client requesting device property document from service.");

  // Get the topic to publish the property document request.
  char property_document_topic_buffer[SAMPLE_MQTT_TOPIC_LENGTH];
  rc = az_iot_hub_client_properties_document_get_publish_topic(
      &hub_client,
      get_request_id(),
      property_document_topic_buffer,
      sizeof(property_document_topic_buffer),
      NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the property document topic");

  // Publish the property document request.
  publish_mqtt_message(property_document_topic_buffer, AZ_SPAN_EMPTY, IOT_SAMPLE_MQTT_PUBLISH_QOS);
}

static void send_adu_in_progress_property(void)
{
  az_result rc;

  // Get the topic to publish the property document request.
  char property_document_topic_buffer[SAMPLE_MQTT_TOPIC_LENGTH];
  rc = az_iot_hub_client_properties_get_reported_publish_topic(
      &hub_client,
      get_request_id(),
      property_document_topic_buffer,
      sizeof(property_document_topic_buffer),
      NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the property document topic");

  char property_payload_buffer[SAMPLE_MQTT_PAYLOAD_LENGTH];
  az_span property_buffer = AZ_SPAN_FROM_BUFFER(property_payload_buffer);
  rc = az_iot_adu_get_properties_payload(
      &adu_device_properties,
      AZ_IOT_ADU_AGENT_STATE_DEPLOYMENT_IN_PROGRESS,
      NULL,
      NULL,
      property_buffer,
      &property_buffer);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get adu properties payload");

  publish_mqtt_message(
      property_document_topic_buffer, property_buffer, IOT_SAMPLE_MQTT_PUBLISH_QOS);
}

static void send_adu_accept_manifest_property(int32_t version_number)
{
  az_result rc;

  // Get the topic to publish the property document request.
  char property_document_topic_buffer[SAMPLE_MQTT_TOPIC_LENGTH];
  rc = az_iot_hub_client_properties_get_reported_publish_topic(
      &hub_client,
      get_request_id(),
      property_document_topic_buffer,
      sizeof(property_document_topic_buffer),
      NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the property document topic");

  char property_payload_buffer[SAMPLE_MQTT_PAYLOAD_LENGTH];
  az_span property_buffer = AZ_SPAN_FROM_BUFFER(property_payload_buffer);
  rc = az_iot_adu_get_service_properties_response(
      version_number, 200, property_buffer, &property_buffer);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get service properties response payload");

  publish_mqtt_message(
      property_document_topic_buffer, property_buffer, IOT_SAMPLE_MQTT_PUBLISH_QOS);
}

static void send_adu_completed_property(void)
{
  az_result rc;

  // Get the topic to publish the property document request.
  char property_document_topic_buffer[SAMPLE_MQTT_TOPIC_LENGTH];
  rc = az_iot_hub_client_properties_get_reported_publish_topic(
      &hub_client,
      get_request_id(),
      property_document_topic_buffer,
      sizeof(property_document_topic_buffer),
      NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the property document topic");

  char property_payload_buffer[SAMPLE_MQTT_PAYLOAD_LENGTH];
  az_span property_buffer = AZ_SPAN_FROM_BUFFER(property_payload_buffer);
  rc = az_iot_adu_get_properties_payload(
      &adu_device_properties,
      AZ_IOT_ADU_AGENT_STATE_IDLE,
      NULL,
      NULL,
      property_buffer,
      &property_buffer);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get completed payload");

  publish_mqtt_message(
      property_document_topic_buffer, property_buffer, IOT_SAMPLE_MQTT_PUBLISH_QOS);
}

static void download_and_write_to_flash(az_span url)
{
  IOT_SAMPLE_LOG_AZ_SPAN("Downloading image from ", url);

  IOT_SAMPLE_LOG("Writing to flash");
}

static void verify_image_and_reboot(void)
{
  IOT_SAMPLE_LOG("Verified image");

  IOT_SAMPLE_LOG("Rebooting device");
}

static void spoof_new_image(void)
{
  // Changing device version to new version.
  az_span new_version = az_span_create(adu_new_version, sizeof(adu_new_version));
  az_span_copy(new_version, xBaseUpdateManifest.update_id.version);
  adu_device_properties.update_id.version = new_version;
  adu_device_properties.update_id.version
      = az_span_slice(new_version, 0, az_span_size(xBaseUpdateManifest.update_id.version));
  IOT_SAMPLE_LOG_AZ_SPAN("New version ", adu_device_properties.update_id.version);
}
// receive_messages_and_send_telemetry_loop will loop to check if there are incoming MQTT
// messages, waiting up to MQTT_TIMEOUT_RECEIVE_MS.  It will also send a telemetry message
// every time through the loop.
static void receive_messages_and_send_telemetry_loop(void)
{
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;
  uint8_t timeout_counter = 0;

  // Continue to receive commands or device property messages while device is operational.
  while (is_device_operational)
  {
    IOT_SAMPLE_LOG(" "); // Formatting
    IOT_SAMPLE_LOG("Waiting for device property message.\n");

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
      // Allow up to MQTT_TIMEOUT_RECEIVE_MAX_MESSAGE_COUNT timeouts before disconnecting.
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

    if (did_parse_update && !did_update)
    {
      IOT_SAMPLE_LOG_AZ_SPAN(
          "Verifying manifest signature: ", xBaseUpdateRequest.update_manifest_signature);

      IOT_SAMPLE_LOG("Manifest has been verified");

      IOT_SAMPLE_LOG("Sending that device update is in progress");

      send_adu_in_progress_property();

      download_and_write_to_flash(xBaseUpdateRequest.file_urls[0].url);

      verify_image_and_reboot();

      spoof_new_image();

      IOT_SAMPLE_LOG("Sending new device version | ADU completed");
      send_adu_completed_property();

      did_update = true;
    }

    send_telemetry_message();
  }
}

// get_request_id sets a request Id into connection_request_id_buffer and monotonically
// increases the counter for the next MQTT operation.
static az_span get_request_id(void)
{
  az_span remainder;
  az_span out_span = az_span_create(
      (uint8_t*)connection_request_id_buffer, sizeof(connection_request_id_buffer));

  connection_request_id++;
  if (connection_request_id == UINT16_MAX)
  {
    // Connection id has looped.  Reset.
    connection_request_id = 1;
  }

  az_result rc = az_span_u32toa(out_span, connection_request_id, &remainder);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get request id");

  return az_span_slice(out_span, 0, az_span_size(out_span) - az_span_size(remainder));
}

// publish_mqtt_message is a wrapper to the underlying Paho PUBLISH method
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

// on_message_received dispatches an MQTT message when the underlying MQTT stack provides one
static void on_message_received(char* topic, int topic_len, MQTTClient_message const* message)
{
  az_result rc;

  az_span const topic_span = az_span_create((uint8_t*)topic, topic_len);
  az_span const message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  az_iot_hub_client_properties_message property_message;
  az_iot_hub_client_command_request command_request;

  // Parse the incoming message topic and handle appropriately.
  // Note that if a topic does not match - e.g. az_iot_hub_client_properties_parse_received_topic is
  // invoked to process a command message - the function returns AZ_ERROR_IOT_TOPIC_NO_MATCH.  This
  // is NOT a fatal error but is used to indicate to the caller to see if the topic matches other
  // topics.
  rc = az_iot_hub_client_properties_parse_received_topic(
      &hub_client, topic_span, &property_message);
  if (az_result_succeeded(rc))
  {
    IOT_SAMPLE_LOG_SUCCESS("Client received a valid topic.");
    IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
    IOT_SAMPLE_LOG_AZ_SPAN("Payload:", message_span);
    IOT_SAMPLE_LOG("Status: %d", property_message.status);

    handle_device_property_message(message, &property_message);
  }
  else
  {
    IOT_SAMPLE_LOG_ERROR("Message from unknown topic: az_result return code 0x%08x.", rc);
    IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
    exit(rc);
  }
}

// handle_device_property_message handles incoming properties from Azure IoT Hub.
static void handle_device_property_message(
    MQTTClient_message const* message,
    az_iot_hub_client_properties_message const* property_message)
{
  az_span const message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  // Invoke appropriate action per message type (3 types only).
  switch (property_message->message_type)
  {
    // A message from a property GET publish message with the property document as a payload.
    case AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE:
      IOT_SAMPLE_LOG("Message Type: GET");
      process_device_property_message(message_span, property_message->message_type);
      break;

    // An update to the desired properties with the properties as a payload.
    case AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED:
      IOT_SAMPLE_LOG("Message Type: Desired Properties");
      process_device_property_message(message_span, property_message->message_type);
      break;

    // When the device publishes a property update, this message type arrives when
    // server acknowledges this.
    case AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_ACKNOWLEDGEMENT:
      IOT_SAMPLE_LOG("Message Type: IoT Hub has acknowledged properties that the device sent");
      break;

    // An error has occurred
    case AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_ERROR:
      IOT_SAMPLE_LOG_ERROR("Message Type: Request Error");
      break;
  }
}

// process_device_property_message handles incoming properties from Azure IoT Hub.
static void process_device_property_message(
    az_span message_span,
    az_iot_hub_client_properties_message_type message_type)
{
  az_json_reader jr;
  az_result rc = az_json_reader_init(&jr, message_span, NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Could not initialize json reader");

  int32_t version_number;
  rc = az_iot_hub_client_properties_get_properties_version(
      &hub_client, &jr, message_type, &version_number);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Could not get property version");

  rc = az_json_reader_init(&jr, message_span, NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Could not initialize json reader");

  az_span component_name;

  az_span xScratchBufferSpan
      = az_span_create(adu_scratch_buffer, (int32_t)sizeof(adu_scratch_buffer));

  // Applications call az_iot_hub_client_properties_get_next_component_property to enumerate
  // properties received.
  while (az_result_succeeded(az_iot_hub_client_properties_get_next_component_property(
      &hub_client, &jr, message_type, AZ_IOT_HUB_CLIENT_PROPERTY_WRITABLE, &component_name)))
  {
    if (az_iot_adu_is_component_device_update(component_name))
    {
      // ADU Component
      rc = az_iot_adu_parse_service_properties(
          &jr, xScratchBufferSpan, &xBaseUpdateRequest, &xScratchBufferSpan);

      if (az_result_failed(rc))
      {
        IOT_SAMPLE_LOG_ERROR("az_iot_adu_parse_service_properties failed: 0x%08x.", rc);
        /* TODO: return individualized/specific errors. */
        return;
      }
      else
      {
        rc = az_iot_adu_parse_update_manifest(
            xBaseUpdateRequest.update_manifest, &xBaseUpdateManifest);

        if (az_result_failed(rc))
        {
          IOT_SAMPLE_LOG_ERROR("az_iot_adu_parse_update_manifest failed: 0x%08x", rc);
          /* TODO: return individualized/specific errors. */
          return;
        }

        IOT_SAMPLE_LOG_SUCCESS("Parsed Azure device update manifest.");

        IOT_SAMPLE_LOG("Sending manifest property accept");

        send_adu_accept_manifest_property(version_number);

        did_parse_update = true;
      }
    }
    else
    {
      IOT_SAMPLE_LOG_AZ_SPAN("Unknown Property Received:", jr.token.slice);
      // The JSON reader must be advanced regardless of whether the property
      // is of interest or not.
      rc = az_json_reader_next_token(&jr);
      if (az_result_failed(rc))
      {
        IOT_SAMPLE_LOG_ERROR("Invalid JSON. Could not move to next property value");
      }

      // Skip children in case the property value is an object
      rc = az_json_reader_skip_children(&jr);
      if (az_result_failed(rc))
      {
        IOT_SAMPLE_LOG_ERROR("Invalid JSON. Could not skip children");
      }

      rc = az_json_reader_next_token(&jr);
      if (az_result_failed(rc))
      {
        IOT_SAMPLE_LOG_ERROR("Invalid JSON. Could not move to next property name");
      }
    }
  }
}

// send_adu_device_reported_property writes a property payload reporting device state and then sends
// it to Azure IoT Hub.
static void send_adu_device_properties_property(void)
{
  az_result rc;

  // Get the property topic to send a reported property update.
  char property_update_topic_buffer[SAMPLE_MQTT_TOPIC_LENGTH];
  rc = az_iot_hub_client_properties_get_reported_publish_topic(
      &hub_client,
      get_request_id(),
      property_update_topic_buffer,
      sizeof(property_update_topic_buffer),
      NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the property update topic");

  // Write the updated reported property message.
  char reported_property_payload_buffer[SAMPLE_MQTT_PAYLOAD_LENGTH];
  az_span reported_property_payload = AZ_SPAN_FROM_BUFFER(reported_property_payload_buffer);

  rc = az_iot_adu_get_properties_payload(
      &adu_device_properties,
      AZ_IOT_ADU_AGENT_STATE_IDLE,
      NULL,
      NULL,
      reported_property_payload,
      &reported_property_payload);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the adu device information payload");

  // Publish the reported property update.
  publish_mqtt_message(
      property_update_topic_buffer, reported_property_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);
  IOT_SAMPLE_LOG_SUCCESS("Client published the device's information.");
  IOT_SAMPLE_LOG_AZ_SPAN("Payload:", reported_property_payload);
}

// send_telemetry_message builds the body of a telemetry message containing the current temperature
// and then sends it to Azure IoT Hub
static void send_telemetry_message(void)
{
  az_result rc;

  // Get the Telemetry topic to publish the telemetry message.
  char telemetry_topic_buffer[SAMPLE_MQTT_TOPIC_LENGTH];
  rc = az_iot_hub_client_telemetry_get_publish_topic(
      &hub_client, NULL, telemetry_topic_buffer, sizeof(telemetry_topic_buffer), NULL);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(rc, "Failed to get the Telemetry topic");

  // Build the telemetry message.
  uint8_t count = 1;
  az_span const names[1] = { telemetry_name };
  double const values[1] = { telemetry_count++ };

  char telemetry_payload_buffer[SAMPLE_MQTT_PAYLOAD_LENGTH];
  az_span telemetry_payload = AZ_SPAN_FROM_BUFFER(telemetry_payload_buffer);
  write_json_payload(count, names, values, telemetry_payload, &telemetry_payload);

  // Publish the telemetry message.
  publish_mqtt_message(telemetry_topic_buffer, telemetry_payload, IOT_SAMPLE_MQTT_PUBLISH_QOS);
  IOT_SAMPLE_LOG_SUCCESS("Client published the Telemetry message.");
  IOT_SAMPLE_LOG_AZ_SPAN("Payload:", telemetry_payload);
}

// write_json_payload writes a desired JSON payload.  The JSON built just needs to conform to
// the DTDLv2 that defined it.
static void write_json_payload(
    uint8_t property_count,
    az_span const names[],
    double const values[],
    az_span json_payload,
    az_span* out_json_payload)
{
  char const* const log_message = "Failed to write property payload";

  az_json_writer jw;
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_init(&jw, json_payload, NULL), log_message);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), log_message);

  for (uint8_t i = 0; i < property_count; i++)
  {
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_property_name(&jw, names[i]), log_message);
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(
        az_json_writer_append_double(&jw, values[i], DOUBLE_DECIMAL_PLACE_DIGITS), log_message);
  }

  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), log_message);
  *out_json_payload = az_json_writer_get_bytes_used_in_destination(&jw);
}

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

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_provisioning_client.h>

#define SAMPLE_TYPE PAHO_IOT_PROVISIONING
#define SAMPLE_NAME PAHO_IOT_PROVISIONING_SAMPLE

#define MQTT_TIMEOUT_RECEIVE_MS (60 * 1000)
#define MQTT_TIMEOUT_DISCONNECT_MS (10 * 1000)

static iot_sample_environment_variables env_vars;
static az_iot_provisioning_client provisioning_client;
static MQTTClient mqtt_client;
static char mqtt_client_username_buffer[128];

// Functions
static void create_and_configure_mqtt_client(void);
static void connect_mqtt_client_to_provisioning_service(void);
static void subscribe_mqtt_client_to_provisioning_service_topics(void);
static void register_device_with_provisioning_service(void);
static void receive_device_registration_status_message(void);
static void disconnect_mqtt_client_from_provisioning_service(void);

static void parse_device_registration_status_message(
    char* topic,
    int topic_len,
    MQTTClient_message const* message,
    az_iot_provisioning_client_register_response* out_register_response,
    az_iot_provisioning_client_operation_status* out_operation_status);
static void handle_device_registration_status_message(
    az_iot_provisioning_client_register_response const* register_response,
    az_iot_provisioning_client_operation_status const* operation_status,
    bool* ref_is_operation_complete);
static void send_operation_query_message(
    az_iot_provisioning_client_register_response const* response);

/*
 * This sample registers a device with the Azure IoT Device Provisioning Service.
 * It will wait to receive the registration status before disconnecting.
 * X509 self-certification is used.
 */
int main(void)
{
  create_and_configure_mqtt_client();
  IOT_SAMPLE_LOG_SUCCESS("Client created and configured.");

  connect_mqtt_client_to_provisioning_service();
  IOT_SAMPLE_LOG_SUCCESS("Client connected to provisioning service.");

  subscribe_mqtt_client_to_provisioning_service_topics();
  IOT_SAMPLE_LOG_SUCCESS("Client subscribed to provisioning service topics.");

  register_device_with_provisioning_service();
  IOT_SAMPLE_LOG_SUCCESS("Client registering with provisioning service.");

  receive_device_registration_status_message();
  IOT_SAMPLE_LOG_SUCCESS("Client received registration status message.");

  disconnect_mqtt_client_from_provisioning_service();
  IOT_SAMPLE_LOG_SUCCESS("Client disconnected from provisioning service.");

  return 0;
}

static void create_and_configure_mqtt_client(void)
{
  int rc;

  // Reads in environment variables set by user for purposes of running sample.
  if (az_failed(rc = iot_sample_read_environment_variables(SAMPLE_TYPE, SAMPLE_NAME, &env_vars)))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to read environment variables: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Build an MQTT endpoint c-string.
  char mqtt_endpoint_buffer[256];
  if (az_failed(
          rc = iot_sample_create_mqtt_endpoint(
              SAMPLE_TYPE, &env_vars, mqtt_endpoint_buffer, sizeof(mqtt_endpoint_buffer))))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to create MQTT endpoint: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Initialize the provisioning client with the provisioning global endpoint and the default
  // connection options.
  if (az_failed(
          rc = az_iot_provisioning_client_init(
              &provisioning_client,
              az_span_create_from_str(mqtt_endpoint_buffer),
              env_vars.provisioning_id_scope,
              env_vars.provisioning_registration_id,
              NULL)))
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to initialize provisioning client: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Get the MQTT client id used for the MQTT connection.
  char mqtt_client_id_buffer[128];
  if (az_failed(
          rc = az_iot_provisioning_client_get_client_id(
              &provisioning_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), NULL)))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get MQTT client id: az_result return code 0x%08x.", rc);
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
    IOT_SAMPLE_LOG_ERROR("Failed to create MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

static void connect_mqtt_client_to_provisioning_service(void)
{
  int rc;

  // Get the MQTT client username.
  if (az_failed(
          rc = az_iot_provisioning_client_get_user_name(
              &provisioning_client,
              mqtt_client_username_buffer,
              sizeof(mqtt_client_username_buffer),
              NULL)))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get MQTT client username: az_result return code 0x%08x.", rc);
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

  // Connect MQTT client to the Azure IoT Device Provisioning Service.
  if ((rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options)) != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to connect: MQTTClient return code %d.\n"
        "If on Windows, confirm the AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH environment variable is "
        "set correctly.",
        rc);
    exit(rc);
  }
}

static void subscribe_mqtt_client_to_provisioning_service_topics(void)
{
  int rc;

  // Messages received on the Register topic will be registration responses from the server.
  if ((rc
       = MQTTClient_subscribe(mqtt_client, AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to subscribe to the Register topic: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

static void register_device_with_provisioning_service(void)
{
  int rc;

  // Get the Register topic to publish the register request.
  char register_topic_buffer[128];
  if (az_failed(
          rc = az_iot_provisioning_client_register_get_publish_topic(
              &provisioning_client, register_topic_buffer, sizeof(register_topic_buffer), NULL)))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get the Register topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Set MQTT message options.
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  pubmsg.payload = NULL; // Empty payload
  pubmsg.payloadlen = 0;
  pubmsg.qos = 1;
  pubmsg.retained = 0;

  // Publish the register request.
  if ((rc = MQTTClient_publishMessage(mqtt_client, register_topic_buffer, &pubmsg, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to publish Register request: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

static void receive_device_registration_status_message(void)
{
  int rc;
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;
  bool is_operation_complete = false;

  // Continue to parse incoming responses from the provisioning pervice until the device
  // has been successfully provisioned or an error occurs.
  do
  {
    IOT_SAMPLE_LOG(" "); // Formatting
    IOT_SAMPLE_LOG("Waiting for registration status message.\n");

    if (((rc
          = MQTTClient_receive(mqtt_client, &topic, &topic_len, &message, MQTT_TIMEOUT_RECEIVE_MS))
         != MQTTCLIENT_SUCCESS)
        && (MQTTCLIENT_TOPICNAME_TRUNCATED != rc))
    {
      IOT_SAMPLE_LOG_ERROR("Failed to receive message: MQTTClient return code %d.", rc);
      exit(rc);
    }
    else if (NULL == message)
    {
      IOT_SAMPLE_LOG_ERROR("Receive message timeout expired: MQTTClient return code %d.", rc);
      exit(rc);
    }
    else if (MQTTCLIENT_TOPICNAME_TRUNCATED == rc)
    {
      topic_len = (int)strlen(topic);
    }
    IOT_SAMPLE_LOG_SUCCESS("Client received a message from the provisioning service.");

    // Parse registration status message.
    az_iot_provisioning_client_register_response register_response;
    az_iot_provisioning_client_operation_status operation_status;
    parse_device_registration_status_message(
        topic, topic_len, message, &register_response, &operation_status);
    IOT_SAMPLE_LOG_SUCCESS("Client parsed registration status message.");

    handle_device_registration_status_message(
        &register_response, &operation_status, &is_operation_complete);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topic);

  } while (!is_operation_complete); // Will loop to receive new operation message.
}

static void disconnect_mqtt_client_from_provisioning_service(void)
{
  int rc;

  if ((rc = MQTTClient_disconnect(mqtt_client, MQTT_TIMEOUT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to disconnect MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }

  MQTTClient_destroy(&mqtt_client);
}

static void parse_device_registration_status_message(
    char* topic,
    int topic_len,
    MQTTClient_message const* message,
    az_iot_provisioning_client_register_response* out_register_response,
    az_iot_provisioning_client_operation_status* out_operation_status)
{
  az_result rc;
  az_span const topic_span = az_span_create((uint8_t*)topic, topic_len);
  az_span const message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  // Parse message and retrieve register_response info.
  if (az_failed(
          rc = az_iot_provisioning_client_parse_received_topic_and_payload(
              &provisioning_client, topic_span, message_span, out_register_response)))
  {
    IOT_SAMPLE_LOG_ERROR("Message from unknown topic: az_result return code 0x%08x.", rc);
    IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
    exit(rc);
  }
  IOT_SAMPLE_LOG_SUCCESS("Client received a valid topic response:");
  IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
  IOT_SAMPLE_LOG_AZ_SPAN("Payload:", message_span);
  IOT_SAMPLE_LOG("Status: %d", out_register_response->status);

  // Retrieve operation_status.
  if (az_failed(
          rc = az_iot_provisioning_client_parse_operation_status(
              out_register_response, out_operation_status)))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to parse operation_status: az_result return code 0x%08x.", rc);
    exit(rc);
  }
}

static void handle_device_registration_status_message(
    az_iot_provisioning_client_register_response const* register_response,
    az_iot_provisioning_client_operation_status const* operation_status,
    bool* ref_is_operation_complete)
{
  *ref_is_operation_complete = az_iot_provisioning_client_operation_complete(*operation_status);

  // If operation is not complete, send query. On return, will loop to receive new operation
  // message.
  if (!*ref_is_operation_complete)
  {
    IOT_SAMPLE_LOG("Operation is still pending.");

    send_operation_query_message(register_response);
    IOT_SAMPLE_LOG_SUCCESS("Client sent operation query message.");
  }
  else // Operation is complete.
  {
    if (AZ_IOT_PROVISIONING_STATUS_ASSIGNED == *operation_status) // Successful assignment
    {
      IOT_SAMPLE_LOG_SUCCESS("Device provisioned:");
      IOT_SAMPLE_LOG_AZ_SPAN(
          "Hub Hostname:", register_response->registration_result.assigned_hub_hostname);
      IOT_SAMPLE_LOG_AZ_SPAN("Device Id:", register_response->registration_result.device_id);
      IOT_SAMPLE_LOG(" "); // Formatting
    }
    else // Unsuccessful assignment (unassigned, failed or disabled states)
    {
      IOT_SAMPLE_LOG_ERROR("Device provisioning failed:");
      IOT_SAMPLE_LOG_AZ_SPAN("Registration state:", register_response->operation_status);
      IOT_SAMPLE_LOG("Last operation status: %d", register_response->status);
      IOT_SAMPLE_LOG_AZ_SPAN("Operation ID:", register_response->operation_id);
      IOT_SAMPLE_LOG("Error code: %u", register_response->registration_result.extended_error_code);
      IOT_SAMPLE_LOG_AZ_SPAN(
          "Error message:", register_response->registration_result.error_message);
      IOT_SAMPLE_LOG_AZ_SPAN(
          "Error timestamp:", register_response->registration_result.error_timestamp);
      IOT_SAMPLE_LOG_AZ_SPAN(
          "Error tracking ID:", register_response->registration_result.error_tracking_id);
      exit((int)register_response->registration_result.extended_error_code);
    }
  }
}

static void send_operation_query_message(
    az_iot_provisioning_client_register_response const* register_response)
{
  int rc;

  // Get the Query Status topic to publish the query status request.
  char query_topic_buffer[256];
  if (az_failed(
          rc = az_iot_provisioning_client_query_status_get_publish_topic(
              &provisioning_client,
              register_response,
              query_topic_buffer,
              sizeof(query_topic_buffer),
              NULL)))
  {
    IOT_SAMPLE_LOG_ERROR(
        "Unable to get query status publish topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // IMPORTANT: Wait the recommended retry-after number of seconds before query
  IOT_SAMPLE_LOG("Querying after %u seconds...", register_response->retry_after_seconds);
  iot_sample_sleep_for_seconds(register_response->retry_after_seconds);

  // Publish the query status request.
  if ((rc = MQTTClient_publish(mqtt_client, query_topic_buffer, 0, NULL, 0, 0, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR("Failed to publish query status request: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

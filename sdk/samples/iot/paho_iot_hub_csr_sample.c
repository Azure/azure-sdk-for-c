// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning(push)
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
#endif
#include <MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <azure/az_core.h>
#include <azure/az_iot.h>

#include "iot_sample_common.h"

#define SAMPLE_TYPE PAHO_IOT_HUB
#define SAMPLE_NAME PAHO_IOT_HUB_CSR_SAMPLE

// Timeout for initial 202 Accepted response (spec says GW has 60s, add grace).
#define MQTT_TIMEOUT_RECEIVE_ACCEPTED_MS (90 * 1000)
// Timeout for final 200/error response (operationExpires could be up to 12 hours).
#define MQTT_TIMEOUT_RECEIVE_COMPLETED_MS (60 * 60 * 1000)
#define MQTT_TIMEOUT_DISCONNECT_MS (10 * 1000)

static az_span const certificate_signing_request_id = AZ_SPAN_LITERAL_FROM_STR("csr-001");

static iot_sample_environment_variables env_vars;
static az_iot_hub_client hub_client;
static MQTTClient mqtt_client;
static char mqtt_client_username_buffer[256];

// Functions
static void create_and_configure_mqtt_client(void);
static void connect_mqtt_client_to_iot_hub(void);
static void subscribe_mqtt_client_to_iot_hub_topics(void);
static void send_certificate_signing_request(void);
static void receive_certificate_signing_response(void);
static void disconnect_mqtt_client_from_iot_hub(void);

static void parse_certificate_signing_response(
    char* topic,
    int topic_len,
    MQTTClient_message const* message,
    az_iot_hub_client_certificate_signing_response* out_response);

static void handle_certificate_signing_response(
    MQTTClient_message const* message,
    az_iot_hub_client_certificate_signing_response const* response);

/*
 * This sample demonstrates how to request certificate issuance from Azure IoT Hub
 * using the credentials API. The device sends a Certificate Signing Request (CSR)
 * to the hub and receives an issued certificate in response.
 *
 * Flow:
 * 1. Connect to IoT Hub using an existing X.509 bootstrap certificate.
 * 2. Subscribe to the credentials response topic.
 * 3. Publish a CSR request with a base64-encoded PKCS#10 CSR.
 * 4. Receive a 202 Accepted response (request committed by the service).
 * 5. Receive a 200 OK response with the issued certificate, or an error.
 * 6. Disconnect.
 *
 * Prerequisites:
 *   Set the following environment variables:
 *     AZ_IOT_HUB_HOSTNAME              - IoT Hub hostname
 *     AZ_IOT_HUB_DEVICE_ID             - Device ID
 *     AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH - Path to the bootstrap X.509 cert+key PEM
 *     AZ_IOT_DEVICE_CSR_BASE64         - Base64-encoded PKCS#10 CSR (no PEM headers)
 *     AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH - (Optional, Windows) Path to trusted CA PEM
 */
int main(void)
{
  create_and_configure_mqtt_client();
  IOT_SAMPLE_LOG_SUCCESS("Client created and configured.");

  connect_mqtt_client_to_iot_hub();
  IOT_SAMPLE_LOG_SUCCESS("Client connected to IoT Hub.");

  subscribe_mqtt_client_to_iot_hub_topics();
  IOT_SAMPLE_LOG_SUCCESS("Client subscribed to credentials response topic.");

  send_certificate_signing_request();
  IOT_SAMPLE_LOG_SUCCESS("Client sent credential issuance request.");

  receive_certificate_signing_response();
  IOT_SAMPLE_LOG_SUCCESS("Client received credential response(s).");

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

  // Initialize the hub client with the default connection options.
  rc = az_iot_hub_client_init(&hub_client, env_vars.hub_hostname, env_vars.hub_device_id, NULL);
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
  // Messages received on the credentials response topic will be issuance responses from the
  // service.
  int rc = MQTTClient_subscribe(
      mqtt_client, AZ_IOT_HUB_CLIENT_CERTIFICATE_SIGNING_RESPONSE_SUBSCRIBE_TOPIC, 1);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to subscribe to the credentials response topic: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

static void send_certificate_signing_request(void)
{
  int rc;

  IOT_SAMPLE_LOG("Client requesting certificate signing from service.");

  // Get the publish topic for the certificate signing request.
  char credential_topic_buffer[128];
  rc = az_iot_hub_client_certificate_signing_request_get_publish_topic(
      &hub_client,
      certificate_signing_request_id,
      credential_topic_buffer,
      sizeof(credential_topic_buffer),
      NULL);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to get the credentials publish topic: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  // Build the credential request JSON payload.
  az_iot_hub_client_certificate_signing_request certificate_signing_request
      = { .csr = env_vars.certificate_signing_request_base64 };

  uint8_t credential_payload_buffer[2560];
  size_t credential_payload_length = 0;
  rc = az_iot_hub_client_certificate_signing_request_get_request_payload(
      &hub_client,
      &certificate_signing_request,
      credential_payload_buffer,
      sizeof(credential_payload_buffer),
      &credential_payload_length);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to build the credential request payload: az_result return code 0x%08x.", rc);
    exit(rc);
  }

  IOT_SAMPLE_LOG("Credential request topic: %s", credential_topic_buffer);
  IOT_SAMPLE_LOG_AZ_SPAN(
      "Credential request payload:",
      az_span_create(credential_payload_buffer, (int32_t)credential_payload_length));

  // Publish the credential issuance request with QoS 1.
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  pubmsg.payload = credential_payload_buffer;
  pubmsg.payloadlen = (int)credential_payload_length;
  pubmsg.qos = 1;
  pubmsg.retained = 0;

  rc = MQTTClient_publishMessage(mqtt_client, credential_topic_buffer, &pubmsg, NULL);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to publish credential request: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

static void receive_certificate_signing_response(void)
{
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;
  bool is_operation_complete = false;
  bool is_accepted = false; // True after receiving 202, switches to longer timeout.

  // Continue receiving responses until the operation completes (200 or error).
  // The first response should be 202 Accepted, followed by 200 OK or an error.
  do
  {
    IOT_SAMPLE_LOG(" "); // Formatting
    IOT_SAMPLE_LOG("Waiting for credential response message.\n");

    // Before 202: use 90s timeout (spec gives GW 60s + grace).
    // After 202: use longer timeout (certificate issuance may take time).
    unsigned long timeout_ms = is_accepted
        ? MQTT_TIMEOUT_RECEIVE_COMPLETED_MS
        : MQTT_TIMEOUT_RECEIVE_ACCEPTED_MS;

    // MQTTCLIENT_SUCCESS or MQTTCLIENT_TOPICNAME_TRUNCATED if a message is received.
    // MQTTCLIENT_SUCCESS can also indicate that the timeout expired, in which case message is NULL.
    // MQTTCLIENT_TOPICNAME_TRUNCATED if the topic contains embedded NULL characters.
    // An error code is returned if there was a problem trying to receive a message.
    int rc = MQTTClient_receive(mqtt_client, &topic, &topic_len, &message, timeout_ms);
    if ((rc != MQTTCLIENT_SUCCESS) && (rc != MQTTCLIENT_TOPICNAME_TRUNCATED))
    {
      IOT_SAMPLE_LOG_ERROR("Failed to receive message: MQTTClient return code %d.", rc);
      exit(rc);
    }
    else if (message == NULL)
    {
      IOT_SAMPLE_LOG_ERROR("Receive message timeout expired: MQTTClient return code %d.", rc);
      exit(rc);
    }
    else if (rc == MQTTCLIENT_TOPICNAME_TRUNCATED)
    {
      topic_len = (int)strlen(topic);
    }
    IOT_SAMPLE_LOG_SUCCESS("Client received a message from the service.");

    // Parse the credential response from the topic.
    az_iot_hub_client_certificate_signing_response credential_response;
    parse_certificate_signing_response(topic, topic_len, message, &credential_response);
    IOT_SAMPLE_LOG_SUCCESS("Client parsed credential response message.");

    // Handle the response based on its type.
    handle_certificate_signing_response(message, &credential_response);

    // The operation is complete unless we received a 202 Accepted (more messages expected).
    if (credential_response.response_type
        == AZ_IOT_HUB_CLIENT_CERTIFICATE_SIGNING_RESPONSE_TYPE_ACCEPTED)
    {
      is_accepted = true;
    }
    else
    {
      is_operation_complete = true;
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topic);

  } while (!is_operation_complete);
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

static void parse_certificate_signing_response(
    char* topic,
    int topic_len,
    MQTTClient_message const* message,
    az_iot_hub_client_certificate_signing_response* out_response)
{
  az_span const topic_span = az_span_create((uint8_t*)topic, topic_len);
  az_span const message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  IOT_SAMPLE_LOG_SUCCESS("Client received a message:");
  IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
  IOT_SAMPLE_LOG_AZ_SPAN("Payload:", message_span);

  // Parse message and retrieve certificate signing response info.
  az_result rc = az_iot_hub_client_certificate_signing_request_parse_received_topic(
      &hub_client, topic_span, out_response);
  if (az_result_failed(rc))
  {
    IOT_SAMPLE_LOG_ERROR("Message from unknown topic: az_result return code 0x%08x.", rc);
    IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
    exit(rc);
  }
  IOT_SAMPLE_LOG_SUCCESS("Client received a valid certificate signing response.");
  IOT_SAMPLE_LOG("Status: %d", (int)out_response->status);
}

static void handle_certificate_signing_response(
    MQTTClient_message const* message,
    az_iot_hub_client_certificate_signing_response const* response)
{
  az_span const message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  switch (response->response_type)
  {
    case AZ_IOT_HUB_CLIENT_CERTIFICATE_SIGNING_RESPONSE_TYPE_ACCEPTED:
    {
      IOT_SAMPLE_LOG("Response Type: Accepted (202)");

      // Parse the 202 accepted response payload.
      az_iot_hub_client_certificate_signing_accepted_response accepted_response;
      az_result rc = az_iot_hub_client_certificate_signing_request_parse_accepted_response(
          &hub_client, message_span, &accepted_response);
      if (az_result_failed(rc))
      {
        IOT_SAMPLE_LOG_ERROR(
            "Failed to parse accepted response: az_result return code 0x%08x.", rc);
        exit(rc);
      }

      IOT_SAMPLE_LOG_AZ_SPAN("Correlation ID:", accepted_response.correlation_id);
      IOT_SAMPLE_LOG_AZ_SPAN("Operation Expires:", accepted_response.operation_expires);
      IOT_SAMPLE_LOG(
          "Request accepted. Waiting for certificate issuance "
          "(no polling needed)...");
      break;
    }

    case AZ_IOT_HUB_CLIENT_CERTIFICATE_SIGNING_RESPONSE_TYPE_COMPLETED:
    {
      IOT_SAMPLE_LOG("Response Type: Completed (200)");
      IOT_SAMPLE_LOG_SUCCESS("Certificate issued successfully!");

      // The 200 response payload contains the issued certificate.
      // Log the raw payload. The exact format is service-defined.
      IOT_SAMPLE_LOG_AZ_SPAN("Certificate response payload:", message_span);
      break;
    }

    case AZ_IOT_HUB_CLIENT_CERTIFICATE_SIGNING_RESPONSE_TYPE_ERROR:
    {
      IOT_SAMPLE_LOG_ERROR("Response Type: Error (%d)", (int)response->status);

      // Parse the error response payload.
      az_iot_hub_client_certificate_signing_error_response error_response;
      az_result rc = az_iot_hub_client_certificate_signing_request_parse_error_response(
          &hub_client, message_span, &error_response);
      if (az_result_failed(rc))
      {
        IOT_SAMPLE_LOG_ERROR(
            "Failed to parse error response: az_result return code 0x%08x.", rc);
        exit(rc);
      }

      IOT_SAMPLE_LOG("Error Code: %u", (unsigned int)error_response.error_code);
      IOT_SAMPLE_LOG_AZ_SPAN("Message:", error_response.message);
      IOT_SAMPLE_LOG_AZ_SPAN("Tracking ID:", error_response.tracking_id);
      IOT_SAMPLE_LOG_AZ_SPAN("Timestamp UTC:", error_response.timestamp_utc);

      if (az_span_size(error_response.correlation_id) > 0)
      {
        IOT_SAMPLE_LOG_AZ_SPAN("Correlation ID:", error_response.correlation_id);
      }
      if (az_span_size(error_response.credential_error) > 0)
      {
        IOT_SAMPLE_LOG_AZ_SPAN("Credential Error:", error_response.credential_error);
      }
      if (az_span_size(error_response.credential_message) > 0)
      {
        IOT_SAMPLE_LOG_AZ_SPAN("Credential Message:", error_response.credential_message);
      }
      if (az_span_size(error_response.info_request_id) > 0)
      {
        IOT_SAMPLE_LOG_AZ_SPAN("Info Request ID:", error_response.info_request_id);
      }
      if (az_span_size(error_response.info_operation_expires) > 0)
      {
        IOT_SAMPLE_LOG_AZ_SPAN("Info Operation Expires:", error_response.info_operation_expires);
      }
      if (error_response.retry_after_seconds > 0)
      {
        IOT_SAMPLE_LOG(
            "Retry After: %u seconds", (unsigned int)error_response.retry_after_seconds);
      }

      exit(1);
      break;
    }
  }
}

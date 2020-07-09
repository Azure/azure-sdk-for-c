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
#ifdef _WIN32
// Required for Sleep(DWORD)
#include <Windows.h>
#else
// Required for sleep(unsigned int)
#include <unistd.h>
#endif

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_provisioning_client.h>

#ifdef _MSC_VER
// "'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead."
#pragma warning(disable : 4996)
#endif

// DO NOT MODIFY: Service information
#define ENV_GLOBAL_PROVISIONING_ENDPOINT_DEFAULT "ssl://global.azure-devices-provisioning.net:8883"
#define ENV_GLOBAL_PROVISIONING_ENDPOINT "AZ_IOT_GLOBAL_PROVISIONING_ENDPOINT"
#define ENV_ID_SCOPE_ENV "AZ_IOT_ID_SCOPE"

// DO NOT MODIFY: Device information
#define ENV_REGISTRATION_ID_ENV "AZ_IOT_REGISTRATION_ID"

// DO NOT MODIFY: the path to a PEM file containing the device certificate and
// key as well as any intermediate certificates chaining to an uploaded group certificate.
#define ENV_DEVICE_X509_CERT_PEM_FILE "AZ_IOT_DEVICE_X509_CERT_PEM_FILE"

// DO NOT MODIFY: the path to a PEM file containing the server trusted CA
// This is usually not needed on Linux or Mac but needs to be set on Windows.
#define ENV_DEVICE_X509_TRUST_PEM_FILE "AZ_IOT_DEVICE_X509_TRUST_PEM_FILE"

#define TIMEOUT_MQTT_RECEIVE_MS (60 * 1000)
#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)

// Logging
#define LOG_ERROR(...) \
  { \
    (void)printf("ERROR: %s:%s:%d: ", __FILE__, __func__, __LINE__); \
    (void)printf(__VA_ARGS__); \
    fflush(stdout); \
    fflush(stderr); \
  }
#define LOG_SUCCESS(...) \
  { \
    (void)printf("SUCCESS: "); \
    (void)printf(__VA_ARGS__); \
  }

// Buffers
static char x509_cert_pem_file_buffer[256] = { 0 };
static char x509_trust_pem_file_buffer[256] = { 0 };
static char global_provisioning_endpoint_buffer[256] = { 0 };
static char id_scope_buffer[16] = { 0 };
static char registration_id_buffer[256] = { 0 };
static char mqtt_client_id_buffer[128];
static char mqtt_username_buffer[128];
static char register_publish_topic_buffer[128];
static char query_topic_buffer[256];

// Clients
static az_iot_provisioning_client provisioning_client;
static MQTTClient mqtt_client;

// Functions
static void create_and_configure_client();
static az_result read_environment_variables(
    az_span* endpoint,
    az_span* id_scope,
    az_span* registration_id);
static az_result read_configuration_entry(
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span buffer,
    az_span* out_value);
static void connect_client_to_provisioning_service();
static void subscribe_client_to_provisioning_service_topics();
static void register_client_with_provisioning_service();
static void receive_registration_status();
static void parse_operation_message(
    char* topic,
    int topic_len,
    const MQTTClient_message* message,
    az_iot_provisioning_client_register_response* response,
    az_iot_provisioning_client_operation_status* operation_status);
static void send_operation_query_message(
    const az_iot_provisioning_client_register_response* response);
static void disconnect_client_from_provisioning_service();
static void sleep_for_seconds(uint32_t seconds);
static void print_az_span(const char* span_description, az_span span);

int main()
{
  create_and_configure_client();
  LOG_SUCCESS("Client created and configured.\n");

  connect_client_to_provisioning_service();
  LOG_SUCCESS("Client connected to \"%s\".\n", global_provisioning_endpoint_buffer);

  subscribe_client_to_provisioning_service_topics();
  LOG_SUCCESS("Client subscribed to provisioning service topics.\n");

  register_client_with_provisioning_service();
  LOG_SUCCESS("Client registered with provisioning service.\n");

  receive_registration_status();
  LOG_SUCCESS("Client received registration status.\n")

  disconnect_client_from_provisioning_service();
  LOG_SUCCESS("Client disconnected from provisioning service.\n");

  return 0;
}

static void create_and_configure_client()
{
  int rc;
  az_span endpoint;
  az_span id_scope;
  az_span registration_id;

  if (az_failed(rc = read_environment_variables(&endpoint, &id_scope, &registration_id)))
  {
    LOG_ERROR("Failed to read evironment variables: az_result return code 0x%04x .\n", rc);
    exit(rc);
  }

  if (az_failed(
          rc = az_iot_provisioning_client_init(
              &provisioning_client, endpoint, id_scope, registration_id, NULL)))
  {
    LOG_ERROR("Failed to initiate provisioning client: az_result return code 0x%04x .\n", rc);
    exit(rc);
  }

  if (az_failed(
          rc = az_iot_provisioning_client_get_client_id(
              &provisioning_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT client id: az_result return code 0x%04x .\n", rc);
    exit(rc);
  }

  if ((rc = MQTTClient_create(
           &mqtt_client,
           (char*)az_span_ptr(endpoint),
           mqtt_client_id_buffer,
           MQTTCLIENT_PERSISTENCE_NONE,
           NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to create MQTT client: MQTTClient return code %d .\n", rc);
    exit(rc);
  }
}

static az_result read_environment_variables(
    az_span* endpoint,
    az_span* id_scope,
    az_span* registration_id)
{
  // Certification variables
  az_span cert = AZ_SPAN_FROM_BUFFER(x509_cert_pem_file_buffer);
  AZ_RETURN_IF_FAILED(
      read_configuration_entry(ENV_DEVICE_X509_CERT_PEM_FILE, NULL, false, cert, &cert));

  az_span trust = AZ_SPAN_FROM_BUFFER(x509_trust_pem_file_buffer);
  AZ_RETURN_IF_FAILED(
      read_configuration_entry(ENV_DEVICE_X509_TRUST_PEM_FILE, "", false, trust, &trust));

  // Connection variables
  *endpoint = AZ_SPAN_FROM_BUFFER(global_provisioning_endpoint_buffer);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_GLOBAL_PROVISIONING_ENDPOINT,
      ENV_GLOBAL_PROVISIONING_ENDPOINT_DEFAULT,
      false,
      *endpoint,
      endpoint));

  *id_scope = AZ_SPAN_FROM_BUFFER(id_scope_buffer);
  AZ_RETURN_IF_FAILED(read_configuration_entry(ENV_ID_SCOPE_ENV, NULL, false, *id_scope, id_scope));

  *registration_id = AZ_SPAN_FROM_BUFFER(registration_id_buffer);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_REGISTRATION_ID_ENV, NULL, false, *registration_id, registration_id));

  putchar('\n');
  return AZ_OK;
}

static az_result read_configuration_entry(
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span buffer,
    az_span* out_value)
{
  (void)printf("%s = ", env_name);
  char* env_value = getenv(env_name);

  if (env_value == NULL && default_value != NULL)
  {
    env_value = default_value;
  }

  if (env_value != NULL)
  {
    (void)printf("%s\n", hide_value ? "***" : env_value);
    az_span env_span = az_span_from_str(env_value);
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(buffer, az_span_size(env_span));
    az_span_copy(buffer, env_span);
    *out_value = az_span_slice(buffer, 0, az_span_size(env_span));
  }
  else
  {
    LOG_ERROR("(missing) Please set the %s environment variable.\n", env_name);
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

static void connect_client_to_provisioning_service()
{
  int rc;

  if (az_failed(
          rc = az_iot_provisioning_client_get_user_name(
              &provisioning_client, mqtt_username_buffer, sizeof(mqtt_username_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT username: az_result return code 0x%04x .\n", rc);
    exit(rc);
  }

  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;
  mqtt_connect_options.username = mqtt_username_buffer;
  mqtt_connect_options.password = NULL; // This sample uses x509 authentication.
  mqtt_connect_options.cleansession = false; // Set to false so can receive any pending messages.
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  mqtt_ssl_options.keyStore = (char*)x509_cert_pem_file_buffer;
  if (*x509_trust_pem_file_buffer != '\0')
  {
    mqtt_ssl_options.trustStore = (char*)x509_trust_pem_file_buffer;
  }
  mqtt_connect_options.ssl = &mqtt_ssl_options;

  if ((rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to connect: MQTTClient return code %d .\n", rc);
    exit(rc);
  }

  return;
}

static void subscribe_client_to_provisioning_service_topics()
{
  int rc;

  if ((rc
       = MQTTClient_subscribe(mqtt_client, AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR(
        "Failed to subscribe to the register topic filter: MQTTClient return code %d .\n", rc);
    exit(rc);
  }

  return;
}

static void register_client_with_provisioning_service()
{
  int rc;

  if (az_failed(
          rc = az_iot_provisioning_client_register_get_publish_topic(
              &provisioning_client,
              register_publish_topic_buffer,
              sizeof(register_publish_topic_buffer),
              NULL)))
  {
    LOG_ERROR("Failed to get MQTT register publish topic: az_result return code 0x%04x .\n", rc);
    exit(rc);
  }

  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  pubmsg.payload = NULL; // empty payload.
  pubmsg.payloadlen = 0;
  pubmsg.qos = 1;
  pubmsg.retained = 0;

  if ((rc = MQTTClient_publishMessage(mqtt_client, register_publish_topic_buffer, &pubmsg, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to publish register request: MQTTClient return code %d .\n", rc);
    exit(rc);
  }

  return;
}

static void receive_registration_status()
{
  int rc;
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;
  az_iot_provisioning_client_register_response response;
  az_iot_provisioning_client_operation_status operation_status;
  bool is_operation_complete = false;

  // Continue to parse incoming responses from the Device Provisioning Service until
  // the device has been successfully provisioned or an error occurs
  do
  {
    if (((rc
          = MQTTClient_receive(mqtt_client, &topic, &topic_len, &message, TIMEOUT_MQTT_RECEIVE_MS))
         != MQTTCLIENT_SUCCESS)
        && (MQTTCLIENT_TOPICNAME_TRUNCATED != rc))
    {
      LOG_ERROR("Failed to receive message: MQTTClient return code %d .\n", rc);
      exit(rc);
    }
    else if (!message)
    {
      LOG_ERROR("Timeout expired: MQTTClient return code %d .\n", rc);
      exit(rc);
    }
    else if (MQTTCLIENT_TOPICNAME_TRUNCATED == rc)
    {
      topic_len = (int)strlen(topic);
    }
    LOG_SUCCESS("Client received a message from the provisioning service.\n");

    parse_operation_message(topic, topic_len, message, &response, &operation_status);
    LOG_SUCCESS("Client parsed operation message.\n");

    // If operation is not complete, send query and loop to receive operation message
    is_operation_complete = az_iot_provisioning_client_operation_complete(operation_status);
    if (!is_operation_complete)
    {
      (void)printf("\t Operation incomplete.\n");
      MQTTClient_freeMessage(&message);
      MQTTClient_free(topic);

      send_operation_query_message(&response);
      LOG_SUCCESS("Client sent operation query message.\n");
    }
  } while (!is_operation_complete);

  // Operation is complete: Successful assignment
  if (operation_status == AZ_IOT_PROVISIONING_STATUS_ASSIGNED)
  {
    LOG_SUCCESS("Client provisioned:\n");
    print_az_span("\t Hub Hostname: ", response.registration_result.assigned_hub_hostname);
    print_az_span("\t Device Id: ", response.registration_result.device_id);
  }
  else // Unsuccesful assignment (unassigned, failed or disabled states)
  {
    LOG_ERROR("Client provisioning failed:\n");
    print_az_span("\tRegistration state: ", response.operation_status);
    (void)printf("\tLast operation status: %d\n", response.status);
    print_az_span("\tOperation ID: ", response.operation_id);
    (void)printf("\tError code: %u\n", response.registration_result.extended_error_code);
    print_az_span("\tError message: ", response.registration_result.error_message);
    print_az_span("\tError timestamp: ", response.registration_result.error_timestamp);
    print_az_span("\tError tracking ID: ", response.registration_result.error_tracking_id);
  }

  MQTTClient_freeMessage(&message);
  MQTTClient_free(topic);
  return;
}

static void parse_operation_message(
    char* topic,
    int topic_len,
    MQTTClient_message const* message,
    az_iot_provisioning_client_register_response* response,
    az_iot_provisioning_client_operation_status* operation_status)
{
  int rc;
  az_span topic_span = az_span_init((uint8_t*)topic, topic_len);
  az_span message_span = az_span_init((uint8_t*)message->payload, message->payloadlen);

  if (az_failed(
          rc = az_iot_provisioning_client_parse_received_topic_and_payload(
              &provisioning_client, topic_span, message_span, response)))
  {
    LOG_ERROR("Message from unknown topic: az_result return code 0x%04x .\n", rc);
    exit(rc);
  }
  LOG_SUCCESS("Received response:\n");
  print_az_span("\t Topic: ", topic_span);
  print_az_span("\t Payload: ", message_span);
  (void)printf("\t Response status: %d\n", response->status);

  if (az_failed(rc = az_iot_provisioning_client_parse_operation_status(response, operation_status)))
  {
    LOG_ERROR("Failed to parse operation_status: az_result return code 0x%04x .\n", rc);
    exit(rc);
  }

  return;
}

static void send_operation_query_message(
    az_iot_provisioning_client_register_response const* response)
{
  int rc;

  if (az_failed(
          rc = az_iot_provisioning_client_query_status_get_publish_topic(
              &provisioning_client, response, query_topic_buffer, sizeof(query_topic_buffer), NULL)))
  {
    LOG_ERROR("Unable to get query status publish topic: az_result return code 0x%04x .\n", rc);
    exit(rc);
  }

  // IMPORTANT: Wait the recommended retry-after number of seconds before query
  (void)printf("\t Querying after %u seconds...\n", response->retry_after_seconds);
  sleep_for_seconds(response->retry_after_seconds);

  if ((rc = MQTTClient_publish(mqtt_client, query_topic_buffer, 0, NULL, 0, 0, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to publish query status request: MQTTClient return code %d .\n", rc);
    exit(rc);
  }

  return;
}

static void disconnect_client_from_provisioning_service()
{
  int rc;

  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to disconnect MQTT client: MQTTClient return code %d .\n", rc);
    exit(rc);
  }

  MQTTClient_destroy(&mqtt_client);

  return;
}

static void sleep_for_seconds(uint32_t seconds)
{
#ifdef _WIN32
  Sleep((DWORD)seconds * 1000);
#else
  sleep(seconds);
#endif
  return;
}

static void print_az_span(char const* span_description, az_span span)
{
  (void)printf("%s", span_description);

  char* buffer = (char*)az_span_ptr(span);
  for (int32_t i = 0; i < az_span_size(span); i++)
  {
    putchar(*buffer++);
  }
  putchar('\n');

  return;
}

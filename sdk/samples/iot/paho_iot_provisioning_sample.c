// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <sample.h>

// Store environment variables
static char global_provisioning_endpoint_buffer[256];
static char id_scope_buffer[16];
static char registration_id_buffer[256];
static char x509_cert_pem_file_path_buffer[256];
static char x509_trust_pem_file_path_buffer[256];

// Clients
static az_iot_provisioning_client provisioning_client;
static MQTTClient mqtt_client;
static char mqtt_client_id_buffer[128];
static char mqtt_username_buffer[128];

// Topics
static char register_publish_topic_buffer[128];
static char query_topic_buffer[256];

// Functions
static void create_and_configure_client();
static void connect_client_to_provisioning_service();
static void subscribe_client_to_provisioning_service_topics();
static void register_client_with_provisioning_service();
static void receive_registration_status();
static void disconnect_client_from_provisioning_service();

static void parse_operation_message(
    char* topic,
    int topic_len,
    const MQTTClient_message* message,
    az_iot_provisioning_client_register_response* response,
    az_iot_provisioning_client_operation_status* operation_status);
static void send_operation_query_message(
    const az_iot_provisioning_client_register_response* response);


int main()
{
  create_and_configure_client();
  LOG_SUCCESS("Client created and configured.");

  connect_client_to_provisioning_service();
  LOG_SUCCESS("Client connected to \"%s\".", global_provisioning_endpoint_buffer);

  subscribe_client_to_provisioning_service_topics();
  LOG_SUCCESS("Client subscribed to provisioning service topics.");

  register_client_with_provisioning_service();
  LOG_SUCCESS("Client registering with provisioning service.");

  receive_registration_status();
  LOG_SUCCESS("Client received registration status.")

  disconnect_client_from_provisioning_service();
  LOG_SUCCESS("Client disconnected from provisioning service.");

  return 0;
}

static void create_and_configure_client()
{
  int rc;
  az_span global_provisioning_endpoint;
  az_span id_scope;
  az_span registration_id;

  if (az_failed(
          rc
          = read_environment_variables(&global_provisioning_endpoint, &id_scope, &registration_id)))
  {
    LOG_ERROR("Failed to read environment variables: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  if (az_failed(
          rc = az_iot_provisioning_client_init(
              &provisioning_client, global_provisioning_endpoint, id_scope, registration_id, NULL)))
  {
    LOG_ERROR("Failed to initialize provisioning client: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  if (az_failed(
          rc = az_iot_provisioning_client_get_client_id(
              &provisioning_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT client id: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  if ((rc = MQTTClient_create(
           &mqtt_client,
           (char*)az_span_ptr(global_provisioning_endpoint),
           mqtt_client_id_buffer,
           MQTTCLIENT_PERSISTENCE_NONE,
           NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to create MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

static az_result read_environment_variables(
    az_span* global_provisioning_endpoint,
    az_span* id_scope,
    az_span* registration_id)
{
  // Connection variables
  *global_provisioning_endpoint = AZ_SPAN_FROM_BUFFER(global_provisioning_endpoint_buffer);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_GLOBAL_PROVISIONING_ENDPOINT,
      ENV_GLOBAL_PROVISIONING_ENDPOINT_DEFAULT,
      false,
      *global_provisioning_endpoint,
      global_provisioning_endpoint));

  *id_scope = AZ_SPAN_FROM_BUFFER(id_scope_buffer);
  AZ_RETURN_IF_FAILED(read_configuration_entry(ENV_ID_SCOPE_ENV, NULL, false, *id_scope, id_scope));

  *registration_id = AZ_SPAN_FROM_BUFFER(registration_id_buffer);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_REGISTRATION_ID_ENV, NULL, false, *registration_id, registration_id));

  // Certification variables
  az_span x509_cert_pem_file_path = AZ_SPAN_FROM_BUFFER(x509_cert_pem_file_path_buffer);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_DEVICE_X509_CERT_PEM_FILE_PATH,
      NULL,
      false,
      x509_cert_pem_file_path,
      &x509_cert_pem_file_path));

  az_span x509_trust_pem_file_path = AZ_SPAN_FROM_BUFFER(x509_trust_pem_file_path_buffer);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_DEVICE_X509_TRUST_PEM_FILE_PATH,
      "",
      false,
      x509_trust_pem_file_path,
      &x509_trust_pem_file_path));

  LOG(" "); // Log formatting
  return AZ_OK;
}

static void connect_client_to_provisioning_service()
{
  int rc;

  if (az_failed(
          rc = az_iot_provisioning_client_get_user_name(
              &provisioning_client, mqtt_username_buffer, sizeof(mqtt_username_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT username: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;
  mqtt_connect_options.username = mqtt_username_buffer;
  mqtt_connect_options.password = NULL; // This sample uses x509 authentication.
  mqtt_connect_options.cleansession = false; // Set to false so can receive any pending messages.
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  mqtt_ssl_options.keyStore = (char*)x509_cert_pem_file_path_buffer;
  if (*x509_trust_pem_file_path_buffer != '\0')
  {
    mqtt_ssl_options.trustStore = (char*)x509_trust_pem_file_path_buffer;
  }
  mqtt_connect_options.ssl = &mqtt_ssl_options;

  if ((rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR(
        "Failed to connect: MQTTClient return code %d.\n"
        "If on Windows, confirm the AZ_IOT_DEVICE_X509_TRUST_PEM_FILE environment variable is set "
        "correctly.",
        rc);
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
    LOG_ERROR("Failed to subscribe to the register topic: MQTTClient return code %d.", rc);
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
    LOG_ERROR("Failed to get MQTT register publish topic: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  pubmsg.payload = NULL; // Empty payload
  pubmsg.payloadlen = 0;
  pubmsg.qos = 1;
  pubmsg.retained = 0;

  if ((rc = MQTTClient_publishMessage(mqtt_client, register_publish_topic_buffer, &pubmsg, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to publish register request: MQTTClient return code %d.", rc);
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
  // the device has been successfully provisioned or an error occurs.
  do
  {
    if (((rc
          = MQTTClient_receive(mqtt_client, &topic, &topic_len, &message, TIMEOUT_MQTT_RECEIVE_MS))
         != MQTTCLIENT_SUCCESS)
        && (MQTTCLIENT_TOPICNAME_TRUNCATED != rc))
    {
      LOG_ERROR("Failed to receive message: MQTTClient return code %d.", rc);
      exit(rc);
    }
    else if (NULL == message)
    {
      LOG_ERROR("Timeout expired: MQTTClient return code %d.", rc);
      exit(rc);
    }
    else if (MQTTCLIENT_TOPICNAME_TRUNCATED == rc)
    {
      topic_len = (int)strlen(topic);
    }
    LOG_SUCCESS("Client received a message from provisioning service.");

    parse_operation_message(topic, topic_len, message, &response, &operation_status);
    LOG_SUCCESS("Client parsed operation message.");

    // If operation is not complete, send query and loop to receive operation message
    is_operation_complete = az_iot_provisioning_client_operation_complete(operation_status);
    if (!is_operation_complete)
    {
      LOG("Operation is still pending.");

      send_operation_query_message(&response);

      MQTTClient_freeMessage(&message);
      MQTTClient_free(topic);
      LOG_SUCCESS("Client sent operation query message.");
    }
  } while (!is_operation_complete);

  // Operation is complete: Successful assignment
  if (AZ_IOT_PROVISIONING_STATUS_ASSIGNED == operation_status)
  {
    LOG_SUCCESS("Client provisioned:");
    LOG_AZ_SPAN("Hub Hostname:", response.registration_result.assigned_hub_hostname);
    LOG_AZ_SPAN("Device Id:", response.registration_result.device_id);
  }
  else // Unsuccessful assignment (unassigned, failed or disabled states)
  {
    LOG_ERROR("Client provisioning failed:");
    LOG_AZ_SPAN("Registration state:", response.operation_status);
    LOG("Last operation status: %d", response.status);
    LOG_AZ_SPAN("Operation ID:", response.operation_id);
    LOG("Error code: %u", response.registration_result.extended_error_code);
    LOG_AZ_SPAN("Error message:", response.registration_result.error_message);
    LOG_AZ_SPAN("Error timestamp:", response.registration_result.error_timestamp);
    LOG_AZ_SPAN("Error tracking ID:", response.registration_result.error_tracking_id);
    exit((int)response.registration_result.extended_error_code);
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
    LOG_ERROR("Message from unknown topic: az_result return code 0x%04x.", rc);
    exit(rc);
  }
  LOG_SUCCESS("Client received a valid topic response:");
  LOG_AZ_SPAN("Topic:", topic_span);
  LOG_AZ_SPAN("Payload:", message_span);
  LOG("Response status: %d", response->status);

  if (az_failed(rc = az_iot_provisioning_client_parse_operation_status(response, operation_status)))
  {
    LOG_ERROR("Failed to parse operation_status: az_result return code 0x%04x.", rc);
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
              &provisioning_client,
              response,
              query_topic_buffer,
              sizeof(query_topic_buffer),
              NULL)))
  {
    LOG_ERROR("Unable to get query status publish topic: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // IMPORTANT: Wait the recommended retry-after number of seconds before query
  LOG("Querying after %u seconds...", response->retry_after_seconds);
  sleep_for_seconds(response->retry_after_seconds);

  if ((rc = MQTTClient_publish(mqtt_client, query_topic_buffer, 0, NULL, 0, 0, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to publish query status request: MQTTClient return code %d.", rc);
    exit(rc);
  }

  return;
}

static void disconnect_client_from_provisioning_service()
{
  int rc;

  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to disconnect MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }

  MQTTClient_destroy(&mqtt_client);

  return;
}

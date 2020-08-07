// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "sample.h"

#define SAMPLE_TYPE PAHO_IOT_PROVISIONING
#define SAMPLE_NAME PAHO_IOT_PROVISIONING_SAS_SAMPLE

// Environment variables
static sample_environment_variables env_vars;

// Clients
static az_iot_provisioning_client provisioning_client;
static MQTTClient mqtt_client;
static char mqtt_client_username_buffer[128];

// Generate SAS key variables
static char sas_signature_buffer[128];
static char sas_b64_decoded_key_buffer[32];
static char sas_encoded_hmac256_signed_signature_buffer[128];
static char sas_b64_encoded_hmac256_signed_signature_buffer[128];
static char mqtt_password_buffer[256];

// Functions
void create_and_configure_client();
void connect_client_to_provisioning_service();
void subscribe_client_to_provisioning_service_topics();
void register_client_with_provisioning_service();
void receive_registration_status();
void disconnect_client_from_provisioning_service();

void parse_operation_message(
    char* topic,
    int topic_len,
    const MQTTClient_message* message,
    az_iot_provisioning_client_register_response* response,
    az_iot_provisioning_client_operation_status* operation_status);
void send_operation_query_message(
    const az_iot_provisioning_client_register_response* response);
void generate_sas_key();

int main()
{
  create_and_configure_client();
  LOG_SUCCESS("Client created and configured.");

  connect_client_to_provisioning_service();
  LOG_SUCCESS("Client connected to provisioning service.");

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

void create_and_configure_client()
{
  int rc;

  // Reads in environment variables set by user for purposes of running sample
  if (az_failed(rc = read_environment_variables(SAMPLE_TYPE, SAMPLE_NAME, &env_vars)))
  {
    LOG_ERROR(
        "Failed to read configuration from environment variables: az_result return code 0x%04x.",
        rc);
    exit(rc);
  }

  // Set mqtt endpoint as null terminated in buffer
  char provisioning_global_endpoint_buffer[256];
  if (az_failed(
          rc = create_mqtt_endpoint(
              SAMPLE_TYPE,
              provisioning_global_endpoint_buffer,
              sizeof(provisioning_global_endpoint_buffer))))
  {
    LOG_ERROR("Failed to create MQTT endpoint: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Initialize the provisioning client with the provisioning global endpoint and the default
  // connection options
  if (az_failed(
          rc = az_iot_provisioning_client_init(
              &provisioning_client,
              az_span_from_str(provisioning_global_endpoint_buffer),
              env_vars.provisioning_id_scope,
              env_vars.provisioning_registration_id,
              NULL)))
  {
    LOG_ERROR("Failed to initialize provisioning client: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Get the MQTT client id used for the MQTT connection
  char mqtt_client_id_buffer[128];
  if (az_failed(
          rc = az_iot_provisioning_client_get_client_id(
              &provisioning_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT client id: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Create the Paho MQTT client
  if ((rc = MQTTClient_create(
           &mqtt_client,
           provisioning_global_endpoint_buffer,
           mqtt_client_id_buffer,
           MQTTCLIENT_PERSISTENCE_NONE,
           NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to create MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }

  generate_sas_key();
  LOG_SUCCESS("Client generated SAS Key.");

  return;
}

void connect_client_to_provisioning_service()
{
  int rc;

  if (az_failed(
          rc = az_iot_provisioning_client_get_user_name(
              &provisioning_client,
              mqtt_client_username_buffer,
              sizeof(mqtt_client_username_buffer),
              NULL)))
  {
    LOG_ERROR("Failed to get MQTT username: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;
  mqtt_connect_options.username = mqtt_client_username_buffer;
  mqtt_connect_options.password = mqtt_password_buffer;
  mqtt_connect_options.cleansession = false; // Set to false so can receive any pending messages.
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  if (*az_span_ptr(env_vars.x509_trust_pem_file_path) != '\0')
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

void subscribe_client_to_provisioning_service_topics()
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

void register_client_with_provisioning_service()
{
  int rc;

  char register_publish_topic_buffer[128];
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

void receive_registration_status()
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
      LOG_SUCCESS("Client sent operation query message.");

      MQTTClient_freeMessage(&message);
      MQTTClient_free(topic);
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

void disconnect_client_from_provisioning_service()
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

void parse_operation_message(
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

void send_operation_query_message(
    az_iot_provisioning_client_register_response const* response)
{
  int rc;

  char query_topic_buffer[256];
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

void generate_sas_key()
{
  int rc;

  // Create the POSIX expiration time from input hours
  uint32_t sas_duration = get_epoch_expiration_time_from_hours(env_vars.sas_key_duration_minutes);

  // Get the signature which will be signed with the decoded key
  az_span sas_signature = AZ_SPAN_FROM_BUFFER(sas_signature_buffer);
  if (az_failed(
          rc = az_iot_provisioning_client_sas_get_signature(
              &provisioning_client, sas_duration, sas_signature, &sas_signature)))
  {
    LOG_ERROR("Could not get the signature for SAS key: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Decode the base64 encoded SAS key to use for HMAC signing
  az_span sas_b64_decoded_key = AZ_SPAN_FROM_BUFFER(sas_b64_decoded_key_buffer);
  if (az_failed(
          rc = sample_base64_decode(
              env_vars.provisioning_sas_key, sas_b64_decoded_key, &sas_b64_decoded_key)))
  {
    LOG_ERROR("Could not decode the SAS key: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // HMAC-SHA256 sign the signature with the decoded key
  az_span sas_encoded_hmac256_signed_signature
      = AZ_SPAN_FROM_BUFFER(sas_encoded_hmac256_signed_signature_buffer);
  if (az_failed(
          rc = sample_hmac_sha256_sign(
              sas_b64_decoded_key,
              sas_signature,
              sas_encoded_hmac256_signed_signature,
              &sas_encoded_hmac256_signed_signature)))
  {
    LOG_ERROR("Could not sign the signature: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // base64 encode the result of the HMAC signing
  az_span sas_b64_encoded_hmac256_signed_signature
      = AZ_SPAN_FROM_BUFFER(sas_b64_encoded_hmac256_signed_signature_buffer);
  if (az_failed(
          rc = sample_base64_encode(
              sas_encoded_hmac256_signed_signature,
              sas_b64_encoded_hmac256_signed_signature,
              &sas_b64_encoded_hmac256_signed_signature)))
  {
    LOG_ERROR("Could not base64 encode the password: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Get the resulting password, passing the base64 encoded, HMAC signed bytes
  size_t mqtt_password_length;
  if (az_failed(
          rc = az_iot_provisioning_client_sas_get_password(
              &provisioning_client,
              sas_b64_encoded_hmac256_signed_signature,
              sas_duration,
              AZ_SPAN_NULL,
              mqtt_password_buffer,
              sizeof(mqtt_password_buffer),
              &mqtt_password_length)))
  {
    LOG_ERROR("Could not get the password: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  return;
}

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
#ifdef _WIN32
// Required for Sleep(DWORD)
#include <Windows.h>
#else
// Required for sleep(unsigned int)
#include <unistd.h>
#endif

#include "sample_sas_utility.h"

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#ifdef _MSC_VER
// "'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead."
#pragma warning(disable : 4996)
#endif

// Comment to use MQTT without WebSockets.
#define USE_WEB_SOCKET
#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)
#define TELEMETRY_SEND_INTERVAL 1
#define NUMBER_OF_MESSAGES 5
#define SAS_TOKEN_EXPIRATION_TIME_DIGITS 4

// DO NOT MODIFY: Device ID Environment Variable Name
#define ENV_DEVICE_ID "AZ_IOT_DEVICE_ID_SAS"

// DO NOT MODIFY: IoT Hub Hostname Environment Variable Name
#define ENV_IOT_HUB_HOSTNAME "AZ_IOT_HUB_HOSTNAME"

// DO NOT MODIFY: IoT Hub SAS Key Environment Variable Name
#define ENV_IOT_HUB_SAS_KEY "AZ_IOT_HUB_DEVICE_SAS_KEY"

// DO NOT MODIFY: IoT Hub SAS Key Duration Environment Variable Name (defaults to 2 hours)
#define ENV_IOT_HUB_SAS_KEY_DURATION_MINUTES "AZ_IOT_HUB_DEVICE_SAS_KEY_DURATION_MINUTES"

// DO NOT MODIFY: the path to a PEM file containing the server trusted CA
// This is usually not needed on Linux or Mac but needs to be set on Windows.
#define ENV_DEVICE_X509_TRUST_PEM_FILE "AZ_IOT_DEVICE_X509_TRUST_PEM_FILE"

static const uint8_t null_terminator = '\0';
static char device_id[64];
static char iot_hub_hostname[128];
static uint32_t iot_hub_sas_key_expiration_minutes;
static char iot_hub_sas_key[128];
static az_span iot_hub_sas_key_span;
static char x509_trust_pem_file[256];
char telemetry_topic[128];

static char mqtt_client_id[128];
static char mqtt_username[128];
static char mqtt_endpoint[128];
static char mqtt_password[256];
static char sas_b64_decoded_key[32];
static char sas_signature_buf[128];
static char sas_signature_hmac_encoded_buf[128];
static char sas_signature_encoded_buf_b64[128];

#ifdef USE_WEB_SOCKET
static az_span mqtt_url_prefix = AZ_SPAN_LITERAL_FROM_STR("wss://");
// Note: Paho fails to connect to Hub when using AZ_IOT_HUB_CLIENT_WEB_SOCKET_PATH or an X509 certificate.
static az_span mqtt_url_suffix = AZ_SPAN_LITERAL_FROM_STR(":443" AZ_IOT_HUB_CLIENT_WEB_SOCKET_PATH_NO_X509_CLIENT_CERT);
#else
static az_span mqtt_url_prefix = AZ_SPAN_LITERAL_FROM_STR("ssl://");
static az_span mqtt_url_suffix = AZ_SPAN_LITERAL_FROM_STR(":8883");
#endif

static const char* telemetry_message_payloads[NUMBER_OF_MESSAGES] = {
  "Message One", "Message Two", "Message Three", "Message Four", "Message Five",
};

static az_iot_hub_client client;
static MQTTClient mqtt_client;

//
// Configuration and connection functions
//
static az_result read_configuration_and_init_client();
static az_result read_configuration_entry(
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span buffer,
    az_span* out_value);
static az_result create_mqtt_endpoint(char* destination, int32_t destination_size, az_span iot_hub);
static az_result generate_sas_key();
static uint64_t get_epoch_expiration_time_from_minutes(uint32_t minutes);
static int connect_device();

//
// Messaging functions
//
static int send_telemetry_messages();
static void sleep_for_seconds(uint32_t seconds);

int main()
{
  int rc;

  // Read in the necessary environment variables and initialize the az_iot_hub_client
  if (az_failed(rc = read_configuration_and_init_client()))
  {
    printf(
        "Failed to read configuration from environment variables, az_result return code %04x\n",
        rc);
    return rc;
  }

  // Get the MQTT client id used for the MQTT connection
  size_t client_id_length;
  if (az_failed(
          rc = az_iot_hub_client_get_client_id(
              &client, mqtt_client_id, sizeof(mqtt_client_id), &client_id_length)))
  {
    printf("Failed to get MQTT client id, az_result return code %04x\n", rc);
    return rc;
  }

  if (az_failed(rc = generate_sas_key()))
  {
    return rc;
  }

  // Create the Paho MQTT client
  if ((rc = MQTTClient_create(
           &mqtt_client, mqtt_endpoint, mqtt_client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to create MQTT client, MQTTClient return code %d\n", rc);
    return rc;
  }

  // Connect to IoT Hub
  if ((rc = connect_device()) != MQTTCLIENT_SUCCESS)
  {
    return rc;
  }

  // Loop and send 5 messages
  if ((rc = send_telemetry_messages()) != MQTTCLIENT_SUCCESS)
  {
    return rc;
  }

  // Gracefully disconnect: send the disconnect packet and close the socket
  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to disconnect MQTT client, MQTTClient return code %d\n", rc);
    return rc;
  }
  printf("Disconnected\n");

  // Clean up and release resources allocated by the mqtt client
  MQTTClient_destroy(&mqtt_client);

  return 0;
}

// Read the user environment variables used to connect to IoT Hub and initialize the IoT Hub client
static az_result read_configuration_and_init_client()
{
  az_span trusted = AZ_SPAN_FROM_BUFFER(x509_trust_pem_file);
  AZ_RETURN_IF_FAILED(
      read_configuration_entry(ENV_DEVICE_X509_TRUST_PEM_FILE, "", false, trusted, &trusted));

  char iot_hub_sas_key_expiration_char[SAS_TOKEN_EXPIRATION_TIME_DIGITS];
  az_span iot_hub_sas_expiration_span = AZ_SPAN_FROM_BUFFER(iot_hub_sas_key_expiration_char);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_IOT_HUB_SAS_KEY_DURATION_MINUTES,
      "120",
      false,
      iot_hub_sas_expiration_span,
      &iot_hub_sas_expiration_span));

  AZ_RETURN_IF_FAILED(az_span_atou32(iot_hub_sas_expiration_span, &iot_hub_sas_key_expiration_minutes));

  iot_hub_sas_key_span = AZ_SPAN_FROM_BUFFER(iot_hub_sas_key);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_IOT_HUB_SAS_KEY, NULL, true, iot_hub_sas_key_span, &iot_hub_sas_key_span));

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
  AZ_RETURN_IF_FAILED(az_iot_hub_client_init(
      &client,
      az_span_slice(iot_hub_hostname_span, 0, (int32_t)strlen(iot_hub_hostname)),
      az_span_slice(device_id_span, 0, (int32_t)strlen(device_id)),
      NULL));

  return AZ_OK;
}

// Read OS environment variables using stdlib function
static az_result read_configuration_entry(
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span buffer,
    az_span* out_value)
{
  printf("%s = ", env_name);
  char* env_value = getenv(env_name);

  if (env_value == NULL && default_value != NULL)
  {
    env_value = default_value;
  }

  if (env_value != NULL)
  {
    printf("%s\n", hide_value ? "***" : env_value);
    az_span env_span = az_span_from_str(env_value);
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(buffer, az_span_size(env_span));
    az_span_copy(buffer, env_span);
    *out_value = az_span_slice(buffer, 0, az_span_size(env_span));
  }
  else
  {
    printf("(missing) Please set the %s environment variable.\n", env_name);
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

// Create mqtt endpoint e.g: ssl//contoso.azure-devices.net:8883
static az_result create_mqtt_endpoint(char* destination, int32_t destination_size, az_span iot_hub)
{
  int32_t iot_hub_length = (int32_t)strlen(iot_hub_hostname);
  int32_t required_size = az_span_size(mqtt_url_prefix) + iot_hub_length
      + az_span_size(mqtt_url_suffix) + (int32_t)sizeof(null_terminator);

  if (required_size > destination_size)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }

  az_span destination_span = az_span_init((uint8_t*)destination, destination_size);
  az_span remainder = az_span_copy(destination_span, mqtt_url_prefix);
  remainder = az_span_copy(remainder, az_span_slice(iot_hub, 0, iot_hub_length));
  remainder = az_span_copy(remainder, mqtt_url_suffix);
  az_span_copy_u8(remainder, null_terminator);

  return AZ_OK;
}

static az_result generate_sas_key()
{
  az_result rc;

  // Create the POSIX expiration time from input hours
  uint64_t sas_expiration = get_epoch_expiration_time_from_minutes(iot_hub_sas_key_expiration_minutes);

  // Decode the base64 encoded SAS key to use for HMAC signing
  az_span decoded_key_span;
  if (az_failed(
          rc = sample_base64_decode(
              iot_hub_sas_key_span, AZ_SPAN_FROM_BUFFER(sas_b64_decoded_key), &decoded_key_span)))
  {
    printf("Could not decode the SAS key, az_result return code %04x\n", rc);
    return rc;
  }

  // Get the signature which will be signed with the decoded key
  az_span sas_signature_span;
  if (az_failed(
          rc = az_iot_hub_client_sas_get_signature(
              &client,
              sas_expiration,
              AZ_SPAN_FROM_BUFFER(sas_signature_buf),
              &sas_signature_span)))
  {
    printf("Could not get the signature for SAS key, az_result return code %04x\n", rc);
    return rc;
  }

  // HMAC-SHA256 sign the signature with the decoded key
  az_span hmac256_signed_span = AZ_SPAN_FROM_BUFFER(sas_signature_hmac_encoded_buf);
  if (az_failed(
          rc = sample_hmac_sha256_sign(
              decoded_key_span, sas_signature_span, hmac256_signed_span, &hmac256_signed_span)))
  {
    printf("Could not sign the signature, az_result return code %04x\n", rc);
    return rc;
  }

  // base64 encode the result of the HMAC signing
  az_span b64_encoded_hmac256_signed_signature;
  if (az_failed(
          rc = sample_base64_encode(
              hmac256_signed_span,
              AZ_SPAN_FROM_BUFFER(sas_signature_encoded_buf_b64),
              &b64_encoded_hmac256_signed_signature)))
  {
    printf("Could not base64 encode the password, az_result return code %04x\n", rc);
    return rc;
  }

  // Get the resulting password, passing the base64 encoded, HMAC signed bytes
  size_t mqtt_password_length;
  if (az_failed(
          rc = az_iot_hub_client_sas_get_password(
              &client,
              b64_encoded_hmac256_signed_signature,
              sas_expiration,
              AZ_SPAN_NULL,
              mqtt_password,
              sizeof(mqtt_password),
              &mqtt_password_length)))
  {
    printf("Could not get the password, az_result return code %04x\n", rc);
    return rc;
  }

  return AZ_OK;
}

static uint64_t get_epoch_expiration_time_from_minutes(uint32_t minutes)
{
  return (uint64_t)(time(NULL) + minutes * 60);
}

static int connect_device()
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
    printf("Failed to get MQTT username, az_result return code %04x\n", rc);
    return rc;
  }

  // This sample uses SAS authentication so the password field is set to the generated password
  mqtt_connect_options.username = mqtt_username;
  mqtt_connect_options.password = mqtt_password;

  if (*x509_trust_pem_file != '\0')
  {
    mqtt_ssl_options.trustStore = (char*)x509_trust_pem_file;
  }

  mqtt_connect_options.ssl = &mqtt_ssl_options;

  // Connect to IoT Hub
  if ((rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to connect, MQTTClient return code %d\n", rc);
    return rc;
  }

  return MQTTCLIENT_SUCCESS;
}

static int send_telemetry_messages()
{
  int rc;

  if (az_failed(
          rc = az_iot_hub_client_telemetry_get_publish_topic(
              &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    printf("Unable to get telemetry publish topic, az_result return code %04x\n", rc);
    return rc;
  }

  for (int i = 0; i < NUMBER_OF_MESSAGES; ++i)
  {
    printf("Sending Message %d\n", i + 1);
    if ((rc = MQTTClient_publish(
             mqtt_client,
             telemetry_topic,
             (int)strlen(telemetry_message_payloads[i]),
             telemetry_message_payloads[i],
             0,
             0,
             NULL))
        != MQTTCLIENT_SUCCESS)
    {
      printf("Failed to publish telemetry message %d, MQTTClient return code %d\n", i + 1, rc);
      return rc;
    }
    sleep_for_seconds(TELEMETRY_SEND_INTERVAL);
  }

  return MQTTCLIENT_SUCCESS;
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

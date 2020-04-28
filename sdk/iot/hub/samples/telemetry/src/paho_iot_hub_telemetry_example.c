// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
#endif
#include <MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(default : 4201)
#endif

#ifdef _WIN32
// Required for Sleep(DWORD)
#include <Windows.h>
#else
// Required for sleep(unsigned int)
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <az_iot_hub_client.h>
#include <az_result.h>
#include <az_span.h>

// TODO: #564 - Remove the use of the _az_cfh.h header in samples.
//              Note: this is required to work-around MQTTClient.h as well as az_span init issues.
#include <_az_cfg.h>

// Service information
#define HUB_URL "ssl://dawalton-hub.azure-devices.net:8883"
#define HUB_FQDN "dawalton-hub.azure-devices.net"
#define DEVICE_ID "paho_example"

// Device information
#define REGISTRATION_ID_ENV "AZ_IOT_REGISTRATION_ID"

// AZ_IOT_DEVICE_X509_CERT_PEM_FILE is the path to a PEM file containing the device certificate and
// key as well as any intermediate certificates chaining to an uploaded group certificate.
#define DEVICE_X509_CERT_PEM_FILE "AZ_IOT_DEVICE_X509_CERT_PEM_FILE"

// AZ_IOT_DEVICE_X509_TRUST_PEM_FILE is the path to a PEM file containing the server trusted CA
// This is usually not needed on Linux or Mac but needs to be set on Windows.
#define DEVICE_X509_TRUST_PEM_FILE "AZ_IOT_DEVICE_X509_TRUST_PEM_FILE"

#define NUMBER_OF_MESSAGES 5

static char x509_cert_pem_file[512] = { 0 };
static char x509_trust_pem_file[256] = { 0 };
char telemetry_topic[128];
static const char* telemetry_message_payloads[NUMBER_OF_MESSAGES] = {
  "Message One", "Message Two", "Message Three", "Message Four", "Message Five",
};

static az_iot_hub_client client;
static MQTTClient mqtt_client;

static void sleep_seconds(uint32_t seconds)
{
#ifdef _WIN32
  Sleep((DWORD)seconds * 1000);
#else
  sleep(seconds);
#endif
}

static az_result read_configuration_entry(
    const char* name,
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span buffer,
    az_span* out_value)
{
  printf("%s = ", name);
  char* env = getenv(env_name);

  if (env != NULL)
  {
    printf("%s\n", hide_value ? "***" : env);
    az_span env_span = az_span_from_str(env);
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(buffer, az_span_size(env_span));
    az_span_copy(buffer, env_span);
    *out_value = az_span_slice(buffer, 0, az_span_size(env_span));
  }
  else if (default_value != NULL)
  {
    printf("%s\n", default_value);
    az_span default_span = az_span_from_str(default_value);
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(buffer, az_span_size(default_span));
    az_span_copy(buffer, default_span);
    *out_value = az_span_slice(buffer, 0, az_span_size(default_span));
  }
  else
  {
    printf("(missing) Please set the %s environment variable.\n", env_name);
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

static az_result read_configuration_and_init_client()
{

  az_span cert = AZ_SPAN_FROM_BUFFER(x509_cert_pem_file);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      "X509 Certificate PEM Store File", DEVICE_X509_CERT_PEM_FILE, NULL, false, cert, &cert));

  az_span trusted = AZ_SPAN_FROM_BUFFER(x509_trust_pem_file);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      "X509 Trusted PEM Store File", DEVICE_X509_TRUST_PEM_FILE, "", false, trusted, &trusted));

  AZ_RETURN_IF_FAILED(az_iot_hub_client_init(
      &client, AZ_SPAN_FROM_STR(HUB_FQDN), AZ_SPAN_FROM_STR(DEVICE_ID), NULL));

  return AZ_OK;
}

static int connect()
{
  int rc;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;

  char username[128] = { 0 };
  size_t username_length;
  if ((rc = az_iot_hub_client_get_user_name(&client, username, sizeof(username), &username_length))
      != AZ_OK)

  {
    printf("Failed to get MQTT clientId, return code %d\n", rc);
    return rc;
  }

  mqtt_connect_options.username = username;
  mqtt_connect_options.password = NULL;

  mqtt_ssl_options.keyStore = (char*)x509_cert_pem_file;
  if (*x509_trust_pem_file != '\0')
  {
    mqtt_ssl_options.trustStore = (char*)x509_trust_pem_file;
  }

  mqtt_connect_options.ssl = &mqtt_ssl_options;

  if ((rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to connect, return code %d\n", rc);
    return rc;
  }

  return 0;
}

static int send_telemetry_messages()
{
  int rc;
  for (int i = 0; i < NUMBER_OF_MESSAGES; ++i)
  {
    printf("Sending Message %i\r\n", i + 1);
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
      printf("Failed to publish get_operation_status, return code %d\n", rc);
      return rc;
    }
    sleep_seconds(1);
  }
  return rc;
}

int main()
{
  int rc;

  if ((rc = read_configuration_and_init_client()) != AZ_OK)
  {
    printf("Failed to read configuration from environment variables, return code %d\n", rc);
    return rc;
  }

  char client_id[128] = { 0 };
  size_t client_id_length;
  if ((rc
       = az_iot_hub_client_get_client_id(&client, client_id, sizeof(client_id), &client_id_length))
      != AZ_OK)
  {
    printf("Failed to get MQTT clientId, return code %d\n", rc);
    return rc;
  }

  if ((rc = MQTTClient_create(&mqtt_client, HUB_URL, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to create MQTT client, return code %d\n", rc);
    return rc;
  }

  if ((rc = connect()) != 0)
  {
    return rc;
  }

  if ((rc = az_iot_hub_client_telemetry_get_publish_topic(
           &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL))
      != AZ_OK)
  {
    return rc;
  }

  // Loop and send 5 messages
  if ((rc = send_telemetry_messages()) != 0)
  {
    return rc;
  }

  printf("Messages Sent [Press ENTER to shut down]\n");
  (void)getchar();

  if ((rc = MQTTClient_disconnect(mqtt_client, 10000)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to disconnect MQTT client, return code %d\n", rc);
    return rc;
  }

  printf("Disconnected.\n");
  MQTTClient_destroy(&mqtt_client);

  return 0;
}

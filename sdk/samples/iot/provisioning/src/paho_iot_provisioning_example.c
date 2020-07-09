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

#define LOG_ERROR(ErrorMsg, ...) {\
  (void)printf("%s:%s:%d: ", __FILE__, __func__, __LINE__); \
  (void)printf(ErrorMsg, __VA_ARGS__); \
  fflush(stdout);\
  fflush(stderr);\
}

static char mqtt_client_id[128];
static char mqtt_username[128];
static char register_publish_topic[128];
static char query_topic[256];
static char global_provisioning_endpoint[256] = { 0 };
static char id_scope[16] = { 0 };
static char registration_id[256] = { 0 };
static char x509_cert_pem_file[256] = { 0 };
static char x509_trust_pem_file[256] = { 0 };

static az_iot_provisioning_client provisioning_client;
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
static int connect_device();
static int subscribe();

//
// Device registration functions
//
static int register_device();
static int get_operation_status();
static az_result parse_operation_message(
    char* topic,
    int topic_len,
    MQTTClient_message const* message,
    az_iot_provisioning_client_register_response* response,
    az_iot_provisioning_client_operation_status* operation_status);
static int send_operation_query_message(
    az_iot_provisioning_client_register_response const* response);

static void sleep_for_seconds(uint32_t seconds);
static void print_az_span(const char* str, az_span span);
static void on_error(int rc);

int main()
{
  int rc;

  // Read in the necessary environment variables and initialize the az_iot_provisioning_client
  if (az_failed(rc = read_configuration_and_init_client()))
  {
    printf(
        "Failed to read configuration from environment variables, az_result return code %04x\n",
        rc);
    return rc;
  }

  // Get the MQTT client id used for the MQTT connection
  if (az_failed(
          rc = az_iot_provisioning_client_get_client_id(
              &provisioning_client, mqtt_client_id, sizeof(mqtt_client_id), NULL)))
  {
    printf("Failed to get MQTT clientId, az_result return code %04x\n", rc);
    return rc;
  }

  // Create the Paho MQTT client
  if ((rc = MQTTClient_create(
           &mqtt_client,
           global_provisioning_endpoint,
           mqtt_client_id,
           MQTTCLIENT_PERSISTENCE_NONE,
           NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to create MQTT client, MQTTClient return code %d\n", rc);
    return rc;
  }

  // Connect to the Provisioning Service
  if ((rc = connect_device()) != MQTTCLIENT_SUCCESS)
  {
    return rc;
  }
  printf("Connected to %s\n", global_provisioning_endpoint);

  // Subscribe to the necessary provisioning topic to receive provisioning responses
  if ((rc = subscribe()) != MQTTCLIENT_SUCCESS)
  {
    return rc;
  }
  printf("Subscribed\n");

  // Begin the registration process by sending a registration request message
  if ((rc = register_device()) != MQTTCLIENT_SUCCESS)
  {
    return rc;
  }

  // Receive the status of the registration request
  if ((rc = get_operation_status()) != MQTTCLIENT_SUCCESS)
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

// Read the user environment variables used to connect to the Provisioning Service and initialize
// the provisioning client
static az_result read_configuration_and_init_client()
{
  az_span endpoint_span = AZ_SPAN_FROM_BUFFER(global_provisioning_endpoint);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_GLOBAL_PROVISIONING_ENDPOINT,
      ENV_GLOBAL_PROVISIONING_ENDPOINT_DEFAULT,
      false,
      endpoint_span,
      &endpoint_span));

  az_span id_scope_span = AZ_SPAN_FROM_BUFFER(id_scope);
  AZ_RETURN_IF_FAILED(
      read_configuration_entry(ENV_ID_SCOPE_ENV, NULL, false, id_scope_span, &id_scope_span));

  az_span registration_id_span = AZ_SPAN_FROM_BUFFER(registration_id);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_REGISTRATION_ID_ENV, NULL, false, registration_id_span, &registration_id_span));

  az_span cert = AZ_SPAN_FROM_BUFFER(x509_cert_pem_file);
  AZ_RETURN_IF_FAILED(
      read_configuration_entry(ENV_DEVICE_X509_CERT_PEM_FILE, NULL, false, cert, &cert));

  az_span trusted = AZ_SPAN_FROM_BUFFER(x509_trust_pem_file);
  AZ_RETURN_IF_FAILED(
      read_configuration_entry(ENV_DEVICE_X509_TRUST_PEM_FILE, "", false, trusted, &trusted));

  // Initialize the provisioning client with the provisioning endpoint and the default connection
  // options
  AZ_RETURN_IF_FAILED(az_iot_provisioning_client_init(
      &provisioning_client, endpoint_span, id_scope_span, registration_id_span, NULL));

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

static int connect_device()
{
  int rc;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;

  // NOTE: We recommend setting clean session to false in order to receive any pending messages
  mqtt_connect_options.cleansession = false;
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  // Get the MQTT username used to connect to the Provisioning Service
  if (az_failed(
          rc = az_iot_provisioning_client_get_user_name(
              &provisioning_client, mqtt_username, sizeof(mqtt_username), NULL)))
  {
    printf("Failed to get MQTT username, az_result return code %0x4\n", rc);
    return rc;
  }

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

  // Connect to the Provisioning Service
  if ((rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to connect, MQTTClient return code %d\n", rc);
    return rc;
  }

  return MQTTCLIENT_SUCCESS;
}

// Subscribe to the necessary topic to receive responses from the Provisioning Service
static int subscribe()
{
  int rc;

  if ((rc
       = MQTTClient_subscribe(mqtt_client, AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to subscribe to the register topic filter, MQTTClient return code %d\n", rc);
    return rc;
  }

  return MQTTCLIENT_SUCCESS;
}

// Send the register pub message to the Provisioning Service
static int register_device()
{
  int rc;

  // Get the topic to send the provisioning request
  if (az_failed(
          rc = az_iot_provisioning_client_register_get_publish_topic(
              &provisioning_client, register_publish_topic, sizeof(register_publish_topic), NULL)))
  {
    printf("Failed to get MQTT register publish topic, az_result return code %04x\n", rc);
    return rc;
  }

  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  pubmsg.payload = NULL; // empty payload.
  pubmsg.payloadlen = 0;
  pubmsg.qos = 1;
  pubmsg.retained = 0;

  // Send the registration request message
  if ((rc = MQTTClient_publishMessage(mqtt_client, register_publish_topic, &pubmsg, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to publish register request, MQTTClient return code %d\n", rc);
    return rc;
  }

  return MQTTCLIENT_SUCCESS;
}

static int get_operation_status()
{
  int rc;
  bool is_operation_complete = false;
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;
  az_iot_provisioning_client_register_response response;
  az_iot_provisioning_client_operation_status operation_status;

  // Continue to parse incoming responses from the Provisioning Service until
  // the device has been successfully provisioned or an error occurs
  do
  {
    // Receive message.
    // If MQTTClient_receive is successful, topic and message memory must later be freed
    if ((rc
         = MQTTClient_receive(mqtt_client, &topic, &topic_len, &message, TIMEOUT_MQTT_RECEIVE_MS))
        != MQTTCLIENT_SUCCESS)
    {
      printf("Failed to receive message, MQTTClient return code %d\n", rc);
      return rc;
    } 
    else if (MQTTCLIENT_TOPICNAME_TRUNCATED == rc) 
    {
      topic_len = (int)strlen(topic);
    }
    printf("Received a message from service.\n");

    // Parse operation message; extract the response and operation status
    if (az_failed(
            rc = parse_operation_message(topic, topic_len, message, &response, &operation_status)))
    {
      printf("Failed to parse operation message, az_result return code %04x\n", rc);
      MQTTClient_freeMessage(&message);
      MQTTClient_free(topic);
      return rc;
    }
    printf("Parsed operation message.\n");

    // If operation is not complete, send query and loop to receive operation message
    is_operation_complete = az_iot_provisioning_client_operation_complete(operation_status);
    if (!is_operation_complete)
    {
      MQTTClient_freeMessage(&message);
      MQTTClient_free(topic);

      if ((rc = send_operation_query_message(&response)) != MQTTCLIENT_SUCCESS)
      {
        return rc;
      }
    }
  } while (!is_operation_complete);

  // Operation is complete
  // Successful assignment - print out the assigned hostname and device id
  if (operation_status == AZ_IOT_PROVISIONING_STATUS_ASSIGNED)
  {
    printf("SUCCESS - Device provisioned:\n");
    print_az_span("\tHub Hostname: ", response.registration_result.assigned_hub_hostname);
    print_az_span("\tDevice Id: ", response.registration_result.device_id);
  }
  else // Unsuccesful assignment (unassigned, failed or disabled states)
  {
    printf("ERROR - Device Provisioning failed:\n");
    print_az_span("\tRegistration state: ", response.operation_status);
    printf("\tLast operation status: %d\n", response.status);
    print_az_span("\tOperation ID: ", response.operation_id);
    printf("\tError code: %u\n", response.registration_result.extended_error_code);
    print_az_span("\tError message: ", response.registration_result.error_message);
    print_az_span("\tError timestamp: ", response.registration_result.error_timestamp);
    print_az_span("\tError tracking ID: ", response.registration_result.error_tracking_id);

    if (response.retry_after_seconds > 0)
    {
      printf("\tRetry-after: %u seconds.", response.retry_after_seconds);
    }
  }

  MQTTClient_freeMessage(&message);
  MQTTClient_free(topic);
  return rc;
}

static az_result parse_operation_message(
    char* topic,
    int topic_len,
    MQTTClient_message const* message,
    az_iot_provisioning_client_register_response* response,
    az_iot_provisioning_client_operation_status* operation_status)
{
  int rc;

  

  az_span topic_span = az_span_init((uint8_t*)topic, topic_len);
  az_span message_span = az_span_init((uint8_t*)message->payload, message->payloadlen);
  
  print_az_span("Topic:\n", topic_span);

  // Parse the incoming message and payload
  if (az_failed(
          rc = az_iot_provisioning_client_parse_received_topic_and_payload(
              &provisioning_client, topic_span, message_span, response)))
  {
    printf("Message from unknown topic, az_result return code %0x4\n", rc);
    return rc;
  }
  print_az_span("Received payload:\n", message_span);
  printf("Response status is %d.\n", response->status);

  // Parse the operation status from a string to an enum
  if (az_failed(rc = az_iot_provisioning_client_parse_operation_status(response, operation_status)))
  {
    printf("Failed to parse operation_status, az_result return code %04x\n", rc);
    return rc;
  }

  return AZ_OK;
}

static void send_operation_query_message(
    az_iot_provisioning_client_register_response const* response)
{
  int rc;

  // Get the topic to send the query message
  if (az_failed(
          rc = az_iot_provisioning_client_query_status_get_publish_topic(
              &provisioning_client, response, query_topic, sizeof(query_topic), NULL)))
  {
    LOG_ERROR("Unable to get query status publish topic: az_result return code %04x\n", rc);
    exit(rc);
  }

  // IMPORTANT: Wait the recommended retry-after number of seconds before query
  printf("Querying after %u seconds...\n", response->retry_after_seconds);
  sleep_for_seconds(response->retry_after_seconds);

  // Publish the query message
  if ((rc = MQTTClient_publish(mqtt_client, query_topic, 0, NULL, 0, 0, NULL))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to publish query status request: MQTTClient return code %d .\n", rc);
    exit(rc);
  }

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

// Print an az_span to the console
static void print_az_span(const char* str, az_span span)
{
  printf("%s", str);

  char* buffer = (char*)az_span_ptr(span);
  for (int32_t i = 0; i < az_span_size(span); i++)
  {
    putchar(*buffer++);
  }

  putchar('\n');
  return;
}

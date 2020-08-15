// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "iot_sample_foundation.h"

#define SAMPLE_TYPE PAHO_IOT_HUB
#define SAMPLE_NAME PAHO_IOT_HUB_PNP_SAMPLE

#define TELEMETRY_SEND_INTERVAL 1
#define TIMEOUT_MQTT_RECEIVE_MS (8 * 1000)
#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)
#define TIMEOUT_MQTT_WAIT_FOR_COMPLETION_MS 1000

/*
#define DEVICE_DO_WORK_SLEEP_MS 2
#define DEFAULT_START_TEMP_CELSIUS 22.0
#define DOUBLE_DECIMAL_PLACE_DIGITS 2
*/

// * PnP Values *
// The model id is the JSON document (also called the Digital Twins Model Identifier or DTMI)
// which defines the capability of your device. The functionality of the device should match what
// is described in the coresponding DTMI. Should you choose to program your own PnP capable device,
// the functionality would need to match the DTMI and you would need to update the below 'model_id'.
// Please see the sample README for more information on this DTMI.
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:com:example:Thermostat;1");

bool device_operational = true;

static sample_environment_variables env_vars;
static az_iot_hub_client hub_client;
static MQTTClient mqtt_client;
static char mqtt_client_username_buffer[128];
static int32_t topic_request_id_int;
static char topic_request_id_buffer[8];

// IoT Hub Telemetry Values
char telemetry_topic[128];
static const az_span telemetry_name = AZ_SPAN_LITERAL_FROM_STR("temperature");
static char telemetry_payload[32];

// IoT Hub Commands Values
static char commands_response_topic[128];
static const az_span report_command_name_span = AZ_SPAN_LITERAL_FROM_STR("getMaxMinReport");
static const az_span report_max_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("maxTemp");
static const az_span report_min_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("minTemp");
static const az_span report_avg_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("avgTemp");
static const az_span report_start_time_name_span = AZ_SPAN_LITERAL_FROM_STR("startTime");
static const az_span report_end_time_name_span = AZ_SPAN_LITERAL_FROM_STR("endTime");
static const az_span report_error_payload = AZ_SPAN_LITERAL_FROM_STR("{}");
static char end_time_buffer[32];
static char commands_response_payload[256];
static char incoming_since_value[32];

// IoT Hub Twin Values

static char reported_property_topic[128];
static const az_span desired_property_name = AZ_SPAN_LITERAL_FROM_STR("desired");
static const az_span desired_property_version_name = AZ_SPAN_LITERAL_FROM_STR("$version");
static const az_span desired_temp_property_name = AZ_SPAN_LITERAL_FROM_STR("targetTemperature");
static const az_span desired_temp_response_value_name = AZ_SPAN_LITERAL_FROM_STR("value");
static const az_span desired_temp_ack_code_name = AZ_SPAN_LITERAL_FROM_STR("ac");
static const az_span desired_temp_ack_version_name = AZ_SPAN_LITERAL_FROM_STR("av");
static const az_span desired_temp_ack_description_name = AZ_SPAN_LITERAL_FROM_STR("ad");
static const az_span max_temp_reported_property_name
    = AZ_SPAN_LITERAL_FROM_STR("maxTempSinceLastReboot");
static char reported_property_payload[128];

// PnP Device Values
static double current_device_temp = DEFAULT_START_TEMP_CELSIUS;
static double device_temperature_avg_total = DEFAULT_START_TEMP_CELSIUS;
static uint32_t device_temperature_avg_count = 1;
static double device_max_temp = DEFAULT_START_TEMP_CELSIUS;
static double device_min_temp = DEFAULT_START_TEMP_CELSIUS;
static double device_avg_temp = DEFAULT_START_TEMP_CELSIUS;

// Functions
void create_and_configure_mqtt_client();
void connect_mqtt_client_to_iot_hub();
void subscribe_mqtt_client_to_iot_hub_topics();
void request_device_twin_document();
void receive_messages();
void disconnect_mqtt_client_from_iot_hub();

void mqtt_publish_message(const char* topic, const az_span* payload, int qos);
void on_message_received(char* topic, int topic_len, const MQTTClient_message* message);
void handle_device_twin_message(
    const az_span* message_span,
    const az_iot_hub_client_twin_response* twin_response);
az_result parse_device_twin_desired_temperature_property(
    az_span twin_payload_span,
    bool is_twin_get,
    double* parsed_temp,
    int32_t* version_number);

az_span get_topic_request_id();

static int send_telemetry_message(void);
static int send_command_response(
    az_iot_hub_client_method_request* request,
    uint16_t status,
    az_span response);
static int send_reported_temperature_property(
    double temp_value,
    int32_t version,
    bool is_max_reported_prop);
static void handle_command_message(
    MQTTClient_message* message,
    az_iot_hub_client_method_request* command_request);
static az_result parse_device_twin_desired_temperature_property(
    az_span twin_payload_span,
    bool is_twin_get,
    double* parsed_value,
    int32_t* version_number);
static az_result invoke_getMaxMinReport(az_span payload, az_span response, az_span* out_response);

/*
 * This sample connects an IoT Plug and Play enabled device with the Digital Twin Model ID (DTMI).
 * X509 self-certification is used.  In short, the capabilities are listed here:
 *
 *
 *
 *
 */
int main(void)
{
  set_program_start_time(); // Set the program start time for command response

  create_and_configure_mqtt_client();
  LOG_SUCCESS("Client created and configured.");

  connect_mqtt_client_to_iot_hub();
  LOG_SUCCESS("Client connected to IoT Hub.");

  subscribe_mqtt_client_to_iot_hub_topics();
  LOG_SUCCESS("Client subscribed to IoT Hub topics.");

  request_device_twin_document();
  LOG_SUCCESS("Client reqeusted twin document.")

  receive_messages();
  LOG_SUCCESS("Client received messages.")

  disconnect_mqtt_client_from_iot_hub();
  LOG_SUCCESS("Client disconnected from IoT Hub.");

  return 0;
}

void create_and_configure_mqtt_client()
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
}

void connect_mqtt_client_to_iot_hub()
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
  mqtt_ssl_options.keyStore = (char*)x509_cert_pem_file_path_buffer;
  if (*az_span_ptr(env_vars.x509_trust_pem_file_path)
      != '\0') // Should only be set if required by OS.
  {
    mqtt_ssl_options.trustStore = (char*)x509_trust_pem_file_path_buffer;
  }
  mqtt_connect_options.ssl = &mqtt_ssl_options;

  // Connect MQTT client to the Azure IoT Hub.
  if ((rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR(
        "Failed to connect: MQTTClient return code %d.\n"
        "If on Windows, confirm the AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH environment variable is "
        "set "
        "correctly.",
        rc);
    exit(rc);
  }
}

void subscribe_mqtt_client_to_iot_hub_topics()
{
  int rc;

  // Messages received on the Twin Patch topic will be updates to the desired properties.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to subscribe to the Twin Patch topic: MQTTClient return code %d.", rc);
    exit(rc);
  }

  // Messages received on Response topic will be response statuses from the server.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to subscribe to the Twin Response topic: MQTTClient return code %d.", rc);
    exit(rc);
  }

  // Messages received on the Methods topic will be method commands to be invoked.
  if ((rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC, 1))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to subscribe to the Methods topic: MQTTClient return code %d.", rc);
    exit(rc);
  }
}

void request_device_twin_document()
{
  int rc;

  LOG("Client requesting device twin document from service.");

  // Get the Twin Document topic to publish the twin document request.
  char twin_document_topic[128];
  az_span topic_request_id = get_topic_request_id();
  if (az_failed(
          rc = az_iot_hub_client_twin_document_get_publish_topic(
              &hub_client,
              topic_request_id,
              twin_document_topic,
              sizeof(twin_document_topic),
              NULL)))
  {
    LOG_ERROR("Failed to get Twin Document publish topic: az_result return code %04x", rc);
    exit(rc);
  }

  // Publish the twin document request.
  az_span payload = AZ_SPAN_NULL;
  mqtt_publish_message(twin_document_topic, &payload, 0);

  return rc;
}

void receive_messges()
{
  int rc;
  char* topic = NULL;
  int topic_len = 0;
  MQTTClient_message* message = NULL;

  // Continue to receive twin messages or method commands while device is operational
  while (device_operational)
  {
    LOG("Waiting for message.");

    if (((rc
          = MQTTClient_receive(mqtt_client, &topic, &topic_len, &message, TIMEOUT_MQTT_RECEIVE_MS))
         != MQTTCLIENT_SUCCESS)
        && (rc != MQTTCLIENT_TOPICNAME_TRUNCATED))
    {
      LOG_ERROR("Failed to receive message: MQTTClient return code %d.", rc);
      exit(rc);
    }
    else if (message == NULL)
    {
      LOG_ERROR("Receive message timeout expired: MQTTClient return code %d.", rc);
      exit(rc);
    }
    else if (rc == MQTTCLIENT_TOPICNAME_TRUNCATED)
    {
      topic_len = (int)strlen(topic);
    }
    LOG_SUCCESS("Client received message from the service.", message_count + 1);

    on_message_received(topic, topic_len, message);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(incoming_message_topic);

    // Send a telemetry message
    send_telemetry_message();
    LOG_SUCCESS("Client sent telemetry message to the service.");
  }
}

void disconnect_mqtt_client_from_iot_hub()
{
  int rc;

  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to disconnect MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }

  MQTTClient_destroy(&mqtt_client);
}

void mqtt_publish_message(const char* topic, const az_span* payload, int qos)
{
  PRECONDITION_NOT_NULL(topic);
  PRECONDITION_NOT_NULL(payload);

  int rc;
  MQTTClient_deliveryToken token;

  if ((rc = MQTTClient_publish(
           mqtt_client, topic, az_span_size(*payload), az_span_ptr(*payload), qos, 0, &token))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to publish twin document request: MQTTClient return code %d", rc);
    exit(rc);
  }

  if (qos > 0)
  {
    if ((rc = MQTTClient_waitForCompletion(mqtt_client, token, TIMEOUT_MQTT_WAIT_FOR_COMPLETION_MS))
        != MQTTCLIENT_SUCCESS)
    {
      LOG_ERROR("Wait for message completion time out expired: MQTTClient return code %d", rc);
      exit(rc);
    }
  }
}

void on_message_received(char* topic, int topic_len, const MQTTClient_message* message)
{
  PRECONDITION_NOT_NULL(topic);
  PRECONDITION_NOT_NULL(message);

  int rc;

  az_span topic_span = az_span_create((uint8_t*)topic, topic_len);
  az_span message_span = az_span_create((uint8_t*)message->payload, message->payloadlen);

  az_iot_hub_client_twin_response twin_response;
  az_iot_hub_client_method_request method_request;

  // Parse the incoming message topic and check which feature it is for
  if (az_succeeded(
          rc
          = az_iot_hub_client_twin_parse_received_topic(&hub_client, topic_span, &twin_response)))
  {
    LOG_SUCCESS("Client received a valid topic response:");
    LOG_AZ_SPAN("Topic:", topic_span);
    LOG_AZ_SPAN("Payload:", message_span);
    LOG("Status: %d", twin_response->status);

    handle_device_twin_message(message_span, &twin_response);
  }
  else if (az_succeeded(az_iot_hub_client_methods_parse_received_topic(
               &hub_client, topic_span, &method_request)))
  {
    LOG_SUCCESS("Client received a valid topic response:");
    LOG_AZ_SPAN("Topic:", topic_span);
    LOG_AZ_SPAN("Payload:", message_span);

    handle_method_message(message_span, &method_request);
  }
  else
  {
    LOG_ERROR("Message from unknown topic: az_result return code 0x%04x.", rc);
    LOG_AZ_SPAN("Topic:", topic_span);
    exit(rc);
  }

  LOG(" "); // Formatting
}

void handle_device_twin_message(
    const az_span* message_span,
    const az_iot_hub_client_twin_response* twin_response)
{
  PRECONDITION_NOT_NULL(message_span);
  PRECONDITION_NOT_NULL(twin_response);

  bool is_twin_get = false;
  bool max_temp_changed = false;
  double desired_temp = 0.0;
  int32_t version_num = 0;

  // Invoke appropriate action per response type (3 Types only).
  switch (twin_response->response_type)
  {
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET:
      LOG("Type: GET");
      is_twin_get = true;
      break;

    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
      LOG("Type: Desired Properties");
      break;

    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
      LOG("Type: Reported Properties");
      break;
  }

  // For a GET response OR a Desired Properties response from the server:
  // 1. Parse for the desired temperature
  // 2. Update device temperature locally
  // 3. Report updated temperature to server
  if (twin_response->response_type == AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET
      || twin_response->response_type == AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES)
  {
    if (az_succeeded(parse_device_twin_desired_temperature_property(
            message_span, is_twin_get, &desired_temp, &version_num)))
    {
      // send_reported_temperature_property(desired_temp, version_num, false);  why send this?
      update_device_temp(desired_temp, &max_temp_changed);

      if (max_temp_changed)
      {
        send_reported_temperature_property(device_max_temp, -1, true);
      }
    }
    // else Desired property not found in payload. Do nothing.
  }
}

az_result parse_device_twin_desired_temperature_property(
    const az_span* twin_payload_span,
    bool is_twin_get,
    double* parsed_temp,
    int32_t* version_number)
{
  PRECONDITION_NOT_NULL(twin_payload_span);

  az_json_reader jr;

  AZ_RETURN_IF_FAILED(az_json_reader_init(&jr, *twin_payload_span, NULL));
  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
  if (jr.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  // Device twin GET response: Parse to the "desired" wrapper if it exists.
  bool desired_found = false;
  if (is_twin_get)
  {
    AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
    while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
    {
      if (az_json_token_is_text_equal(&jr.token, desired_property_name))
      {
        desired_found = true;
        AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
        break;
      }
      else
      {
        AZ_RETURN_IF_FAILED(az_json_reader_skip_children(&jr)); // Ignore children tokens.
      }

      AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr)); // Check next sibling token.
    }

    if (!desired_found)
    {
      LOG_ERROR("Desired property object not found in device twin GET response.");
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
  }

  // Device twin get response OR desired property response:
  // Parse for the desired temperature property
  bool temp_found = false;
  bool version_found = false;

  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
  while (!(temp_found && version_found) && (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT))
  {
    if (az_json_token_is_text_equal(&jr.token, desired_temp_property_name))
    {
      AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      AZ_RETURN_IF_FAILED(az_json_token_get_double(&jr.token, parsed_temp));
      temp_found = true;
    }
    else if (az_json_token_is_text_equal(&jr.token, desired_property_version_name))
    {
      AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      AZ_RETURN_IF_FAILED(az_json_token_get_uint32(&jr.token, (uint32_t*)version_number));
      version_found = true;
    }
    else
    {
      AZ_RETURN_IF_FAILED(az_json_reader_skip_children(&jr)); // Ignore children tokens.
    }
    AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr)); // Check next sibling token.
  }

  if (!(temp_found && version_found))
  {
    LOG_ERROR("Temperature or version properties were not found in desired property response.");
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  LOG("Parsed desired temperature: %2f", *parsed_temp);
  LOG("Parsed version number: %d", *version_number);
  return AZ_OK;
}

static az_result build_command_response_payload(
    az_json_writer* json_builder,
    az_span start_time_span,
    az_span end_time_span,
    az_span* response_payload)
{
  // Build the command response payload
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_builder));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_max_temp_name_span));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_double(json_builder, device_max_temp, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_min_temp_name_span));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_double(json_builder, device_min_temp, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_avg_temp_name_span));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_double(json_builder, device_avg_temp, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_builder, report_start_time_name_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(json_builder, start_time_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_end_time_name_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(json_builder, end_time_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(json_builder));

  *response_payload = az_json_writer_get_bytes_used_in_destination(json_builder);

  return AZ_OK;
}

// Invoke the command requested from the service. Here, it generates a report for max, min, and avg
// temperatures.
static az_result invoke_getMaxMinReport(az_span payload, az_span response, az_span* out_response)
{
  // Parse the "since" field in the payload.
  az_span start_time_span = AZ_SPAN_NULL;
  az_json_reader jp;
  AZ_RETURN_IF_FAILED(az_json_reader_init(&jp, payload, NULL));
  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jp));
  int32_t incoming_since_value_len;
  AZ_RETURN_IF_FAILED(az_json_token_get_string(
      &jp.token, incoming_since_value, sizeof(incoming_since_value), &incoming_since_value_len));
  start_time_span = az_span_create((uint8_t*)incoming_since_value, incoming_since_value_len);

  // Set the response payload to error if the "since" field was not sent
  if (az_span_ptr(start_time_span) == NULL)
  {
    response = report_error_payload;
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  // Get the current time as a string
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  size_t len = strftime(end_time_buffer, sizeof(end_time_buffer), iso_spec_time_format, timeinfo);
  az_span end_time_span = az_span_create((uint8_t*)end_time_buffer, (int32_t)len);

  az_json_writer json_builder;
  AZ_RETURN_IF_FAILED(az_json_writer_init(&json_builder, response, NULL));
  AZ_RETURN_IF_FAILED(
      build_command_response_payload(&json_builder, start_time_span, end_time_span, out_response));

  return AZ_OK;
}

// Send the response of the command invocation
static int send_command_response(
    az_iot_hub_client_method_request* request,
    uint16_t status,
    az_span response)
{
  int rc;
  // Get the response topic to publish the command response
  if (az_failed(
          rc = az_iot_hub_client_methods_response_get_publish_topic(
              &client,
              request->request_id,
              status,
              commands_response_topic,
              sizeof(commands_response_topic),
              NULL)))
  {
    printf("Unable to get method response publish topic\n");
    return rc;
  }

  printf("Status: %u\tPayload:", status);
  char* payload_char = (char*)az_span_ptr(response);
  if (payload_char != NULL)
  {
    for (int32_t i = 0; i < az_span_size(response); i++)
    {
      putchar(*(payload_char + i));
    }
  }
  putchar('\n');

  // Send the commands response
  if ((rc = mqtt_publish_message(commands_response_topic, response, 0)) == 0)
  {
    printf("Sent response\n");
  }

  return rc;
}

// Build the JSON payload for the reported property
static az_result build_confirmed_reported_property(
    az_json_writer* json_builder,
    az_span property_name,
    double property_val,
    int32_t ack_code_value,
    int32_t ack_version_value,
    az_span ack_description_value)
{
  AZ_RETURN_IF_FAILED(
      az_json_writer_init(json_builder, AZ_SPAN_FROM_BUFFER(reported_property_payload), NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_builder));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, property_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_builder));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_builder, desired_temp_response_value_name));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_double(json_builder, property_val, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_builder, desired_temp_ack_code_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(json_builder, ack_code_value));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_builder, desired_temp_ack_version_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(json_builder, ack_version_value));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_builder, desired_temp_ack_description_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(json_builder, ack_description_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(json_builder));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(json_builder));

  return AZ_OK;
}

static az_result build_reported_property(
    az_json_writer* json_builder,
    az_span property_name,
    double property_val)
{
  AZ_RETURN_IF_FAILED(
      az_json_writer_init(json_builder, AZ_SPAN_FROM_BUFFER(reported_property_payload), NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_builder));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, property_name));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_double(json_builder, property_val, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(json_builder));

  return AZ_OK;
}
// Send the twin reported property to the service
static int send_reported_temperature_property(
    double temp_value,
    int32_t version,
    bool is_max_reported_prop)
{
  int rc;
  printf("Sending reported property\n");

  // Get the topic used to send a reported property update
  az_span request_id_span = get_request_id();
  if (az_failed(
          rc = az_iot_hub_client_twin_patch_get_publish_topic(
              &client,
              request_id_span,
              reported_property_topic,
              sizeof(reported_property_topic),
              NULL)))
  {
    printf("Unable to get twin document publish topic, return code %d\n", rc);
    return rc;
  }

  // Twin reported properties must be in JSON format. The payload is constructed here.
  az_json_writer json_builder;
  if (is_max_reported_prop)
  {
    if (az_failed(
            rc
            = build_reported_property(&json_builder, max_temp_reported_property_name, temp_value)))
    {
      return rc;
    }
  }
  else
  {
    if (az_failed(
            rc = build_confirmed_reported_property(
                &json_builder,
                desired_temp_property_name,
                temp_value,
                200,
                version,
                AZ_SPAN_FROM_STR("success"))))
    {
      return rc;
    }
  }
  az_span json_payload = az_json_writer_get_bytes_used_in_destination(&json_builder);

  printf("Payload: %.*s\n", az_span_size(json_payload), (char*)az_span_ptr(json_payload));

  // Publish the reported property payload to IoT Hub
  rc = mqtt_publish_message(reported_property_topic, json_payload, 0);

  return rc;
}

static void update_device_temp(double temp, bool* max_temp_changed)
{
  current_device_temp = temp;

  bool ret = false;
  if (current_device_temp > device_max_temp)
  {
    device_max_temp = current_device_temp;
    ret = true;
  }
  if (current_device_temp < device_min_temp)
  {
    device_min_temp = current_device_temp;
  }

  // Increment the avg count, add the new temp to the total, and calculate the new avg
  device_temperature_avg_count++;
  device_temperature_avg_total += current_device_temp;
  device_avg_temp = device_temperature_avg_total / device_temperature_avg_count;

  *max_temp_changed = ret;
}

// Invoke the requested command if supported and return status | Return error otherwise
static void handle_command_message(
    MQTTClient_message* message,
    az_iot_hub_client_method_request* command_request)
{

  if (az_span_is_content_equal(report_command_name_span, command_request->name))
  {
    az_span command_response_span = AZ_SPAN_FROM_BUFFER(commands_response_payload);
    az_span command_response_payload_span
        = az_span_create((uint8_t*)message->payload, (int32_t)message->payloadlen);

    // Invoke command
    uint16_t return_code;
    az_result response = invoke_getMaxMinReport(
        command_response_payload_span, command_response_span, &command_response_span);
    if (response != AZ_OK)
    {
      return_code = 400;
    }
    else
    {
      return_code = 200;
    }

    // Send command response with report as JSON payload
    int rc;
    if ((rc = send_command_response(command_request, return_code, command_response_span)) != 0)
    {
      printf("Unable to send %u response, status %d\n", return_code, rc);
    }
  }
  else
  {
    // Unsupported command
    printf(
        "Unsupported command received: %.*s.\n",
        az_span_size(command_request->name),
        az_span_ptr(command_request->name));

    int rc;
    if ((rc = send_command_response(command_request, 404, report_error_payload)) != 0)
    {
      printf("Unable to send %d response, status %d\n", 404, rc);
    }
  }
}

/

    static az_result build_telemetry_message(az_span* out_payload)
{
  az_json_writer json_builder;
  AZ_RETURN_IF_FAILED(
      az_json_writer_init(&json_builder, AZ_SPAN_FROM_BUFFER(telemetry_payload), NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&json_builder));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&json_builder, telemetry_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_double(
      &json_builder, current_device_temp, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&json_builder));
  *out_payload = az_json_writer_get_bytes_used_in_destination(&json_builder);

  return AZ_OK;
}

// Send JSON formatted telemetry messages
static int send_telemetry_message(void)
{
  int rc;

  if (az_failed(
          rc = az_iot_hub_client_telemetry_get_publish_topic(
              &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    return rc;
  }

  az_span telemetry_payload_span;
  if (az_failed(rc = build_telemetry_message(&telemetry_payload_span)))
  {
    printf("Could not build telemetry payload, az_result %d\n", rc);
    return rc;
  }

  printf("Sending Telemetry Message\n");
  rc = mqtt_publish_message(telemetry_topic, telemetry_payload_span, 0);

  // New line to separate messages on the console
  putchar('\n');
  return rc;
}

// Create request id span which increments source int each call. Capable of holding 8 digit number.
az_span get_topic_request_id(void)
{
  az_span remainder;
  az_span out_span
      = az_span_create((uint8_t*)topic_request_id_buffer, sizeof(topic_request_id_buffer));
  az_result result = az_span_i32toa(out_span, topic_request_id_int++, &remainder);
  (void)remainder;
  (void)result;
  return out_span;
}

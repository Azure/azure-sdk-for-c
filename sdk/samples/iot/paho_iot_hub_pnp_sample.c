// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "iot_sample_foundation.h"

#define SAMPLE_TYPE PAHO_IOT_HUB
#define SAMPLE_NAME PAHO_IOT_HUB_PNP_SAMPLE

#define TELEMETRY_SEND_INTERVAL 1
#define TIMEOUT_MQTT_RECEIVE_MS (20 * 1000)
#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)
#define TIMEOUT_MQTT_WAIT_FOR_COMPLETION_MS 1000

#define DEFAULT_START_TEMP_AVG_COUNT 1
#define DEFAULT_START_TEMP_CELSIUS 22.0
#define DOUBLE_DECIMAL_PLACE_DIGITS 2

// * PnP Values *
// The model id is the JSON document (also called the Digital Twins Model Identifier or DTMI)
// which defines the capability of your device. The functionality of the device should match what
// is described in the coresponding DTMI. Should you choose to program your own PnP capable device,
// the functionality would need to match the DTMI and you would need to update the below 'model_id'.
// Please see the sample README for more information on this DTMI.
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:com:example:Thermostat;1");

static const az_span gl_desired_name = AZ_SPAN_LITERAL_FROM_STR("desired");
static const az_span gl_version_name = AZ_SPAN_LITERAL_FROM_STR("$version");
static const az_span gl_success_name = AZ_SPAN_LITERAL_FROM_STR("success");
static const az_span gl_value_name = AZ_SPAN_LITERAL_FROM_STR("value");
static const az_span gl_ack_code_name = AZ_SPAN_LITERAL_FROM_STR("ac");
static const az_span gl_ack_version_name = AZ_SPAN_LITERAL_FROM_STR("av");
static const az_span gl_ack_description_name = AZ_SPAN_LITERAL_FROM_STR("ad");
static const az_span gl_temperature_name = AZ_SPAN_LITERAL_FROM_STR("temperature");
static const az_span gl_max_temp_name = AZ_SPAN_LITERAL_FROM_STR("maxTemp");
static const az_span gl_min_temp_name = AZ_SPAN_LITERAL_FROM_STR("minTemp");
static const az_span gl_avg_temp_name = AZ_SPAN_LITERAL_FROM_STR("avgTemp");
static const az_span gl_start_time_name = AZ_SPAN_LITERAL_FROM_STR("startTime");
static const az_span gl_end_time_name = AZ_SPAN_LITERAL_FROM_STR("endTime");

// IoT Hub Twin Values
static const az_span desired_temp_property_name = AZ_SPAN_LITERAL_FROM_STR("targetTemperature");
static const az_span reported_max_temp_property_name
    = AZ_SPAN_LITERAL_FROM_STR("maxTempSinceLastReboot");

// IoT Hub Method Values
static const az_span report_method_name = AZ_SPAN_LITERAL_FROM_STR("getMaxMinReport");
static const az_span report_error_payload = AZ_SPAN_LITERAL_FROM_STR("{}");
static char incoming_since_value_buffer[32];
static char end_time_buffer[32];
static char method_response_payload_buffer[256];

// PnP Device Values
static double device_current_temp = DEFAULT_START_TEMP_CELSIUS;
static double device_temp_avg_total = DEFAULT_START_TEMP_CELSIUS;
static uint32_t device_temp_avg_count = DEFAULT_START_TEMP_AVG_COUNT;
static double device_max_temp = DEFAULT_START_TEMP_CELSIUS;
static double device_min_temp = DEFAULT_START_TEMP_CELSIUS;
static double device_avg_temp = DEFAULT_START_TEMP_CELSIUS;

static char request_id_buffer[16];
static int32_t request_id_int = 0;
static bool device_operational = true;
static const char iso_spec_time_format[] = "%Y-%m-%dT%H:%M:%S%z"; // ISO8601 Time Format

typedef enum
{
  MSG_DELIVERED_AT_MOST_ONCE,
  MSG_DELIVERED_AT_LEAST_ONCE,
  MSG_DELIVERED_EXACTLY_ONCE
} iot_quality_of_service;

static sample_environment_variables env_vars;
static az_iot_hub_client hub_client;
static MQTTClient mqtt_client;
static char mqtt_client_username_buffer[256];

// Functions
static void create_and_configure_mqtt_client();
static void connect_mqtt_client_to_iot_hub();
static void subscribe_mqtt_client_to_iot_hub_topics();
static void request_device_twin_document();
static void receive_messages();
static void disconnect_mqtt_client_from_iot_hub();

static void on_message_received(char* topic, int topic_len, const MQTTClient_message* message);
static void handle_device_twin_message(
    const az_span message_span,
    const az_iot_hub_client_twin_response* twin_response);
static void handle_method_message(
    const az_span message_span,
    const az_iot_hub_client_method_request* method_request);

static az_result parse_device_twin_desired_temperature_property(
    az_span twin_payload_span,
    bool is_twin_get,
    double* parsed_temp,
    int32_t* version_number);
static void update_device_temp(double temp, bool* is_max_temp_changed);
static void send_reported_property(
    az_span name,
    double value,
    int32_t version,
    bool is_confirm_required);
static void send_method_response(
    const az_iot_hub_client_method_request* method_request,
    uint16_t status,
    az_span response_payload);
static az_result get_max_min_report_method(
    az_span payload,
    az_span response_destination,
    az_span* response_out);
static void send_telemetry_message(void);

static void mqtt_publish_message(char* topic, az_span payload, int qos);
static az_span get_request_id(void);

static az_result build_payload(
    uint8_t property_count,
    const az_span* name,
    const double* value,
    const az_span* times,
    az_span payload_destination,
    az_span* payload_out);
static az_result build_payload_confirm(
    az_span name,
    double value,
    int32_t ack_code_value,
    int32_t ack_version_value,
    az_span ack_description_value,
    az_span payload_destination,
    az_span* payload_out);

/*
 * This sample connects an IoT Plug and Play enabled device with the Digital Twin Model ID (DTMI).
 * X509 self-certification is used.  In short, the capabilities are listed here:
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

static void create_and_configure_mqtt_client()
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

static void connect_mqtt_client_to_iot_hub()
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

static void subscribe_mqtt_client_to_iot_hub_topics()
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

static void request_device_twin_document()
{
  int rc;

  LOG("Client requesting device twin document from service.");

  // Get the Twin Document topic to publish the twin document request.
  char twin_document_topic_buffer[128];
  az_span request_id = get_request_id();
  if (az_failed(
          rc = az_iot_hub_client_twin_document_get_publish_topic(
              &hub_client,
              request_id,
              twin_document_topic_buffer,
              sizeof(twin_document_topic_buffer),
              NULL)))
  {
    LOG_ERROR("Failed to get Twin Document publish topic: az_result return code %04x", rc);
    exit(rc);
  }

  // Publish the twin document request.
  mqtt_publish_message(twin_document_topic_buffer, AZ_SPAN_NULL, 0);
}

static void receive_messages()
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
    LOG_SUCCESS("Client received message from the service.");

    on_message_received(topic, topic_len, message);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topic);

    // Send a telemetry message
    send_telemetry_message();

    LOG_SUCCESS("Client sent telemetry message to the service.");
  }
}

static void disconnect_mqtt_client_from_iot_hub()
{
  int rc;

  if ((rc = MQTTClient_disconnect(mqtt_client, TIMEOUT_MQTT_DISCONNECT_MS)) != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to disconnect MQTT client: MQTTClient return code %d.", rc);
    exit(rc);
  }

  MQTTClient_destroy(&mqtt_client);
}

static void mqtt_publish_message(char* topic, az_span payload, int qos)
{
  int rc;
  MQTTClient_deliveryToken token;

  if ((rc = MQTTClient_publish(
           mqtt_client, topic, az_span_size(payload), az_span_ptr(payload), qos, 0, &token))
      != MQTTCLIENT_SUCCESS)
  {
    LOG_ERROR("Failed to publish message: MQTTClient return code %d", rc);
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

static void on_message_received(char* topic, int topic_len, const MQTTClient_message* message)
{
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
    LOG("Status: %d", twin_response.status);

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

static void handle_device_twin_message(
    const az_span message_span,
    const az_iot_hub_client_twin_response* twin_response)
{
  // Invoke appropriate action per response type (3 Types only).
  bool is_twin_get = false;

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
  double desired_temp = 0.0;
  int32_t version_num = 0;

  if (twin_response->response_type == AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET
      || twin_response->response_type == AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES)
  {
    if (az_succeeded(parse_device_twin_desired_temperature_property(
            message_span, is_twin_get, &desired_temp, &version_num)))
    {
      bool confirm = true;
      bool is_max_temp_changed = false;

      update_device_temp(desired_temp, &is_max_temp_changed);
      LOG_SUCCESS("Client updated desired temperature locally.");

      send_reported_property(desired_temp_property_name, desired_temp, version_num, confirm);
      LOG_SUCCESS("Client sent temperature reported property message.");

      if (is_max_temp_changed)
      {
        confirm = false;
        send_reported_property(
            reported_max_temp_property_name, device_max_temp, version_num, confirm);
        LOG_SUCCESS("Client sent max temperature reported property message.");
      }
    }
    // else Desired property not found in payload. Do nothing.
  }
}

static void handle_method_message(
    const az_span message_span,
    const az_iot_hub_client_method_request* method_request)
{
  if (az_span_is_content_equal(report_method_name, method_request->name))
  {
    az_iot_status status;
    az_span method_response_payload = AZ_SPAN_FROM_BUFFER(method_response_payload_buffer);

    LOG_AZ_SPAN("message span:", message_span);
    if (az_failed(get_max_min_report_method(
            message_span, method_response_payload, &method_response_payload)))
    {
      status = AZ_IOT_STATUS_BAD_REQUEST;
    }
    else
    {
      status = AZ_IOT_STATUS_OK;
    }
    LOG_SUCCESS("Client invoked 'get_max_min_report_method'.");

    send_method_response(method_request, status, method_response_payload);
  }
  else
  {
    LOG_AZ_SPAN("Method not supported:", method_request->name);
    send_method_response(method_request, AZ_IOT_STATUS_NOT_FOUND, report_error_payload);
  }
}

static az_result parse_device_twin_desired_temperature_property(
    const az_span twin_payload_span,
    bool is_twin_get,
    double* parsed_temp,
    int32_t* version_number)
{
  az_json_reader jr;

  AZ_RETURN_IF_FAILED(az_json_reader_init(&jr, twin_payload_span, NULL));
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
      if (az_json_token_is_text_equal(&jr.token, gl_desired_name))
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
    else if (az_json_token_is_text_equal(&jr.token, gl_version_name))
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

static void update_device_temp(double temp, bool* is_max_temp_changed)
{
  device_current_temp = temp;

  bool ret = false;
  if (device_current_temp > device_max_temp)
  {
    device_max_temp = device_current_temp;
    ret = true;
  }
  if (device_current_temp < device_min_temp)
  {
    device_min_temp = device_current_temp;
  }

  // Increment the avg count, add the new temp to the total, and calculate the new avg
  device_temp_avg_count++;
  device_temp_avg_total += device_current_temp;
  device_avg_temp = device_temp_avg_total / device_temp_avg_count;

  *is_max_temp_changed = ret;
}

static void send_reported_property(az_span name, double value, int32_t version, bool confirm)
{
  int rc;

  LOG("Client sending reported property to service.");

  // Get the Twin Patch topic to send a reported property update.
  char twin_patch_topic_buffer[128];
  az_span request_id_span = get_request_id();
  if (az_failed(
          rc = az_iot_hub_client_twin_patch_get_publish_topic(
              &hub_client,
              request_id_span,
              twin_patch_topic_buffer,
              sizeof(twin_patch_topic_buffer),
              NULL)))
  {
    LOG_ERROR("Failed to get Twin Patch publish topic: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Build the updated reported property message.
  char reported_property_payload_buffer[128];
  az_span reported_property_payload = AZ_SPAN_FROM_BUFFER(reported_property_payload_buffer);
  if (confirm)
  {
    if (az_failed(
            rc = build_payload_confirm(
                name,
                value,
                AZ_IOT_STATUS_OK,
                version,
                gl_success_name,
                reported_property_payload,
                &reported_property_payload)))
    {
      LOG_ERROR("Failed to build confirmed payload : az_result return code 0x%04x.", rc);
      exit(rc);
    }
  }
  else
  {
    const uint8_t count = 1;
    az_span names[1] = { name };
    double values[1] = { value };

    if (az_failed(
            rc = build_payload(
                count, names, values, NULL, reported_property_payload, &reported_property_payload)))
    {
      LOG_ERROR("Failed to build payload: az_result return code 0x%04x.", rc);
      exit(rc);
    }
  }

  // Publish the reported property update.
  mqtt_publish_message(twin_patch_topic_buffer, reported_property_payload, 0);
  LOG_AZ_SPAN("Payload:", reported_property_payload);
}

static void send_method_response(
    const az_iot_hub_client_method_request* method_request,
    uint16_t status,
    az_span response_payload)
{
  int rc;

  // Get the Methods response topic to publish the method response.
  char methods_response_topic_buffer[128];
  if (az_failed(
          rc = az_iot_hub_client_methods_response_get_publish_topic(
              &hub_client,
              method_request->request_id,
              status,
              methods_response_topic_buffer,
              sizeof(methods_response_topic_buffer),
              NULL)))
  {
    LOG_ERROR("Failed to get Methods response publish topic: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Publish the method response.
  mqtt_publish_message(methods_response_topic_buffer, response_payload, 0);
  LOG_SUCCESS("Client published method response:");
  LOG("Status: %u", status);
  LOG_AZ_SPAN("Payload:", response_payload);
}

static az_result get_max_min_report_method(
    az_span payload,
    az_span response_destination,
    az_span* response_out)
{
  LOG("in max min report");
  // Parse the "since" field in the payload.
  int32_t incoming_since_value_len = 0;
  az_json_reader jr;

  AZ_RETURN_IF_FAILED(az_json_reader_init(&jr, payload, NULL));
  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
  AZ_RETURN_IF_FAILED(az_json_token_get_string(
      &jr.token,
      incoming_since_value_buffer,
      sizeof(incoming_since_value_buffer),
      &incoming_since_value_len));
  az_span start_time_span
      = az_span_create((uint8_t*)incoming_since_value_buffer, incoming_since_value_len);

   LOG_AZ_SPAN("start time:", start_time_span);

  // Set the response payload to error if the "since" field was not sent
  if (az_span_ptr(start_time_span) == NULL)
  {
    response_destination = report_error_payload;
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  // Get the current time as a string

  time_t rawtime;
  struct tm* timeinfo;

  time(&rawtime);
  timeinfo = localtime(&rawtime);
  size_t length
      = strftime(end_time_buffer, sizeof(end_time_buffer), iso_spec_time_format, timeinfo);
  az_span end_time_span = az_span_create((uint8_t*)end_time_buffer, (int32_t)length);


  LOG_AZ_SPAN("end time:", end_time_span);

  // Build method response message.
  const uint8_t count = 3;
  const az_span names[3] = { gl_max_temp_name, gl_min_temp_name, gl_avg_temp_name };
  const double values[3] = { device_max_temp, device_min_temp, device_avg_temp };
  const az_span times[2] = { start_time_span, end_time_span };

  int rc;
  if (az_failed(
          rc = build_payload(count, names, values, times, response_destination, response_out)))
  {
    LOG_ERROR("Failed to build method response payload: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  return AZ_OK;
}

static void send_telemetry_message(void)
{
  int rc;

  // Get the Telemetry topic to publish the telemetry message.
  char telemetry_topic_buffer[128];
  if (az_failed(
          rc = az_iot_hub_client_telemetry_get_publish_topic(
              &hub_client, NULL, telemetry_topic_buffer, sizeof(telemetry_topic_buffer), NULL)))
  {
    LOG_ERROR("Failed to get Telemetry publish topic: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Build the telemetry message.
  const uint8_t count = 1;
  const az_span names[1] = { gl_temperature_name };
  const double values[1] = { device_current_temp };

  char telemetry_payload_buffer[128];
  az_span telemetry_payload = AZ_SPAN_FROM_BUFFER(telemetry_payload_buffer);
  if (az_failed(
          rc = build_payload(count, names, values, NULL, telemetry_payload, &telemetry_payload)))
  {
    LOG_ERROR("Failed to build telemetry payload: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Publish the telemetry message.
  mqtt_publish_message(telemetry_topic_buffer, telemetry_payload, 0);
  LOG_AZ_SPAN("Payload:", telemetry_payload);

  LOG(" "); // Formatting
}

static az_span get_request_id(void)
{
  az_result rc;
  az_span out_span;
  az_span destination = az_span_create((uint8_t*)request_id_buffer, sizeof(request_id_buffer));

  if (az_failed(rc = az_span_i32toa(destination, request_id_int++, &out_span)))
  {
    LOG_ERROR("Failed to get request id: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  return destination;
}

static az_result build_payload(
    uint8_t property_count,
    const az_span* name,
    const double* value,
    const az_span* times,
    az_span payload_destination,
    az_span* payload_out)
{
  az_json_writer jw;
  LOG("building payload");
  AZ_RETURN_IF_FAILED(az_json_writer_init(&jw, payload_destination, NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));

  for (uint8_t i = 0; i < property_count; i++)
  {
    AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, name[i]));
    AZ_RETURN_IF_FAILED(az_json_writer_append_double(&jw, value[i], DOUBLE_DECIMAL_PLACE_DIGITS));
    LOG_AZ_SPAN("name:", name[i]);
    LOG("value: %f", value[i]);
  }

  if (times != NULL)
  {
    AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, gl_start_time_name));
    AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, times[0]));
    LOG_AZ_SPAN("name:", gl_start_time_name);
    LOG_AZ_SPAN("value:", times[0]);
    AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, gl_end_time_name));
    AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, times[1]));
    LOG_AZ_SPAN("name:", gl_end_time_name);
    LOG_AZ_SPAN("value:", times[1]);
  }

  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));
  *payload_out = az_json_writer_get_bytes_used_in_destination(&jw);

  return AZ_OK;
}

static az_result build_payload_confirm(
    az_span name,
    double value,
    int32_t ack_code_value,
    int32_t ack_version_value,
    az_span ack_description_value,
    az_span payload_destination,
    az_span* payload_out)
{
  az_json_writer jw;

  AZ_RETURN_IF_FAILED(az_json_writer_init(&jw, payload_destination, NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, gl_value_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_double(&jw, value, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, gl_ack_code_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(&jw, ack_code_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, gl_ack_version_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(&jw, ack_version_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, gl_ack_description_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, ack_description_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));

  *payload_out = az_json_writer_get_bytes_used_in_destination(&jw);

  return AZ_OK;
}

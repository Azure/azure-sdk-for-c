// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4996: 'localtime': This function or variable may be unsafe. Consider using localtime_s
// instead.
#pragma warning(disable : 4996)
#endif

#include <azure/iot/az_iot_pnp_client.h>
#include <iot_sample_common.h>

#include "pnp_mqtt_message.h"

#include "pnp_thermostat_component.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <azure/az_core.h>
#include <azure/az_iot.h>

#include <iot_sample_common.h>

#include "pnp_mqtt_message.h"
#include "pnp_thermostat_component.h"

#define DOUBLE_DECIMAL_PLACE_DIGITS 2
#define DEFAULT_START_TEMP_COUNT 1

static char const iso_spec_time_format[] = "%Y-%m-%dT%H:%M:%SZ"; // ISO8601 Time Format

// IoT Hub Device Property Values
static az_span const property_desired_temperature_property_name
    = AZ_SPAN_LITERAL_FROM_STR("targetTemperature");
static az_span const property_reported_maximum_temperature_property_name
    = AZ_SPAN_LITERAL_FROM_STR("maxTempSinceLastReboot");
static az_span const property_response_success = AZ_SPAN_LITERAL_FROM_STR("success");

// IoT Hub Commands Values
static az_span const command_getMaxMinReport_name = AZ_SPAN_LITERAL_FROM_STR("getMaxMinReport");
static az_span const command_max_temp_name = AZ_SPAN_LITERAL_FROM_STR("maxTemp");
static az_span const command_min_temp_name = AZ_SPAN_LITERAL_FROM_STR("minTemp");
static az_span const command_avg_temp_name = AZ_SPAN_LITERAL_FROM_STR("avgTemp");
static az_span const command_start_time_name = AZ_SPAN_LITERAL_FROM_STR("startTime");
static az_span const command_end_time_name = AZ_SPAN_LITERAL_FROM_STR("endTime");
static az_span const command_empty_response_payload = AZ_SPAN_LITERAL_FROM_STR("{}");
static char command_start_time_value_buffer[32];
static char command_end_time_value_buffer[32];

// IoT Hub Telemetry Values
static az_span const telemetry_temperature_name = AZ_SPAN_LITERAL_FROM_STR("temperature");

static void build_command_response_payload(
    pnp_thermostat_component const* thermostat_component,
    az_span start_time,
    az_span end_time,
    az_span payload,
    az_span* out_payload)
{
  char const* const log = "Failed to build command response payload";

  az_json_writer jw;
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_init(&jw, payload, NULL), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, command_max_temp_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_double(
          &jw, thermostat_component->maximum_temperature, DOUBLE_DECIMAL_PLACE_DIGITS),
      log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, command_min_temp_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_double(
          &jw, thermostat_component->minimum_temperature, DOUBLE_DECIMAL_PLACE_DIGITS),
      log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, command_avg_temp_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_double(
          &jw, thermostat_component->average_temperature, DOUBLE_DECIMAL_PLACE_DIGITS),
      log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, command_start_time_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_string(&jw, start_time), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, command_end_time_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_string(&jw, end_time), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), log);

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);
}

static bool invoke_getMaxMinReport(
    pnp_thermostat_component const* thermostat_component,
    az_span payload,
    az_span response,
    az_span* out_response)
{
  int32_t incoming_since_value_len = 0;

  // Parse the `since` field in the payload.
  char const* const log = "Failed to parse for `since` field in payload";

  az_json_reader jr;
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_init(&jr, payload, NULL), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_next_token(&jr), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_token_get_string(
          &jr.token,
          command_start_time_value_buffer,
          sizeof(command_start_time_value_buffer),
          &incoming_since_value_len),
      log);

  // Set the response payload to error if the `since` field was empty.
  if (incoming_since_value_len == 0)
  {
    *out_response = command_empty_response_payload;
    return false;
  }

  az_span start_time_span
      = az_span_create((uint8_t*)command_start_time_value_buffer, incoming_since_value_len);

  IOT_SAMPLE_LOG_AZ_SPAN("Start Time:", start_time_span);

  // Get the current time as a string.
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  size_t length = strftime(
      command_end_time_value_buffer,
      sizeof(command_end_time_value_buffer),
      iso_spec_time_format,
      timeinfo);
  az_span end_time_span = az_span_create((uint8_t*)command_end_time_value_buffer, (int32_t)length);

  IOT_SAMPLE_LOG_AZ_SPAN("End Time:", end_time_span);

  // Build command response message.
  build_command_response_payload(
      thermostat_component, start_time_span, end_time_span, response, out_response);

  return true;
}

az_result pnp_thermostat_init(
    pnp_thermostat_component* out_thermostat_component,
    az_span component_name,
    double initial_temperature)
{
  if (out_thermostat_component == NULL)
  {
    return AZ_ERROR_ARG;
  }

  out_thermostat_component->component_name = component_name;
  out_thermostat_component->average_temperature = initial_temperature;
  out_thermostat_component->current_temperature = initial_temperature;
  out_thermostat_component->maximum_temperature = initial_temperature;
  out_thermostat_component->minimum_temperature = initial_temperature;
  out_thermostat_component->temperature_count = DEFAULT_START_TEMP_COUNT;
  out_thermostat_component->temperature_summation = initial_temperature;
  out_thermostat_component->send_maximum_temperature_property = true;

  return AZ_OK;
}

void pnp_thermostat_build_telemetry_message(
    pnp_thermostat_component* thermostat_component,
    az_span payload,
    az_span* out_payload)
{
  az_json_writer jw;

  char const* const log = "Failed to build telemetry payload";

  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_init(&jw, payload, NULL), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, telemetry_temperature_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_double(
          &jw, thermostat_component->current_temperature, DOUBLE_DECIMAL_PLACE_DIGITS),
      log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), log);

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);
}

void pnp_thermostat_build_maximum_temperature_reported_property(
    az_iot_pnp_client const* pnp_client,
    pnp_thermostat_component* thermostat_component,
    az_span payload,
    az_span* out_payload,
    az_span* out_property_name)
{
  *out_property_name = property_reported_maximum_temperature_property_name;

  az_json_writer jw;

  const char* const log = "Failed to build maximum temperature reported property payload";

  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_init(&jw, payload, NULL), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_iot_pnp_client_property_builder_begin_component(
          pnp_client, &jw, thermostat_component->component_name),
      log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, property_reported_maximum_temperature_property_name),
      log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_double(
          &jw, thermostat_component->current_temperature, DOUBLE_DECIMAL_PLACE_DIGITS),
      log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_iot_pnp_client_property_builder_end_component(pnp_client, &jw), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), log);

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);
}

az_result pnp_thermostat_process_property_update(
    az_iot_pnp_client const* pnp_client,
    pnp_thermostat_component* ref_thermostat_component,
    az_json_reader* property_name_and_value,
    int32_t version,
    az_span payload,
    az_span* out_payload)
{
  double parsed_property_value = 0;

  if (!az_json_token_is_text_equal(
          &property_name_and_value->token, property_desired_temperature_property_name))
  {
    return false;
  }
  else
  {
    char const* const log = "Failed to process property update";

    az_span property_name_span = property_name_and_value->token.slice;
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_next_token(property_name_and_value), log);

    IOT_SAMPLE_EXIT_IF_AZ_FAILED(
        az_json_token_get_double(&property_name_and_value->token, &parsed_property_value), log);

    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_next_token(property_name_and_value), log);

    // Update variables locally.
    ref_thermostat_component->current_temperature = parsed_property_value;
    if (ref_thermostat_component->current_temperature
        > ref_thermostat_component->maximum_temperature)
    {
      ref_thermostat_component->maximum_temperature = ref_thermostat_component->current_temperature;
      ref_thermostat_component->send_maximum_temperature_property = true;
    }

    if (ref_thermostat_component->current_temperature
        < ref_thermostat_component->minimum_temperature)
    {
      ref_thermostat_component->minimum_temperature = ref_thermostat_component->current_temperature;
    }

    // Calculate and update the new average temperature.
    ref_thermostat_component->temperature_count++;
    ref_thermostat_component->temperature_summation
        += ref_thermostat_component->current_temperature;
    ref_thermostat_component->average_temperature = ref_thermostat_component->temperature_summation
        / ref_thermostat_component->temperature_count;

    IOT_SAMPLE_LOG_SUCCESS("Client updated desired temperature variables locally.");
    IOT_SAMPLE_LOG("Current Temperature: %2f", ref_thermostat_component->current_temperature);
    IOT_SAMPLE_LOG("Maximum Temperature: %2f", ref_thermostat_component->maximum_temperature);
    IOT_SAMPLE_LOG("Minimum Temperature: %2f", ref_thermostat_component->minimum_temperature);
    IOT_SAMPLE_LOG("Average Temperature: %2f", ref_thermostat_component->average_temperature);

    az_json_writer jw;
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(
        az_json_writer_init(&jw, payload, NULL), "Failed to initialize the json writer");

    const char* const property_log = "Failed to create property with status payload";

    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), property_log);
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(
        az_iot_pnp_client_property_builder_begin_component(
            pnp_client, &jw, ref_thermostat_component->component_name),
        property_log);
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(
        az_iot_pnp_client_property_builder_begin_reported_status(
            pnp_client,
            &jw,
            property_name_span,
            (int32_t)AZ_IOT_STATUS_OK,
            version,
            property_response_success),
        property_log);
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(
        az_json_writer_append_double(&jw, parsed_property_value, DOUBLE_DECIMAL_PLACE_DIGITS),
        property_log);
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(
        az_iot_pnp_client_property_builder_end_reported_status(pnp_client, &jw), property_log);
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(
        az_iot_pnp_client_property_builder_end_component(pnp_client, &jw), property_log);

    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), property_log);

    *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);
  }

  return true;
}

bool pnp_thermostat_process_command_request(
    pnp_thermostat_component const* thermostat_component,
    az_span command_name,
    az_span command_received_payload,
    az_span payload,
    az_span* out_payload,
    az_iot_status* out_status)
{
  if (az_span_is_content_equal(command_getMaxMinReport_name, command_name))
  {
    // Invoke command.
    if (invoke_getMaxMinReport(
            thermostat_component, command_received_payload, payload, out_payload))
    {
      *out_status = AZ_IOT_STATUS_OK;
    }
    else
    {
      *out_payload = command_empty_response_payload;
      *out_status = AZ_IOT_STATUS_BAD_REQUEST;
      IOT_SAMPLE_LOG_AZ_SPAN(
          "Bad request when invoking command on Thermostat Sensor component:", command_name);
      return false;
    }
  }
  else // Unsupported command
  {
    *out_payload = command_empty_response_payload;
    *out_status = AZ_IOT_STATUS_NOT_FOUND;
    IOT_SAMPLE_LOG_AZ_SPAN("Command not supported on Thermostat Sensor component:", command_name);
    return false;
  }

  return true;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4996: 'localtime': This function or variable may be unsafe.  Consider using localtime_s
// instead.
#pragma warning(disable : 4996)
#endif

#include "sample_pnp_thermostat_component.h"
#include "iot_samples_common.h"
#include "sample_pnp.h"
#include "sample_pnp_mqtt_component.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#define DOUBLE_DECIMAL_PLACE_DIGITS 2

// IoT Telemetry Values
static const az_span telemetry_name = AZ_SPAN_LITERAL_FROM_STR("temperature");

// IoT Hub Commands Values
static const az_span report_command_name_span = AZ_SPAN_LITERAL_FROM_STR("getMaxMinReport");
static const az_span report_max_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("maxTemp");
static const az_span report_min_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("minTemp");
static const az_span report_avg_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("avgTemp");
static const az_span report_start_time_name_span = AZ_SPAN_LITERAL_FROM_STR("startTime");
static const az_span report_end_time_name_span = AZ_SPAN_LITERAL_FROM_STR("endTime");
static const az_span report_error_payload = AZ_SPAN_LITERAL_FROM_STR("{}");
static char end_time_buffer[32];
static char incoming_since_value_buffer[32];

// Twin Values
static const az_span desired_temp_property_name = AZ_SPAN_LITERAL_FROM_STR("targetTemperature");
static const az_span temp_response_description_success = AZ_SPAN_LITERAL_FROM_STR("success");
static const az_span temp_response_description_failed = AZ_SPAN_LITERAL_FROM_STR("failed");
static const az_span max_temp_reported_property_name
    = AZ_SPAN_LITERAL_FROM_STR("maxTempSinceLastReboot");

// ISO8601 Time Format
static const char iso_spec_time_format[] = "%Y-%m-%dT%H:%M:%S%z";

static bool build_command_response_payload(
    const pnp_thermostat_component* thermostat_component,
    az_span start_time_span,
    az_span end_time_span,
    az_span payload,
    az_span* out_payload)
{
  double avg_temp = thermostat_component->device_temperature_avg_total
      / thermostat_component->device_temperature_avg_count;

  // Build the command response payload
  az_json_writer jw;
  if (az_failed(az_json_writer_init(&jw, payload, NULL))
      || az_failed(az_json_writer_append_begin_object(&jw))
      || az_failed(az_json_writer_append_property_name(&jw, report_max_temp_name_span))
      || az_failed(az_json_writer_append_double(
          &jw, thermostat_component->max_temperature, DOUBLE_DECIMAL_PLACE_DIGITS))
      || az_failed(az_json_writer_append_property_name(&jw, report_min_temp_name_span))
      || az_failed(az_json_writer_append_double(
          &jw, thermostat_component->min_temperature, DOUBLE_DECIMAL_PLACE_DIGITS))
      || az_failed(az_json_writer_append_property_name(&jw, report_avg_temp_name_span))
      || az_failed(az_json_writer_append_double(&jw, avg_temp, DOUBLE_DECIMAL_PLACE_DIGITS))
      || az_failed(az_json_writer_append_property_name(&jw, report_start_time_name_span))
      || az_failed(az_json_writer_append_string(&jw, start_time_span))
      || az_failed(az_json_writer_append_property_name(&jw, report_end_time_name_span))
      || az_failed(az_json_writer_append_string(&jw, end_time_span))
      || az_failed(az_json_writer_append_end_object(&jw)))
  {
    return false;
  }

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);

  return true;
}

static bool build_telemetry_message(
    const pnp_thermostat_component* thermostat_component,
    az_span payload,
    az_span* out_payload)
{
  az_json_writer jw;
  if (az_failed(az_json_writer_init(&jw, payload, NULL))
      || az_failed(az_json_writer_append_begin_object(&jw))
      || az_failed(az_json_writer_append_property_name(&jw, telemetry_name))
      || az_failed(az_json_writer_append_double(
          &jw, thermostat_component->current_temperature, DOUBLE_DECIMAL_PLACE_DIGITS))
      || az_failed(az_json_writer_append_end_object(&jw)))
  {
    return false;
  }

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);

  return true;
}

// Invoke the command requested from the service. Here, it generates a report for max, min, and avg
// temperatures.
static bool invoke_getMaxMinReport(
    const pnp_thermostat_component* thermostat_component,
    az_span payload,
    az_span response,
    az_span* out_response)
{
  // Parse the "since" field in the payload.
  az_span start_time_span = AZ_SPAN_NULL;
  az_json_reader jr;

  if (az_failed(az_json_reader_init(&jr, payload, NULL))
      || az_failed(az_json_reader_next_token(&jr)))
  {
    return false;
  }

  int32_t incoming_since_value_buffer_len;
  if (az_failed(az_json_token_get_string(
          &jr.token,
          incoming_since_value_buffer,
          sizeof(incoming_since_value_buffer),
          &incoming_since_value_buffer_len)))
  {
    return false;
  }

  start_time_span
      = az_span_create((uint8_t*)incoming_since_value_buffer, incoming_since_value_buffer_len);

  // Set the response payload to error if the "since" field was not sent
  if (az_span_ptr(start_time_span) == NULL)
  {
    response = report_error_payload;
    return false;
  }

  // Get the current time as a string
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  size_t len = strftime(end_time_buffer, sizeof(end_time_buffer), iso_spec_time_format, timeinfo);
  az_span end_time_span = az_span_create((uint8_t*)end_time_buffer, (int32_t)len);

  return build_command_response_payload(
      thermostat_component, start_time_span, end_time_span, response, out_response));
}

static bool append_double(az_json_writer* jw, void* value)
{
  double value_as_double = *(double*)value;

  return az_succeeded(
      az_json_writer_append_double(jw, value_as_double, DOUBLE_DECIMAL_PLACE_DIGITS));
}

bool pnp_thermostat_init(
    pnp_thermostat_component* thermostat_component,
    az_span component_name,
    double initial_temp)
{
  if (thermostat_component == NULL)
  {
    return false;
  }

  thermostat_component->component_name = component_name;
  thermostat_component->current_temperature = initial_temp;
  thermostat_component->min_temperature = initial_temp;
  thermostat_component->max_temperature = initial_temp;
  thermostat_component->device_temperature_avg_count = 1;
  thermostat_component->device_temperature_avg_total = initial_temp;
  thermostat_component->avg_temperature = initial_temp;
  thermostat_component->send_max_temp_property = true;

  return true;
}

bool pnp_thermostat_get_telemetry_message(
    const az_iot_hub_client* client,
    const pnp_thermostat_component* thermostat_component,
    pnp_mqtt_message* mqtt_message)
{
  if (!pnp_get_telemetry_topic(
          client,
          NULL,
          thermostat_component->component_name,
          mqtt_message->topic,
          mqtt_message->topic_length,
          NULL))
  {
    LOG_ERROR("Failed to get pnp Telemetry topic.");
    return false;
  }

  if (!build_telemetry_message(
          thermostat_component, mqtt_message->payload_span, &mqtt_message->out_payload_span))
  {
    LOG_ERROR("Failed to build telemetry payload.");
    return false;
  }

  return true;
}

bool pnp_thermostat_get_max_temp_report(
    const az_iot_hub_client* client,
    pnp_thermostat_component* thermostat_component,
    pnp_mqtt_message* mqtt_message)
{
  if (!thermostat_component->send_max_temp_property)
  {
    return false;
  }

  az_result rc;
  if (!pnp_create_reported_property(
          mqtt_message->payload_span,
          thermostat_component->component_name,
          max_temp_reported_property_name,
          append_double,
          &thermostat_component->max_temperature,
          &mqtt_message->out_payload_span))
  {
    LOG_ERROR("Failed to get reported property.");
    return false;
  }
  else if (az_failed(
               rc = az_iot_hub_client_twin_patch_get_publish_topic(
                   client,
                   get_request_id(),
                   mqtt_message->topic,
                   mqtt_message->topic_length,
                   NULL)))
  {
    LOG_ERROR(
        "Failed to get reported property topic with status: az_result return code 0x%08x.", rc);
    return false;
  }

  thermostat_component->send_max_temp_property = false;

  return true;
}

bool pnp_thermostat_process_property_update(
    const az_iot_hub_client* client,
    pnp_thermostat_component* thermostat_component,
    az_span component_name,
    const az_json_token* property_name,
    const az_json_reader* property_value,
    int32_t version,
    pnp_mqtt_message* mqtt_message)
{
  bool result = true;
  if (!az_span_is_content_equal(thermostat_component->component_name, component_name))
  {
    return false;
  }

  if (!az_json_token_is_text_equal(property_name, desired_temp_property_name))
  {
    LOG_AZ_SPAN("PnP property is not supported on thermostat component:", property_name->slice);
  }

  double parsed_value = 0;
  if (az_failed(az_json_token_get_double(&(property_value->token), &parsed_value)))
  {
    result = pnp_create_reported_property_with_status(
        mqtt_message->payload_span,
        component_name,
        property_name->slice,
        append_double,
        (void*)&parsed_value,
        401,
        version,
        temp_response_description_failed,
        &mqtt_message->out_payload_span);
  }
  else
  {
    thermostat_component->current_temperature = parsed_value;
    if (thermostat_component->current_temperature > thermostat_component->max_temperature)
    {
      thermostat_component->max_temperature = thermostat_component->current_temperature;
      thermostat_component->send_max_temp_property = true;
    }

    if (thermostat_component->current_temperature < thermostat_component->min_temperature)
    {
      thermostat_component->min_temperature = thermostat_component->current_temperature;
    }

    /* Increment the avg count, add the new temp to the total, and calculate the new avg */
    thermostat_component->device_temperature_avg_count++;
    thermostat_component->device_temperature_avg_total += thermostat_component->current_temperature;
    thermostat_component->avg_temperature = thermostat_component->device_temperature_avg_total
        / thermostat_component->device_temperature_avg_count;

    result = pnp_create_reported_property_with_status(
        mqtt_message->payload_span,
        component_name,
        property_name->slice,
        append_double,
        (void*)&parsed_value,
        200,
        version,
        temp_response_description_success,
        &mqtt_message->out_payload_span);

    if (!result)
    {
      LOG_ERROR("Failed to get reported property payload with status.");
    }
  }

  result = !az_failed(az_iot_hub_client_twin_patch_get_publish_topic(
      client, get_request_id(), mqtt_message->topic, mqtt_message->topic_length, NULL));

  if (!result)
  {
    LOG_ERROR("Failed to get reported property topic with status.");
  }

  return result;
}

bool pnp_thermostat_process_command(
    const az_iot_hub_client* client,
    const pnp_thermostat_component* thermostat_component,
    const az_iot_hub_client_method_request* command_request,
    az_span component_name,
    az_span command_name,
    az_span command_payload,
    pnp_mqtt_message* mqtt_message,
    az_iot_status* status)
{
  if (!az_span_is_content_equal(thermostat_component->component_name, component_name)
      || !az_span_is_content_equal(report_command_name_span, command_name))
  {
    return false;
  }

  // Invoke command
  *status = invoke_getMaxMinReport(
                thermostat_component,
                command_payload,
                mqtt_message->payload_span,
                &mqtt_message->out_payload_span)
      ? AZ_IOT_STATUS_OK
      : AZ_IOT_STATUS_BAD_REQUEST;

  az_result rc;
  if (az_failed(
          rc = az_iot_hub_client_methods_response_get_publish_topic(
              client,
              command_request->request_id,
              (uint16_t)*status,
              mqtt_message->topic,
              mqtt_message->topic_length,
              mqtt_message->out_topic_length)))
  {
    LOG_ERROR("Failed to get methods response publish topic: az_result return code 0x%08x.", rc);

    return false;
  }

  return true;
}

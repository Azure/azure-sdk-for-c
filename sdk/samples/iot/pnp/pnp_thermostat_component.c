// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4996: 'localtime': This function or variable may be unsafe.  Consider using localtime_s
// instead.
#pragma warning(disable : 4996)
#endif

#include <iot_sample_common.h>

#include "pnp_mqtt_message.h"
#include "pnp_protocol.h"

#include "pnp_thermostat_component.h"

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

static az_result build_command_response_payload(
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
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_init(&jw, payload, NULL));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, report_max_temp_name_span));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_double(
      &jw, thermostat_component->max_temperature, DOUBLE_DECIMAL_PLACE_DIGITS));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, report_min_temp_name_span));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_double(
      &jw, thermostat_component->min_temperature, DOUBLE_DECIMAL_PLACE_DIGITS));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, report_avg_temp_name_span));
  IOT_SAMPLE_RETURN_IF_FAILED(
      az_json_writer_append_double(&jw, avg_temp, DOUBLE_DECIMAL_PLACE_DIGITS));
  IOT_SAMPLE_RETURN_IF_FAILED(
      az_json_writer_append_property_name(&jw, report_start_time_name_span));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_string(&jw, start_time_span));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, report_end_time_name_span));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_string(&jw, end_time_span));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);

  return AZ_OK;
}

static az_result build_telemetry_message(
    const pnp_thermostat_component* thermostat_component,
    az_span payload,
    az_span* out_payload)
{
  az_json_writer jw;
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_init(&jw, payload, NULL));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, telemetry_name));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_double(
      &jw, thermostat_component->current_temperature, DOUBLE_DECIMAL_PLACE_DIGITS));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));
  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);

  return AZ_OK;
}

// Invoke the command requested from the service. Here, it generates a report for max, min, and avg
// temperatures.
static az_result invoke_getMaxMinReport(
    const pnp_thermostat_component* thermostat_component,
    az_span payload,
    az_span response,
    az_span* out_response)
{
  // az_result result;
  // Parse the "since" field in the payload.
  az_span start_time_span = AZ_SPAN_EMPTY;
  az_json_reader jr;
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_reader_init(&jr, payload, NULL));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
  int32_t incoming_since_value_buffer_len;
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_token_get_string(
      &jr.token,
      incoming_since_value_buffer,
      sizeof(incoming_since_value_buffer),
      &incoming_since_value_buffer_len));

  // Set the response payload to error if the "since" field was not sent
  if (incoming_since_value_buffer_len == 0)
  {
    response = report_error_payload;
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  start_time_span
      = az_span_create((uint8_t*)incoming_since_value_buffer, incoming_since_value_buffer_len);

  // Get the current time as a string
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  size_t len = strftime(end_time_buffer, sizeof(end_time_buffer), iso_spec_time_format, timeinfo);
  az_span end_time_span = az_span_create((uint8_t*)end_time_buffer, (int32_t)len);

  IOT_SAMPLE_RETURN_IF_FAILED(build_command_response_payload(
      thermostat_component, start_time_span, end_time_span, response, out_response));

  return AZ_OK;
}

static az_result append_double(az_json_writer* jw, void* value)
{
  double value_as_double = *(double*)value;

  return az_json_writer_append_double(jw, value_as_double, DOUBLE_DECIMAL_PLACE_DIGITS);
}

az_result pnp_thermostat_init(
    pnp_thermostat_component* thermostat_component,
    az_span component_name,
    double initial_temp)
{
  if (thermostat_component == NULL)
  {
    return AZ_ERROR_ARG;
  }

  thermostat_component->component_name = component_name;
  thermostat_component->current_temperature = initial_temp;
  thermostat_component->min_temperature = initial_temp;
  thermostat_component->max_temperature = initial_temp;
  thermostat_component->device_temperature_avg_count = 1;
  thermostat_component->device_temperature_avg_total = initial_temp;
  thermostat_component->avg_temperature = initial_temp;
  thermostat_component->send_max_temp_property = true;

  return AZ_OK;
}

az_result pnp_thermostat_get_telemetry_message(
    const az_iot_hub_client* client,
    const pnp_thermostat_component* thermostat_component,
    pnp_mqtt_message* mqtt_message)
{
  az_result rc;
  if (az_result_failed(
          rc = pnp_get_telemetry_topic(
              client,
              NULL,
              thermostat_component->component_name,
              mqtt_message->topic,
              mqtt_message->topic_length,
              NULL)))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get pnp Telemetry topic: az_result return code 0x%08x.", rc);
    return rc;
  }

  if (az_result_failed(
          rc = build_telemetry_message(
              thermostat_component, mqtt_message->payload_span, &mqtt_message->out_payload_span)))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to build telemetry payload: az_result return code 0x%08x.", rc);
    return rc;
  }

  return rc;
}

bool pnp_thermostat_get_max_temp_report(
    const az_iot_hub_client* client,
    pnp_thermostat_component* thermostat_component,
    pnp_mqtt_message* mqtt_message)
{
  az_result rc;

  if (!thermostat_component->send_max_temp_property)
  {
    return false;
  }

  if (az_result_failed(
          rc = pnp_create_reported_property(
              mqtt_message->payload_span,
              thermostat_component->component_name,
              max_temp_reported_property_name,
              append_double,
              &thermostat_component->max_temperature,
              &mqtt_message->out_payload_span)))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get reported property: az_result return code 0x%08x.", rc);
    return false;
  }
  else if (az_result_failed(
               rc = az_iot_hub_client_twin_patch_get_publish_topic(
                   client,
                   get_request_id(),
                   mqtt_message->topic,
                   mqtt_message->topic_length,
                   NULL)))
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to get reported property topic with status: az_result return code 0x%08x.", rc);
    return false;
  }

  thermostat_component->send_max_temp_property = false;

  return true;
}

az_result pnp_thermostat_process_property_update(
    const az_iot_hub_client* client,
    pnp_thermostat_component* thermostat_component,
    az_span component_name,
    const az_json_token* property_name,
    const az_json_reader* property_value,
    int32_t version,
    pnp_mqtt_message* mqtt_message)
{
  az_result result;

  if (!az_span_is_content_equal(thermostat_component->component_name, component_name))
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  if (!az_json_token_is_text_equal(property_name, desired_temp_property_name))
  {
    IOT_SAMPLE_LOG_AZ_SPAN(
        "PnP property is not supported on thermostat component:", property_name->slice);
  }

  double parsed_value = 0;
  if (az_result_failed(az_json_token_get_double(&(property_value->token), &parsed_value)))
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

    if (az_result_failed(
            result = pnp_create_reported_property_with_status(
                mqtt_message->payload_span,
                component_name,
                property_name->slice,
                append_double,
                (void*)&parsed_value,
                200,
                version,
                temp_response_description_success,
                &mqtt_message->out_payload_span)))
    {
      IOT_SAMPLE_LOG_ERROR(
          "Failed to get reported property payload with status: az_result return code 0x%08x.",
          result);
    }
  }

  if (az_result_failed(
          result = az_iot_hub_client_twin_patch_get_publish_topic(
              client, get_request_id(), mqtt_message->topic, mqtt_message->topic_length, NULL)))
  {
    IOT_SAMPLE_LOG_ERROR(
        "Failed to get reported property topic with status: az_result return code 0x%08x.", result);
  }

  return result;
}

az_result pnp_thermostat_process_command(
    const az_iot_hub_client* client,
    const pnp_thermostat_component* thermostat_component,
    const az_iot_hub_client_method_request* command_request,
    az_span component_name,
    az_span command_name,
    az_span command_payload,
    pnp_mqtt_message* mqtt_message,
    az_iot_status* status)
{
  az_result result;

  if (az_span_is_content_equal(thermostat_component->component_name, component_name)
      && az_span_is_content_equal(report_command_name_span, command_name))
  {
    // Invoke command
    az_result response = invoke_getMaxMinReport(
        thermostat_component,
        command_payload,
        mqtt_message->payload_span,
        &mqtt_message->out_payload_span);
    if (az_result_failed(response)
    {
      *status = AZ_IOT_STATUS_BAD_REQUEST;
    }
    else
    {
      *status = AZ_IOT_STATUS_OK;
    }

    if (az_result_failed(
            result = az_iot_hub_client_methods_response_get_publish_topic(
                client,
                command_request->request_id,
                (uint16_t)*status,
                mqtt_message->topic,
                mqtt_message->topic_length,
                mqtt_message->out_topic_length)))
    {
      IOT_SAMPLE_LOG_ERROR(
          "Failed to get methods response publish topic: az_result return code 0x%08x.", result);
      return result;
    }
  }
  else
  {
    // Unsupported command or not this component's command
    result = AZ_ERROR_ITEM_NOT_FOUND;
  }

  return result;
}

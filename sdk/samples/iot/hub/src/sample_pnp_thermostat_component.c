// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include "pnp_helper.h"
#include "sample_pnp_thermostat_component.h"

#ifdef _MSC_VER
// "'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead."
#pragma warning(disable : 4996)
#endif

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
    sample_pnp_thermostat_component* handle,
    az_json_writer* json_builder,
    az_span start_time_span,
    az_span end_time_span,
    az_span* response_payload)
{
  double avg_temp = handle->device_temperature_avg_total / handle->device_temperature_avg_count;
  // Build the command response payload
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_builder));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_max_temp_name_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_double(
      json_builder, handle->max_temperature, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_min_temp_name_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_double(
      json_builder, handle->min_temperature, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_avg_temp_name_span));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_double(json_builder, avg_temp, DOUBLE_DECIMAL_PLACE_DIGITS));
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
static az_result invoke_getMaxMinReport(
    sample_pnp_thermostat_component* handle,
    az_span payload,
    az_span response,
    az_span* out_response)
{
  // az_result result;
  // Parse the "since" field in the payload.
  az_span start_time_span = AZ_SPAN_NULL;
  az_json_reader jp;
  AZ_RETURN_IF_FAILED(az_json_reader_init(&jp, payload, NULL));
  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jp));
  int32_t incoming_since_value_buffer_len;
  AZ_RETURN_IF_FAILED(az_json_token_get_string(
      &jp.token,
      incoming_since_value_buffer,
      sizeof(incoming_since_value_buffer),
      &incoming_since_value_buffer_len));
  start_time_span
      = az_span_init((uint8_t*)incoming_since_value_buffer, incoming_since_value_buffer_len);

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
  az_span end_time_span = az_span_init((uint8_t*)end_time_buffer, (int32_t)len);

  az_json_writer json_builder;
  AZ_RETURN_IF_FAILED(az_json_writer_init(&json_builder, response, NULL));
  AZ_RETURN_IF_FAILED(build_command_response_payload(
      handle, &json_builder, start_time_span, end_time_span, out_response));

  return AZ_OK;
}

static az_result build_telemetry_message(
    sample_pnp_thermostat_component* handle,
    az_span payload,
    az_span* out_payload)
{
  az_json_writer json_builder;
  AZ_RETURN_IF_FAILED(az_json_writer_init(&json_builder, payload, NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&json_builder));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&json_builder, telemetry_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_double(
      &json_builder, handle->current_temperature, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&json_builder));
  *out_payload = az_json_writer_get_bytes_used_in_destination(&json_builder);

  return AZ_OK;
}

static az_result append_double(az_json_writer* json_writer, void* context)
{
  double value = *(double*)context;

  return az_json_writer_append_double(json_writer, value, DOUBLE_DECIMAL_PLACE_DIGITS);
}

az_result sample_pnp_thermostat_init(
    sample_pnp_thermostat_component* handle,
    az_span component_name,
    double initial_temp)
{
  if (handle == NULL)
  {
    return AZ_ERROR_ARG;
  }

  handle->component_name = component_name;
  handle->current_temperature = initial_temp;
  handle->min_temperature = initial_temp;
  handle->max_temperature = initial_temp;
  handle->device_temperature_avg_count = 1;
  handle->device_temperature_avg_total = initial_temp;
  handle->avg_temperature = initial_temp;
  handle->send_max_temp_property = true;

  return AZ_OK;
}

bool sample_pnp_thermostat_get_max_temp_report(
    az_iot_hub_client* client,
    sample_pnp_thermostat_component* handle,
    sample_pnp_mqtt_message* mqtt_message)
{
  az_result result;
  if (handle->send_max_temp_property)
  {
    if ((result = pnp_helper_create_reported_property(
             mqtt_message->payload_span,
             handle->component_name,
             max_temp_reported_property_name,
             append_double,
             &handle->max_temperature,
             &mqtt_message->out_payload_span))
        != AZ_OK)
    {
      printf("Could not get reported property: error code = 0x%08x\n", result);
      return false;
    }
    else if (
        (result = az_iot_hub_client_twin_patch_get_publish_topic(
             client, get_request_id(), mqtt_message->topic, mqtt_message->topic_length, NULL))
        != AZ_OK)
    {
      printf("Error to get reported property topic with status: error code = 0x%08x\n", result);
      return false;
    }

    handle->send_max_temp_property = false;

    return true;
  }
  else
  {
    return false;
  }
}

az_result sample_pnp_thermostat_get_telemetry_message(
    az_iot_hub_client* client,
    sample_pnp_thermostat_component* handle,
    sample_pnp_mqtt_message* mqtt_message)
{
  az_result result;
  if (az_failed(
          result = pnp_helper_get_telemetry_topic(
              client,
              NULL,
              handle->component_name,
              mqtt_message->topic,
              mqtt_message->topic_length,
              NULL)))
  {
    return result;
  }

  if (az_failed(
          result = build_telemetry_message(
              handle, mqtt_message->payload_span, &mqtt_message->out_payload_span)))
  {
    printf("Could not build telemetry payload: error code = 0x%08x\n", result);
    return result;
  }

  return result;
}

az_result sample_pnp_thermostat_process_property_update(
    az_iot_hub_client* client,
    sample_pnp_thermostat_component* handle,
    az_span component_name,
    az_span property_name,
    az_json_token* property_value,
    int32_t version,
    sample_pnp_mqtt_message* mqtt_message)
{
  az_result result;

  if (!az_span_is_content_equal(handle->component_name, component_name))
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  if (!az_span_is_content_equal(desired_temp_property_name, property_name))
  {
    printf(
        "PnP property=%.*s is not supported on thermostat component\r\n",
        az_span_size(property_name),
        az_span_ptr(property_name));
  }

  double parsed_value = 0;
  if (az_failed(az_json_token_get_double(property_value, &parsed_value)))
  {
    result = pnp_helper_create_reported_property_with_status(
        mqtt_message->payload_span,
        component_name,
        property_name,
        append_double,
        (void*)&parsed_value,
        401,
        version,
        temp_response_description_failed,
        &mqtt_message->out_payload_span);
  }
  else
  {
    handle->current_temperature = parsed_value;
    if (handle->current_temperature > handle->max_temperature)
    {
      handle->max_temperature = handle->current_temperature;
      handle->send_max_temp_property = true;
    }

    if (handle->current_temperature < handle->min_temperature)
    {
      handle->min_temperature = handle->current_temperature;
    }

    /* Increment the avg count, add the new temp to the total, and calculate the new avg */
    handle->device_temperature_avg_count++;
    handle->device_temperature_avg_total += handle->current_temperature;
    handle->avg_temperature
        = handle->device_temperature_avg_total / handle->device_temperature_avg_count;

    if ((result = pnp_helper_create_reported_property_with_status(
             mqtt_message->payload_span,
             component_name,
             property_name,
             append_double,
             (void*)&parsed_value,
             200,
             version,
             temp_response_description_success,
             &mqtt_message->out_payload_span))
        != AZ_OK)
    {
      printf("Error to get reported property payload with status: error code = 0x%08x", result);
    }
  }

  if ((result = az_iot_hub_client_twin_patch_get_publish_topic(
           client, get_request_id(), mqtt_message->topic, mqtt_message->topic_length, NULL))
      != AZ_OK)
  {
    printf("Error to get reported property topic with status: error code = 0x%08x", result);
  }

  return result;
}

az_result sample_pnp_thermostat_process_command(
    az_iot_hub_client* client,
    sample_pnp_thermostat_component* handle,
    az_iot_hub_client_method_request* command_request,
    az_span component_name,
    az_span command_name,
    az_span command_payload,
    sample_pnp_mqtt_message* mqtt_message)
{
  az_result result;

  if (az_span_is_content_equal(handle->component_name, component_name)
      && az_span_is_content_equal(report_command_name_span, command_name))
  {
    // Invoke command
    uint16_t return_code;
    az_result response = invoke_getMaxMinReport(
        handle, command_payload, mqtt_message->payload_span, &mqtt_message->out_payload_span);
    if (response != AZ_OK)
    {
      return_code = 400;
    }
    else
    {
      return_code = 200;
    }

    if (az_failed(
            result = az_iot_hub_client_methods_response_get_publish_topic(
                client,
                command_request->request_id,
                return_code,
                mqtt_message->topic,
                mqtt_message->topic_length,
                mqtt_message->out_topic_length)))
    {
      printf("Unable to get twin document publish topic\n");
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

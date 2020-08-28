// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "iot_sample_common.h"

#include "sample_pnp.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#define JSON_DOUBLE_DIGITS 2

// Property values
static char pnp_properties_buffer[64];

// Device twin values
static const az_span component_telemetry_prop_span = AZ_SPAN_LITERAL_FROM_STR("$.sub");
static const az_span desired_temp_response_value_name = AZ_SPAN_LITERAL_FROM_STR("value");
static const az_span desired_temp_ack_code_name = AZ_SPAN_LITERAL_FROM_STR("ac");
static const az_span desired_temp_ack_version_name = AZ_SPAN_LITERAL_FROM_STR("av");
static const az_span desired_temp_ack_description_name = AZ_SPAN_LITERAL_FROM_STR("ad");
static const az_span component_specifier_name = AZ_SPAN_LITERAL_FROM_STR("__t");
static const az_span component_specifier_value = AZ_SPAN_LITERAL_FROM_STR("c");
static const az_span command_separator = AZ_SPAN_LITERAL_FROM_STR("/");
static const az_span iot_hub_twin_desired_version = AZ_SPAN_LITERAL_FROM_STR("$version");
static const az_span iot_hub_twin_desired = AZ_SPAN_LITERAL_FROM_STR("desired");

// Visit each valid property for the component
static az_result visit_component_properties(
    az_span component_name,
    az_json_reader* jr,
    int32_t version,
    pnp_property_callback property_callback,
    void* context_ptr)
{
  while (az_result_succeeded(az_json_reader_next_token(jr)))
  {
    if (jr->token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
    {
      if (az_json_token_is_text_equal(&(jr->token), component_specifier_name)
          || az_json_token_is_text_equal(&(jr->token), iot_hub_twin_desired_version))
      {
        if (az_result_failed(az_json_reader_next_token(jr)))
        {
          IOT_SAMPLE_LOG_ERROR("Failed to get next token.");
          return AZ_ERROR_UNEXPECTED_CHAR;
        }
        continue;
      }

      az_json_token property_name = jr->token;

      if (az_result_failed(az_json_reader_next_token(jr)))
      {
        IOT_SAMPLE_LOG_ERROR("Failed to get next token.");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

      property_callback(component_name, &property_name, *jr, version, context_ptr);
    }

    if (jr->token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (az_result_failed(az_json_reader_skip_children(jr)))
      {
        IOT_SAMPLE_LOG_ERROR("Failed to skip children of object.");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
    }
    else if (jr->token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      break;
    }
  }

  return AZ_OK;
}

// Move reader to the value of property name
static az_result json_child_token_move(az_json_reader* jr, az_span property_name)
{
  while (az_result_succeeded(az_json_reader_next_token(jr)))
  {
    if ((jr->token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
        && az_json_token_is_text_equal(&(jr->token), property_name))
    {
      if (az_result_failed(az_json_reader_next_token(jr)))
      {
        IOT_SAMPLE_LOG_ERROR("Failed to get next token.");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

      return AZ_OK;
    }
    else if (jr->token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (az_result_failed(az_json_reader_skip_children(jr)))
      {
        IOT_SAMPLE_LOG_ERROR("Failed to skip child of complex object.");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
    }
    else if (jr->token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
  }

  return AZ_ERROR_ITEM_NOT_FOUND;
}

// Check if the component name is in the model
static az_result is_component_in_model(
    az_span component_name,
    const az_span** components_ptr,
    int32_t components_num,
    int32_t* out_index)
{
  int32_t index = 0;

  if (az_span_ptr(component_name) == NULL || az_span_size(component_name) == 0)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  while (index < components_num)
  {
    if (az_span_is_content_equal(component_name, *components_ptr[index]))
    {
      *out_index = index;
      return AZ_OK;
    }

    index++;
  }

  return AZ_ERROR_UNEXPECTED_CHAR;
}

// Get the telemetry topic for PnP
az_result pnp_get_telemetry_topic(
    const az_iot_hub_client* client,
    az_iot_message_properties* properties,
    az_span component_name,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  az_iot_message_properties pnp_properties;

  if (az_span_ptr(component_name) != NULL)
  {
    if (properties == NULL)
    {
      properties = &pnp_properties;

      AZ_RETURN_IF_FAILED(az_iot_message_properties_init(
          properties, AZ_SPAN_FROM_BUFFER(pnp_properties_buffer), 0));
    }

    AZ_RETURN_IF_FAILED(az_iot_message_properties_append(
        properties, component_telemetry_prop_span, component_name));
  }

  AZ_RETURN_IF_FAILED(az_iot_hub_client_telemetry_get_publish_topic(
      client,
      az_span_ptr(component_name) != NULL ? properties : NULL,
      mqtt_topic,
      mqtt_topic_size,
      out_mqtt_topic_length));

  return AZ_OK;
}

// Parse the component name and command name from a span
void pnp_parse_command_name(
    az_span component_command,
    az_span* component_name,
    az_span* pnp_command_name)
{
  int32_t index = az_span_find(component_command, command_separator);
  if (index > 0)
  {
    *component_name = az_span_slice(component_command, 0, index);
    *pnp_command_name
        = az_span_slice(component_command, index + 1, az_span_size(component_command));
  }
  else
  {
    *component_name = AZ_SPAN_NULL;
    *pnp_command_name = component_command;
  }
}

// Create a reported property payload
az_result pnp_create_reported_property(
    az_span json_buffer,
    az_span component_name,
    az_span property_name,
    pnp_append_property_callback append_callback,
    void* context,
    az_span* out_span)
{
  az_json_writer jw;
  AZ_RETURN_IF_FAILED(az_json_writer_init(&jw, json_buffer, NULL));

  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));

  if (az_span_ptr(component_name) != NULL)
  {
    AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, component_name));
    AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
    AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, component_specifier_name));
    AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, component_specifier_value));
  }

  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, property_name));
  AZ_RETURN_IF_FAILED(append_callback(&jw, context));

  if (az_span_ptr(component_name) != NULL)
  {
    AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));
  }

  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));

  *out_span = az_json_writer_get_bytes_used_in_destination(&jw);

  return AZ_OK;
}

// Create a reported property payload with status
az_result pnp_create_reported_property_with_status(
    az_span json_buffer,
    az_span component_name,
    az_span property_name,
    pnp_append_property_callback append_callback,
    void* context,
    int32_t ack_code,
    int32_t ack_version,
    az_span ack_description,
    az_span* out_span)
{
  az_json_writer jw;

  AZ_RETURN_IF_FAILED(az_json_writer_init(&jw, json_buffer, NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  if (az_span_ptr(component_name) != NULL)
  {
    AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, component_name));
    AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
    AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, component_specifier_name));
    AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, component_specifier_value));
  }

  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, property_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, desired_temp_response_value_name));
  AZ_RETURN_IF_FAILED(append_callback(&jw, context));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, desired_temp_ack_code_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(&jw, ack_code));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, desired_temp_ack_version_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(&jw, ack_version));

  if (az_span_ptr(ack_description) != NULL)
  {
    AZ_RETURN_IF_FAILED(
        az_json_writer_append_property_name(&jw, desired_temp_ack_description_name));
    AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, ack_description));
  }

  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));

  if (az_span_ptr(component_name) != NULL)
  {
    AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));
  }

  *out_span = az_json_writer_get_bytes_used_in_destination(&jw);

  return AZ_OK;
}

// Process the twin properties and invoke user callback for each property
az_result pnp_process_device_twin_message(
    az_span twin_message_span,
    bool is_partial,
    const az_span** components_ptr,
    int32_t components_num,
    pnp_property_callback property_callback,
    void* context_ptr)
{
  az_json_reader jr;
  az_json_reader copy_jr;
  int32_t version;
  int32_t index;

  AZ_RETURN_IF_FAILED(az_json_reader_init(&jr, twin_message_span, NULL));
  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

  if (!is_partial && az_result_failed(json_child_token_move(&jr, iot_hub_twin_desired)))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get desired property.");
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  copy_jr = jr;
  if (az_result_failed(json_child_token_move(&copy_jr, iot_hub_twin_desired_version))
      || az_result_failed(az_json_token_get_int32(&(copy_jr.token), (int32_t*)&version)))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get version.");
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  az_json_token property_name;

  while (az_result_succeeded(az_json_reader_next_token(&jr)))
  {
    if (jr.token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
    {
      if (az_json_token_is_text_equal(&(jr.token), iot_hub_twin_desired_version))
      {
        if (az_result_failed(az_json_reader_next_token(&jr)))
        {
          IOT_SAMPLE_LOG_ERROR("Failed to get next token.");
          return AZ_ERROR_UNEXPECTED_CHAR;
        }
        continue;
      }

      property_name = jr.token;

      if (az_result_failed(az_json_reader_next_token(&jr)))
      {
        IOT_SAMPLE_LOG_ERROR("Failed to get next token.");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

      if (jr.token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT && components_ptr != NULL
          && (az_result_succeeded(
              is_component_in_model(property_name.slice, components_ptr, components_num, &index))))
      {
        if (az_result_failed(visit_component_properties(
                *components_ptr[index], &jr, version, property_callback, context_ptr)))
        {
          IOT_SAMPLE_LOG_ERROR("Failed to visit component properties.");
          return AZ_ERROR_UNEXPECTED_CHAR;
        }
      }
      else
      {
        property_callback(AZ_SPAN_NULL, &property_name, jr, version, context_ptr);
      }
    }
    else if (jr.token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (az_result_failed(az_json_reader_skip_children(&jr)))
      {
        IOT_SAMPLE_LOG_ERROR("Failed to skip children of object.");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
    }
    else if (jr.token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      break;
    }
  }

  return AZ_OK;
}

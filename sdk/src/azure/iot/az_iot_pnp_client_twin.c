// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/iot/az_iot_pnp_client.h>

#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

static const az_span iot_hub_twin_desired = AZ_SPAN_LITERAL_FROM_STR("desired");
static const az_span iot_hub_twin_desired_version = AZ_SPAN_LITERAL_FROM_STR("$version");
static const az_span property_response_value_name = AZ_SPAN_LITERAL_FROM_STR("value");
static const az_span property_ack_code_name = AZ_SPAN_LITERAL_FROM_STR("ac");
static const az_span property_ack_version_name = AZ_SPAN_LITERAL_FROM_STR("av");
static const az_span property_ack_description_name = AZ_SPAN_LITERAL_FROM_STR("ad");
static const az_span component_property_label_name = AZ_SPAN_LITERAL_FROM_STR("__t");
static const az_span component_property_label_value = AZ_SPAN_LITERAL_FROM_STR("c");

AZ_NODISCARD az_result az_iot_pnp_client_twin_parse_received_topic(
    az_iot_pnp_client const* client,
    az_span received_topic,
    az_iot_pnp_client_twin_response* out_twin_response)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _az_PRECONDITION_NOT_NULL(out_twin_response);

  az_iot_hub_client_twin_response hub_twin_response;
  _az_RETURN_IF_FAILED(az_iot_hub_client_twin_parse_received_topic(
      &client->_internal.iot_hub_client, received_topic, &hub_twin_response));

  out_twin_response->request_id = hub_twin_response.request_id;
  out_twin_response->response_type = hub_twin_response.response_type;
  out_twin_response->status = hub_twin_response.status;
  out_twin_response->version = hub_twin_response.version;

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_pnp_client_twin_property_begin_component(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer,
    az_span component_name)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(json_writer);
  _az_PRECONDITION_VALID_SPAN(component_name, 1, false);

  (void)client;

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, component_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_writer));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_writer, component_property_label_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_string(json_writer, component_property_label_value));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_pnp_client_twin_property_end_component(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(json_writer);

  (void)client;

  return az_json_writer_append_end_object(json_writer);
}

AZ_NODISCARD az_result az_iot_pnp_client_twin_begin_property_with_status(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer,
    az_span component_name,
    az_span property_name,
    int32_t ack_code,
    int32_t ack_version,
    az_span ack_description)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(json_writer);
  _az_PRECONDITION_VALID_SPAN(property_name, 1, false);

  (void)client;

  if (az_span_size(component_name) != 0)
  {
    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, component_name));
    _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_writer));
    _az_RETURN_IF_FAILED(
        az_json_writer_append_property_name(json_writer, component_property_label_name));
    _az_RETURN_IF_FAILED(az_json_writer_append_string(json_writer, component_property_label_value));
  }

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, property_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_writer));
  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, property_ack_code_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_int32(json_writer, ack_code));
  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, property_ack_version_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_int32(json_writer, ack_version));

  if (az_span_size(ack_description) != 0)
  {
    _az_RETURN_IF_FAILED(
        az_json_writer_append_property_name(json_writer, property_ack_description_name));
    _az_RETURN_IF_FAILED(az_json_writer_append_string(json_writer, ack_description));
  }

  _az_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_writer, property_response_value_name));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_pnp_client_twin_end_property_with_status(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer,
    az_span component_name)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(json_writer);

  (void)client;

  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(json_writer));

  if (az_span_size(component_name) != 0)
  {
    _az_RETURN_IF_FAILED(az_json_writer_append_end_object(json_writer));
  }

  return AZ_OK;
}

// Move reader to the value of property name
static az_result json_child_token_move(az_json_reader* jr, az_span property_name)
{
  do
  {
    if ((jr->token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
        && az_json_token_is_text_equal(&(jr->token), property_name))
    {
      if (az_result_failed(az_json_reader_next_token(jr)))
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

      return AZ_OK;
    }
    else if (jr->token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (az_result_failed(az_json_reader_skip_children(jr)))
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
    }
    else if (jr->token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
  } while (az_result_succeeded(az_json_reader_next_token(jr)));

  return AZ_ERROR_ITEM_NOT_FOUND;
}

// Check if the component name is in the model
static az_result is_component_in_model(
    az_iot_pnp_client const* client,
    az_json_token* component_name,
    az_span* out_component_name)
{
  int32_t index = 0;

  while (index < client->_internal.options.component_names_length)
  {
    if (az_json_token_is_text_equal(
            component_name, client->_internal.options.component_names[index]))
    {
      *out_component_name = client->_internal.options.component_names[index];
      return AZ_OK;
    }

    index++;
  }

  return AZ_ERROR_UNEXPECTED_CHAR;
}

AZ_NODISCARD az_result az_iot_pnp_client_twin_get_property_version(
    az_iot_pnp_client const* client,
    az_json_reader* json_reader,
    bool is_partial,
    int32_t* out_version)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(json_reader);

  (void)client;

  az_json_reader copy_json_reader = *json_reader;

  if ((az_result_failed(az_json_reader_next_token(&copy_json_reader)))
      || (copy_json_reader.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
      || (az_result_failed(az_json_reader_next_token(&copy_json_reader))))
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  if (!is_partial
      && (az_result_failed(json_child_token_move(&copy_json_reader, iot_hub_twin_desired))
          || (az_result_failed(az_json_reader_next_token(&copy_json_reader)))))
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  if (az_result_failed(json_child_token_move(&copy_json_reader, iot_hub_twin_desired_version))
      || az_result_failed(az_json_token_get_int32(&copy_json_reader.token, out_version)))
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  return AZ_OK;
}

static az_result check_if_skipable(az_json_reader* jr, bool is_partial)
{
  // First time move
  if (jr->_internal.bit_stack._internal.current_depth == 0)
  {
    if ((az_result_failed(az_json_reader_next_token(jr)))
        || (jr->token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
        || (az_result_failed(az_json_reader_next_token(jr))))
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }

    if (!is_partial
        && (az_result_failed(json_child_token_move(jr, iot_hub_twin_desired))
            || (az_result_failed(az_json_reader_next_token(jr)))))
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
  }
  while (1)
  {
    if ((is_partial && jr->_internal.bit_stack._internal.current_depth == 1)
        || (!is_partial && jr->_internal.bit_stack._internal.current_depth == 2))
    {
      if ((az_json_token_is_text_equal(&jr->token, iot_hub_twin_desired_version)))
      {
        if ((az_result_failed(az_json_reader_next_token(jr)))
            || (az_result_failed(az_json_reader_next_token(jr))))
        {
          return AZ_ERROR_UNEXPECTED_CHAR;
        }
        continue;
      }
      else
      {
        return AZ_OK;
      }
    }
    else if (
        (is_partial && jr->_internal.bit_stack._internal.current_depth == 2)
        || (!is_partial && jr->_internal.bit_stack._internal.current_depth == 3))
    {
      if (az_json_token_is_text_equal(&jr->token, component_property_label_name))
      {
        if (az_result_failed(az_json_reader_next_token(jr))
            || az_result_failed(az_json_reader_next_token(jr)))
        {
          return AZ_ERROR_UNEXPECTED_CHAR;
        }
        continue;
      }
      else
      {
        return AZ_OK;
      }
    }
    else
    {
      return AZ_OK;
    }
  }
}

AZ_NODISCARD az_result az_iot_pnp_client_twin_get_next_component_property(
    az_iot_pnp_client const* client,
    az_json_reader* json_reader,
    bool is_partial,
    az_span* out_component_name,
    az_json_token* out_property_name,
    az_json_reader* out_property_value)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(json_reader);
  _az_PRECONDITION_NOT_NULL(out_property_name);
  _az_PRECONDITION_NOT_NULL(out_property_value);

  (void)client;

  while (1)
  {
    if (az_result_failed(check_if_skipable(json_reader, is_partial)))
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }

    if ((json_reader->token.kind == AZ_JSON_TOKEN_END_OBJECT))
    {
      if ((is_partial && json_reader->_internal.bit_stack._internal.current_depth == 0)
          || (!is_partial && json_reader->_internal.bit_stack._internal.current_depth == 1))
      {
        return AZ_IOT_END_OF_PROPERTIES;
      }

      if (az_result_failed(az_json_reader_next_token(json_reader)))
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      // Continue loop if at the end of the component
      continue;
    }

    break;
  }

  // Check if in component depth
  if ((is_partial && json_reader->_internal.bit_stack._internal.current_depth == 1)
      || (!is_partial && json_reader->_internal.bit_stack._internal.current_depth == 2))
  {
    if (az_result_succeeded(is_component_in_model(client, &json_reader->token, out_component_name)))
    {
      if (az_result_failed(az_json_reader_next_token(json_reader))
          || (json_reader->token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
          || (az_result_failed(az_json_reader_next_token(json_reader))))
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      if (az_result_failed(check_if_skipable(json_reader, is_partial)))
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
    }
    else
    {
      *out_component_name = AZ_SPAN_EMPTY;
    }
  }

  *out_property_name = json_reader->token;

  if (az_result_failed(az_json_reader_next_token(json_reader)))
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  *out_property_value = *json_reader;

  if (az_result_failed(az_json_reader_skip_children(json_reader))
      || az_result_failed(az_json_reader_next_token(json_reader)))
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  return AZ_OK;
}

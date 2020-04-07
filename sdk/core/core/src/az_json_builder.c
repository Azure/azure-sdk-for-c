// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_hex_private.h"
#include "az_json_string_private.h"
#include "az_span_private.h"
#include <az_json.h>

#include <_az_cfg.h>

AZ_NODISCARD static az_result az_json_builder_append_str(az_json_builder* self, az_span value)
{
  AZ_PRECONDITION_NOT_NULL(self);

  az_span* json = &self->_internal.json;

  int32_t required_length = az_span_length(value) + 2;

  AZ_RETURN_IF_SPAN_CAPACITY_TOO_SMALL(*json, required_length);

  *json = az_span_append_uint8(*json, '"');
  *json = az_span_append(*json, value);
  *json = az_span_append_uint8(*json, '"');
  return AZ_OK;
}

static AZ_NODISCARD az_result _az_json_builder_write_span(az_json_builder* self, az_span value)
{
  AZ_PRECONDITION_NOT_NULL(self);

  az_span* json = &self->_internal.json;

  AZ_RETURN_IF_SPAN_CAPACITY_TOO_SMALL(*json, 1);
  *json = az_span_append_uint8(*json, '"');

  for (int32_t i = 0; i < az_span_length(value); ++i)
  {
    uint8_t const c = az_span_ptr(value)[i];

    // check if the character has to be escaped.
    {
      az_span const esc = _az_json_esc_encode(c);
      int32_t escaped_length = az_span_length(esc);
      if (escaped_length)
      {
        AZ_RETURN_IF_SPAN_CAPACITY_TOO_SMALL(*json, escaped_length);
        *json = az_span_append(*json, esc);
        continue;
      }
    }
    // check if the character has to be escaped as a UNICODE escape sequence.
    if (c < 0x20)
    {
      uint8_t array[6] = {
        '\\',
        'u',
        '0',
        '0',
        _az_number_to_upper_hex((uint8_t)(c / 16)),
        _az_number_to_upper_hex((uint8_t)(c % 16)),
      };

      AZ_RETURN_IF_SPAN_CAPACITY_TOO_SMALL(*json, sizeof(array));
      *json = az_span_append(*json, AZ_SPAN_FROM_INITIALIZED_BUFFER(array));
      continue;
    }

    AZ_RETURN_IF_SPAN_CAPACITY_TOO_SMALL(*json, 1);
    *json = az_span_append_uint8(*json, az_span_ptr(value)[i]);
  }
  AZ_RETURN_IF_SPAN_CAPACITY_TOO_SMALL(*json, 1);
  *json = az_span_append_uint8(*json, '"');

  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_builder_append_token(az_json_builder* json_builder, az_json_token token)
{
  AZ_PRECONDITION_NOT_NULL(json_builder);
  az_span* json = &json_builder->_internal.json;

  AZ_RETURN_IF_SPAN_CAPACITY_TOO_SMALL(*json, 1);

  switch (token.kind)
  {
    case AZ_JSON_TOKEN_NULL:
    {
      AZ_RETURN_IF_SPAN_CAPACITY_TOO_SMALL(*json, 4);
      json_builder->_internal.need_comma = true;
      *json = az_span_append(*json, AZ_SPAN_FROM_STR("null"));
      break;
    }
    case AZ_JSON_TOKEN_BOOLEAN:
    {
      az_span boolean_literal_string
          = token._internal.boolean ? AZ_SPAN_FROM_STR("true") : AZ_SPAN_FROM_STR("false");
      AZ_RETURN_IF_SPAN_CAPACITY_TOO_SMALL(*json, az_span_length(boolean_literal_string));
      json_builder->_internal.need_comma = true;
      *json = az_span_append(*json, boolean_literal_string);
      break;
    }
    case AZ_JSON_TOKEN_NUMBER:
    {
      json_builder->_internal.need_comma = true;
      return az_span_append_dtoa(*json, token._internal.number, json);
    }
    case AZ_JSON_TOKEN_STRING:
    {
      json_builder->_internal.need_comma = true;
      return az_json_builder_append_str(json_builder, token._internal.string);
    }
    case AZ_JSON_TOKEN_OBJECT:
    {
      AZ_RETURN_IF_SPAN_CAPACITY_TOO_SMALL(*json, az_span_length(token._internal.span));
      json_builder->_internal.need_comma = true;
      *json = az_span_append(*json, token._internal.span);
      break;
    }
    case AZ_JSON_TOKEN_OBJECT_START:
    {
      json_builder->_internal.need_comma = false;
      *json = az_span_append_uint8(*json, '{');
      break;
    }
    case AZ_JSON_TOKEN_OBJECT_END:
    {
      json_builder->_internal.need_comma = true;
      *json = az_span_append_uint8(*json, '}');
      break;
    }
    case AZ_JSON_TOKEN_ARRAY_START:
    {
      json_builder->_internal.need_comma = false;
      *json = az_span_append_uint8(*json, '[');
      break;
    }
    case AZ_JSON_TOKEN_ARRAY_END:
    {
      json_builder->_internal.need_comma = true;
      *json = az_span_append_uint8(*json, ']');
      break;
    }
    case AZ_JSON_TOKEN_SPAN:
    {
      json_builder->_internal.need_comma = true;
      return _az_json_builder_write_span(json_builder, token._internal.span);
    }
    default:
    {
      return AZ_ERROR_ARG;
    }
  }

  return AZ_OK;
}

AZ_NODISCARD static az_result az_json_builder_write_comma(az_json_builder* self)
{
  AZ_PRECONDITION_NOT_NULL(self);

  if (self->_internal.need_comma)
  {
    AZ_RETURN_IF_SPAN_CAPACITY_TOO_SMALL(self->_internal.json, 1);
    self->_internal.json = az_span_append_uint8(self->_internal.json, ',');
  }
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_builder_append_object(az_json_builder* json_builder, az_span name, az_json_token token)
{
  AZ_PRECONDITION_NOT_NULL(json_builder);

  AZ_RETURN_IF_FAILED(az_json_builder_write_comma(json_builder));
  AZ_RETURN_IF_FAILED(az_json_builder_append_str(json_builder, name));

  AZ_RETURN_IF_SPAN_CAPACITY_TOO_SMALL(json_builder->_internal.json, 1);
  json_builder->_internal.json = az_span_append_uint8(json_builder->_internal.json, ':');

  AZ_RETURN_IF_FAILED(az_json_builder_append_token(json_builder, token));
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_builder_append_array_item(az_json_builder* json_builder, az_json_token token)
{
  AZ_PRECONDITION_NOT_NULL(json_builder);

  AZ_RETURN_IF_FAILED(az_json_builder_write_comma(json_builder));
  AZ_RETURN_IF_FAILED(az_json_builder_append_token(json_builder, token));
  return AZ_OK;
}

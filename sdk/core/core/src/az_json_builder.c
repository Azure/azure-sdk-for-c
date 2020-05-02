// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_hex_private.h"
#include "az_json_string_private.h"
#include "az_span_private.h"
#include <az_json.h>
#include <az_span_internal.h>

#include <_az_cfg.h>

static _az_NODISCARD az_span _get_remaining_span(az_json_builder* json_builder)
{
  return az_span_slice_to_end(json_builder->_internal.json, json_builder->_internal.length);
}

_az_NODISCARD static az_result az_json_builder_append_str(az_json_builder* self, az_span value)
{
  _az_PRECONDITION_NOT_NULL(self);

  az_span remaining_json = _get_remaining_span(self);

  int32_t required_length = az_span_size(value) + 2;

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_length);

  remaining_json = az_span_copy_u8(remaining_json, '"');
  remaining_json = az_span_copy(remaining_json, value);
  az_span_copy_u8(remaining_json, '"');

  self->_internal.length += required_length;
  return AZ_OK;
}

static _az_NODISCARD az_result _az_json_builder_write_span(az_json_builder* self, az_span value)
{
  _az_PRECONDITION_NOT_NULL(self);

  az_span remaining_json = _get_remaining_span(self);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, 1);
  remaining_json = az_span_copy_u8(remaining_json, '"');
  self->_internal.length++;

  for (int32_t i = 0; i < az_span_size(value); ++i)
  {
    uint8_t const c = az_span_ptr(value)[i];

    // check if the character has to be escaped.
    {
      az_span const esc = _az_json_esc_encode(c);
      int32_t escaped_length = az_span_size(esc);
      if (escaped_length)
      {
        AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, escaped_length);
        remaining_json = az_span_copy(remaining_json, esc);
        self->_internal.length += escaped_length;
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

      AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, (int32_t)sizeof(array));
      remaining_json = az_span_copy(remaining_json, AZ_SPAN_FROM_BUFFER(array));
      self->_internal.length += (int32_t)sizeof(array);
      continue;
    }

    AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, 1);
    remaining_json = az_span_copy_u8(remaining_json, az_span_ptr(value)[i]);
    self->_internal.length++;
  }
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, 1);
  remaining_json = az_span_copy_u8(remaining_json, '"');
  self->_internal.length++;

  return AZ_OK;
}

_az_NODISCARD az_result
az_json_builder_append_token(az_json_builder* json_builder, az_json_token token)
{
  _az_PRECONDITION_NOT_NULL(json_builder);
  az_span remaining_json = _get_remaining_span(json_builder);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, 1);

  switch (token.kind)
  {
    case AZ_JSON_TOKEN_NULL:
    {
      AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, 4);
      json_builder->_internal.need_comma = true;
      remaining_json = az_span_copy(remaining_json, AZ_SPAN_FROM_STR("null"));
      json_builder->_internal.length += 4;
      break;
    }
    case AZ_JSON_TOKEN_BOOLEAN:
    {
      az_span boolean_literal_string
          = token._internal.boolean ? AZ_SPAN_FROM_STR("true") : AZ_SPAN_FROM_STR("false");
      AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, az_span_size(boolean_literal_string));
      json_builder->_internal.need_comma = true;
      remaining_json = az_span_copy(remaining_json, boolean_literal_string);
      json_builder->_internal.length += az_span_size(boolean_literal_string);
      break;
    }
    case AZ_JSON_TOKEN_NUMBER:
    {
      json_builder->_internal.need_comma = true;
      az_span remainder;
      AZ_RETURN_IF_FAILED(az_span_dtoa(remaining_json, token._internal.number, &remainder));
      json_builder->_internal.length += _az_span_diff(remainder, remaining_json);
      remaining_json = remainder;
      break;
    }
    case AZ_JSON_TOKEN_STRING:
    {
      json_builder->_internal.need_comma = true;
      return az_json_builder_append_str(json_builder, token._internal.string);
    }
    case AZ_JSON_TOKEN_OBJECT:
    {
      AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, az_span_size(token._internal.span));
      json_builder->_internal.need_comma = true;
      remaining_json = az_span_copy(remaining_json, token._internal.span);
      json_builder->_internal.length += az_span_size(token._internal.span);
      break;
    }
    case AZ_JSON_TOKEN_OBJECT_START:
    {
      json_builder->_internal.need_comma = false;
      remaining_json = az_span_copy_u8(remaining_json, '{');
      json_builder->_internal.length++;
      break;
    }
    case AZ_JSON_TOKEN_OBJECT_END:
    {
      json_builder->_internal.need_comma = true;
      remaining_json = az_span_copy_u8(remaining_json, '}');
      json_builder->_internal.length++;
      break;
    }
    case AZ_JSON_TOKEN_ARRAY_START:
    {
      json_builder->_internal.need_comma = false;
      remaining_json = az_span_copy_u8(remaining_json, '[');
      json_builder->_internal.length++;
      break;
    }
    case AZ_JSON_TOKEN_ARRAY_END:
    {
      json_builder->_internal.need_comma = true;
      remaining_json = az_span_copy_u8(remaining_json, ']');
      json_builder->_internal.length++;
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

_az_NODISCARD static az_result az_json_builder_write_comma(az_json_builder* self)
{
  _az_PRECONDITION_NOT_NULL(self);

  if (self->_internal.need_comma)
  {
    az_span remaining_json = _get_remaining_span(self);
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, 1);
    az_span_copy_u8(remaining_json, ',');
    self->_internal.length++;
  }
  return AZ_OK;
}

_az_NODISCARD az_result
az_json_builder_append_object(az_json_builder* json_builder, az_span name, az_json_token token)
{
  _az_PRECONDITION_NOT_NULL(json_builder);

  AZ_RETURN_IF_FAILED(az_json_builder_write_comma(json_builder));
  AZ_RETURN_IF_FAILED(az_json_builder_append_str(json_builder, name));

  az_span remaining_json = _get_remaining_span(json_builder);
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, 1);
  az_span_copy_u8(remaining_json, ':');
  json_builder->_internal.length++;

  AZ_RETURN_IF_FAILED(az_json_builder_append_token(json_builder, token));
  return AZ_OK;
}

_az_NODISCARD az_result
az_json_builder_append_array_item(az_json_builder* json_builder, az_json_token token)
{
  _az_PRECONDITION_NOT_NULL(json_builder);

  AZ_RETURN_IF_FAILED(az_json_builder_write_comma(json_builder));
  AZ_RETURN_IF_FAILED(az_json_builder_append_token(json_builder, token));
  return AZ_OK;
}

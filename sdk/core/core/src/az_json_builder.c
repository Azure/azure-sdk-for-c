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

  AZ_RETURN_IF_FAILED(az_span_append(*json, AZ_SPAN_FROM_STR("\""), json));
  AZ_RETURN_IF_FAILED(az_span_append(*json, value, json));
  AZ_RETURN_IF_FAILED(az_span_append(*json, AZ_SPAN_FROM_STR("\""), json));
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_builder_write_span(az_json_builder* self, az_span value)
{
  AZ_PRECONDITION_NOT_NULL(self);

  az_span* json = &self->_internal.json;

  AZ_RETURN_IF_FAILED(az_span_append(*json, AZ_SPAN_FROM_STR("\""), json));

  for (int32_t i = 0; i < az_span_length(value); ++i)
  {
    uint8_t const c = az_span_ptr(value)[i];

    // check if the character has to be escaped.
    {
      az_span const esc = _az_json_esc_encode(c);
      if (az_span_length(esc))
      {
        AZ_RETURN_IF_FAILED(az_span_append(*json, esc, json));
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
      AZ_RETURN_IF_FAILED(az_span_append(*json, AZ_SPAN_FROM_INITIALIZED_BUFFER(array), json));
      continue;
    }
    AZ_RETURN_IF_FAILED(az_span_append(*json, az_span_init(&az_span_ptr(value)[i], 1, 1), json));
  }
  return az_span_append(*json, AZ_SPAN_FROM_STR("\""), json);
}

AZ_NODISCARD az_result az_json_builder_append_token(az_json_builder* self, az_json_token token)
{
  AZ_PRECONDITION_NOT_NULL(self);
  az_span* json = &self->_internal.json;

  switch (token.kind)
  {
    case AZ_JSON_TOKEN_NULL:
    {
      self->_internal.need_comma = true;
      return az_span_append(*json, AZ_SPAN_FROM_STR("null"), json);
    }
    case AZ_JSON_TOKEN_BOOLEAN:
    {
      self->_internal.need_comma = true;
      return az_span_append(
          *json, token.value.boolean ? AZ_SPAN_FROM_STR("true") : AZ_SPAN_FROM_STR("false"), json);
    }
    case AZ_JSON_TOKEN_STRING:
    {
      self->_internal.need_comma = true;
      return az_json_builder_append_str(self, token.value.string);
    }
    case AZ_JSON_TOKEN_NUMBER:
    {
      self->_internal.need_comma = true;
      return az_span_append_double(*json, token.value.number, json);
    }
    case AZ_JSON_TOKEN_OBJECT:
    {
      self->_internal.need_comma = false;
      return az_span_append(*json, AZ_SPAN_FROM_STR("{"), json);
    }
    case AZ_JSON_TOKEN_ARRAY:
    {
      self->_internal.need_comma = false;
      return az_span_append(*json, AZ_SPAN_FROM_STR("["), json);
    }
    case AZ_JSON_TOKEN_SPAN:
    {
      self->_internal.need_comma = true;
      return az_json_builder_write_span(self, token.value.span);
    }
    default:
    {
      return AZ_ERROR_ARG;
    }
  }
}

AZ_NODISCARD static az_result az_json_builder_write_comma(az_json_builder* self)
{
  AZ_PRECONDITION_NOT_NULL(self);

  if (self->_internal.need_comma)
  {
    return az_span_append(self->_internal.json, AZ_SPAN_FROM_STR(","), &self->_internal.json);
  }
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_builder_append_object(az_json_builder* self, az_span name, az_json_token token)
{
  AZ_PRECONDITION_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_json_builder_write_comma(self));
  AZ_RETURN_IF_FAILED(az_json_builder_append_str(self, name));
  AZ_RETURN_IF_FAILED(
      az_span_append(self->_internal.json, AZ_SPAN_FROM_STR(":"), &self->_internal.json));
  AZ_RETURN_IF_FAILED(az_json_builder_append_token(self, token));
  return AZ_OK;
}

AZ_NODISCARD static az_result az_json_builder_write_close(az_json_builder* self, az_span close)
{
  AZ_PRECONDITION_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_span_append(self->_internal.json, close, &self->_internal.json));
  self->_internal.need_comma = true;
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_builder_append_object_close(az_json_builder* self)
{
  AZ_PRECONDITION_NOT_NULL(self);

  return az_json_builder_write_close(self, AZ_SPAN_FROM_STR("}"));
}

AZ_NODISCARD az_result az_json_builder_append_array_item(az_json_builder* self, az_json_token token)
{
  AZ_PRECONDITION_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_json_builder_write_comma(self));
  AZ_RETURN_IF_FAILED(az_json_builder_append_token(self, token));
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_builder_append_array_close(az_json_builder* self)
{
  AZ_PRECONDITION_NOT_NULL(self);

  return az_json_builder_write_close(self, AZ_SPAN_FROM_STR("]"));
}

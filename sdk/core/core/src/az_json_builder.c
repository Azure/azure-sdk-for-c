// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_hex_private.h"
#include "az_json_private.h"
#include "az_json_string_private.h"
#include "az_span_private.h"
#include <az_json.h>
#include <az_span_internal.h>

#include <_az_cfg.h>

enum
{
  // Max size for an already escaped string value (~ half of INT_MAX)
  _az_MAX_ESCAPED_STRING_SIZE = 1000000000,

  // In the worst case, an ASCII character represented as a single UTF-8 byte could expand 6x when
  // escaped.
  // For example: '+' becomes '\u0043'
  // Escaping surrogate pairs (represented by 3 or 4 UTF-8 bytes) would expand to 12 bytes (which is
  // still <= 6x).
  _az_MAX_EXPANSION_FACTOR_WHILE_ESCAPING = 6,

  _az_MAX_UNESCAPED_STRING_SIZE
  = _az_MAX_ESCAPED_STRING_SIZE / _az_MAX_EXPANSION_FACTOR_WHILE_ESCAPING, // 166_666_666 bytes
};

static AZ_NODISCARD az_span _get_remaining_span(az_json_builder* json_builder)
{
  _az_PRECONDITION_NOT_NULL(json_builder);

  return az_span_slice_to_end(
      json_builder->_internal.destination_buffer, json_builder->_internal.bytes_written);
}

// TODO: Make this a precondition?
static AZ_NODISCARD bool _az_is_appending_value_valid(az_json_builder* json_builder)
{
  _az_PRECONDITION_NOT_NULL(json_builder);

  az_json_token_kind kind = json_builder->_internal.token_kind;

  if (_az_json_stack_peek(&json_builder->_internal.bit_stack))
  {
    // Cannot write a JSON value within an object without a property name first.
    // That includes writing the start of an object or array without a property name.
    if (kind != AZ_JSON_TOKEN_PROPERTY_NAME)
    {
      // Given we are within a JSON object, kind cannot be start of an array or none.
      _az_PRECONDITION(kind != AZ_JSON_TOKEN_NONE && kind != AZ_JSON_TOKEN_ARRAY_START);

      return false;
    }
  }
  else
  {
    // Adding a JSON value within a JSON array is allowed and it is also allowed to add a standalone
    // single JSON value. However, it is invalid to add multiple JSON values that aren't within a
    // container, or outside an existing closed object/array.

    // That includes writing the start of an object or array after a single JSON value or outside of
    // an existing closed object/array.

    // Given we are not within a JSON object, kind cannot be property name.
    _az_PRECONDITION(kind != AZ_JSON_TOKEN_PROPERTY_NAME && kind != AZ_JSON_TOKEN_OBJECT_START);

    // It is more likely for current_depth to not equal 0 when writing valid JSON, so check that
    // first to rely on short-circuiting and return quickly.
    if (json_builder->_internal.bit_stack._internal.current_depth == 0
        && kind != AZ_JSON_TOKEN_NONE)
    {
      return false;
    }
  }

  // JSON builder state is valid and a primitive value or start of a container can be appended.
  return true;
}

// TODO: Make this a precondition?
static AZ_NODISCARD bool _az_is_appending_property_name_valid(az_json_builder* json_builder)
{
  _az_PRECONDITION_NOT_NULL(json_builder);

  az_json_token_kind kind = json_builder->_internal.token_kind;

  // Cannot write a JSON property within an array or as the first JSON token.
  // Cannot write a JSON property name following another property name. A JSON value is missing.
  if (!_az_json_stack_peek(&json_builder->_internal.bit_stack)
      || kind == AZ_JSON_TOKEN_PROPERTY_NAME)
  {
    _az_PRECONDITION(kind != AZ_JSON_TOKEN_OBJECT_START);
    return false;
  }

  // JSON builder state is valid and a property name can be appended.
  return true;
}

// TODO: Make this a precondition?
static AZ_NODISCARD bool _az_is_appending_container_end_valid(az_json_builder* json_builder)
{
  _az_PRECONDITION_NOT_NULL(json_builder);

  az_json_token_kind kind = json_builder->_internal.token_kind;

  // Cannot write an end of a container without a matching start.
  // This includes writing the end token as the first token in the JSON or right after a property
  // name.
  if (json_builder->_internal.bit_stack._internal.current_depth <= 0
      || kind == AZ_JSON_TOKEN_PROPERTY_NAME)
  {
    return false;
  }

  // JSON builder state is valid and an end of a container can be appended.
  return true;
}

AZ_NODISCARD int32_t
_az_json_builder_escaped_length(az_span value, int32_t* out_index_of_first_escaped_char)
{
  _az_PRECONDITION_NOT_NULL(out_index_of_first_escaped_char);
  _az_PRECONDITION_VALID_SPAN(value, 0, false);

  int32_t value_size = az_span_size(value);
  _az_PRECONDITION(value_size <= _az_MAX_UNESCAPED_STRING_SIZE);

  int32_t escaped_length = 0;
  *out_index_of_first_escaped_char = -1;

  int32_t i = 0;
  uint8_t* value_ptr = az_span_ptr(value);

  while (i < value_size)
  {
    uint8_t const ch = value_ptr[i];

    switch (ch)
    {
      case '\\':
      case '"':
      case '\b':
      case '\f':
      case '\n':
      case '\r':
      case '\t':
      {
        escaped_length += 2; // Use the two-character sequence escape for these.
        break;
      }
      default:
      {
        // Check if the character has to be escaped as a UNICODE escape sequence.
        if (ch < 0x20)
        {
          escaped_length += 6;
        }
        else
        {
          escaped_length++; // No escaping required.
        }
        break;
      }
    }

    i++;

    // If this is the first time that we found a character that needs to be escaped,
    // set out_index_of_first_escaped_char to the corresponding index.
    // If escaped_length == i, then we haven't found a character that needs to be escaped yet.
    if (escaped_length != i && *out_index_of_first_escaped_char == -1)
    {
      *out_index_of_first_escaped_char = i - 1;
    }

    // If the length overflows, in case the precondition is not honored, stop processing and break
    // The caller will return AZ_ERROR_INSUFFICIENT_SPAN_SIZE since az_span can't contain it.
    // TODO: Consider removing this if it is too costly.
    if (escaped_length < 0)
    {
      escaped_length = INT32_MAX;
      break;
    }
  }

  // In most common cases, escaped_length will equal value_size and out_index_of_first_escaped_char
  // will be -1.
  return escaped_length;
}

AZ_NODISCARD az_span _az_json_builder_escape_and_copy(az_span destination, az_span source)
{
  _az_PRECONDITION_VALID_SPAN(source, 1, false);

  int32_t src_size = az_span_size(source);
  _az_PRECONDITION(src_size <= _az_MAX_UNESCAPED_STRING_SIZE);
  _az_PRECONDITION_VALID_SPAN(destination, src_size + 1, false);

  int32_t i = 0;
  uint8_t* value_ptr = az_span_ptr(source);

  az_span remaining_destination = destination;

  while (i < src_size)
  {
    uint8_t const ch = value_ptr[i];

    switch (ch)
    {
      case '\\':
      case '"':
      {
        remaining_destination = az_span_copy_u8(remaining_destination, '\\');
        remaining_destination = az_span_copy_u8(remaining_destination, ch);
        break;
      }
      case '\b':
      {
        remaining_destination = az_span_copy_u8(remaining_destination, '\\');
        remaining_destination = az_span_copy_u8(remaining_destination, 'b');
        break;
      }
      case '\f':
      {
        remaining_destination = az_span_copy_u8(remaining_destination, '\\');
        remaining_destination = az_span_copy_u8(remaining_destination, 'f');
        break;
      }
      case '\n':
      {
        remaining_destination = az_span_copy_u8(remaining_destination, '\\');
        remaining_destination = az_span_copy_u8(remaining_destination, 'n');
        break;
      }
      case '\r':
      {
        remaining_destination = az_span_copy_u8(remaining_destination, '\\');
        remaining_destination = az_span_copy_u8(remaining_destination, 'r');
        break;
      }
      case '\t':
      {
        remaining_destination = az_span_copy_u8(remaining_destination, '\\');
        remaining_destination = az_span_copy_u8(remaining_destination, 't');
        break;
      }
      default:
      {
        // Check if the character has to be escaped as a UNICODE escape sequence.
        if (ch < 0x20)
        {
          // TODO: Consider moving this array outside the loop.
          uint8_t array[6] = {
            '\\',
            'u',
            '0',
            '0',
            _az_number_to_upper_hex((uint8_t)(ch / 16)),
            _az_number_to_upper_hex((uint8_t)(ch % 16)),
          };
          remaining_destination = az_span_copy(remaining_destination, AZ_SPAN_FROM_BUFFER(array));
        }
        else
        {
          remaining_destination = az_span_copy_u8(remaining_destination, ch);
        }
        break;
      }
    }

    i++;
  }

  return remaining_destination;
}

AZ_NODISCARD az_result az_json_builder_append_string(az_json_builder* json_builder, az_span value)
{
  _az_PRECONDITION_NOT_NULL(json_builder);
  _az_PRECONDITION_VALID_SPAN(value, 0, true);
  _az_PRECONDITION(az_span_size(value) <= _az_MAX_UNESCAPED_STRING_SIZE);

  if (!_az_is_appending_value_valid(json_builder))
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  if (az_span_ptr(value) == NULL)
  {
    return az_json_builder_append_null(json_builder);
  }

  az_span remaining_json = _get_remaining_span(json_builder);

  int32_t required_size = 2; // For the surrounding quotes.

  if (json_builder->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  int32_t index_of_first_escaped_char = -1;
  required_size += _az_json_builder_escaped_length(value, &index_of_first_escaped_char);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (json_builder->_internal.need_comma)
  {
    remaining_json = az_span_copy_u8(remaining_json, ',');
  }

  remaining_json = az_span_copy_u8(remaining_json, '"');

  // No character needed to be escaped, copy the whole string as is.
  if (index_of_first_escaped_char == -1)
  {
    remaining_json = az_span_copy(remaining_json, value);
  }
  else
  {
    // Bulk copy the characters that didn't need to be escaped before dropping to the byte-by-byte
    // encode and copy.
    remaining_json
        = az_span_copy(remaining_json, az_span_slice(value, 0, index_of_first_escaped_char));
    remaining_json = _az_json_builder_escape_and_copy(
        remaining_json, az_span_slice_to_end(value, index_of_first_escaped_char));
  }

  az_span_copy_u8(remaining_json, '"');

  json_builder->_internal.bytes_written += required_size;
  json_builder->_internal.need_comma = true;
  json_builder->_internal.token_kind = AZ_JSON_TOKEN_STRING;
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_builder_append_property_name(az_json_builder* json_builder, az_span name)
{
  _az_PRECONDITION_NOT_NULL(json_builder);
  _az_PRECONDITION_VALID_SPAN(name, 0, false);
  _az_PRECONDITION(az_span_size(name) <= _az_MAX_UNESCAPED_STRING_SIZE);

  if (!_az_is_appending_property_name_valid(json_builder))
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  az_span remaining_json = _get_remaining_span(json_builder);

  int32_t required_size = 3; // For the surrounding quotes and the key:value separator colon.

  if (json_builder->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  int32_t index_of_first_escaped_char = -1;
  required_size += _az_json_builder_escaped_length(name, &index_of_first_escaped_char);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (json_builder->_internal.need_comma)
  {
    remaining_json = az_span_copy_u8(remaining_json, ',');
  }

  remaining_json = az_span_copy_u8(remaining_json, '"');

  // No character needed to be escaped, copy the whole string as is.
  if (index_of_first_escaped_char == -1)
  {
    remaining_json = az_span_copy(remaining_json, name);
  }
  else
  {
    // Bulk copy the characters that didn't need to be escaped before dropping to the byte-by-byte
    // encode and copy.
    remaining_json
        = az_span_copy(remaining_json, az_span_slice(name, 0, index_of_first_escaped_char));
    remaining_json = _az_json_builder_escape_and_copy(
        remaining_json, az_span_slice_to_end(name, index_of_first_escaped_char));
  }

  remaining_json = az_span_copy_u8(remaining_json, '"');
  remaining_json = az_span_copy_u8(remaining_json, ':');

  json_builder->_internal.bytes_written += required_size;
  json_builder->_internal.need_comma = false;
  json_builder->_internal.token_kind = AZ_JSON_TOKEN_PROPERTY_NAME;
  return AZ_OK;
}

AZ_NODISCARD az_result _az_json_builder_append_literal(
    az_json_builder* json_builder,
    az_span literal,
    az_json_token_kind literal_kind)
{
  _az_PRECONDITION_NOT_NULL(json_builder);
  _az_PRECONDITION(
      literal_kind == AZ_JSON_TOKEN_NULL || literal_kind == AZ_JSON_TOKEN_TRUE
      || literal_kind == AZ_JSON_TOKEN_FALSE);
  _az_PRECONDITION_VALID_SPAN(literal, 4, false);
  _az_PRECONDITION(az_span_size(literal) <= 5); // null, true, or false

  if (!_az_is_appending_value_valid(json_builder))
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  az_span remaining_json = _get_remaining_span(json_builder);

  int32_t required_size = az_span_size(literal);

  if (json_builder->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (json_builder->_internal.need_comma)
  {
    remaining_json = az_span_copy_u8(remaining_json, ',');
  }

  remaining_json = az_span_copy(remaining_json, literal);

  json_builder->_internal.bytes_written += required_size;
  json_builder->_internal.need_comma = true;
  json_builder->_internal.token_kind = literal_kind;
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_builder_append_bool(az_json_builder* json_builder, bool value)
{
  az_result result;
  if (value)
  {
    result = _az_json_builder_append_literal(
        json_builder, AZ_SPAN_FROM_STR("true"), AZ_JSON_TOKEN_TRUE);
  }
  else
  {
    result = _az_json_builder_append_literal(
        json_builder, AZ_SPAN_FROM_STR("false"), AZ_JSON_TOKEN_FALSE);
  }
  return result;
}

AZ_NODISCARD az_result az_json_builder_append_null(az_json_builder* json_builder)
{
  return _az_json_builder_append_literal(
      json_builder, AZ_SPAN_FROM_STR("null"), AZ_JSON_TOKEN_NULL);
}

AZ_NODISCARD az_result az_json_builder_append_number(az_json_builder* json_builder, double value)
{
  _az_PRECONDITION_NOT_NULL(json_builder);

  if (!_az_is_appending_value_valid(json_builder))
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  az_span remaining_json = _get_remaining_span(json_builder);

  int32_t required_size = 1; // Need space to write at least one digit.

  if (json_builder->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (json_builder->_internal.need_comma)
  {
    remaining_json = az_span_copy_u8(remaining_json, ',');
  }

  az_span leftover;
  AZ_RETURN_IF_FAILED(az_span_dtoa(remaining_json, value, &leftover));

  // We already accounted for the first digit above, so therefore subtract one.
  json_builder->_internal.bytes_written
      += required_size + _az_span_diff(leftover, remaining_json) - 1;
  json_builder->_internal.need_comma = true;
  json_builder->_internal.token_kind = AZ_JSON_TOKEN_NUMBER;
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_builder_append_int32_number(az_json_builder* json_builder, int32_t value)
{
  _az_PRECONDITION_NOT_NULL(json_builder);

  if (!_az_is_appending_value_valid(json_builder))
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  az_span remaining_json = _get_remaining_span(json_builder);

  int32_t required_size = 1; // Need space to write at least one digit.

  if (json_builder->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (json_builder->_internal.need_comma)
  {
    remaining_json = az_span_copy_u8(remaining_json, ',');
  }

  az_span leftover;
  AZ_RETURN_IF_FAILED(az_span_i32toa(remaining_json, value, &leftover));

  // We already accounted for the first digit above, so therefore subtract one.
  json_builder->_internal.bytes_written
      += required_size + _az_span_diff(leftover, remaining_json) - 1;
  json_builder->_internal.need_comma = true;
  json_builder->_internal.token_kind = AZ_JSON_TOKEN_NUMBER;
  return AZ_OK;
}

AZ_NODISCARD az_result _az_json_builder_append_container_start(
    az_json_builder* json_builder,
    uint8_t byte,
    az_json_token_kind container_kind)
{
  _az_PRECONDITION_NOT_NULL(json_builder);
  _az_PRECONDITION(
      container_kind == AZ_JSON_TOKEN_OBJECT_START || container_kind == AZ_JSON_TOKEN_ARRAY_START);

  if (!_az_is_appending_value_valid(json_builder))
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  // The current depth is equal to or larger than the maximum allowed depth of 64. Cannot write the
  // next JSON object or array.
  // TODO: Make this a precondition?
  if (json_builder->_internal.bit_stack._internal.current_depth >= _az_MAX_JSON_STACK_SIZE)
  {
    return AZ_ERROR_JSON_NESTING_OVERFLOW;
  }

  az_span remaining_json = _get_remaining_span(json_builder);

  int32_t required_size = 1; // For the start object or array byte.

  if (json_builder->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (json_builder->_internal.need_comma)
  {
    remaining_json = az_span_copy_u8(remaining_json, ',');
  }

  remaining_json = az_span_copy_u8(remaining_json, byte);

  json_builder->_internal.bytes_written += required_size;
  json_builder->_internal.need_comma = false;
  json_builder->_internal.token_kind = container_kind;
  if (container_kind == AZ_JSON_TOKEN_OBJECT_START)
  {
    _az_json_stack_push(&json_builder->_internal.bit_stack, _az_JSON_STACK_OBJECT);
  }
  else
  {
    _az_json_stack_push(&json_builder->_internal.bit_stack, _az_JSON_STACK_ARRAY);
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_json_builder_append_object_start(az_json_builder* json_builder)
{
  return _az_json_builder_append_container_start(json_builder, '{', AZ_JSON_TOKEN_OBJECT_START);
}

AZ_NODISCARD az_result az_json_builder_append_array_start(az_json_builder* json_builder)
{
  return _az_json_builder_append_container_start(json_builder, '[', AZ_JSON_TOKEN_ARRAY_START);
}

AZ_NODISCARD az_result az_json_builder_append_container_end(az_json_builder* json_builder)
{
  _az_PRECONDITION_NOT_NULL(json_builder);

  if (!_az_is_appending_container_end_valid(json_builder))
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  az_span remaining_json = _get_remaining_span(json_builder);

  int32_t required_size = 1; // For the end object or array byte.

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (_az_json_stack_peek(&json_builder->_internal.bit_stack))
  {
    remaining_json = az_span_copy_u8(remaining_json, '}');
    json_builder->_internal.token_kind = AZ_JSON_TOKEN_OBJECT_END;
  }
  else
  {
    remaining_json = az_span_copy_u8(remaining_json, ']');
    json_builder->_internal.token_kind = AZ_JSON_TOKEN_ARRAY_END;
  }

  json_builder->_internal.bytes_written += required_size;
  json_builder->_internal.need_comma = true;
  _az_json_stack_pop(&json_builder->_internal.bit_stack);

  return AZ_OK;
}

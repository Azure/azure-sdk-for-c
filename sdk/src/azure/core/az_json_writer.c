// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_hex_private.h"
#include "az_json_private.h"
#include "az_span_private.h"
#include <azure/core/az_json.h>
#include <azure/core/internal/az_span_internal.h>
#include <math.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_json_writer_init(
    az_json_writer* json_writer,
    az_span destination_buffer,
    az_json_writer_options const* options)
{
  _az_PRECONDITION_NOT_NULL(json_writer);

  *json_writer = (az_json_writer){ ._internal = {
                                       .destination_buffer = destination_buffer,
                                       .bytes_written = 0,
                                       .need_comma = false,
                                       .token_kind = AZ_JSON_TOKEN_NONE,
                                       .bit_stack = { 0 },
                                       .options = options == NULL ? az_json_writer_options_default()
                                                                  : *options,
                                   } };
  return AZ_OK;
}

static AZ_NODISCARD az_span _get_remaining_span(az_json_writer* json_writer)
{
  _az_PRECONDITION_NOT_NULL(json_writer);

  return az_span_slice_to_end(
      json_writer->_internal.destination_buffer, json_writer->_internal.bytes_written);
}

#ifndef AZ_NO_PRECONDITION_CHECKING
static AZ_NODISCARD bool _az_is_appending_value_valid(az_json_writer* json_writer)
{
  _az_PRECONDITION_NOT_NULL(json_writer);

  az_json_token_kind kind = json_writer->_internal.token_kind;

  if (_az_json_stack_peek(&json_writer->_internal.bit_stack))
  {
    // Cannot write a JSON value within an object without a property name first.
    // That includes writing the start of an object or array without a property name.
    if (kind != AZ_JSON_TOKEN_PROPERTY_NAME)
    {
      // Given we are within a JSON object, kind cannot be start of an array or none.
      _az_PRECONDITION(kind != AZ_JSON_TOKEN_NONE && kind != AZ_JSON_TOKEN_BEGIN_ARRAY);

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
    _az_PRECONDITION(kind != AZ_JSON_TOKEN_PROPERTY_NAME && kind != AZ_JSON_TOKEN_BEGIN_OBJECT);

    // It is more likely for current_depth to not equal 0 when writing valid JSON, so check that
    // first to rely on short-circuiting and return quickly.
    if (json_writer->_internal.bit_stack._internal.current_depth == 0 && kind != AZ_JSON_TOKEN_NONE)
    {
      return false;
    }
  }

  // JSON writer state is valid and a primitive value or start of a container can be appended.
  return true;
}

static AZ_NODISCARD bool _az_is_appending_property_name_valid(az_json_writer* json_writer)
{
  _az_PRECONDITION_NOT_NULL(json_writer);

  az_json_token_kind kind = json_writer->_internal.token_kind;

  // Cannot write a JSON property within an array or as the first JSON token.
  // Cannot write a JSON property name following another property name. A JSON value is missing.
  if (!_az_json_stack_peek(&json_writer->_internal.bit_stack)
      || kind == AZ_JSON_TOKEN_PROPERTY_NAME)
  {
    _az_PRECONDITION(kind != AZ_JSON_TOKEN_BEGIN_OBJECT);
    return false;
  }

  // JSON writer state is valid and a property name can be appended.
  return true;
}

static AZ_NODISCARD bool _az_is_appending_container_end_valid(
    az_json_writer* json_writer,
    uint8_t byte)
{
  _az_PRECONDITION_NOT_NULL(json_writer);

  az_json_token_kind kind = json_writer->_internal.token_kind;

  // Cannot write an end of a container without a matching start.
  // This includes writing the end token as the first token in the JSON or right after a property
  // name.
  if (json_writer->_internal.bit_stack._internal.current_depth <= 0
      || kind == AZ_JSON_TOKEN_PROPERTY_NAME)
  {
    return false;
  }

  _az_json_stack_item stack_item = _az_json_stack_peek(&json_writer->_internal.bit_stack);

  if (byte == ']')
  {
    // If inside a JSON object, then appending an end bracket is invalid:
    if (stack_item)
    {
      _az_PRECONDITION(kind != AZ_JSON_TOKEN_NONE);
      return false;
    }
  }
  else
  {
    _az_PRECONDITION(byte == '}');

    // If not inside a JSON object, then appending an end brace is invalid:
    if (!stack_item)
    {
      return false;
    }
  }

  // JSON writer state is valid and an end of a container can be appended.
  return true;
}
#endif // AZ_NO_PRECONDITION_CHECKING

// Returns the length of the JSON string within the az_span after it has been escaped.
// The out parameter contains the index where the first character to escape is found.
// If no chars need to be escaped then return the size of value with the out parameter set to -1.
static AZ_NODISCARD int32_t
_az_json_writer_escaped_length(az_span value, int32_t* out_index_of_first_escaped_char)
{
  _az_PRECONDITION_NOT_NULL(out_index_of_first_escaped_char);
  _az_PRECONDITION_VALID_SPAN(value, 0, true);

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

static AZ_NODISCARD az_span _az_json_writer_escape_and_copy(az_span destination, az_span source)
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

    uint8_t escaped = 0;

    switch (ch)
    {
      case '\\':
      case '"':
      {
        escaped = ch;
        break;
      }
      case '\b':
      {
        escaped = 'b';
        break;
      }
      case '\f':
      {
        escaped = 'f';
        break;
      }
      case '\n':
      {
        escaped = 'n';
        break;
      }
      case '\r':
      {
        escaped = 'r';
        break;
      }
      case '\t':
      {
        escaped = 't';
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

    // If escaped is non-zero, then we found one of the characters that needs to be escaped.
    // Otherwise, we hit the default case in the switch above, in which case, we already wrote
    // the character.
    if (escaped)
    {
      remaining_destination = az_span_copy_u8(remaining_destination, '\\');
      remaining_destination = az_span_copy_u8(remaining_destination, escaped);
    }

    i++;
  }

  return remaining_destination;
}

AZ_INLINE void _az_update_json_writer_state(
    az_json_writer* json_writer,
    int32_t required_size,
    bool need_comma,
    az_json_token_kind token_kind)
{
  json_writer->_internal.bytes_written += required_size;
  json_writer->_internal.need_comma = need_comma;
  json_writer->_internal.token_kind = token_kind;
}

AZ_NODISCARD az_result az_json_writer_append_string(az_json_writer* json_writer, az_span value)
{
  _az_PRECONDITION_NOT_NULL(json_writer);
  // A null span is allowed, and we write an empty JSON string for it.
  _az_PRECONDITION_VALID_SPAN(value, 0, true);
  _az_PRECONDITION(az_span_size(value) <= _az_MAX_UNESCAPED_STRING_SIZE);
  _az_PRECONDITION(_az_is_appending_value_valid(json_writer));

  az_span remaining_json = _get_remaining_span(json_writer);

  int32_t required_size = 2; // For the surrounding quotes.

  if (json_writer->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  int32_t index_of_first_escaped_char = -1;
  required_size += _az_json_writer_escaped_length(value, &index_of_first_escaped_char);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (json_writer->_internal.need_comma)
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
    remaining_json = _az_json_writer_escape_and_copy(
        remaining_json, az_span_slice_to_end(value, index_of_first_escaped_char));
  }

  az_span_copy_u8(remaining_json, '"');

  _az_update_json_writer_state(json_writer, required_size, true, AZ_JSON_TOKEN_STRING);
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_writer_append_property_name(az_json_writer* json_writer, az_span name)
{
  // TODO: Consider refactoring to reduce duplication between writing property name and string.
  _az_PRECONDITION_NOT_NULL(json_writer);
  _az_PRECONDITION_VALID_SPAN(name, 0, false);
  _az_PRECONDITION(az_span_size(name) <= _az_MAX_UNESCAPED_STRING_SIZE);
  _az_PRECONDITION(_az_is_appending_property_name_valid(json_writer));

  az_span remaining_json = _get_remaining_span(json_writer);

  int32_t required_size = 3; // For the surrounding quotes and the key:value separator colon.

  if (json_writer->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  int32_t index_of_first_escaped_char = -1;
  required_size += _az_json_writer_escaped_length(name, &index_of_first_escaped_char);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (json_writer->_internal.need_comma)
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
    remaining_json = _az_json_writer_escape_and_copy(
        remaining_json, az_span_slice_to_end(name, index_of_first_escaped_char));
  }

  remaining_json = az_span_copy_u8(remaining_json, '"');
  remaining_json = az_span_copy_u8(remaining_json, ':');

  _az_update_json_writer_state(json_writer, required_size, false, AZ_JSON_TOKEN_PROPERTY_NAME);
  return AZ_OK;
}

static AZ_NODISCARD az_result _az_json_writer_append_literal(
    az_json_writer* json_writer,
    az_span literal,
    az_json_token_kind literal_kind)
{
  _az_PRECONDITION_NOT_NULL(json_writer);
  _az_PRECONDITION(
      literal_kind == AZ_JSON_TOKEN_NULL || literal_kind == AZ_JSON_TOKEN_TRUE
      || literal_kind == AZ_JSON_TOKEN_FALSE);
  _az_PRECONDITION_VALID_SPAN(literal, 4, false);
  _az_PRECONDITION(az_span_size(literal) <= 5); // null, true, or false
  _az_PRECONDITION(_az_is_appending_value_valid(json_writer));

  az_span remaining_json = _get_remaining_span(json_writer);

  int32_t required_size = az_span_size(literal);

  if (json_writer->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (json_writer->_internal.need_comma)
  {
    remaining_json = az_span_copy_u8(remaining_json, ',');
  }

  remaining_json = az_span_copy(remaining_json, literal);

  _az_update_json_writer_state(json_writer, required_size, true, literal_kind);
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_writer_append_bool(az_json_writer* json_writer, bool value)
{
  az_result result;
  if (value)
  {
    result
        = _az_json_writer_append_literal(json_writer, AZ_SPAN_FROM_STR("true"), AZ_JSON_TOKEN_TRUE);
  }
  else
  {
    result = _az_json_writer_append_literal(
        json_writer, AZ_SPAN_FROM_STR("false"), AZ_JSON_TOKEN_FALSE);
  }
  return result;
}

AZ_NODISCARD az_result az_json_writer_append_null(az_json_writer* json_writer)
{
  return _az_json_writer_append_literal(json_writer, AZ_SPAN_FROM_STR("null"), AZ_JSON_TOKEN_NULL);
}

AZ_NODISCARD az_result az_json_writer_append_int32(az_json_writer* json_writer, int32_t value)
{
  _az_PRECONDITION_NOT_NULL(json_writer);
  _az_PRECONDITION(_az_is_appending_value_valid(json_writer));

  az_span remaining_json = _get_remaining_span(json_writer);

  int32_t required_size = 1; // Need space to write at least one digit.

  if (json_writer->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (json_writer->_internal.need_comma)
  {
    remaining_json = az_span_copy_u8(remaining_json, ',');
  }

  az_span leftover;
  AZ_RETURN_IF_FAILED(az_span_i32toa(remaining_json, value, &leftover));

  // We already accounted for the first digit above, so therefore subtract one.
  _az_update_json_writer_state(
      json_writer,
      required_size + _az_span_diff(leftover, remaining_json) - 1,
      true,
      AZ_JSON_TOKEN_NUMBER);
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_writer_append_double(az_json_writer* json_writer, double value, int32_t fractional_digits)
{
  _az_PRECONDITION_NOT_NULL(json_writer);
  _az_PRECONDITION(_az_is_appending_value_valid(json_writer));
  // Non-finite numbers are not supported because they lead to invalid JSON.
  // Unquoted strings such as nan and -inf are invalid as JSON numbers.
  _az_PRECONDITION(isfinite(value));
  _az_PRECONDITION_RANGE(0, fractional_digits, _az_MAX_SUPPORTED_FRACTIONAL_DIGITS);

  az_span remaining_json = _get_remaining_span(json_writer);

  int32_t required_size = 1; // Need space to write at least one digit.

  if (json_writer->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (json_writer->_internal.need_comma)
  {
    remaining_json = az_span_copy_u8(remaining_json, ',');
  }

  az_span leftover;
  AZ_RETURN_IF_FAILED(az_span_dtoa(remaining_json, value, fractional_digits, &leftover));

  // We already accounted for the first digit above, so therefore subtract one.
  _az_update_json_writer_state(
      json_writer,
      required_size + _az_span_diff(leftover, remaining_json) - 1,
      true,
      AZ_JSON_TOKEN_NUMBER);
  return AZ_OK;
}

static AZ_NODISCARD az_result _az_json_writer_append_container_start(
    az_json_writer* json_writer,
    uint8_t byte,
    az_json_token_kind container_kind)
{
  _az_PRECONDITION_NOT_NULL(json_writer);
  _az_PRECONDITION(
      container_kind == AZ_JSON_TOKEN_BEGIN_OBJECT || container_kind == AZ_JSON_TOKEN_BEGIN_ARRAY);
  _az_PRECONDITION(_az_is_appending_value_valid(json_writer));

  // The current depth is equal to or larger than the maximum allowed depth of 64. Cannot write the
  // next JSON object or array.
  if (json_writer->_internal.bit_stack._internal.current_depth >= _az_MAX_JSON_STACK_SIZE)
  {
    return AZ_ERROR_JSON_NESTING_OVERFLOW;
  }

  az_span remaining_json = _get_remaining_span(json_writer);

  int32_t required_size = 1; // For the start object or array byte.

  if (json_writer->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (json_writer->_internal.need_comma)
  {
    remaining_json = az_span_copy_u8(remaining_json, ',');
  }

  remaining_json = az_span_copy_u8(remaining_json, byte);

  _az_update_json_writer_state(json_writer, required_size, false, container_kind);
  if (container_kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    _az_json_stack_push(&json_writer->_internal.bit_stack, _az_JSON_STACK_OBJECT);
  }
  else
  {
    _az_json_stack_push(&json_writer->_internal.bit_stack, _az_JSON_STACK_ARRAY);
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_json_writer_append_begin_object(az_json_writer* json_writer)
{
  return _az_json_writer_append_container_start(json_writer, '{', AZ_JSON_TOKEN_BEGIN_OBJECT);
}

AZ_NODISCARD az_result az_json_writer_append_begin_array(az_json_writer* json_writer)
{
  return _az_json_writer_append_container_start(json_writer, '[', AZ_JSON_TOKEN_BEGIN_ARRAY);
}

static AZ_NODISCARD az_result az_json_writer_append_container_end(
    az_json_writer* json_writer,
    uint8_t byte,
    az_json_token_kind container_kind)
{
  _az_PRECONDITION_NOT_NULL(json_writer);
  _az_PRECONDITION(
      container_kind == AZ_JSON_TOKEN_END_OBJECT || container_kind == AZ_JSON_TOKEN_END_ARRAY);
  _az_PRECONDITION(_az_is_appending_container_end_valid(json_writer, byte));

  az_span remaining_json = _get_remaining_span(json_writer);

  int32_t required_size = 1; // For the end object or array byte.

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  remaining_json = az_span_copy_u8(remaining_json, byte);

  _az_update_json_writer_state(json_writer, required_size, true, container_kind);
  _az_json_stack_pop(&json_writer->_internal.bit_stack);

  return AZ_OK;
}

AZ_NODISCARD az_result az_json_writer_append_end_object(az_json_writer* json_writer)
{
  return az_json_writer_append_container_end(json_writer, '}', AZ_JSON_TOKEN_END_OBJECT);
}

AZ_NODISCARD az_result az_json_writer_append_end_array(az_json_writer* json_writer)
{
  return az_json_writer_append_container_end(json_writer, ']', AZ_JSON_TOKEN_END_ARRAY);
}

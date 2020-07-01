// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_json_private.h"
#include "az_span_private.h"

#include <az_precondition.h>

#include <ctype.h>

#include <_az_cfg.h>

AZ_NODISCARD static az_span _get_remaining_json(az_json_parser* json_builder)
{
  _az_PRECONDITION_NOT_NULL(json_builder);

  return az_span_slice_to_end(
      json_builder->_internal.json_buffer, json_builder->_internal.bytes_consumed);
}

static void _az_json_parser_update_state(
    az_json_parser* json_parser,
    az_json_token_kind token_kind,
    az_span token_slice,
    int32_t consumed)
{
  json_parser->token.kind = token_kind;
  json_parser->token.slice = token_slice;
  json_parser->_internal.bytes_consumed += consumed;
}

AZ_NODISCARD static az_span _az_json_parser_skip_white_space(az_json_parser* json_parser)
{
  az_span remaining = _get_remaining_json(json_parser);
  az_span json = _az_span_trim_white_space_from_start(remaining);

  // Find out how many whitespace characters were trimmed.
  json_parser->_internal.bytes_consumed += az_span_size(remaining) - az_span_size(json);

  return json;
}

AZ_NODISCARD static az_result _az_json_parser_process_container_end(
    az_json_parser* json_parser,
    az_json_token_kind token_kind)
{
  // The JSON payload is invalid if it has a mismatched container end without a matching open.
  if ((token_kind == AZ_JSON_TOKEN_END_OBJECT
       && _az_json_stack_peek(&json_parser->_internal.bit_stack) != _az_JSON_STACK_OBJECT)
      || (token_kind == AZ_JSON_TOKEN_END_ARRAY
          && _az_json_stack_peek(&json_parser->_internal.bit_stack) != _az_JSON_STACK_ARRAY))
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }

  az_span token = _get_remaining_json(json_parser);
  _az_json_stack_pop(&json_parser->_internal.bit_stack);
  _az_json_parser_update_state(json_parser, token_kind, az_span_slice(token, 0, 1), 1);
  return AZ_OK;
}

AZ_NODISCARD static az_result _az_json_parser_process_container_start(
    az_json_parser* json_parser,
    az_json_token_kind token_kind,
    _az_json_stack_item container_kind)
{
  // The current depth is equal to or larger than the maximum allowed depth of 64. Cannot read the
  // next JSON object or array.
  if (json_parser->_internal.bit_stack._internal.current_depth >= _az_MAX_JSON_STACK_SIZE)
  {
    return AZ_ERROR_JSON_NESTING_OVERFLOW;
  }

  az_span token = _get_remaining_json(json_parser);

  _az_json_stack_push(&json_parser->_internal.bit_stack, container_kind);
  _az_json_parser_update_state(json_parser, token_kind, az_span_slice(token, 0, 1), 1);
  return AZ_OK;
}

AZ_NODISCARD static bool _az_is_valid_escaped_character(uint8_t byte)
{
  switch (byte)
  {
    case '\\':
    case '"':
    case '/':
    case 'b':
    case 'f':
    case 'n':
    case 'r':
    case 't':
      return true;
    default:
      return false;
  }
}

// A hex digit is valid if it is in the range: [0..9] | [A..F] | [a..f]
// Otherwise, return false.
AZ_NODISCARD static bool _az_is_hex_digit(uint8_t byte)
{
  return (byte >= '0' && byte <= '9') || (byte >= 'a' && byte <= 'f')
      || (byte >= 'A' && byte <= 'F');
}

AZ_NODISCARD static bool _az_validate_hex_digits(uint8_t* token_ptr, int32_t index)
{
  // The caller already guaranteed that we have at least 4 bytes in the buffer.
  for (int32_t i = 0; i < 4; i++)
  {
    uint8_t next_byte = token_ptr[index + i];

    if (!_az_is_hex_digit(next_byte))
    {
      return false;
    }
  }
  return true;
}

AZ_NODISCARD static az_result _az_json_parser_process_string(az_json_parser* json_parser)
{
  // Move past the first '"' character
  json_parser->_internal.bytes_consumed++;

  az_span token = _get_remaining_json(json_parser);
  uint8_t* token_ptr = az_span_ptr(token);
  int32_t remaining_size = az_span_size(token);

  int32_t string_length = 0;
  uint8_t next_byte = token_ptr[0];

  // Clear the state of any previous string token.
  json_parser->token._internal.string_has_escaped_chars = false;

  while (true)
  {
    if (next_byte == '"')
    {
      break;
    }
    else if (next_byte == '\\')
    {
      json_parser->token._internal.string_has_escaped_chars = true;
      string_length++;
      if (string_length >= remaining_size)
      {
        return AZ_ERROR_EOF;
      }
      next_byte = token_ptr[string_length];

      if (next_byte == 'u')
      {
        string_length++;
        // Expecting 4 hex digits to follow the escaped 'u'
        if (string_length > remaining_size - 4)
        {
          return AZ_ERROR_EOF;
        }

        if (!_az_validate_hex_digits(token_ptr, string_length))
        {
          return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
        }

        // Skip past the 4 hex digits, the loop accounts for incrementing by 1 more.
        string_length += 3;
      }
      else
      {
        if (!_az_is_valid_escaped_character(next_byte))
        {
          return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
        }
      }
    }
    else
    {
      // Control characters are invalid within a JSON string and should be correctly escaped.
      if (next_byte < 0x20)
      {
        return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
      }
    }

    string_length++;
    if (string_length >= remaining_size)
    {
      return AZ_ERROR_EOF;
    }
    next_byte = token_ptr[string_length];
  }

  // Add 1 to number of bytes consumed to account for the last '"' character.
  _az_json_parser_update_state(
      json_parser, AZ_JSON_TOKEN_STRING, az_span_slice(token, 0, string_length), string_length + 1);

  return AZ_OK;
}

AZ_NODISCARD static az_result _az_json_parser_process_property_name(az_json_parser* json_parser)
{
  AZ_RETURN_IF_FAILED(_az_json_parser_process_string(json_parser));

  az_span json = _az_json_parser_skip_white_space(json_parser);

  // Expected a colon to indicate that a value will follow after the property name, but instead
  // either reached end of data or some other character, which is invalid.
  if (az_span_size(json) < 1)
  {
    return AZ_ERROR_EOF;
  }
  if (az_span_ptr(json)[0] != ':')
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }

  // We don't need to set the json_parser->token.slice since that was already done
  // in _az_json_parser_process_string when processing the string portion of the property name.
  // Therefore, we don't call _az_json_parser_update_state here.
  json_parser->token.kind = AZ_JSON_TOKEN_PROPERTY_NAME;
  json_parser->_internal.bytes_consumed += 1; // For the name / value separator

  return AZ_OK;
}

// Used to search for possible valid end of a number character, when we have complex JSON payloads
// (i.e. not a single JSON value).
// Whitespace characters, comma, or a container end character indicate the end of a JSON number.
static const az_span json_delimiters = AZ_SPAN_LITERAL_FROM_STR(",}] \n\r\t");

AZ_NODISCARD static bool _az_finished_consuming_json_number(
    uint8_t next_byte,
    az_span expected_next_bytes,
    az_result* result)
{
  az_span next_byte_span = az_span_init(&next_byte, 1);

  // Checking if we are done processing a JSON number
  int32_t index = az_span_find(json_delimiters, next_byte_span);
  if (index != -1)
  {
    *result = AZ_OK;
    return true;
  }

  // The next character after a "0" or a set of digits must either be a decimal or 'e'/'E' to
  // indicate scientific notation. For example "01" or "123f" is invalid.
  // The next charcter after "[-][digits].[digits]" must be 'e'/'E' if we haven't reached the end of
  // the number yet. For example, "1.1f" or "1.1-" are invalid.
  index = az_span_find(expected_next_bytes, next_byte_span);
  if (index == -1)
  {
    *result = AZ_ERROR_PARSER_UNEXPECTED_CHAR;
    return true;
  }

  return false;
}

AZ_NODISCARD static int32_t _az_json_parser_consume_digits(az_span token)
{
  int32_t token_size = az_span_size(token);
  uint8_t* next_byte_ptr = az_span_ptr(token);

  int32_t counter = 0;
  while (counter < token_size)
  {
    if (isdigit(*next_byte_ptr))
    {
      counter++;
      next_byte_ptr++;
    }
    else
    {
      break;
    }
  }

  return counter;
}

AZ_NODISCARD static az_result _az_json_parser_update_number_state_if_single_value(
    az_json_parser* json_parser,
    az_span token_slice,
    int32_t consumed_count)
{
  if (json_parser->_internal.is_complex_json)
  {
    return AZ_ERROR_EOF;
  }

  _az_json_parser_update_state(json_parser, AZ_JSON_TOKEN_NUMBER, token_slice, consumed_count);

  return AZ_OK;
}

AZ_NODISCARD static az_result _az_validate_next_byte_is_digit(az_span remaining_number)
{
  if (az_span_size(remaining_number) < 1)
  {
    return AZ_ERROR_EOF;
  }

  if (!isdigit(az_span_ptr(remaining_number)[0]))
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }

  return AZ_OK;
}

AZ_NODISCARD static az_result _az_json_parser_process_number(az_json_parser* json_parser)
{
  az_span token = _get_remaining_json(json_parser);

  int32_t token_size = az_span_size(token);
  uint8_t* next_byte_ptr = az_span_ptr(token);

  int32_t consumed_count = 0;

  uint8_t next_byte = next_byte_ptr[consumed_count];
  if (next_byte == '-')
  {
    consumed_count++;

    // A negative sign must be followed by at least one digit.
    AZ_RETURN_IF_FAILED(
        _az_validate_next_byte_is_digit(az_span_slice_to_end(token, consumed_count)));

    next_byte = next_byte_ptr[consumed_count];
  }

  if (next_byte == '0')
  {
    consumed_count++;

    if (consumed_count >= token_size)
    {
      // If there is no more JSON, this is a valid end state only when the JSON payload contains a
      // single value: "[-]0"
      // Otherwise, the payload is incomplete and ending too early.
      return _az_json_parser_update_number_state_if_single_value(
          json_parser, az_span_slice(token, 0, consumed_count), consumed_count);
    }

    next_byte = next_byte_ptr[consumed_count];
    az_result result = AZ_OK;
    if (_az_finished_consuming_json_number(next_byte, AZ_SPAN_FROM_STR(".eE"), &result))
    {
      if (result == AZ_OK)
      {
        _az_json_parser_update_state(
            json_parser,
            AZ_JSON_TOKEN_NUMBER,
            az_span_slice(token, 0, consumed_count),
            consumed_count);
      }
      return result;
    }
  }
  else
  {
    _az_PRECONDITION(isdigit(next_byte));
    // Integer part before decimal
    consumed_count += _az_json_parser_consume_digits(az_span_slice_to_end(token, consumed_count));

    if (consumed_count >= token_size)
    {
      // If there is no more JSON, this is a valid end state only when the JSON payload contains a
      // single value: "[-][digits]"
      // Otherwise, the payload is incomplete and ending too early.
      return _az_json_parser_update_number_state_if_single_value(
          json_parser, az_span_slice(token, 0, consumed_count), consumed_count);
    }

    next_byte = next_byte_ptr[consumed_count];
    az_result result = AZ_OK;
    if (_az_finished_consuming_json_number(next_byte, AZ_SPAN_FROM_STR(".eE"), &result))
    {
      if (result == AZ_OK)
      {
        _az_json_parser_update_state(
            json_parser,
            AZ_JSON_TOKEN_NUMBER,
            az_span_slice(token, 0, consumed_count),
            consumed_count);
      }
      return result;
    }
  }

  if (next_byte == '.')
  {
    consumed_count++;

    // A decimal point must be followed by at least one digit.
    AZ_RETURN_IF_FAILED(
        _az_validate_next_byte_is_digit(az_span_slice_to_end(token, consumed_count)));

    // Integer part after decimal
    consumed_count += _az_json_parser_consume_digits(az_span_slice_to_end(token, consumed_count));

    if (consumed_count >= token_size)
    {
      // If there is no more JSON, this is a valid end state only when the JSON payload contains a
      // single value: "[-][digits].[digits]"
      // Otherwise, the payload is incomplete and ending too early.
      return _az_json_parser_update_number_state_if_single_value(
          json_parser, az_span_slice(token, 0, consumed_count), consumed_count);
    }

    next_byte = next_byte_ptr[consumed_count];
    az_result result = AZ_OK;
    if (_az_finished_consuming_json_number(next_byte, AZ_SPAN_FROM_STR("eE"), &result))
    {
      if (result == AZ_OK)
      {
        _az_json_parser_update_state(
            json_parser,
            AZ_JSON_TOKEN_NUMBER,
            az_span_slice(token, 0, consumed_count),
            consumed_count);
      }
      return result;
    }
  }

  // Move past 'e'/'E'
  consumed_count++;

  // The 'e'/'E' character must be followed by a sign or at least one digit.
  if (consumed_count >= token_size)
  {
    return AZ_ERROR_EOF;
  }

  next_byte = next_byte_ptr[consumed_count];
  if (next_byte == '-' || next_byte == '+')
  {
    consumed_count++;

    // A sign must be followed by at least one digit.
    AZ_RETURN_IF_FAILED(
        _az_validate_next_byte_is_digit(az_span_slice_to_end(token, consumed_count)));
  }

  // Integer part after the 'e'/'E'
  consumed_count += _az_json_parser_consume_digits(az_span_slice_to_end(token, consumed_count));

  if (consumed_count >= token_size)
  {
    // If there is no more JSON, this is a valid end state only when the JSON payload contains a
    // single value: "[-][digits].[digits]e[+|-][digits]"
    // Otherwise, the payload is incomplete and ending too early.
    return _az_json_parser_update_number_state_if_single_value(
        json_parser, az_span_slice(token, 0, consumed_count), consumed_count);
  }

  // Checking if we are done processing a JSON number
  next_byte = next_byte_ptr[consumed_count];
  int32_t index = az_span_find(json_delimiters, az_span_init(&next_byte, 1));
  if (index == -1)
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }

  _az_json_parser_update_state(
      json_parser, AZ_JSON_TOKEN_NUMBER, az_span_slice(token, 0, consumed_count), consumed_count);

  return AZ_OK;
}

AZ_NODISCARD static az_result _az_json_parser_process_literal(
    az_json_parser* json_parser,
    az_span literal,
    az_json_token_kind kind)
{
  az_span token = _get_remaining_json(json_parser);

  int32_t token_size = az_span_size(token);
  int32_t expected_literal_size = az_span_size(literal);

  // Return EOF because the token is smaller than the expected literal.
  if (token_size < expected_literal_size)
  {
    return AZ_ERROR_EOF;
  }

  token = az_span_slice(token, 0, expected_literal_size);
  if (az_span_is_content_equal(token, literal))
  {
    _az_json_parser_update_state(json_parser, kind, token, expected_literal_size);
    return AZ_OK;
  }
  return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
}

AZ_NODISCARD static az_result _az_json_parser_process_value(
    az_json_parser* json_parser,
    uint8_t next_byte)
{
  if (next_byte == '"')
    return _az_json_parser_process_string(json_parser);
  else if (next_byte == '{')
    return _az_json_parser_process_container_start(
        json_parser, AZ_JSON_TOKEN_BEGIN_OBJECT, _az_JSON_STACK_OBJECT);
  else if (next_byte == '[')
    return _az_json_parser_process_container_start(
        json_parser, AZ_JSON_TOKEN_BEGIN_ARRAY, _az_JSON_STACK_ARRAY);
  else if (isdigit(next_byte) || next_byte == '-')
    return _az_json_parser_process_number(json_parser);
  else if (next_byte == 'f')
    return _az_json_parser_process_literal(
        json_parser, AZ_SPAN_FROM_STR("false"), AZ_JSON_TOKEN_FALSE);
  else if (next_byte == 't')
    return _az_json_parser_process_literal(
        json_parser, AZ_SPAN_FROM_STR("true"), AZ_JSON_TOKEN_TRUE);
  else if (next_byte == 'n')
    return _az_json_parser_process_literal(
        json_parser, AZ_SPAN_FROM_STR("null"), AZ_JSON_TOKEN_NULL);
  else
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
}

AZ_NODISCARD static az_result _az_json_parser_read_first_token(
    az_json_parser* json_parser,
    az_span json,
    uint8_t first_byte)
{
  if (first_byte == '{')
  {
    _az_json_stack_push(&json_parser->_internal.bit_stack, _az_JSON_STACK_OBJECT);
    _az_json_parser_update_state(
        json_parser, AZ_JSON_TOKEN_BEGIN_OBJECT, az_span_slice(json, 0, 1), 1);
    json_parser->_internal.is_complex_json = true;
    return AZ_OK;
  }
  else if (first_byte == '[')
  {
    _az_json_stack_push(&json_parser->_internal.bit_stack, _az_JSON_STACK_ARRAY);
    _az_json_parser_update_state(
        json_parser, AZ_JSON_TOKEN_BEGIN_ARRAY, az_span_slice(json, 0, 1), 1);
    json_parser->_internal.is_complex_json = true;
    return AZ_OK;
  }
  else
  {
    return _az_json_parser_process_value(json_parser, first_byte);
  }
}

AZ_NODISCARD static az_result _az_json_parser_process_next_byte(
    az_json_parser* json_parser,
    uint8_t next_byte)
{
  // Extra data after a single JSON value (complete object or array or one primitive value) is
  // invalid. Expected end of data.
  if (json_parser->_internal.bit_stack._internal.current_depth == 0)
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }

  bool within_object
      = _az_json_stack_peek(&json_parser->_internal.bit_stack) == _az_JSON_STACK_OBJECT;

  if (next_byte == ',')
  {
    json_parser->_internal.bytes_consumed++;

    az_span json = _az_json_parser_skip_white_space(json_parser);

    // Expected start of a property name or value, but instead reached end of data.
    if (az_span_size(json) < 1)
    {
      return AZ_ERROR_EOF;
    }

    next_byte = az_span_ptr(json)[0];

    if (within_object)
    {
      // Expected start of a property name after the comma since we are within a JSON object.
      if (next_byte != '"')
      {
        return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
      }
      return _az_json_parser_process_property_name(json_parser);
    }
    else
    {
      return _az_json_parser_process_value(json_parser, next_byte);
    }
  }
  else if (next_byte == '}')
  {
    return _az_json_parser_process_container_end(json_parser, AZ_JSON_TOKEN_END_OBJECT);
  }
  else if (next_byte == ']')
  {
    return _az_json_parser_process_container_end(json_parser, AZ_JSON_TOKEN_END_ARRAY);
  }
  else
  {
    // No other character is a valid token delimiter within JSON.
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
}

AZ_NODISCARD az_result az_json_parser_move_to_next_token(az_json_parser* json_parser)
{
  _az_PRECONDITION_NOT_NULL(json_parser);

  az_span json = _az_json_parser_skip_white_space(json_parser);

  // An empty JSON payload is invalid.
  // TODO: Should this return invalid state instead?
  if (az_span_size(json) < 1)
  {
    return AZ_ERROR_EOF;
  }

  uint8_t first_byte = az_span_ptr(json)[0];

  switch (json_parser->token.kind)
  {
    case AZ_JSON_TOKEN_NONE:
      return _az_json_parser_read_first_token(json_parser, json, first_byte);
    case AZ_JSON_TOKEN_BEGIN_OBJECT:
    {
      if (first_byte == '}')
      {
        return _az_json_parser_process_container_end(json_parser, AZ_JSON_TOKEN_END_OBJECT);
      }
      else
      {
        // We expect the start of a property name as the first non-white-space character within a
        // JSON object.
        if (first_byte != '"')
        {
          return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
        }
        return _az_json_parser_process_property_name(json_parser);
      }
    }
    case AZ_JSON_TOKEN_BEGIN_ARRAY:
    {
      if (first_byte == ']')
      {
        return _az_json_parser_process_container_end(json_parser, AZ_JSON_TOKEN_END_ARRAY);
      }
      else
      {
        return _az_json_parser_process_value(json_parser, first_byte);
      }
    }
    case AZ_JSON_TOKEN_PROPERTY_NAME:
      return _az_json_parser_process_value(json_parser, first_byte);
    case AZ_JSON_TOKEN_END_OBJECT:
    case AZ_JSON_TOKEN_END_ARRAY:
    case AZ_JSON_TOKEN_STRING:
    case AZ_JSON_TOKEN_NUMBER:
    case AZ_JSON_TOKEN_TRUE:
    case AZ_JSON_TOKEN_FALSE:
    case AZ_JSON_TOKEN_NULL:
      return _az_json_parser_process_next_byte(json_parser, first_byte);
    default:
      return AZ_ERROR_JSON_INVALID_STATE;
  }
}

AZ_NODISCARD az_result az_json_parser_skip_children(az_json_parser* json_parser)
{
  _az_PRECONDITION_NOT_NULL(json_parser);

  if (json_parser->token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
  {
    AZ_RETURN_IF_FAILED(az_json_parser_move_to_next_token(json_parser));
  }

  az_json_token_kind token_kind = json_parser->token.kind;
  if (token_kind == AZ_JSON_TOKEN_BEGIN_OBJECT || token_kind == AZ_JSON_TOKEN_BEGIN_ARRAY)
  {
    // Keep moving the parser until we come back to the same depth.
    int32_t depth = json_parser->_internal.bit_stack._internal.current_depth;
    do
    {
      AZ_RETURN_IF_FAILED(az_json_parser_move_to_next_token(json_parser));
    } while (depth < json_parser->_internal.bit_stack._internal.current_depth);
  }
  return AZ_OK;
}
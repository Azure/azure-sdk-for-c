// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json.h>

#include "az_json_private.h"
#include "az_json_string_private.h"
#include "az_span_private.h"
#include <az_span.h>

#include <az_precondition.h>

#include <ctype.h>
#include <math.h>

#include <_az_cfg.h>

/**
 * @brief check if @p c is either an 'e' or an 'E'. This is a helper function to handle exponential
 * numbers like 10e10
 *
 */
AZ_NODISCARD AZ_INLINE bool az_json_is_e(uint8_t c)
{
  switch (c)
  {
    case 'e':
    case 'E':
      return true;
  }
  return false;
}

// 18 decimal digits. 10^18 - 1.
//                        0         1
//                        012345678901234567
#define AZ_DEC_NUMBER_MAX 999999999999999999ull

typedef struct
{
  int sign;
  uint64_t value;
  bool remainder;
  int16_t exp;
} _az_dec_number;

/* Calculate 10 ^ exp with O(log exp) by doing incremental multiplication
 If result goes beyone double limits (Overflow), infinite is returned based on standard IEEE_754
 https://en.wikipedia.org/wiki/IEEE_754.
 If Underflow, 0 is returned
*/
AZ_NODISCARD static double _ten_to_exp(int16_t exp)
{
  double result = 1;
  double incrementing_base = 10;
  int16_t abs_exp = (int16_t)(exp < 0 ? -exp : exp);

  while (abs_exp > 0)
  {
    // odd exp would update result to current incremented base
    if (abs_exp & 1)
    {
      result = exp < 0 ? result / incrementing_base : result * incrementing_base;
    }

    abs_exp /= 2;
    incrementing_base = incrementing_base * incrementing_base;
  }
  return result;
}

// double result follows IEEE_754 https://en.wikipedia.org/wiki/IEEE_754
static AZ_NODISCARD az_result _az_json_number_to_double(_az_dec_number const* p, double* out)
{
  *out = (double)p->value * _ten_to_exp(p->exp) * (double)p->sign;
  return AZ_OK;
}

AZ_NODISCARD static az_result az_span_reader_get_json_number_int(
    az_span* self,
    _az_dec_number* p_n,
    int16_t e_offset,
    uint8_t first)
{
  uint8_t c = first;
  // read an integer part of the number
  while (true)
  {
    uint64_t d = (uint64_t)(c - '0');
    if (p_n->value <= (AZ_DEC_NUMBER_MAX - d) / 10)
    {
      p_n->value = p_n->value * 10 + d;
      p_n->exp = (int16_t)((p_n->exp + e_offset) & 0xFFFF);
    }
    else
    {
      if (d != 0)
      {
        p_n->remainder = true;
      }
      p_n->exp = (int16_t)((p_n->exp + e_offset + 1) & 0xFFFF);
    }
    *self = az_span_slice_to_end(*self, 1);
    if (az_span_size(*self) == 0)
    {
      return AZ_OK; // end of reader is fine. Means int number is over
    }
    c = az_span_ptr(*self)[0];
    if (!isdigit(c))
    {
      return AZ_OK;
    }
  };
}

AZ_NODISCARD static az_result az_span_reader_get_json_number_digit_rest(
    az_span* self,
    double* out_value)
{
  _az_dec_number i = {
    .sign = 1,
    .value = 0,
    .remainder = false,
    .exp = 0,
  };

  // integer part
  {
    uint8_t o = az_span_ptr(*self)[0];
    if (o == '-')
    {
      i.sign = -1;
      *self = az_span_slice_to_end(*self, 1);
      if (az_span_size(*self) == 0)
      {
        return AZ_ERROR_EOF;
      }
      o = az_span_ptr(*self)[0];
      if (!isdigit(o))
      {
        return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
      }
    }
    if (o != '0')
    {
      AZ_RETURN_IF_FAILED(az_span_reader_get_json_number_int(self, &i, 0, o));
    }
    else
    {
      *self = az_span_slice_to_end(*self, 1);
    }
  }
  if (az_span_size(*self) == 0)
  {
    AZ_RETURN_IF_FAILED(_az_json_number_to_double(&i, out_value));
    return AZ_OK; // it's fine is int finish here (no fraction or something else)
  }

  // fraction
  if (az_span_ptr(*self)[0] == '.')
  {
    *self = az_span_slice_to_end(*self, 1);
    if (az_span_size(*self) == 0)
    {
      return AZ_ERROR_EOF; // uncompleted number
    }
    uint8_t o = az_span_ptr(*self)[0];
    if (!isdigit(o))
    {
      return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
    }
    AZ_RETURN_IF_FAILED(az_span_reader_get_json_number_int(self, &i, -1, o));
  }

  if (az_span_size(*self) == 0)
  {
    AZ_RETURN_IF_FAILED(_az_json_number_to_double(&i, out_value));
    return AZ_OK; // fine if number ends after a fraction
  }

  // exp
  if (az_json_is_e(az_span_ptr(*self)[0]))
  {
    // skip 'e' or 'E'
    *self = az_span_slice_to_end(*self, 1);
    if (az_span_size(*self) == 0)
    {
      return AZ_ERROR_EOF; // mising expo info
    }
    uint8_t c = az_span_ptr(*self)[0];

    // read sign, if any.
    int8_t e_sign = 1;
    switch (c)
    {
      case '-':
        e_sign = -1;
        _az_FALLTHROUGH;
      case '+':
        *self = az_span_slice_to_end(*self, 1);
        if (az_span_size(*self) == 0)
        {
          return AZ_ERROR_EOF; // uncompleted exp data
        }
        c = az_span_ptr(*self)[0];
    }

    // expect at least one digit.
    if (!isdigit(c))
    {
      return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
    }

    int16_t e_int = 0;
    do
    {
      e_int = (int16_t)((e_int * 10 + (int16_t)(c - '0')) & 0xFFFF);
      *self = az_span_slice_to_end(*self, 1);
      if (az_span_size(*self) == 0)
      {
        break; // nothing more to read
      }
      c = az_span_ptr(*self)[0];
    } while (isdigit(c));
    i.exp = (int16_t)((i.exp + (e_int * e_sign)) & 0xFFFF);
  }

  AZ_RETURN_IF_FAILED(_az_json_number_to_double(&i, out_value));
  return AZ_OK;
}

AZ_NODISCARD static az_result az_span_reader_get_json_string_rest(az_span* self, az_span* string)
{
  // skip '"'
  int32_t reader_initial_length = az_span_size(*self);
  uint8_t* p_reader = az_span_ptr(*self);
  while (true)
  {
    uint32_t ignore = { 0 };
    az_result const result = _az_span_reader_read_json_string_char(self, &ignore);
    switch (result)
    {
      case AZ_ERROR_JSON_STRING_END:
      {
        int32_t read_count = reader_initial_length - az_span_size(*self);
        *string = az_span_init(p_reader, read_count);
        *self = az_span_slice_to_end(*self, 1);
        return AZ_OK;
      }
      case AZ_ERROR_ITEM_NOT_FOUND:
      {
        return AZ_ERROR_EOF;
      }
      default:
      {
        AZ_RETURN_IF_FAILED(result);
      }
    }
  }
}

// _value_
AZ_NODISCARD static az_result az_json_parser_get_value(
    az_json_parser* json_parser,
    az_json_token* out_token)
{
  az_span* p_reader = &json_parser->_internal.reader;

  if (az_span_size(*p_reader) == 0)
  {
    return AZ_ERROR_EOF;
  }

  uint8_t c = az_span_ptr(*p_reader)[0];
  if (isdigit(c))
  {
    out_token->kind = AZ_JSON_TOKEN_NUMBER;
    return az_span_reader_get_json_number_digit_rest(p_reader, &out_token->_internal.number);
  }
  switch (c)
  {
    case 't':
      out_token->kind = AZ_JSON_TOKEN_TRUE;
      out_token->_internal.boolean = true;
      return _az_is_expected_span(p_reader, AZ_SPAN_FROM_STR("true"));
    case 'f':
      out_token->kind = AZ_JSON_TOKEN_FALSE;
      out_token->_internal.boolean = false;
      return _az_is_expected_span(p_reader, AZ_SPAN_FROM_STR("false"));
    case 'n':
      out_token->kind = AZ_JSON_TOKEN_NULL;
      return _az_is_expected_span(p_reader, AZ_SPAN_FROM_STR("null"));
    case '"':
      out_token->kind = AZ_JSON_TOKEN_STRING;
      *p_reader = az_span_slice_to_end(*p_reader, 1);
      return az_span_reader_get_json_string_rest(p_reader, &out_token->_internal.string);
    case '-':
      out_token->kind = AZ_JSON_TOKEN_NUMBER;
      return az_span_reader_get_json_number_digit_rest(p_reader, &out_token->_internal.number);
    case '{':
      out_token->kind = AZ_JSON_TOKEN_BEGIN_OBJECT;
      *p_reader = az_span_slice_to_end(*p_reader, 1);
      _az_json_stack_push(&json_parser->_internal.bit_stack, _az_JSON_STACK_OBJECT);
      return AZ_OK;
    case '[':
      out_token->kind = AZ_JSON_TOKEN_BEGIN_ARRAY;
      *p_reader = az_span_slice_to_end(*p_reader, 1);
      _az_json_stack_push(&json_parser->_internal.bit_stack, _az_JSON_STACK_ARRAY);
      return AZ_OK;
  }
  return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
}

AZ_NODISCARD static az_result az_json_parser_get_value_space(
    az_json_parser* p_state,
    az_json_token* out_token)
{
  AZ_RETURN_IF_FAILED(az_json_parser_get_value(p_state, out_token));
  if (az_span_size(p_state->_internal.reader) > 0)
  {
    p_state->_internal.reader = _az_span_trim_white_space_from_start(p_state->_internal.reader);
  }
  return AZ_OK;
}

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

AZ_NODISCARD static az_result _az_json_parser_process_container_end(
    az_json_parser* json_parser,
    az_json_token_kind token_kind)
{
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
  while (true)
  {
    if (next_byte == '"')
    {
      break;
    }
    else if (next_byte == '\\')
    {
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
        if (string_length >= remaining_size - 4)
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
    if (consumed_count >= token_size)
    {
      return AZ_ERROR_EOF;
    }

    next_byte = next_byte_ptr[consumed_count];
    if (!isdigit(next_byte))
    {
      return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
    }
  }

  if (next_byte == '0')
  {
    consumed_count++;

    if (consumed_count >= token_size)
    {
      // If there is no more JSON, this is a valid end state only when the JSON payload contains a
      // single value: "[-]0"
      // Otherwise, the payload is incomplete and ending too early.
      return json_parser->_internal.is_complex_json ? AZ_ERROR_EOF : AZ_OK;
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
    // Integer part before decimal
    consumed_count += _az_json_parser_consume_digits(az_span_slice_to_end(token, consumed_count));

    if (consumed_count >= token_size)
    {
      // If there is no more JSON, this is a valid end state only when the JSON payload contains a
      // single value: "[-][digits]"
      // Otherwise, the payload is incomplete and ending too early.
      return json_parser->_internal.is_complex_json ? AZ_ERROR_EOF : AZ_OK;
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
    // Integer part after decimal
    consumed_count += _az_json_parser_consume_digits(az_span_slice_to_end(token, consumed_count));

    if (consumed_count >= token_size)
    {
      // If there is no more JSON, this is a valid end state only when the JSON payload contains a
      // single value: "[-][digits].[digits]"
      // Otherwise, the payload is incomplete and ending too early.
      return json_parser->_internal.is_complex_json ? AZ_ERROR_EOF : AZ_OK;
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
  if (next_byte != '-' && next_byte != '+')
  {
    consumed_count++;

    // A sign must be followed by at least one digit.
    if (consumed_count >= token_size)
    {
      return AZ_ERROR_EOF;
    }
  }

  // Integer part after the 'e'/'E'
  consumed_count += _az_json_parser_consume_digits(az_span_slice_to_end(token, consumed_count));

  if (consumed_count >= token_size)
  {
    // If there is no more JSON, this is a valid end state only when the JSON payload contains a
    // single value: "[-][digits].[digits]e[+|-][digits]"
    // Otherwise, the payload is incomplete and ending too early.
    return json_parser->_internal.is_complex_json ? AZ_ERROR_EOF : AZ_OK;
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
  if (token_size <= expected_literal_size)
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
  switch (next_byte)
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
}

AZ_NODISCARD static az_result _az_json_parser_read_first_token(
    az_json_parser* json_parser,
    az_span json)
{
  uint8_t first = az_span_ptr(json)[0];

  if (first == '{')
  {
    _az_json_stack_push(&json_parser->_internal.bit_stack, _az_JSON_STACK_OBJECT);
    _az_json_parser_update_state(
        json_parser, AZ_JSON_TOKEN_BEGIN_OBJECT, az_span_slice(json, 0, 1), 1);
    json_parser->_internal.is_complex_json = true;
    return AZ_OK;
  }
  else if (first == '[')
  {
    _az_json_stack_push(&json_parser->_internal.bit_stack, _az_JSON_STACK_ARRAY);
    _az_json_parser_update_state(
        json_parser, AZ_JSON_TOKEN_BEGIN_ARRAY, az_span_slice(json, 0, 1), 1);
    json_parser->_internal.is_complex_json = true;
    return AZ_OK;
  }
  else
  {
    return _az_json_parser_process_value(json_parser, first);
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

  if (next_byte == ',')
  {

  }
  else if (next_byte == '}')
  {

  }
  else if (next_byte == ']')
  {

  }
  else 
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
}

AZ_NODISCARD az_result az_json_parser_move_to_next_token(az_json_parser* json_parser)
{
  az_PRECONDITION_NOT_NULL(json_parser);

  az_span json = _az_span_trim_white_space_from_start(json_parser->_internal.json_buffer);

  // Find out how many whitespace characters were trimmed.
  json_parser->_internal.bytes_consumed
      += az_json_size(json_parser->_internal.json_buffer) - az_json_size(json);

  // An empty JSON payload is invalid.
  if (az_span_size(json) < 1)
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  uint8_t first_byte = az_span_ptr(json)[0];

  switch (json_parser->token.kind)
  {
    case AZ_JSON_TOKEN_NONE:
      return _az_json_parser_read_first_token(json_parser, json);
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

  // An empty JSON payload is invalid.
  if (az_span_size(json) < 1)
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  if (!az_json_parser_stack_is_empty(json_parser))
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }
}

AZ_NODISCARD az_result
az_json_parser_parse_token(az_json_parser* json_parser, az_json_token* out_token)
{
  _az_PRECONDITION_NOT_NULL(json_parser);
  _az_PRECONDITION_NOT_NULL(out_token);

  if (!az_json_parser_stack_is_empty(json_parser))
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }
  az_span* p_reader = &json_parser->_internal.reader;
  *p_reader = _az_span_trim_white_space_from_start(*p_reader);
  AZ_RETURN_IF_FAILED(az_json_parser_get_value_space(json_parser, out_token));
  bool const is_empty = az_span_size(*p_reader) == 0; // everything was read
  switch (out_token->kind)
  {
    case AZ_JSON_TOKEN_BEGIN_ARRAY:
    case AZ_JSON_TOKEN_BEGIN_OBJECT:
      return is_empty ? AZ_ERROR_EOF : AZ_OK;
    default:
      break;
  }
  return is_empty ? AZ_OK : AZ_ERROR_PARSER_UNEXPECTED_CHAR;
}

AZ_NODISCARD AZ_INLINE uint8_t az_json_stack_item_to_close(_az_json_stack_item item)
{
  return item == _az_JSON_STACK_OBJECT ? '}' : ']';
}

AZ_NODISCARD static az_result az_json_parser_read_comma_or_close(az_json_parser* json_parser)
{
  az_span* p_reader = &json_parser->_internal.reader;
  uint8_t const c = az_span_ptr(*p_reader)[0];
  if (c == ',')
  {
    // skip ',' and read all whitespaces.
    *p_reader = az_span_slice_to_end(*p_reader, 1);
    *p_reader = _az_span_trim_white_space_from_start(*p_reader);
    return AZ_OK;
  }
  uint8_t const close = az_json_stack_item_to_close(az_json_parser_stack_last(json_parser));
  if (c != close)
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
  return AZ_OK;
}

AZ_NODISCARD static az_result az_json_parser_check_item_begin(
    az_json_parser* json_parser,
    _az_json_stack_item stack_item)
{
  if (az_json_parser_stack_is_empty(json_parser)
      || az_json_parser_stack_last(json_parser) != stack_item)
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }
  az_span* p_reader = &json_parser->_internal.reader;
  if (az_span_size(*p_reader) == 0)
  {
    return AZ_ERROR_EOF;
  }
  uint8_t const c = az_span_ptr(*p_reader)[0];
  if (c != az_json_stack_item_to_close(stack_item))
  {
    return AZ_OK;
  }
  // c == close
  AZ_RETURN_IF_FAILED(az_json_parser_pop_stack(json_parser));
  *p_reader = az_span_slice_to_end(*p_reader, 1);
  *p_reader = _az_span_trim_white_space_from_start(*p_reader);

  if (!az_json_parser_stack_is_empty(json_parser))
  {
    AZ_RETURN_IF_FAILED(az_json_parser_read_comma_or_close(json_parser));
  }
  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_NODISCARD static az_result az_json_parser_check_item_end(
    az_json_parser* json_parser,
    az_json_token value)
{
  switch (value.kind)
  {
    case AZ_JSON_TOKEN_BEGIN_OBJECT:
    case AZ_JSON_TOKEN_BEGIN_ARRAY:
      return AZ_OK;
    default:
      break;
  }
  return az_json_parser_read_comma_or_close(json_parser);
}

AZ_NODISCARD az_result az_json_parser_parse_token_member(
    az_json_parser* json_parser,
    az_json_token_member* out_token_member)
{
  _az_PRECONDITION_NOT_NULL(json_parser);
  _az_PRECONDITION_NOT_NULL(out_token_member);

  az_span* p_reader = &json_parser->_internal.reader;
  AZ_RETURN_IF_FAILED(az_json_parser_check_item_begin(json_parser, _az_JSON_STACK_OBJECT));
  AZ_RETURN_IF_FAILED(_az_is_expected_span(p_reader, AZ_SPAN_FROM_STR("\"")));
  AZ_RETURN_IF_FAILED(az_span_reader_get_json_string_rest(p_reader, &out_token_member->name));
  *p_reader = _az_span_trim_white_space(*p_reader);
  AZ_RETURN_IF_FAILED(_az_is_expected_span(p_reader, AZ_SPAN_FROM_STR(":")));
  *p_reader = _az_span_trim_white_space(*p_reader);
  AZ_RETURN_IF_FAILED(az_json_parser_get_value_space(json_parser, &out_token_member->token));
  return az_json_parser_check_item_end(json_parser, out_token_member->token);
}

AZ_NODISCARD az_result
az_json_parser_parse_array_item(az_json_parser* json_parser, az_json_token* out_token)
{
  _az_PRECONDITION_NOT_NULL(json_parser);
  _az_PRECONDITION_NOT_NULL(out_token);

  AZ_RETURN_IF_FAILED(az_json_parser_check_item_begin(json_parser, _az_JSON_STACK_ARRAY));
  AZ_RETURN_IF_FAILED(az_json_parser_get_value_space(json_parser, out_token));
  return az_json_parser_check_item_end(json_parser, *out_token);
}

AZ_NODISCARD az_result az_json_parser_done(az_json_parser* json_parser)
{
  _az_PRECONDITION_NOT_NULL(json_parser);

  if (az_span_size(json_parser->_internal.reader) > 0
      || !az_json_parser_stack_is_empty(json_parser))
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_parser_skip_children(az_json_parser* json_parser, az_json_token token)
{
  _az_PRECONDITION_NOT_NULL(json_parser);

  switch (token.kind)
  {
    case AZ_JSON_TOKEN_BEGIN_OBJECT:
    case AZ_JSON_TOKEN_BEGIN_ARRAY:
    {
      break;
    }
    default:
    {
      return AZ_OK;
    }
  }

  _az_json_stack target_stack = json_parser->_internal.stack;
  AZ_RETURN_IF_FAILED(az_json_stack_pop(&target_stack));

  while (true)
  {
    // az_json_parser_get_stack
    switch (az_json_parser_stack_last(json_parser))
    {
      case _az_JSON_STACK_OBJECT:
      {
        az_json_token_member member = { 0 };
        az_result const result = az_json_parser_parse_token_member(json_parser, &member);
        if (result != AZ_ERROR_ITEM_NOT_FOUND)
        {
          AZ_RETURN_IF_FAILED(result);
        }
        break;
      }
      default:
      {
        az_json_token element = _az_JSON_TOKEN_DEFAULT;
        az_result result = az_json_parser_parse_array_item(json_parser, &element);
        if (result != AZ_ERROR_ITEM_NOT_FOUND)
        {
          AZ_RETURN_IF_FAILED(result);
        }
        break;
      }
    }
    if (json_parser->_internal.stack == target_stack)
    {
      return AZ_OK;
    }
  }
}

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

AZ_NODISCARD AZ_INLINE bool az_json_parser_stack_is_empty(az_json_parser const* json_parser)
{
  return json_parser->_internal.stack == 1;
}

AZ_NODISCARD AZ_INLINE _az_json_stack_item
az_json_parser_stack_last(az_json_parser const* json_parser)
{
  return json_parser->_internal.stack & 1;
}

AZ_NODISCARD AZ_INLINE az_result
az_json_parser_push_stack(az_json_parser* json_parser, _az_json_stack json_stack)
{
  if (json_parser->_internal.stack >> (_az_MAX_JSON_STACK_SIZE - 1) != 0)
  {
    return AZ_ERROR_JSON_NESTING_OVERFLOW;
  }
  json_parser->_internal.stack = (json_parser->_internal.stack << 1) | json_stack;
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result az_json_stack_pop(_az_json_stack* json_stack)
{
  if (*json_stack <= 1)
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }
  *json_stack >>= 1;
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result az_json_parser_pop_stack(az_json_parser* json_parser)
{
  return az_json_stack_pop(&json_parser->_internal.stack);
}

AZ_NODISCARD az_result az_json_parser_init(az_json_parser* json_parser, az_span json_buffer)
{
  *json_parser = (az_json_parser){ ._internal = { .reader = json_buffer, .stack = 1 } };
  return AZ_OK;
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
      return az_json_parser_push_stack(json_parser, _az_JSON_STACK_OBJECT);
    case '[':
      out_token->kind = AZ_JSON_TOKEN_BEGIN_ARRAY;
      *p_reader = az_span_slice_to_end(*p_reader, 1);
      return az_json_parser_push_stack(json_parser, _az_JSON_STACK_ARRAY);
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
        az_json_token element = { 0 };
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

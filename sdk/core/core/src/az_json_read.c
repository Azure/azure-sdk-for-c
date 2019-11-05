// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_read.h>

#include <az_contract.h>

#include <ctype.h>
#include <math.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE bool az_json_is_white_space(az_result_byte const c) {
  switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      return true;
  }
  return false;
}

AZ_NODISCARD AZ_INLINE bool az_json_is_esc(az_result_byte const c) {
  switch (c) {
    case '\\':
    case '"':
    case '/':
    case 'b':
    case 'f':
    case 'n':
    case 'r':
    case 't':
      return true;
  }
  return false;
}

AZ_NODISCARD AZ_INLINE bool az_json_is_e(az_result_byte const c) {
  switch (c) {
    case 'e':
    case 'E':
      return true;
  }
  return false;
}

AZ_NODISCARD AZ_INLINE bool az_json_stack_is_empty(az_json_state const * const p) {
  return p->stack == 1;
}

AZ_NODISCARD AZ_INLINE az_json_stack_item az_json_stack_last(az_json_state const * const p) {
  return p->stack & 1;
}

AZ_NODISCARD AZ_INLINE az_result
az_json_stack_push(az_json_state * const p_state, az_json_stack const stack) {
  if (p_state->stack >> AZ_JSON_STACK_SIZE != 0) {
    return AZ_ERROR_JSON_STACK_OVERFLOW;
  }
  p_state->stack = (p_state->stack << 1) | stack;
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result az_json_stack_pop(az_json_state * const p_state) {
  if (p_state->stack <= 1) {
    return AZ_ERROR_JSON_INVALID_STATE;
  }
  p_state->stack >>= 1;
  return AZ_OK;
}

AZ_NODISCARD az_json_state az_json_state_create(az_span const buffer) {
  return (az_json_state){
    .reader = az_span_reader_create(buffer),
    .stack = 1,
  };
}

static void az_json_read_white_space(az_span_reader * const p_reader) {
  while (az_json_is_white_space(az_span_reader_current(p_reader))) {
    az_span_reader_next(p_reader);
  }
}

AZ_NODISCARD static az_result az_json_read_keyword_rest(
    az_span_reader * const p_reader,
    az_span const keyword) {
  az_span_reader_next(p_reader);
  az_span_reader k = az_span_reader_create(keyword);
  while (true) {
    az_result_byte const ko = az_span_reader_current(&k);
    if (ko == AZ_ERROR_EOF) {
      return AZ_OK;
    }
    az_result_byte const o = az_span_reader_current(p_reader);
    if (o != ko) {
      return az_error_unexpected_char(o);
    }
    az_span_reader_next(p_reader);
    az_span_reader_next(&k);
  }
}

// 18 decimal digits. 10^18 - 1.
//                        0         1
//                        012345678901234567
#define AZ_DEC_NUMBER_MAX 999999999999999999ull

typedef struct {
  int sign;
  uint64_t value;
  bool remainder;
  int16_t exp;
} az_dec_number;

AZ_NODISCARD static double az_json_number_to_double(az_dec_number const * p) {
  return p->value * pow(10, p->exp) * p->sign;
}

AZ_NODISCARD static az_result az_json_number_int_parse(
    az_span_reader * const p_reader,
    az_dec_number * const p_n,
    int16_t const e_offset,
    az_result_byte const first) {
  az_result_byte c = first;
  // read an integer part of the number
  while (true) {
    int d = c - '0';
    if (p_n->value <= (AZ_DEC_NUMBER_MAX - d) / 10) {
      p_n->value = p_n->value * 10 + d;
      p_n->exp += e_offset;
    } else {
      if (d != 0) {
        p_n->remainder = true;
      }
      p_n->exp += e_offset + 1;
    }
    az_span_reader_next(p_reader);
    c = az_span_reader_current(p_reader);
    if (!isdigit(c)) {
      return AZ_OK;
    }
  };
}

AZ_NODISCARD static az_result az_json_read_number_digit_rest(
    az_span_reader * const p_reader,
    double * const out_value) {
  az_dec_number i = {
    .sign = 1,
    .value = 0,
    .remainder = false,
    .exp = 0,
  };

  // integer part
  {
    az_result_byte o = az_span_reader_current(p_reader);
    if (o == '-') {
      i.sign = -1;
      az_span_reader_next(p_reader);
      o = az_span_reader_current(p_reader);
      if (!isdigit(o)) {
        return az_error_unexpected_char(o);
      }
    }
    if (o != '0') {
      AZ_RETURN_IF_FAILED(az_json_number_int_parse(p_reader, &i, 0, o));
    } else {
      az_span_reader_next(p_reader);
    }
  }

  // fraction
  if (az_span_reader_current(p_reader) == '.') {
    az_span_reader_next(p_reader);
    az_result_byte o = az_span_reader_current(p_reader);
    if (!isdigit(o)) {
      return az_error_unexpected_char(o);
    }
    AZ_RETURN_IF_FAILED(az_json_number_int_parse(p_reader, &i, -1, o));
  }

  // exp
  if (az_json_is_e(az_span_reader_current(p_reader))) {
    // skip 'e' or 'E'
    az_span_reader_next(p_reader);
    az_result_byte c = az_span_reader_current(p_reader);

    // read sign, if any.
    int8_t e_sign = 1;
    switch (c) {
      case '-':
        e_sign = -1;
        AZ_FALLTHROUGH;
      case '+':
        az_span_reader_next(p_reader);
        c = az_span_reader_current(p_reader);
    }

    // expect at least one digit.
    if (!isdigit(c)) {
      return az_error_unexpected_char(c);
    }

    int16_t e_int = 0;
    do {
      e_int = e_int * 10 + (int16_t)(c - '0');
      az_span_reader_next(p_reader);
      c = az_span_reader_current(p_reader);
    } while (isdigit(c));
    i.exp += e_int * e_sign;
  }

  *out_value = az_json_number_to_double(&i);
  return AZ_OK;
}

AZ_NODISCARD static az_result az_json_read_string_rest(
    az_span_reader * const p_reader,
    az_span * const string) {
  // skip '"'
  size_t const begin = p_reader->i;
  while (true) {
    az_result_byte c = az_span_reader_current(p_reader);
    switch (c) {
        // end of the string
      case '"': {
        *string = az_span_sub(p_reader->span, begin, p_reader->i);
        az_span_reader_next(p_reader);
        return AZ_OK;
      }
        // escape sequence
      case '\\': {
        az_span_reader_next(p_reader);
        c = az_span_reader_current(p_reader);
        if (az_json_is_esc(c)) {
          az_span_reader_next(p_reader);
        } else if (c == 'u') {
          az_span_reader_next(p_reader);
          for (size_t i = 0; i < 4; ++i, az_span_reader_next(p_reader)) {
            c = az_span_reader_current(p_reader);
            if (!isxdigit(c)) {
              return az_error_unexpected_char(c);
            }
          }
        } else {
          return az_error_unexpected_char(c);
        }
        break;
      }
      default: {
        if (c < 0x20) {
          return az_error_unexpected_char(c);
        }
        az_span_reader_next(p_reader);
      }
    }
  }
}

// _value_
AZ_NODISCARD static az_result az_json_read_value(
    az_json_state * const p_state,
    az_json_value * const out_value) {
  az_span_reader * const p_reader = &p_state->reader;
  az_result_byte const c = az_span_reader_current(p_reader);
  if (isdigit(c)) {
    out_value->kind = AZ_JSON_VALUE_NUMBER;
    return az_json_read_number_digit_rest(p_reader, &out_value->data.number);
  }
  switch (c) {
    case 't':
      out_value->kind = AZ_JSON_VALUE_BOOLEAN;
      out_value->data.boolean = true;
      return az_json_read_keyword_rest(p_reader, AZ_STR("rue"));
    case 'f':
      out_value->kind = AZ_JSON_VALUE_BOOLEAN;
      out_value->data.boolean = false;
      return az_json_read_keyword_rest(p_reader, AZ_STR("alse"));
    case 'n':
      out_value->kind = AZ_JSON_VALUE_NULL;
      return az_json_read_keyword_rest(p_reader, AZ_STR("ull"));
    case '"':
      out_value->kind = AZ_JSON_VALUE_STRING;
      az_span_reader_next(p_reader);
      return az_json_read_string_rest(p_reader, &out_value->data.string);
    case '-':
      out_value->kind = AZ_JSON_VALUE_NUMBER;
      return az_json_read_number_digit_rest(p_reader, &out_value->data.number);
    case '{':
      out_value->kind = AZ_JSON_VALUE_OBJECT;
      az_span_reader_next(p_reader);
      return az_json_stack_push(p_state, AZ_JSON_STACK_OBJECT);
    case '[':
      out_value->kind = AZ_JSON_VALUE_ARRAY;
      az_span_reader_next(p_reader);
      return az_json_stack_push(p_state, AZ_JSON_STACK_ARRAY);
  }
  return az_error_unexpected_char(c);
}

AZ_NODISCARD static az_result az_json_read_value_space(
    az_json_state * const p_state,
    az_json_value * const out_value) {
  AZ_RETURN_IF_FAILED(az_json_read_value(p_state, out_value));
  az_json_read_white_space(&p_state->reader);
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_read(az_json_state * const p_state, az_json_value * const out_value) {
  AZ_CONTRACT_ARG_NOT_NULL(p_state);
  AZ_CONTRACT_ARG_NOT_NULL(out_value);

  if (!az_json_stack_is_empty(p_state)) {
    return AZ_ERROR_JSON_INVALID_STATE;
  }
  az_json_read_white_space(&p_state->reader);
  AZ_RETURN_IF_FAILED(az_json_read_value_space(p_state, out_value));
  bool const is_empty = az_span_reader_is_empty(&p_state->reader);
  switch (out_value->kind) {
    case AZ_JSON_VALUE_ARRAY:
    case AZ_JSON_VALUE_OBJECT:
      return is_empty ? AZ_ERROR_EOF : AZ_OK;
    default:
      break;
  }
  return is_empty ? AZ_OK : AZ_ERROR_PARSER_UNEXPECTED_CHAR;
}

AZ_NODISCARD AZ_INLINE uint8_t az_json_stack_item_to_close(az_json_stack_item const item) {
  return item == AZ_JSON_STACK_OBJECT ? '}' : ']';
}

AZ_NODISCARD static az_result az_json_read_comma_or_close(az_json_state * const p_state) {
  az_result_byte const c = az_span_reader_current(&p_state->reader);
  if (c == ',') {
    // skip ',' and read all whitespaces.
    az_span_reader_next(&p_state->reader);
    az_json_read_white_space(&p_state->reader);
    return AZ_OK;
  }
  uint8_t const close = az_json_stack_item_to_close(az_json_stack_last(p_state));
  if (c != close) {
    return az_error_unexpected_char(c);
  }
  return AZ_OK;
}

AZ_NODISCARD static az_result az_json_check_item_begin(
    az_json_state * const p_state,
    az_json_stack_item const stack_item) {
  if (az_json_stack_is_empty(p_state) || az_json_stack_last(p_state) != stack_item) {
    return AZ_ERROR_JSON_INVALID_STATE;
  }
  az_result_byte const c = az_span_reader_current(&p_state->reader);
  if (c != az_json_stack_item_to_close(stack_item)) {
    return AZ_OK;
  }
  // c == close
  AZ_RETURN_IF_FAILED(az_json_stack_pop(p_state));
  az_span_reader_next(&p_state->reader);
  az_json_read_white_space(&p_state->reader);
  if (!az_json_stack_is_empty(p_state)) {
    AZ_RETURN_IF_FAILED(az_json_read_comma_or_close(p_state));
  }
  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_NODISCARD static az_result az_json_check_item_end(
    az_json_state * const p_state,
    az_json_value const value) {
  switch (value.kind) {
    case AZ_JSON_VALUE_OBJECT:
    case AZ_JSON_VALUE_ARRAY:
      return AZ_OK;
    default:
      break;
  }
  return az_json_read_comma_or_close(p_state);
}

AZ_NODISCARD az_result
az_json_read_object_member(
    az_json_state * const p_state,
    az_json_member * const out_member) {
  AZ_CONTRACT_ARG_NOT_NULL(p_state);
  AZ_CONTRACT_ARG_NOT_NULL(out_member);

  az_span_reader * const p_reader = &p_state->reader;
  AZ_RETURN_IF_FAILED(az_json_check_item_begin(p_state, AZ_JSON_STACK_OBJECT));
  AZ_RETURN_IF_FAILED(az_span_reader_expect_char(p_reader, '"'));
  AZ_RETURN_IF_FAILED(az_json_read_string_rest(p_reader, &out_member->name));
  az_json_read_white_space(p_reader);
  AZ_RETURN_IF_FAILED(az_span_reader_expect_char(p_reader, ':'));
  az_json_read_white_space(p_reader);
  AZ_RETURN_IF_FAILED(az_json_read_value_space(p_state, &out_member->value));
  return az_json_check_item_end(p_state, out_member->value);
}

AZ_NODISCARD az_result
az_json_read_array_element(
    az_json_state * const p_state,
    az_json_value * const out_element) {
  AZ_CONTRACT_ARG_NOT_NULL(p_state);
  AZ_CONTRACT_ARG_NOT_NULL(out_element);

  AZ_RETURN_IF_FAILED(az_json_check_item_begin(p_state, AZ_JSON_STACK_ARRAY));
  AZ_RETURN_IF_FAILED(az_json_read_value_space(p_state, out_element));
  return az_json_check_item_end(p_state, *out_element);
}

AZ_NODISCARD az_result az_json_state_done(az_json_state const * const p_state) {
  AZ_CONTRACT_ARG_NOT_NULL(p_state);

  if (!az_span_reader_is_empty(&p_state->reader) || !az_json_stack_is_empty(p_state)) {
    return AZ_ERROR_JSON_INVALID_STATE;
  }
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_get_object_member_value(
    az_span const json,
    az_span const name,
    az_json_value * const out_value) {
  AZ_CONTRACT_ARG_NOT_NULL(out_value);
  AZ_CONTRACT_ARG_VALID_SPAN(json);
  AZ_CONTRACT_ARG_VALID_SPAN(name);

  az_json_state state = az_json_state_create(json);
  az_json_value value;
  AZ_RETURN_IF_FAILED(az_json_read(&state, &value));

  if (value.kind == AZ_JSON_VALUE_OBJECT) {
    az_json_member member;
    while (az_json_read_object_member(&state, &member) != AZ_ERROR_ITEM_NOT_FOUND) {
      if (az_span_eq(member.name, name)) {
        *out_value = member.value;
        return AZ_OK;
      }
    }
  }

  return AZ_ERROR_ITEM_NOT_FOUND;
}

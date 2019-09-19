// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_read.h>

#include <math.h>
#include <ctype.h>

inline bool az_json_is_white_space(char const c) {
  switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      return true;
  }
  return false;
}

inline bool az_json_is_esc(char const c) {
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

inline bool az_json_is_e(char const c) {
  switch (c) {
    case 'e':
    case 'E':
      return true;
  }
  return false;
}

inline bool az_json_stack_is_empty(az_json_state const *const p) {
  return p->stack == 1;
}

inline az_json_stack_item az_json_stack_last(az_json_state const *const p) {
  return p->stack & 1;
}

inline az_result az_json_stack_push(az_json_state* const p_state, az_json_stack const stack) {
  if (p_state->stack >> AZ_JSON_STACK_SIZE != 0) {
    return AZ_JSON_ERROR_STACK_OVERFLOW;
  }
  p_state->stack = (p_state->stack << 1) | stack;
  return AZ_OK;
}

inline az_result az_json_stack_pop(az_json_state* const p_state) {
  if (p_state->stack <= 1) {
    return AZ_JSON_ERROR_INVALID_STATE;
  }
  p_state->stack >>= 1;
  return AZ_OK;
}

az_json_state az_json_state_create(az_const_str const buffer) {
  return (az_json_state){
    .buffer = buffer,
    .i = 0,
    .stack = 1,
  };
}

az_result az_json_get_char(az_const_str const buffer, size_t const i, char *const out_char) {
  if (i == buffer.size) {
    return AZ_JSON_ERROR_UNEXPECTED_END;
  }
  *out_char = az_const_str_item(buffer, i);
  return AZ_OK;
}

az_result az_json_expect_char(az_const_str const buffer, size_t *const p_i, char const expected) {
  char c;
  AZ_RETURN_IF_NOT_OK(az_json_get_char(buffer, *p_i, &c));
  if (c != expected) {
    return AZ_JSON_ERROR_UNEXPECTED_CHAR;
  }
  *p_i += 1;
  return AZ_OK;
}

az_result az_json_read_white_space(az_const_str const buffer, size_t *const p) {
  for (; *p != buffer.size; *p += 1) {
    if (!az_json_is_white_space(az_const_str_item(buffer, *p))) {
      break;
    }
  }
  return AZ_OK;
}

az_result az_json_read_keyword_rest(
  az_const_str const buffer,
  size_t *const p,
  az_const_str const keyword
) {
  *p += 1;
  for (size_t k = 0; k != keyword.size; ++*p, ++k) {
    if (*p == buffer.size) {
      return AZ_JSON_ERROR_UNEXPECTED_END;
    }
    if (az_const_str_item(buffer, *p) != az_const_str_item(keyword, k)) {
      return AZ_JSON_ERROR_UNEXPECTED_CHAR;
    }
  }
  return AZ_OK;
}

// 17 decimal digits. 10^17 - 1.
//                        01234567890123456
#define AZ_DEC_NUMBER_MAX 99999999999999999ull

typedef struct {
  int sign;
  uint64_t value;
  int16_t exp;
} az_dec_number;

double az_json_number_to_double(az_dec_number const* p) {
  return p->value * pow(10, p->exp) * p->sign;
}

az_result az_json_number_int_parse(
  az_const_str const buffer, 
  size_t *const p, 
  az_dec_number *const p_n,
  int16_t const e_offset, 
  char const first
) {
  char c = first;
  // read an integer part of the number
  do {
    int d = c - '0';
    if (p_n->value <= (AZ_DEC_NUMBER_MAX - d) / 10) {
      p_n->value = p_n->value * 10 + d;
      p_n->exp += e_offset;
    }
    else {
      p_n->exp += e_offset + 1;
    }
    *p += 1;
    if (*p == buffer.size) {
      break;
    }
    c = az_const_str_item(buffer, *p);
  } while (isdigit(c));
  return AZ_OK;
}

az_result az_json_read_number_digit_rest(
  az_const_str const buffer,
  size_t *const p,
  double *const out_value
) {
  az_dec_number i = { 
    .sign = 1,
    .value = 0,
    .remainder = false,
    .exp = 0,
  };

  // integer part
  {
    char c = az_const_str_item(buffer, *p);
    if (c == '-') {
      i.sign = -1;
      *p += 1;
      if (*p == buffer.size) {
        return AZ_JSON_ERROR_UNEXPECTED_END;
      }
      c = az_const_str_item(buffer, *p);
      if (!isdigit(c)) {
        return AZ_JSON_ERROR_UNEXPECTED_CHAR;
      }
    }
    if (c != '0') {
      AZ_RETURN_IF_NOT_OK(az_json_number_int_parse(buffer, p, &i, 0, c));
    } else {
      *p += 1;
    }
  }

  // fraction
  if (*p != buffer.size && az_const_str_item(buffer, *p) == '.') {
    *p += 1;
    if (*p == buffer.size) {
      return AZ_JSON_ERROR_UNEXPECTED_END;
    }
    char c = az_const_str_item(buffer, *p);
    if (!isdigit(c)) {
      return AZ_JSON_ERROR_UNEXPECTED_CHAR;
    }
    AZ_RETURN_IF_NOT_OK(az_json_number_int_parse(buffer, p, &i, -1, c));
  }

  // exp
  if (*p != buffer.size && az_json_is_e(az_const_str_item(buffer, *p))) {
    // skip 'e' or 'E'
    *p += 1;

    if (*p == buffer.size) {
      return AZ_JSON_ERROR_UNEXPECTED_END;
    }
    char c = az_const_str_item(buffer, *p);

    // read sign, if any.
    int e_sign = 1;
    switch (c) {
      case '-':
        e_sign = -1;
      case '+':
        *p += 1;
        if (*p == buffer.size) {
          return AZ_JSON_ERROR_UNEXPECTED_END;
        }
        c = az_const_str_item(buffer, *p);
    }

    // expect at least one digit.
    if (!isdigit(c)) {
      return AZ_JSON_ERROR_UNEXPECTED_CHAR;
    }

    int16_t e_int = 0;
    do {
      e_int = e_int * 10 + (c - '0');
      *p += 1;
      if (*p == buffer.size) {
        break;
      }
      c = az_const_str_item(buffer, *p);
    } while (isdigit(c));
    i.exp += e_int * e_sign;
  }

  *out_value = az_json_number_to_double(&i);
  return AZ_OK;
}

az_result az_json_read_string_rest(az_const_str const buffer, size_t *const p, az_const_str *const string) {
  // skip '"'
  size_t const begin = *p;
  while (true) {
    if (*p == buffer.size) {
      return AZ_JSON_ERROR_UNEXPECTED_END;
    };
    char const c = az_const_str_item(buffer, *p);
    switch (c) {
      // end of the string
      case '"':
      {
        *string = (az_const_str){ .begin = buffer.begin + begin, .size = *p - begin };
        *p += 1;
        return AZ_OK;
      }
      // escape sequence
      case '\\':
      {
        *p += 1;
        if (*p == buffer.size) {
          return AZ_JSON_ERROR_UNEXPECTED_END;
        }
        char const c = az_const_str_item(buffer, *p);
        if (az_json_is_esc(c)) {
          *p += 1;
        }
        else {
          AZ_RETURN_IF_NOT_OK(az_json_expect_char(buffer, p, 'u'));
          for (size_t const u = *p + 4; *p != u; *p += 1) {
            if (*p == buffer.size) {
              return AZ_JSON_ERROR_UNEXPECTED_END;
            }
            if (!isxdigit(az_const_str_item(buffer, *p))) {
              return AZ_JSON_ERROR_UNEXPECTED_CHAR;
            }
          }
        }
        break;
      }
      default:
      {
        if (c < 0x20) {
          return AZ_JSON_ERROR_UNEXPECTED_CHAR;
        }
        *p += 1;
      }
    }
  }
}

// _value_
az_result az_json_read_value(az_json_state *const p_state, az_json_value *const out_value) {
  az_const_str const buffer = p_state->buffer;
  size_t *const p = &p_state->i;
  if (*p == buffer.size) {
	  return AZ_JSON_ERROR_UNEXPECTED_END;
  }
  char const c = az_const_str_item(buffer, *p);
  if (isdigit(c)) {
    out_value->kind = AZ_JSON_VALUE_NUMBER;
    return az_json_read_number_digit_rest(buffer, p, &out_value->data.number);
  }
  switch (c) {
    case 't':
      out_value->kind = AZ_JSON_VALUE_BOOLEAN;
      out_value->data.boolean = true;
      return az_json_read_keyword_rest(buffer, p, AZ_CONST_STR("rue"));
    case 'f':
      out_value->kind = AZ_JSON_VALUE_BOOLEAN;
      out_value->data.boolean = false;
      return az_json_read_keyword_rest(buffer, p, AZ_CONST_STR("alse"));
    case 'n':
      out_value->kind = AZ_JSON_VALUE_NULL;
      return az_json_read_keyword_rest(buffer, p, AZ_CONST_STR("ull"));
    case '"':
      out_value->kind = AZ_JSON_VALUE_STRING;
      *p += 1;
      return az_json_read_string_rest(buffer, p, &out_value->data.string);
    case '-':
      out_value->kind = AZ_JSON_VALUE_NUMBER;
      return az_json_read_number_digit_rest(buffer, p, &out_value->data.number);
    case '{':
      out_value->kind = AZ_JSON_VALUE_OBJECT;
      *p += 1;
      return az_json_stack_push(p_state, AZ_JSON_STACK_OBJECT);
    case '[':
      out_value->kind = AZ_JSON_VALUE_ARRAY;
      *p += 1;
      return az_json_stack_push(p_state, AZ_JSON_STACK_ARRAY);
  }
  return AZ_JSON_ERROR_UNEXPECTED_CHAR;
}

az_result az_json_read_value_space(az_json_state *const p_state, az_json_value *const out_value) {
  AZ_RETURN_IF_NOT_OK(az_json_read_value(p_state, out_value));
  return az_json_read_white_space(p_state->buffer, &p_state->i);
}

az_result az_json_read(az_json_state *const p_state, az_json_value *const out_value) {
  if (!az_json_stack_is_empty(p_state)) {
    return AZ_JSON_ERROR_INVALID_STATE;
  }
  AZ_RETURN_IF_NOT_OK(az_json_read_white_space(p_state->buffer, &p_state->i));
  AZ_RETURN_IF_NOT_OK(az_json_read_value_space(p_state, out_value));
  bool const is_empty = p_state->i == p_state->buffer.size;
  switch (out_value->kind) {
    case AZ_JSON_VALUE_ARRAY:
    case AZ_JSON_VALUE_OBJECT:
      return is_empty ? AZ_JSON_ERROR_UNEXPECTED_END : AZ_OK;
  }
  return is_empty ? AZ_OK: AZ_JSON_ERROR_UNEXPECTED_CHAR;
}

az_result az_json_read_comma_or_close(az_json_state* const p_state) {
  char c;
  AZ_RETURN_IF_NOT_OK(az_json_get_char(p_state->buffer, p_state->i, &c));
  if (c == ',') {
    // skip ',' and read all whitespaces.
    p_state->i += 1;
    return az_json_read_white_space(p_state->buffer, &p_state->i);
  }
  char const close = az_json_stack_last(p_state) == AZ_JSON_STACK_OBJECT ? '}' : ']';
  return c == close ? AZ_OK : AZ_JSON_ERROR_UNEXPECTED_CHAR;
}

az_result az_json_check_item_begin(
  az_json_state *const p_state,
  uint8_t stack_item,
  char const close
) {
  if (az_json_stack_is_empty(p_state) || az_json_stack_last(p_state) != stack_item) {
    return AZ_JSON_ERROR_INVALID_STATE;
  }
  char c;
  AZ_RETURN_IF_NOT_OK(az_json_get_char(p_state->buffer, p_state->i, &c));
  if (c != close) {
    return AZ_OK;
  }
  // c == close
  AZ_RETURN_IF_NOT_OK(az_json_stack_pop(p_state));
  p_state->i += 1;
  AZ_RETURN_IF_NOT_OK(az_json_read_white_space(p_state->buffer, &p_state->i));
  if (!az_json_stack_is_empty(p_state)) {
    AZ_RETURN_IF_NOT_OK(az_json_read_comma_or_close(p_state));
  }
  return AZ_JSON_NO_MORE_ITEMS;
}

az_result az_json_check_item_end(az_json_state *const p_state, az_json_value const value) {
  switch (value.kind) {
  case AZ_JSON_VALUE_OBJECT:
  case AZ_JSON_VALUE_ARRAY:
    return AZ_OK;
  }
  return az_json_read_comma_or_close(p_state);
}

az_result az_json_read_object_member(az_json_state *const p_state, az_json_member *const out_member) {
  AZ_RETURN_IF_NOT_OK(az_json_check_item_begin(p_state, AZ_JSON_STACK_OBJECT, '}'));
  AZ_RETURN_IF_NOT_OK(az_json_expect_char(p_state->buffer, &p_state->i, '"'));
  AZ_RETURN_IF_NOT_OK(az_json_read_string_rest(p_state->buffer, &p_state->i, &out_member->name));
  AZ_RETURN_IF_NOT_OK(az_json_read_white_space(p_state->buffer, &p_state->i));
  AZ_RETURN_IF_NOT_OK(az_json_expect_char(p_state->buffer, &p_state->i, ':'));
  AZ_RETURN_IF_NOT_OK(az_json_read_white_space(p_state->buffer, &p_state->i));
  AZ_RETURN_IF_NOT_OK(az_json_read_value_space(p_state, &out_member->value));
  return az_json_check_item_end(p_state, out_member->value);
}

az_result az_json_read_array_element(az_json_state *const p_state, az_json_value *const out_element) {
  AZ_RETURN_IF_NOT_OK(az_json_check_item_begin(p_state, AZ_JSON_STACK_ARRAY, ']'));
  AZ_RETURN_IF_NOT_OK(az_json_read_value_space(p_state, out_element));
  return az_json_check_item_end(p_state, *out_element);
}

az_result az_json_state_done(az_json_state const *const p_state) {
  if (p_state->i != p_state->buffer.size || !az_json_stack_is_empty(p_state)) {
    return AZ_JSON_ERROR_INVALID_STATE;
  }
  return AZ_OK;
}

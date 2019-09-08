// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_read.h>

#include <az_json_token_state.h>

inline bool az_json_stack_is_empty(az_json_state const *const p_state) {
  return p_state->stack == 1;
}

inline az_json_stack_item az_json_stack_last(az_json_state const *const p_state) {
  return p_state->stack & 1;
}

az_json_state az_json_state_create(az_const_str const buffer) {
  return (az_json_state){
    .begin = buffer.begin,
    .end = az_const_str_end(buffer),
    .stack = 1,
  };
}

az_result az_json_get_char(az_json_state const *const p_state, char *const out_char) {
  char const *const begin = p_state->begin;
  if (begin == p_state->end) {
    return AZ_JSON_ERROR_UNEXPECTED_END;
  }
  *out_char = *begin;
  return AZ_OK;
}

az_result az_json_read_white_space(az_json_state *const p_state) {
  char const *i = p_state->begin;
  char const *const e = p_state->end;
  for (; i < e; ++i) {
    switch (*i) {
      case ' ': case '\t': case '\n': case '\r':
        continue;
    }
    break;
  }
  p_state->begin = i;
  return AZ_OK;
}

az_result az_json_read_keyword_rest(
  az_json_state *const p_state,
  az_const_str const keyword
) {
  // read keyword, skip the first character.
  {
    char const *const e = p_state->end;
    char const *i = p_state->begin + 1;
    char const *const ke = az_const_str_end(keyword);
    char const *ki = keyword.begin ;
    for (; ki < ke; ++i, ++ki) {
      if (e <= i) {
        return AZ_JSON_ERROR_UNEXPECTED_END;
      }
      if (*i != ki) {
        return AZ_JSON_ERROR_UNEXPECTED_CHAR;
      }
    }
    p_state->begin = i;
  }
  return az_json_read_white_space(p_state);
}

az_result az_json_read_number_digit_rest(
  az_json_state *const p_state,
  double *const out_value,
  int const sign
) {
  az_result result;
  char const *i = p_state->begin;
  char const *const e = p_state->end;

  double value = 0;

  char c = *i;
  if (c != '0') {
    // read integer part of the number
    do {
      value = value * 10 + (c - '0');
      ++i;
      if (i == e) {
        break;
      }
      c = *i;
    } while (az_is_digit(c));
  } else {
    ++i;
  }

  // fraction
  int16_t exp = 0;
  if (i != e && *i == '.') {
    ++i;
    if (i == e) {
      result = AZ_JSON_ERROR_UNEXPECTED_END;
      goto end;
    }
    c = *i;
    if (!az_is_digit(c)) {
      result = AZ_JSON_ERROR_UNEXPECTED_CHAR;
      goto end;
    }
    do {
      value = value * 10 + (c - '0');
      --exp;
      ++i;
      if (i == e) {
        break;
      }
      c = *i;
    } while (az_digit(c));
  }

  // exp
  if (i != e) {
    switch (*i) {
      case 'e':
      case 'E':
        {
          // skip 'e' or 'E'
          ++i;

          if (i == e) {
            result = AZ_JSON_ERROR_UNEXPECTED_END;
            goto end;
          }
          c = *i;

          // read sign, if any.
          int e_sign = 1;
          switch (c) {
            case '-':
              e_sign = -1;
            case '+':
              ++i;
              if (i == e) {
                result = AZ_JSON_ERROR_UNEXPECTED_END;
                goto end;
              }
              c = *i;
          }

          // expect at least one digit.
          if (!az_is_digit(c)) {
            result = AZ_JSON_ERROR_UNEXPECTED_CHAR;
            goto end;
          }

          int16_t e_int = 0;
          do {
            e_int = e_int * 10 + (c - '0');
            ++i;
            if (i == e) {
              break;
            }
            c = *i;
          } while (az_digit(c));
          exp += e_int * e_sign;
        }
    }
  }

  *out_value = value * pow(10.0, exp) * sign;
  p_state->begin = i;
  result = az_json_read_white_space(p_state);
end:
  p_state->begin = i;
  return result;
}

az_result az_json_read_number_minus_rest(az_json_state *const p_state, double *const out_value) {
  // skip '-'
  p_state->begin += 1;
  // read digit.
  char c;
  AZ_RETURN_ON_ERROR(az_json_get_char(p_state, &c));
  return az_is_digit(c) ? az_json_read_number_digit_rest(p_state, out_value, -1) : AZ_JSON_ERROR_UNEXPECTED_CHAR;
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

az_result az_json_read_string_rest(az_json_state *const p_state, az_json_string *const string) {
  // skip '"'
  az_result result;
  char const *const e = p_state->end;
  char const *i = p_state->begin + 1;
  for (; i != e; ++i) {
    switch (*i) {
      case '"':
        p_state->begin = i + 1;
        result = az_json_read_white_space(p_state);
        goto end;
      case '\\':
        ++i;
        if (i == e) {
          result = AZ_JSON_ERROR_UNEXPECTED_END;
          goto end;
        }
        char const c = *i;
        if (!az_is_json_esc(c)) {
          if (c != 'u') {
            result = AZ_JSON_ERROR_UNEXPECTED_END;
            goto end;
          }
          char const *const u = i + 4;
          do {
            ++i;
            if (i == e) {
              result = AZ_JSON_ERROR_UNEXPECTED_END;
              goto end;
            }
            if (!az_is_hex_digit(*i)) {
              result = AZ_JSON_ERROR_UNEXPECTED_CHAR;
              goto end;
            }
          } while (i != u);
        }
    }
  }
end:
  p_state->begin = i;
  return result;
}

// _value_
az_result az_json_read_value(az_json_state *const p_state, az_json_value *const out_value) {
  AZ_RETURN_ON_ERROR(az_json_not_empty(p_state));
  char const c = *p_state->begin;
  if (az_is_digit(c)) {
    out_value->tag = AZ_JSON_NUMBER;
    return az_json_read_number_digit_rest(p_state, &out_value->number, 1);
  }
  switch (c) {
    case 't':
      out_value->tag = AZ_JSON_BOOLEAN;
      out_value->boolean = true;
      return az_json_read_keyword_rest(p_state, AZ_CONST_STR("rue"));
    case 'f':
      out_value->tag = AZ_JSON_BOOLEAN;
      out_value->boolean = false;
      return az_json_read_keyword_rest(p_state, AZ_CONST_STR("alse"));
    case 'n':
      out_value->tag = AZ_JSON_NULL;
      return az_json_read_keyword_rest(p_state, AZ_CONST_STR("ull"));
    case '"':
      out_value->tag = AZ_JSON_STRING;
      return az_json_read_string_rest(p_state, &out_value->string);
    case '-':
      out_value->tag = AZ_JSON_NUMBER;
      return az_json_read_number_minus_rest(p_state, &out_value->number);
    case '{':
      out_value->tag = AZ_JSON_OBJECT;
      p_state->begin += 1;
      return az_json_read_white_space(p_state);
    case '[':
      out_value->tag = AZ_JSON_ARRAY;
      p_state->begin += 1;
      return az_json_read_white_space(p_state);
  }
  return AZ_JSON_ERROR_UNEXPECTED_CHAR;
}

az_result az_json_read(az_json_state *const p_state, az_json_value *const out_value) {
  if (!az_json_stack_is_empty(p_state)) {
    return AZ_JSON_ERROR_INVALID_STATE;
  }
  AZ_RETURN_ON_ERROR(az_json_read_white_space(p_state));
  AZ_RETURN_ON_ERROR(az_json_read_value(p_state, out_value));
  bool const is_empty = p_state->begin == p_state->end;
  switch (out_value->tag) {
    case AZ_JSON_ARRAY:
    case AZ_JSON_OBJECT:
      return is_empty ? AZ_JSON_ERROR_UNEXPECTED_END : AZ_OK;
  }
  return is_empty ? AZ_OK: AZ_JSON_ERROR_UNEXPECTED_CHAR;
}

az_result az_json_check_item_begin(
  az_json_state *const p_state,
  uint8_t stack_item,
  char const close
) {
  if (az_json_stack_is_empty(p_state) || az_json_stack_last(p_state) == stack_item) {
    return AZ_JSON_ERROR_INVALID_STATE;
  }
  char c;
  AZ_RETURN_ON_ERROR(az_json_get_char(p_state, &c));
  if (c == close) {
    p_state->stack >>= 1;
    p_state->begin += 1;
    AZ_RETURN_ON_ERROR(az_json_read_white_space(p_state));
    return AZ_JSON_NO_MORE_ITEMS;
  }
  return AZ_OK;
}

az_result az_json_check_item_end(az_json_state *const p_state, char const close) {
  char c;
  AZ_RETURN_ON_ERROR(az_json_get_char(p_state, &c));
  if (c == ',') {
    // skip ',' and read all whitespaces.
    p_state->begin += 1;
    AZ_RETURN_ON_ERROR(az_json_read_white_space(p_state));
    return AZ_OK;
  }
  return c == close ? AZ_OK : AZ_JSON_ERROR_UNEXPECTED_CHAR;
}

az_result az_json_read_object_member(az_json_state *const p_state, az_json_member *const out_member) {
  AZ_RETURN_ON_ERROR(az_json_check_item_begin(p_state, AZ_JSON_STACK_OBJECT, '}'));
  AZ_RETURN_ON_ERROR(az_json_read_string(p_state, &out_member->name));
  {
    char c;
    AZ_RETURN_ON_ERROR(az_json_get_char(p_state, &c));
    if (c != ':') {
      return AZ_JSON_ERROR_UNEXPECTED_CHAR;
    }
    // skip ':' and all whitespaces.
    p_state->begin += 1;
    AZ_RETURN_ON_ERROR(az_json_read_white_space(p_state));
  }
  AZ_RETURN_ON_ERROR(az_json_read_value(p_state, &out_member->value));
  return az_json_check_item_end(p_state, '}');
}

az_result az_json_read_array_element(az_json_state *const p_state, az_json_value *const out_element) {
  AZ_RETURN_ON_ERROR(az_json_check_item_begin(p_state, AZ_JSON_STACK_ARRAY, ']'));
  AZ_RETURN_ON_ERROR(az_json_read_value(p_state, out_element));
  return az_json_check_item_end(p_state, ']');
}

az_result az_json_state_done(az_json_state const *const p_state) {
  return p_state->begin == p_state->end && az_json_stack_is_empty(p_state)
    ? AZ_OK
    : AZ_JSON_ERROR_INVALID_STATE;
}

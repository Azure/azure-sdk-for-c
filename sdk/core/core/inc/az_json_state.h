// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_STATE_H
#define AZ_JSON_STATE_H

#include <stdint.h>

#include <az_digit.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  AZ_JSON_STATE_NONE,

  // terminal
  AZ_JSON_STATE_ERROR,

  AZ_JSON_STATE_N,
  AZ_JSON_STATE_NU,
  AZ_JSON_STATE_NUL,
  // terminal
  AZ_JSON_STATE_NULL,

  AZ_JSON_STATE_F,
  AZ_JSON_STATE_FA,
  AZ_JSON_STATE_FAL,
  AZ_JSON_STATE_FALS,
  // terminal
  AZ_JSON_STATE_FALSE,

  AZ_JSON_STATE_T,
  AZ_JSON_STATE_TR,
  AZ_JSON_STATE_TRU,
  // terminal
  AZ_JSON_STATE_TRUE,

  AZ_JSON_STATE_STRING_OPEN,
  AZ_JSON_STATE_STRING_CHAR,
  AZ_JSON_STATE_STRING_ESC,
  AZ_JSON_STATE_STRING_ESC_CHAR,
  AZ_JSON_STATE_STRING_ESC_U,
  AZ_JSON_STATE_STRING_ESC_U1,
  AZ_JSON_STATE_STRING_ESC_U2,
  AZ_JSON_STATE_STRING_ESC_U3,
  AZ_JSON_STATE_STRING_ESC_U4,
  // terminal
  AZ_JSON_STATE_STRING,

  AZ_JSON_STATE_NUMBER_MINUS,
  AZ_JSON_STATE_NUMBER_ZERO,
  AZ_JSON_STATE_NUMBER_DIGIT,
  AZ_JSON_STATE_NUMBER_DOT,
  AZ_JSON_STATE_NUMBER_FRACTION,
  AZ_JSON_STATE_NUMBER_E,
  AZ_JSON_STATE_NUMBER_E_SIGN,
  AZ_JSON_STATE_NUMBER_E_DIGIT,
  // terminal - 1
  AZ_JSON_STATE_NUMBER,

  AZ_JSON_STATE_OBJECT_OPEN,
  // terminal
  AZ_JSON_STATE_OBJECT_EMPTY,
  // terminal - 1
  AZ_JSON_STATE_OBJECT_PROPERTY,

  AZ_JSON_STATE_ARRAY_OPEN,
  // terminal
  AZ_JSON_STATE_ARRAY_EMPTY,
  // terminal - 1
  AZ_JSON_STATE_ARRAY_ITEM,
};

typedef uint8_t az_json_state;

inline az_json_state az_json_state_string_u_parse(az_json_state const state, char const c) {
  return az_is_digit(c) || az_in_range('a', 'f', c) || az_in_range('A', 'F', c)
    ? state
    : AZ_JSON_STATE_ERROR;
}

inline az_json_state az_json_state_number_minus_parse(char const c) {
  if (c == '0') {
    return AZ_JSON_STATE_NUMBER_ZERO;
  }
  if (az_is_digit(c)) {
    return AZ_JSON_STATE_NUMBER_DIGIT;
  }
  return AZ_JSON_STATE_ERROR;
}

inline az_json_state az_json_state_number_zero_parse(char const c) {
  switch (c) {
    case '.': return AZ_JSON_STATE_NUMBER_DOT;
    case 'e': case 'E': return AZ_JSON_STATE_NUMBER_E;
  }
  return AZ_JSON_STATE_NUMBER;
}

inline az_json_state az_json_state_number_digit_parse(char const c) {
  switch (c) {
    case '.': return AZ_JSON_STATE_NUMBER_DOT;
    case 'e': case 'E': return AZ_JSON_STATE_NUMBER_E;
  }
  if (az_is_digit(c)) {
    return AZ_JSON_STATE_NUMBER_DIGIT;
  }
  return AZ_JSON_STATE_NUMBER;
}

inline az_json_state az_json_state_number_dot_parse(char const c) {
  if (az_is_digit(c)) {
    return AZ_JSON_STATE_NUMBER_FRACTION;
  }
  return AZ_JSON_STATE_ERROR;
}

inline az_json_state az_json_state_none_parse(char const c) {
  switch (c)
  {
    case ' ': case '\t': case '\n': case '\r':
      return AZ_JSON_STATE_NONE;
    case 'n':
      return AZ_JSON_STATE_N;
    case 'f':
      return AZ_JSON_STATE_F;
    case 't':
      return AZ_JSON_STATE_T;
    case '"':
      return AZ_JSON_STATE_STRING_OPEN;
    case '-':
      return AZ_JSON_STATE_NUMBER_MINUS;
    case '0':
      return AZ_JSON_STATE_NUMBER_ZERO;
    case '{':
      return AZ_JSON_STATE_OBJECT_OPEN;
    case '[':
      return AZ_JSON_STATE_ARRAY_OPEN;
  }
  if (az_is_digit(c)) {
    return AZ_JSON_STATE_NUMBER_DIGIT;
  }
  return AZ_JSON_STATE_ERROR;
}

inline az_json_state az_json_state_expect(char const c, char const expect, az_json_state const state) {
  return c == expect ? state : AZ_JSON_STATE_ERROR;
}

inline az_json_state az_json_state_string_char_parse(char const c) {
  switch (c) {
    case '\\':
      return AZ_JSON_STATE_STRING_ESC;
    case '"':
      return AZ_JSON_STATE_STRING;
  }
  return c < ' '
    ? AZ_JSON_STATE_ERROR
    : AZ_JSON_STATE_STRING_ESC_CHAR;
}

inline az_json_state az_json_state_string_esc_parse(char const c) {
  switch (c) {
    case '"':
    case '\\':
    case '/':
      return AZ_JSON_STATE_STRING_CHAR;
    case 'b':
    case 'f':
    case 'n':
    case 'r':
    case 't':
      return AZ_JSON_STATE_STRING_ESC_CHAR;
    case 'u':
      return AZ_JSON_STATE_STRING_ESC_U;
  }
  return AZ_JSON_STATE_ERROR;
}

inline az_json_state az_json_state_value_parse(az_json_state const state, char const c) {
  switch (state) {
    case AZ_JSON_STATE_NONE:
      return az_json_state_none_parse(c);

    case AZ_JSON_STATE_N:
      return az_json_state_expect(c, 'u', AZ_JSON_STATE_NU);
    case AZ_JSON_STATE_NU:
      return az_json_state_expect(c, 'l', AZ_JSON_STATE_NUL);
    case AZ_JSON_STATE_NUL:
      return az_json_state_expect(c, 'l', AZ_JSON_STATE_NULL);

    case AZ_JSON_STATE_F:
      return az_json_state_expect(c, 'a', AZ_JSON_STATE_FA);
    case AZ_JSON_STATE_FA:
      return az_json_state_expect(c, 'l', AZ_JSON_STATE_FAL);
    case AZ_JSON_STATE_FAL:
      return az_json_state_expect(c, 's', AZ_JSON_STATE_FALS);
    case AZ_JSON_STATE_FALS:
      return az_json_state_expect(c, 'e', AZ_JSON_STATE_FALSE);

    case AZ_JSON_STATE_T:
      return az_json_state_expect(c, 'r', AZ_JSON_STATE_TR);
    case AZ_JSON_STATE_TR:
      return az_json_state_expect(c, 'u', AZ_JSON_STATE_TRU);
    case AZ_JSON_STATE_TRU:
      return az_json_state_expect(c, 'e', AZ_JSON_STATE_TRUE);

    case AZ_JSON_STATE_STRING_OPEN:
    case AZ_JSON_STATE_STRING_CHAR:
    case AZ_JSON_STATE_STRING_ESC_CHAR:
    case AZ_JSON_STATE_STRING_ESC_U4:
      return az_json_state_string_char_parse(c);
    case AZ_JSON_STATE_STRING_ESC:
      return az_json_state_string_esc_parse(c);
    case AZ_JSON_STATE_STRING_ESC_U:
      return az_json_state_string_u_parse(AZ_JSON_STATE_STRING_ESC_U1, c);
    case AZ_JSON_STATE_STRING_ESC_U1:
      return az_json_state_string_u_parse(AZ_JSON_STATE_STRING_ESC_U2, c);
    case AZ_JSON_STATE_STRING_ESC_U2:
      return az_json_state_string_u_parse(AZ_JSON_STATE_STRING_ESC_U3, c);
    case AZ_JSON_STATE_STRING_ESC_U3:
      return az_json_state_string_u_parse(AZ_JSON_STATE_STRING_ESC_U4, c);

    case AZ_JSON_STATE_NUMBER_MINUS:
      return az_json_state_number_minus_parse(c);
    case AZ_JSON_STATE_NUMBER_ZERO:
      return az_json_state_number_zero_parse(c);
    case AZ_JSON_STATE_NUMBER_DIGIT:
      return az_json_state_number_digit_parse(c);
    case AZ_JSON_STATE_NUMBER_DOT:
      return az_json_state_number_dot_parse(c);
  }
  return AZ_JSON_STATE_ERROR;
}

#ifdef __cplusplus
}
#endif

#endif

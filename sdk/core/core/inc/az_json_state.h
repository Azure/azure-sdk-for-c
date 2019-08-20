// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_STATE_H
#define AZ_JSON_STATE_H

#include <stdint.h>
#include <az_static_assert.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  AZ_JSON_TERMINAL = 0xFF,
};

enum {
  // intermidiate states:

  // initial state
  AZ_JSON_STATE_WHITE_SPACE = 0,

  // 'u' => NU, ... => ERROR
  AZ_JSON_STATE_N,
  // 'l' => NUL, ... => ERROR
  AZ_JSON_STATE_NU,
  // 'l' => NULL, ... => ERROR
  AZ_JSON_STATE_NUL,

  // 'a' => FA, ... => ERROR
  AZ_JSON_STATE_F,
  // 'l' => FAL, ... => ERROR
  AZ_JSON_STATE_FA,
  // 's' => FALS, ... => ERROR
  AZ_JSON_STATE_FAL,
  // 'e' => FALSE, ... => ERROR
  AZ_JSON_STATE_FALS,

  // 'r' => TR, ... => ERROR
  AZ_JSON_STATE_T,
  // 'u' => TRU, ... => ERROR
  AZ_JSON_STATE_TR,
  // 'e' => TRUE, ... => ERROR
  AZ_JSON_STATE_TRU,

  // '"' => EMPTY_STRING, '\\' => STRING_ESC, ' '... => STRING_CHAR (string_char = c), ... => ERROR
  AZ_JSON_STATE_STRING_OPEN,

  // '"' => STRING,
  // '\\' => STRING_ESC,
  // ' '... => STRING_CHAR (string_char = utf8_merge(string_char, c)),
  // ... => ERROR
  AZ_JSON_STATE_STRING_CHAR,

  // '"'|'\'|'/'|'b'|'f'|'n'|'r'|'t' => STRING_CHAR (string_char = c),
  // 'u' => STRING_U (string_char = 0),
  // ... => ERROR
  AZ_JSON_STATE_STRING_ESC,

  // '0'..'9'|'a'..'f'|'A'..'F' => STRING_U1 (string_char = c)
  AZ_JSON_STATE_STRING_U,

  // '0'..'9'|'a'..'f'|'A'..'F' => STRING_U2 (string_char = string_char << 4 + c)
  AZ_JSON_STATE_STRING_U1,

  // '0'..'9'|'a'..'f'|'A'..'F' => STRING_U3 (string_char = string_char << 4 + c)
  AZ_JSON_STATE_STRING_U2,

  // '0'..'9'|'a'..'f'|'A'..'F' => STRING_CHAR (string_char = string_char << 4 + c)
  AZ_JSON_STATE_STRING_U3,

  // '0' => NUMBER_ZERO (negative = true), '1'..'9' => NUMBER_DIGIT (integer = d, negative = true)
  AZ_JSON_STATE_NUMBER_MINUS,

  // '.' => NUMBER_FRACTION, 'e'|'E' => NUMBER_E, '1'..'9' => ERROR, ... => INTEGER
  AZ_JSON_STATE_NUMBER_ZERO,

  // '0'..'9' => NUMBER_DIGIT, '.' => NUMBER_DOT, 'e'|'E' => NUMBER_E, ... => INTEGER | NUMBER
  AZ_JSON_STATE_NUMBER_DIGIT,

  // '0'..'9' => NUMBER_FRACTION, ... => ERROR
  AZ_JSON_STATE_NUMBER_DOT,

  // '0'..'9' => NUMBER_FRACTION, 'e'|'E' => NUMBER_E, ... => NUMBER
  AZ_JSON_STATE_NUMBER_FRACTION,

  // '-'|'+'|'0'..'9' => NUMBER_E_DIGIT, ... => ERROR
  AZ_JSON_STATE_NUMBER_E,

  // '0'..'9' => NUMBER_E_DIGIT
  AZ_JSON_STATE_NUMBER_E_DIGIT,

  // '}' => EMPTY_OBJECT, ws => OBJECT_OPEN, ... => OBJECT
  AZ_JSON_STATE_OBJECT_OPEN,

  // ']' => EMPTY_ARRAY, ws => ARRAY_OPEN, ... => ARRAY
  AZ_JSON_STATE_ARRAY_OPEN,

  // terminal states:

  AZ_JSON_STATE_NULL,
  AZ_JSON_STATE_FALSE,
  AZ_JSON_STATE_TRUE,
  AZ_JSON_STATE_EMPTY_STRING,
  AZ_JSON_STATE_STRING,
  AZ_JSON_STATE_INTEGER,
  AZ_JSON_STATE_NUMBER,

  AZ_JSON_STATE_EMPTY_OBJECT,
  AZ_JSON_STATE_OBJECT,

  AZ_JSON_STATE_EMPTY_ARRAY,
  AZ_JSON_STATE_ARRAY,

  AZ_JSON_STATE_ERROR,

  //
  AZ_JSON_STATE_MAX,
};

typedef uint8_t az_json_state_tag;

typedef struct {
  az_json_state_tag tag;
  bool repeat;
  // 32 bits
  union {
    struct {
      bool negative;
      bool e_negative;
      int16_t e;
    } number_info;
    int32_t string_char;
  };
  // terminal states, 64 bits
  union {
    size_t string;
    double number;
    int64_t integer;
  };
} az_json_state;

AZ_STATIC_ASSERT(sizeof(az_json_state) == 16);

inline bool az_white_space_check(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

inline bool az_digit_1_9_check(char c) {
  return '1' <= c && c <= '9';
}

inline az_json_state az_json_state_error() {
  return (az_json_state){ .tag = AZ_JSON_STATE_ERROR };
}

inline az_json_state az_json_state_number_digit(char digit) {
  return (az_json_state){ .tag = AZ_JSON_STATE_NUMBER_DIGIT, .integer = digit - '0' };
}

inline az_json_state az_json_state_white_space_parse(char c) {
  if (az_white_space_check(c)) {
    return (az_json_state){ .tag = AZ_JSON_STATE_WHITE_SPACE };
  }
  if (az_digit_1_9_check(c)) {
    return az_json_state_number_digit(c);
  }
  switch (c) {
    case '{':
      return (az_json_state){ .tag = AZ_JSON_STATE_OBJECT_OPEN };
    case '[':
      return (az_json_state){ .tag = AZ_JSON_STATE_ARRAY_OPEN };
    case '"':
      return (az_json_state){ .tag = AZ_JSON_STATE_STRING_OPEN };
    case '-':
      return (az_json_state){ .tag = AZ_JSON_STATE_NUMBER_MINUS };
    case '0':
      return (az_json_state){ .tag = AZ_JSON_STATE_NUMBER_ZERO };
    case 'n':
      return (az_json_state){ .tag = AZ_JSON_STATE_N };
    case 'f':
      return (az_json_state){ .tag = AZ_JSON_STATE_F };
    case 't':
      return (az_json_state){ .tag = AZ_JSON_STATE_T };
    default:
      return az_json_state_error();
  }
}

inline bool az_json_state_terminal_check(az_json_state_tag const tag) {
  return AZ_JSON_STATE_NULL <= tag;
}

inline az_json_state az_json_state_keyword_parse(char const e, az_json_state_tag const tag, char const c) {
  return e == c ? (az_json_state){ .tag = AZ_JSON_STATE_N } : az_json_state_error();
}

inline az_json_state az_json_state_string_open_parse(char c) {
  switch(c) {
    case '"':
      return (az_json_state){ .tag = AZ_JSON_STATE_EMPTY_STRING };
    case '\\':
      return (az_json_state){ .tag = AZ_JSON_STATE_STRING_ESC };
    default:
      return ' ' <= c
        ? (az_json_state){ .tag = AZ_JSON_STATE_STRING_CHAR, .string_char = c }
        : az_json_state_error();
  }
}

inline az_json_state az_json_parse(az_json_state const state, char const c) {
  if (az_json_state_terminal_check(c)) {
    return az_json_state_white_space_parse(c);
  }
  switch (state.tag) {
    // null
    case AZ_JSON_STATE_N:
      return az_json_state_keyword_parse('u', AZ_JSON_STATE_NU, c);
    case AZ_JSON_STATE_NU:
      return az_json_state_keyword_parse('l', AZ_JSON_STATE_NUL, c);
    case AZ_JSON_STATE_NUL:
      return az_json_state_keyword_parse('l', AZ_JSON_STATE_NULL, c);
    // false
    case AZ_JSON_STATE_F:
      return az_json_state_keyword_parse('a', AZ_JSON_STATE_FA, c);
    case AZ_JSON_STATE_FA:
      return az_json_state_keyword_parse('l', AZ_JSON_STATE_FAL, c);
    case AZ_JSON_STATE_FAL:
      return az_json_state_keyword_parse('s', AZ_JSON_STATE_FALS, c);
    case AZ_JSON_STATE_FALS:
      return az_json_state_keyword_parse('e', AZ_JSON_STATE_FALSE, c);
    // true
    case AZ_JSON_STATE_T:
      return az_json_state_keyword_parse('r', AZ_JSON_STATE_TR, c);
    case AZ_JSON_STATE_TR:
      return az_json_state_keyword_parse('u', AZ_JSON_STATE_TRU, c);
    case AZ_JSON_STATE_TRU:
      return az_json_state_keyword_parse('e', AZ_JSON_STATE_TRUE, c);
    // string
    case AZ_JSON_STATE_STRING_OPEN:
      return az_json_state_string_open_parse(c);
    //
    case AZ_JSON_STATE_WHITE_SPACE:
    case AZ_JSON_STATE_NULL:
    case AZ_JSON_STATE_FALSE:
    case AZ_JSON_STATE_TRUE:
    case AZ_JSON_STATE_EMPTY_STRING:
    case AZ_JSON_STATE_STRING:
    case AZ_JSON_STATE_INTEGER:
    case AZ_JSON_STATE_NUMBER:
    case AZ_JSON_STATE_EMPTY_OBJECT:
    case AZ_JSON_STATE_EMPTY_ARRAY:
      return az_json_state_white_space_parse(c);
    case AZ_JSON_STATE_ARRAY:
    case AZ_JSON_STATE_OBJECT:
    case AZ_JSON_STATE_ERROR:
    default:
      return az_json_state_error();
  }
}

#ifdef __cplusplus
}
#endif

#endif

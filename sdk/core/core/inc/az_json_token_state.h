// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_TOKEN_STATE_H
#define AZ_JSON_TOKEN_STATE_H

#include <stdint.h>

#include <az_json_read.h>
#include <az_digit.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  AZ_JTS_PREFIX_MASK       = 0xF0,

  AZ_JTS_SPACE             = 0,

  AZ_JTS_ERROR             = 1,

  AZ_JTS_VALUE_SPACE       = 2,
  AZ_JTS_OBJECT_OPEN_SPACE = 3,
  AZ_JTS_ARRAY_OPEN_SPACE  = 4,

  AZ_JTS_COMMA             = 5,
  AZ_JTS_COLON             = 6,

  // null
  AZ_JTS_NULL_PREFIX       = AZ_JSON_NULL << 4,

  AZ_JTS_N                 = AZ_JTS_NULL_PREFIX + 0,
  AZ_JTS_NU                = AZ_JTS_NULL_PREFIX + 1,
  AZ_JTS_NUL               = AZ_JTS_NULL_PREFIX + 2,
  AZ_JTS_NULL              = AZ_JTS_NULL_PREFIX + 3,

  // boolean
  AZ_JTS_BOOLEAN_PREFIX    = AZ_JSON_BOOLEAN << 4,

  AZ_JTS_F                 = AZ_JTS_BOOLEAN_PREFIX + 0,
  AZ_JTS_FA                = AZ_JTS_BOOLEAN_PREFIX + 1,
  AZ_JTS_FAL               = AZ_JTS_BOOLEAN_PREFIX + 2,
  AZ_JTS_FALS              = AZ_JTS_BOOLEAN_PREFIX + 3,
  AZ_JTS_FALSE             = AZ_JTS_BOOLEAN_PREFIX + 4,

  AZ_JTS_T                 = AZ_JTS_BOOLEAN_PREFIX + 8,
  AZ_JTS_TR                = AZ_JTS_BOOLEAN_PREFIX + 9,
  AZ_JTS_TRU               = AZ_JTS_BOOLEAN_PREFIX + 0xA,
  AZ_JTS_TRUE              = AZ_JTS_BOOLEAN_PREFIX + 0xB,

  // number
  AZ_JTS_NUMBER_PREFIX     = AZ_JSON_NUMBER << 4,

  AZ_JTS_NUMBER_MINUS      = AZ_JTS_NUMBER_PREFIX + 0,
  AZ_JTS_NUMBER_ZERO       = AZ_JTS_NUMBER_PREFIX + 1,
  AZ_JTS_NUMBER_DIGIT      = AZ_JTS_NUMBER_PREFIX + 2,

  AZ_JTS_NUMBER_DOT        = AZ_JTS_NUMBER_PREFIX + 4,
  AZ_JTS_NUMBER_FRACTION   = AZ_JTS_NUMBER_PREFIX + 5,

  AZ_JTS_NUMBER_E          = AZ_JTS_NUMBER_PREFIX + 8,
  AZ_JTS_NUMBER_E_SIGN     = AZ_JTS_NUMBER_PREFIX + 9,
  AZ_JTS_NUMBER_E_DIGIT    = AZ_JTS_NUMBER_PREFIX + 0xA,

  // string
  AZ_JTS_STRING_PREFIX     = AZ_JSON_STRING << 4,

  AZ_JTS_STRING_OPEN       = AZ_JSON_STRING + 1,
  AZ_JTS_STRING_CLOSE      = AZ_JSON_STRING + 2,

  AZ_JTS_STRING_CHAR       = AZ_JSON_STRING + 3,
  AZ_JTS_STRING_ESC        = AZ_JSON_STRING + 4,
  AZ_JTS_STRING_ESC_CHAR   = AZ_JSON_STRING + 5,

  AZ_JTS_STRING_ESC_U      = AZ_JSON_STRING + 8,
  AZ_JTS_STRING_ESC_U1     = AZ_JSON_STRING + 9,
  AZ_JTS_STRING_ESC_U2     = AZ_JSON_STRING + 0xA,
  AZ_JTS_STRING_ESC_U3     = AZ_JSON_STRING + 0xB,
  AZ_JTS_STRING_ESC_U4     = AZ_JSON_STRING + 0xC,

  // object
  AZ_JTS_OBJECT_PREFIX     = AZ_JSON_OBJECT << 4,

  AZ_JTS_OBJECT_OPEN       = AZ_JTS_OBJECT_PREFIX + 0,
  AZ_JTS_OBJECT_CLOSE      = AZ_JTS_OBJECT_PREFIX + 1,

  // array
  AZ_JTS_ARRAY_PREFIX      = AZ_JSON_ARRAY << 4,

  AZ_JTS_ARRAY_OPEN        = AZ_JTS_ARRAY_PREFIX + 0,
  AZ_JTS_ARRAY_CLOSE       = AZ_JTS_ARRAY_PREFIX + 1,
};

typedef uint8_t az_jts;

inline az_jts az_jts_space_only(char const c, az_jts state) {
  switch (c) {
    case ' ': case '\n': case '\r': case '\t':
      return state;
  }
  return AZ_JTS_ERROR;
}

inline az_jts az_jts_space_next(char const c, az_jts state) {
  switch (c) {
    case 'n':
      return AZ_JTS_N;
    case 'f':
      return AZ_JTS_F;
    case 't':
      return AZ_JTS_T;
    case '-':
      return AZ_JTS_NUMBER_MINUS;
    case '0':
      return AZ_JTS_NUMBER_ZERO;
    case '"':
      return AZ_JTS_STRING_OPEN;
    case '}':
      return AZ_JTS_OBJECT_CLOSE;
    case ']':
      return AZ_JTS_ARRAY_CLOSE;
  }
  if (az_is_digit(c)) {
    return AZ_JTS_NUMBER_DIGIT;
  }
  return az_jts_space_only(c, state);
}

inline az_jts az_jts_value_space_next(char const c) {
  switch (c) {
    case ':':
      return AZ_JTS_COLON;
    case ',':
      return AZ_JTS_COMMA;
    case '}':
      return AZ_JTS_OBJECT_CLOSE;
    case ']':
      return AZ_JTS_ARRAY_CLOSE;
  }
  return az_jts_space_only(c, AZ_JTS_VALUE_SPACE);
}

inline az_jts az_jts_expect(az_const_str const expected, az_jts const state, char const c) {
  az_jts const i = (state & 7) + 1;
  return expected.size <= i ? az_jts_value_space_next(c)
    : az_const_str_item(expected, i) == c ? state + 1
    : AZ_JTS_ERROR;
}

inline az_jts az_jts_null_next(az_jts state, char const c) {
  return az_jts_expect(AZ_CONST_STR("null"), state, c);
}

inline az_jts az_jts_false_next(az_jts state, char const c) {
  return az_jts_expect(AZ_CONST_STR("false"), state, c);
}

inline az_jts az_jts_true_next(az_jts state, char const c) {
  return az_jts_expect(AZ_CONST_STR("true"), state, c);
}

inline az_jts az_jts_boolean_next(az_jts state, char const c) {
  return (state & 8) == 0 ? az_jts_false_next(state, c) : az_jts_true_next(state, c);
}

inline az_jts az_jts_string_next(az_jts state, char const c) {
  switch (state) {
    case AZ_JTS_STRING_OPEN:
    case AZ_JTS_STRING_CHAR:
    case AZ_JTS_STRING_ESC_CHAR:
    case AZ_JTS_STRING_ESC_U4:
      {
        switch (c) {
          case '\\':
            return AZ_JTS_STRING_ESC;
          case '"':
            return AZ_JTS_STRING_CLOSE;
        }
        return c < ' ' ? AZ_JTS_ERROR : AZ_JTS_STRING_CHAR;
      }
    case AZ_JTS_STRING_ESC:
      {
        switch (c) {
          case '"': case '\\': case '/': case 'b': case 'f': case 'n': case 'r': case 't':
            return AZ_JTS_STRING_ESC_CHAR;
        }
        return AZ_JTS_ERROR;
      }
    case AZ_JTS_STRING_ESC_U:
    case AZ_JTS_STRING_ESC_U1:
    case AZ_JTS_STRING_ESC_U2:
    case AZ_JTS_STRING_ESC_U3:
      return az_is_hex_digit(c) ? state + 1 : AZ_JTS_ERROR;
    case AZ_JTS_STRING_CLOSE:
      return az_jts_value_space_next(c);
  }
  return AZ_JTS_ERROR;
}

inline az_jts az_jts_number_fraction_end(char const c) {
  switch (c) {
    case 'e': case 'E':
      return AZ_JTS_NUMBER_E;
  }
  return az_jts_value_space_next(c);
}

inline az_jts az_jts_number_digit_end(char const c) {
  return c == '.' ? AZ_JTS_NUMBER_DOT : az_jts_number_fraction_end(c);
}

inline az_jts az_jts_number_e_sign_end(char const c) {
  return az_is_digit(c) ? AZ_JTS_NUMBER_E_DIGIT : AZ_JTS_ERROR;
}

inline az_jts az_jts_number_next(az_jts const state, char const c) {
  switch (state) {

    case AZ_JTS_NUMBER_MINUS:
      return c == '0' ? AZ_JTS_NUMBER_ZERO
        : az_is_digit(c) ? AZ_JTS_NUMBER_DIGIT
        : AZ_JTS_ERROR;
    case AZ_JTS_NUMBER_ZERO:
      return az_jts_number_digit_end(c);
    case AZ_JTS_NUMBER_DIGIT:
      return az_is_digit(c) ? AZ_JTS_NUMBER_DIGIT : az_jts_number_digit_end(c);

    case AZ_JTS_NUMBER_DOT:
      return az_is_digit(c) ? AZ_JTS_NUMBER_FRACTION : AZ_JTS_ERROR;
    case AZ_JTS_NUMBER_FRACTION:
      return az_is_digit(c) ? AZ_JTS_NUMBER_FRACTION : az_jts_number_fraction_end(c);

    case AZ_JTS_NUMBER_E:
      {
        switch (c) {
          case '-':
          case '+':
            return AZ_JTS_NUMBER_E_SIGN;
        }
        return az_jts_number_e_sign_end(c);
      }

    case AZ_JTS_NUMBER_E_SIGN:
      return az_jts_number_e_sign_end(c);

    case AZ_JTS_NUMBER_E_DIGIT:
      return az_is_digit(c) ? AZ_JTS_NUMBER_E_DIGIT : az_jts_value_space_next(c);
  }
  return AZ_JTS_ERROR;
}

inline az_jts az_jts_object_open_next(char const c) {
  switch (c) {
    case '"':
      return AZ_JTS_STRING_OPEN;
    case '}':
      return AZ_JTS_OBJECT_CLOSE;
  }
  return az_jts_space_only(c, AZ_JTS_OBJECT_OPEN_SPACE);
}

inline az_jts az_jts_array_open_next(char const c) {
  return c == ']' ? AZ_JTS_ARRAY_CLOSE : az_jts_space_next(c, AZ_JTS_ARRAY_OPEN_SPACE);
}

inline az_jts az_jts_next(az_jts const state, char const c) {
  switch (state & AZ_JTS_PREFIX_MASK) {
    case AZ_JTS_NULL_PREFIX:
      return az_jts_null_next(state, c);
    case AZ_JTS_BOOLEAN_PREFIX:
      return az_jts_boolean_next(state, c);
    case AZ_JTS_NUMBER_PREFIX:
      return az_jts_number_next(state, c);
  }
  switch (state) {
    case AZ_JTS_COMMA:
    case AZ_JTS_COLON:
    case AZ_JTS_SPACE:
      return az_jts_space_next(c, AZ_JTS_SPACE);

    case AZ_JTS_OBJECT_OPEN:
    case AZ_JTS_OBJECT_OPEN_SPACE:
      return az_jts_object_open_next(c);

    case AZ_JTS_ARRAY_OPEN:
    case AZ_JTS_ARRAY_OPEN_SPACE:
      return az_jts_array_open_next(c);

    case AZ_JTS_OBJECT_CLOSE:
    case AZ_JTS_ARRAY_CLOSE:
    case AZ_JTS_VALUE_SPACE:
      return az_jts_value_space_next(c);
  }
  return AZ_JTS_ERROR;
}

#ifdef __cplusplus
}
#endif

#endif

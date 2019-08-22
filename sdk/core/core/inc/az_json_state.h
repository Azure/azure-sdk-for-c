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
  AZ_JSON_TERMINAL = 0xFF
};

enum {
  AZ_JSON_TOKEN_WHITE_SPACE  = 0,
  AZ_JSON_TOKEN_ERROR        = 'E',
  AZ_JSON_TOKEN_NULL         = 'n',
  AZ_JSON_TOKEN_FALSE        = 'f',
  AZ_JSON_TOKEN_TRUE         = 't',
  AZ_JSON_TOKEN_STRING       = '"',
  AZ_JSON_TOKEN_NUMBER       = '.',
  AZ_JSON_TOKEN_INTEGER      = '0',
  AZ_JSON_TOKEN_OBJECT_OPEN  = '{',
  AZ_JSON_TOKEN_OBJECT_CLOSE = '}',
  AZ_JSON_TOKEN_ARRAY_OPEN   = '[',
  AZ_JSON_TOKEN_ARRAY_CLOSE  = ']',
  AZ_JSON_TOKEN_COMMA        = ',',
  AZ_JSON_TOKEN_COLON        = ':',
};

typedef char az_json_token_tag;

enum {
  AZ_JSON_TOKEN_STRING_OPEN  = 0,
  AZ_JSON_TOKEN_STRING_CHAR  = 'c',
  AZ_JSON_TOKEN_STRING_ESC   = '\\',
  AZ_JSON_TOKEN_STRING_U     = 'u',
  AZ_JSON_TOKEN_STRING_CLOSE = '"',
};

typedef char az_json_token_string_tag;

enum {
  AZ_JSON_TOKEN_NUMBER_DIGIT    = 0,
  AZ_JSON_TOKEN_NUMBER_DOT      = '.',
  AZ_JSON_TOKEN_NUMBER_FRACTION = '/',
  AZ_JSON_TOKEN_NUMBER_E        = 'e',
  AZ_JSON_TOKEN_NUMBER_E_SIGN   = '+',
  AZ_JSON_TOKEN_NUMBER_E_DIGIT  = '1',
};

typedef char az_json_token_number_tag;

typedef struct {
  int16_t integer_e;
  int16_t e;
} az_json_token_number_e;

typedef struct {
  bool done;
  uint8_t tag;
  // 8 bits
  union {
    // done == true
    char next;
    // tag == AZ_JSON_TOKEN_NULL || tag == AZ_JSON_TOKEN_FALSE || tag == AZ_JSON_TOKEN_TRUE
    uint8_t keyword_position;
    // tag == AZ_JSON_TOKEN_STRING
    az_json_token_string_tag string_tag;
    // tag == AZ_JSON_TOKEN_NUMBER
    az_json_token_number_tag number_tag;
  };
  // 8 bits
  union {
    // string_tag == AZ_JSON_TOKEN_STRING_U
    uint8_t string_u_position;
    struct {
      bool integer_negative: 1;
      bool e_negative: 1;
    };
  };
  // 32 bits
  union {
    // string_tag == AZ_JSON_TOKEN_STRING_CHAR || string_tag == AZ_JSON_TOKEN_STRING_U
    int32_t string_char;
    // number_tag == AZ_JSON_TOKEN_NUMBER_E_...
    az_json_token_number_e number_e;
  };
  // 64 bits
  union {
    // tag == AZ_JSON_TOKEN_STRING
    int32_t string;
    // tag == AZ_JSON_TOKEN_NUMBER
    double number;
    // tag == AZ_JSON_TOKEN_INTEGER
    int64_t integer;
  };
} az_json_token;

AZ_STATIC_ASSERT(sizeof(az_json_token) <= 16);

az_json_token az_json_parse(az_json_token const token, char const c);

#ifdef __cplusplus
}
#endif

#endif

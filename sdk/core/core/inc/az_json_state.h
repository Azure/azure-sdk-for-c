// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_STATE_H
#define AZ_JSON_STATE_H

#include <stdint.h>
#include <az_static_assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  AZ_JSON_TOKEN_NONE = 0,
  AZ_JSON_TOKEN_ERROR,
  AZ_JSON_TOKEN_NULL,
  AZ_JSON_TOKEN_FALSE,
  AZ_JSON_TOKEN_TRUE,
  AZ_JSON_TOKEN_STRING,
  AZ_JSON_TOKEN_NUMBER,
  AZ_JSON_TOKEN_INTEGER,
  AZ_JSON_TOKEN_OBJECT_OPEN,
  AZ_JSON_TOKEN_OBJECT_CLOSE,
  AZ_JSON_TOKEN_ARRAY_OPEN,
  AZ_JSON_TOKEN_ARRAY_CLOSE,
  AZ_JSON_TOKEN_COMMA,
  AZ_JSON_TOKEN_COLON,
  AZ_JSON_TOKEN_MAX,
} az_json_token_tag;

AZ_STATIC_ASSERT(AZ_JSON_TOKEN_MAX <= 1 << 4);

typedef struct {
  az_json_token_tag tag;
  union {
    int32_t string;
    double number;
    int64_t integer;
  };
} az_json_token;

AZ_STATIC_ASSERT(sizeof(az_json_token) == 16);

typedef enum {
  // '\\' => ESC(string.position++),
  // '\x20'.. => CHAR(string.char = c, string.position++),
  // '"' => { .result = STRING }
  AZ_JSON_STATE_STRING_CHAR,
  // '\\'|'/'|'b'|'f'|'n'|'r'|'t' => CHAR(string.char = c, string.postion++)
  // 'u' => AZ_JSON_STATE_STRING_U
  AZ_JSON_STATE_STRING_ESC,
  AZ_JSON_STATE_STRING_ESC_U,
  AZ_JSON_STATE_STRING_MAX,
} az_json_state_string_tag;

/*
typedef struct {
  unsigned int tag: 8;
  unsigned int u: 8;
  unsigned int position: 32;
  unsigned int char_: 32;
} az_json_state_string;

typedef struct {
  unsigned int tag: 8;
  union {
    unsigned int keyword: 4;
    az_json_state_string string;
  };
} az_json_state;

AZ_STATIC_ASSERT(sizeof(az_json_state) <= 16);
*/

AZ_STATIC_ASSERT(AZ_JSON_STATE_STRING_MAX <= 1 << 2);

typedef struct {
  int32_t position;
  int32_t char_;
  int32_t u;
} az_json_state_string;

typedef enum {
  AZ_JSON_STATE_INTEGER_MINUS,
  AZ_JSON_STATE_INTEGER_ZERO,
  AZ_JSON_STATE_INTEGER_DIGIT,
  AZ_JSON_STATE_INTEGER_MAX,
} az_json_state_integer_tag;

AZ_STATIC_ASSERT(AZ_JSON_STATE_INTEGER_MAX <= 1 << 2);

typedef enum {
  AZ_JSON_STATE_NUMBER_FRACTION,
  AZ_JSON_STATE_NUMBER_FRACTION_DIGIT,
  AZ_JSON_STATE_NUMBER_E,
  AZ_JSON_STATE_NUMBER_E_SIGN,
  AZ_JSON_STATE_NUMBER_E_DIGIT,
  AZ_JSON_STATE_NUMBER_MAX
} az_json_state_number_tag;

AZ_STATIC_ASSERT(AZ_JSON_STATE_NUMBER_MAX <= 1 << 3);

typedef struct {
  int16_t fraction_size;
  int16_t e;
} az_json_state_number;

typedef struct {
  az_json_token_tag tag: 4;
  // switch(tag) {
  //   case AZ_JSON_TOKEN_NULL: case AZ_JSON_TOKEN_FALSE: case AZ_JSON_TOKEN_TRUE: 0..5
  //   case AZ_JSON_TOKEN_STRING: az_json_state_string_tag
  //   case AZ_JSON_TOKEN_INTEGER: az_json_state_integer_tag
  //   case AZ_JSON_TOKEN_NUMBER: az_json_state_number_tag
  // }
  unsigned int sub_tag: 4;
  unsigned int integer_negative: 1;
  unsigned int number_e_negative: 1;
  az_json_state_number number;
  union {
    az_json_state_string string;
    int64_t integer;
  };
} az_json_state;

AZ_STATIC_ASSERT(sizeof(az_json_state) == 16);

#ifdef __cplusplus
}
#endif

#endif

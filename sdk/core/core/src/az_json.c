// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json.h>

inline bool is_white_space(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

inline bool is_digit(char c) {
  return c <= '0' && c <= '9';
}

inline az_error result(char const * *const p_begin, char const *i, az_json_value *const p_value, az_json_value_type type) {
  
  return AZ_OK;
}

az_error az_json_read_value(char const * *const p_begin, char const *const end, az_json_value *const p_value) {
  char const *i = *p_begin;
  for (; i != end; ++i) {
    char const c = *i;
    if (!is_white_space(c)) {
      if (is_digit(c)) {

      }
      switch (c) {
        case '"':
          *p_begin = i;
          p_value->type = AZ_JSON_VALUE_STRING;
          return AZ_OK;
        case '{':
          *p_begin = i;
          p_value->type = AZ_JSON_VALUE_OBJECT;
          return AZ_OK;
        case '[':
          *p_begin = i;
          p_value->type = AZ_JSON_VALUE_ARRAY;
          return AZ_OK;
        default:
          return AZ_JSON_ERROR;
      }
    }
  }
  return AZ_JSON_ERROR;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_SYMBOL_H
#define AZ_JSON_SYMBOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  AZ_JSON_SYMBOL_NONE,
  AZ_JSON_SYMBOL_ERROR,
  AZ_JSON_SYMBOL_DONE,
  AZ_JSON_SYMBOL_CHAR,
};

typedef int8_t az_json_symbol_tag;

enum {
  AZ_JSON_SYMBOL_OBJECT_OPEN = '{',
  AZ_JSON_SYMBOL_OBJECT_CLOSE = '}',
  AZ_JSON_SYMBOL_ARRAY_OPEN = '[',
  AZ_JSON_SYMBOL_ARRAY_CLOSE = ']',
  AZ_JSON_SYMBOL_COMMA = ',',
  AZ_JSON_SYMBOL_COLON = ':',
};

typedef char az_json_symbol_type;

typedef struct {
  char char_;
  char next;
} az_json_symbol_done;

typedef struct {
  az_json_symbol_tag tag;
  union {
    az_json_symbol_type char_;
    az_json_symbol_done done;
  };
} az_json_symbol;

inline az_json_symbol az_json_symbol_create_none() {
  return (az_json_symbol){ .tag = AZ_JSON_SYMBOL_NONE };
}

inline az_json_symbol az_json_symbol_create_char(char const c) {
  return (az_json_symbol){ .tag = AZ_JSON_SYMBOL_CHAR, .char_ = c };
}

inline az_json_symbol az_json_symbol_create_done(az_json_symbol_done const done) {
  return (az_json_symbol){ .tag = AZ_JSON_SYMBOL_DONE, .done = done };
}

inline az_json_symbol az_json_symbol_create_error() {
  return (az_json_symbol){ .tag = AZ_JSON_SYMBOL_ERROR };
}

az_json_symbol az_json_symbol_parse(az_json_symbol const state, char const c);

#ifdef __cplusplus
}
#endif

#endif

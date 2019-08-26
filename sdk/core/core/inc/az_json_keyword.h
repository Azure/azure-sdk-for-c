// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_KEYWORD_H
#define AZ_JSON_KEYWORD_H

#include <stdint.h>
#include <az_sign.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  AZ_JSON_KEYWORD_NONE,
  AZ_JSON_KEYWORD_CHAR,
  AZ_JSON_KEYWORD_DONE,
  AZ_JSON_KEYWORD_ERROR,
};

typedef int8_t az_json_keyword_tag;

enum {
  AZ_JSON_KEYWORD_NULL,
  AZ_JSON_KEYWORD_TRUE,
  AZ_JSON_KEYWORD_FALSE,
};

typedef int8_t az_json_keyword_type;

typedef struct {
  az_json_keyword_type type;
  int8_t size;
} az_json_keyword_char;

typedef struct {
  az_json_keyword_type type;
  char next;
} az_json_keyword_done;

typedef struct {
  az_json_keyword_tag tag;
  union {
    az_json_keyword_char char_;
    az_json_keyword_done done;
  };
} az_json_keyword;

inline az_json_keyword az_json_keyword_create_none() {
  return (az_json_keyword){ .tag = AZ_JSON_KEYWORD_NONE };
}

inline az_json_keyword az_json_keyword_create_char(az_json_keyword_char const char_) {
  return (az_json_keyword){ .tag = AZ_JSON_KEYWORD_CHAR, .char_ = char_ };
}

inline az_json_keyword az_json_keyword_create_error() {
  return (az_json_keyword){ .tag = AZ_JSON_KEYWORD_ERROR };
}

inline az_json_keyword az_json_keyword_create_done(az_json_keyword_done done) {
  return (az_json_keyword){ .tag = AZ_JSON_KEYWORD_DONE, .done = done };
}

az_json_keyword az_json_keyword_parse(az_json_keyword const state, char const c);

#ifdef __cplusplus
}
#endif

#endif

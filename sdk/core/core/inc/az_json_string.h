// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_STRING_H
#define AZ_JSON_STRING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  AZ_JSON_STRING_NONE,

  AZ_JSON_STRING_ERROR,

  AZ_JSON_STRING_DONE,

  AZ_JSON_STRING_OPEN,
  AZ_JSON_STRING_CHAR,
  AZ_JSON_STRING_ESC,
  AZ_JSON_STRING_U,
  AZ_JSON_STRING_CLOSE,
};

typedef int8_t az_json_string_tag;

typedef struct {
  int32_t size;
  int32_t code;
} az_json_string_char;

typedef int32_t az_json_string_esc;

typedef struct {
  int32_t size;
  int32_t code;
  int8_t i;
} az_json_string_u;

typedef int32_t az_json_string_close;

typedef struct {
  size_t size;
  char next;
} az_json_string_done;

typedef struct {
  az_json_string_tag tag;
  union {
    az_json_string_char char_;
    az_json_string_esc esc;
    az_json_string_u u;
    az_json_string_close close;
    az_json_string_done done;
  };
} az_json_string;

inline az_json_string az_json_string_create_none() {
  return (az_json_string){ .tag = AZ_JSON_STRING_NONE };
}

inline az_json_string az_json_string_create_open() {
  return (az_json_string){ .tag = AZ_JSON_STRING_OPEN };
}

inline az_json_string az_json_string_create_char(az_json_string_char const char_) {
  return (az_json_string){ .tag = AZ_JSON_STRING_CHAR, .char_ = char_ };
}

inline az_json_string az_json_string_create_esc(az_json_string_esc const esc) {
  return (az_json_string){ .tag = AZ_JSON_STRING_ESC, .esc = esc };
}

inline az_json_string az_json_string_create_u(az_json_string_u const u) {
  return (az_json_string){ .tag = AZ_JSON_STRING_U, .u = u };
}

inline az_json_string az_json_string_create_close(az_json_string_close const close) {
  return (az_json_string){ .tag = AZ_JSON_STRING_CLOSE, .close = close };
}

inline az_json_string az_json_string_create_done(az_json_string_done const done) {
  return (az_json_string){ .tag = AZ_JSON_STRING_DONE, .done = done };
}

inline az_json_string az_json_string_create_error() {
  return (az_json_string){ .tag = AZ_JSON_STRING_ERROR };
}

az_json_string az_json_string_parse(az_json_string const state, char const c);

#ifdef __cplusplus
}
#endif

#endif

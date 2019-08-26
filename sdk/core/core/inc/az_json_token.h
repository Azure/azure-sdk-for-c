// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_TOKEN_H
#define AZ_JSON_TOKEN_H

#include <stdint.h>

#include <az_json_keyword.h>
#include <az_json_string.h>
#include <az_json_number.h>
#include <az_json_symbol.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  AZ_JSON_TOKEN_PROGRESS,
  AZ_JSON_TOKEN_ERROR,
  AZ_JSON_TOKEN_KEYWORD,
  AZ_JSON_TOKEN_STRING,
  AZ_JSON_TOKEN_NUMBER,
  AZ_JSON_TOKEN_SYMBOL,
};

typedef int8_t az_json_token_tag;

typedef struct {
  az_json_token_tag tag;
  union {
    az_json_keyword keyword;
    az_json_string string;
    az_json_number number;
    az_json_symbol symbol;
  };
} az_json_progress;

inline az_json_progress az_json_progress_create_none() {
  return (az_json_progress){ .tag = AZ_JSON_TOKEN_PROGRESS };
}

inline az_json_progress az_json_progress_create_keyword(az_json_keyword const keyword) {
  return (az_json_progress){ .tag = AZ_JSON_TOKEN_KEYWORD, .keyword = keyword };
}

inline az_json_progress az_json_progress_create_string(az_json_string const string) {
  return (az_json_progress){ .tag = AZ_JSON_TOKEN_STRING, .string = string };
}

inline az_json_progress az_json_progress_create_number(az_json_number const number) {
  return (az_json_progress){ .tag = AZ_JSON_TOKEN_NUMBER, .number = number };
}

inline az_json_progress az_json_progress_create_symbol(az_json_symbol const symbol) {
  return (az_json_progress){ .tag = AZ_JSON_TOKEN_SYMBOL, .symbol = symbol };
}

inline az_json_progress az_json_progress_create_error() {
  return (az_json_progress){ .tag = AZ_JSON_TOKEN_ERROR };
}

typedef struct {
  az_json_token_tag tag;
  union {
    az_json_progress progress;
    az_json_keyword_done keyword;
    az_json_string_done string;
    az_json_number_done number;
    az_json_symbol_done symbol;
  };
} az_json_token;

inline az_json_token az_json_token_create_progress(az_json_progress const progress) {
  return (az_json_token){ .tag = AZ_JSON_TOKEN_PROGRESS, .progress = progress };
}

inline az_json_token az_json_token_create_keyword(az_json_keyword_done const keyword) {
  return (az_json_token){ .tag = AZ_JSON_TOKEN_KEYWORD, .keyword = keyword };
}

inline az_json_token az_json_token_create_string(az_json_string_done const string) {
  return (az_json_token){ .tag = AZ_JSON_TOKEN_STRING, .string = string };
}

inline az_json_token az_json_token_create_number(az_json_number_done const number) {
  return (az_json_token){ .tag = AZ_JSON_TOKEN_NUMBER, .number = number };
}

inline az_json_token az_json_token_create_symbol(az_json_symbol_done const symbol) {
  return (az_json_token){ .tag = AZ_JSON_TOKEN_SYMBOL, .symbol = symbol };
}

inline az_json_token az_json_token_create_error() {
  return (az_json_token){ .tag = AZ_JSON_TOKEN_ERROR };
}

az_json_token az_json_token_parse(az_json_token const token, char const c);

#ifdef __cplusplus
}
#endif

#endif

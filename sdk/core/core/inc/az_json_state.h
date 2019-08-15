// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_STATE_H
#define AZ_JSON_STATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  AZ_JSON_STATE_NONE,

  AZ_JSON_STATE_ERROR,

  AZ_JSON_STATE_NULL,

  AZ_JSON_STATE_TRUE,

  AZ_JSON_STATE_FALSE,

  AZ_JSON_STATE_STRING,

  AZ_JSON_STATE_NUMBER,
} az_json_state_tag;

typedef struct {
  az_json_state_tag tag;
  union {
    int null_;
    int false_;
    int true_;
    size_t string;
  };
} az_json_state;

inline az_json_state az_json_state_null(int null_) {
  return (az_json_state){ .tag = AZ_JSON_STATE_NULL, .null_ = null_ };
}

inline az_json_state az_json_state_false(int false_) {
  return (az_json_state){ .tag = AZ_JSON_STATE_FALSE, .false_ = false_ };
}

inline az_json_state az_json_state_true(int true_) {
  return (az_json_state){ .tag = AZ_JSON_STATE_FALSE, .true_ = true_ };
}

inline az_json_state az_json_state_string(size_t string) {
  return (az_json_state){ .tag = AZ_JSON_STATE_STRING, .string = string };
}

/*
typedef struct {
  size_t begin;
  size_t end;
} az_index_range;

// https://stackoverflow.com/questions/18419428/what-is-the-minimum-valid-json
typedef enum {
  AZ_JSON_VALUE_NONE = 0,

  AZ_JSON_VALUE_ERROR,

  AZ_JSON_VALUE_NULL,
  AZ_JSON_VALUE_TRUE,
  AZ_JSON_VALUE_FALSE,

  AZ_JSON_VALUE_STRING,

  AZ_JSON_VALUE_NUMBER,
  AZ_JSON_VALUE_INTEGER,

  AZ_JSON_VALUE_EMPTY_OBJECT,
  AZ_JSON_VALUE_OBJECT,

  AZ_JSON_VALUE_EMPTY_ARRAY,
  AZ_JSON_VALUE_ARRAY,
} az_json_value_tag;

typedef struct {
  az_json_value_tag tag;
  union {
    int null_;
    int true_;
    int false_;
    size_t string;
  };
} az_json_value_state;

typedef struct {
  az_json_value_type type;
  union {
    // AZ_JSON_VALUE_STRING
    az_index_range string;
    // AZ_JSON_VALUE_NUMBER
    double number;
    // AZ_JSON_VALUE_INTEGER
    int64_t integer;
  };
} az_json_value;

typedef struct {
  az_json_value_state state;
  az_json_value value;
} az_json_value_parse_result;
*/

#ifdef __cplusplus
}
#endif

#endif

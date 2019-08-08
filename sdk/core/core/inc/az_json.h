// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_H
#define AZ_JSON_H

#include <stdbool.h>
#include <stdint.h>
#include <az_cstr.h>
#include <az_error.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  AZ_JSON_VALUE_NULL,
  AZ_JSON_VALUE_FALSE,
  AZ_JSON_VALUE_TRUE,
  AZ_JSON_VALUE_NUMBER,
  AZ_JSON_VALUE_STRING,
  AZ_JSON_VALUE_OBJECT,
  AZ_JSON_VALUE_ARRAY,
} az_json_value_type;

typedef struct {
  az_json_value_type type;
  union {
    double number;
    az_cstr string;
  };
} az_json_value;

az_error az_json_read_value(az_cstr *p_state, az_json_value *p_value);

typedef enum {
  AZ_JSON_OBJECT_TOKEN_END,
  AZ_JSON_OBJECT_TOKEN_PROPERTY,
} az_json_object_token_type;

typedef struct {
  az_cstr name;
  az_json_value value;
} az_json_property;

typedef struct {
  az_json_object_token_type type;
  union {
    az_json_property property;
  };
} az_json_object_token;

az_error az_json_read_object_token(az_cstr *p_state, az_json_object_token *p_token);

typedef enum {
  AZ_JSON_ARRAY_TOKEN_END,
  AZ_JSON_ARRAY_TOKEN_ITEM,
} az_json_array_token_type;

typedef struct {
  az_json_array_token_type type;
  union {
    az_json_value item;
  };
} az_json_array_token;

az_error az_json_read_array_token(az_cstr *p_state, az_json_array_token *p_token);

#ifdef __cplusplus
}
#endif

#endif

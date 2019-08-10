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

typedef struct {
  size_t begin;
  size_t end;
} az_index_range;

typedef enum {
  AZ_JSON_ERROR_UNEXPECTED_END = AZ_JSON_ERROR + 1,
  AZ_JSON_ERROR_UNEXPECTED_SYMBOL = AZ_JSON_ERROR + 2,
} az_json_error;

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
    az_index_range string;
    double number;
  };
} az_json_value;

az_error az_json_parse_value(az_cstr const s, size_t *const p_i, az_json_value *const out_value);

#ifdef __cplusplus
}
#endif

#endif

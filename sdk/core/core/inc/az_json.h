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
  AZ_JSON_VALUE_INTEGER,
  AZ_JSON_VALUE_STRING,
  AZ_JSON_VALUE_EMPTY_OBJECT,
  AZ_JSON_VALUE_OBJECT,
  AZ_JSON_VALUE_EMPTY_ARRAY,
  AZ_JSON_VALUE_ARRAY,
} az_json_value_type;

typedef struct {
  az_json_value_type type;
  union {
    az_index_range string;
    double number;
    int64_t integer;
  };
} az_json_value;

az_error az_json_parse_value(az_cstr const s, size_t *const p_i, az_json_value *const out_value);

typedef struct {
  az_cstr name;
  az_json_value value;
  bool has_next;
} az_json_object_property;

// call it only if `az_json_value.type == AZ_JSON_VALUE_OBJECT` or `az_json_object_property.has_next`.
az_error az_json_parse_object_property(az_cstr const s, size_t *const p_i, az_json_object_property *const out_property);

typedef struct {
  az_json_value value;
  bool has_next;
} az_json_array_item;

// call it only if `az_json_value.type == AZ_JSON_VALUE_ARRAY` or `az_json_array_item.has_next`.
az_error az_json_parse_array_item(az_cstr const s, size_t *const p_i, az_json_array_item *const out_item);

#ifdef __cplusplus
}
#endif

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_PARSER_H
#define AZ_JSON_PARSER_H

#include <az_result.h>
#include <az_span_reader.h>
#include <az_str.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

typedef enum {
  AZ_JSON_VALUE_NONE = 0,
  AZ_JSON_VALUE_NULL = 1,
  AZ_JSON_VALUE_BOOLEAN = 2,
  AZ_JSON_VALUE_NUMBER = 3,
  AZ_JSON_VALUE_STRING = 4,
  AZ_JSON_VALUE_OBJECT = 5,
  AZ_JSON_VALUE_ARRAY = 6,
} az_json_value_kind;

typedef struct {
  az_json_value_kind kind;
  union {
    bool boolean;
    az_span string;
    double number;
  } data;
} az_json_value;

typedef struct {
  az_span name;
  az_json_value value;
} az_json_member;

enum { AZ_JSON_STACK_SIZE = 63 };

typedef uint64_t az_json_stack;

typedef enum {
  AZ_JSON_STACK_OBJECT,
  AZ_JSON_STACK_ARRAY,
} az_json_stack_item;

typedef struct {
  az_span_reader reader;
  az_json_stack stack;
} az_json_parser;

AZ_NODISCARD az_json_parser az_json_parser_create(az_span const buffer);

AZ_NODISCARD az_result az_json_parser_get(az_json_parser * const self, az_json_value * const out_value);

AZ_NODISCARD az_result
az_json_parser_get_object_member(
    az_json_parser * const self,
    az_json_member * const out_member);

AZ_NODISCARD az_result
az_json_parser_get_array_element(
    az_json_parser * const self,
    az_json_value * const out_value);

AZ_NODISCARD az_result az_json_parser_done(az_json_parser const * const self);

AZ_NODISCARD az_result az_json_get_object_member_value(
    az_span const json,
    az_span const name,
    az_json_value * const out_value);

AZ_NODISCARD AZ_INLINE az_result az_json_get_object_member_string_value(
    az_span const json,
    az_span const name,
    az_span * const out_value) {
  AZ_CONTRACT_ARG_NOT_NULL(out_value);
  az_json_value value;
  AZ_RETURN_IF_FAILED(az_json_get_object_member_value(json, name, &value));

  if (value.kind != AZ_JSON_VALUE_STRING) {
    return AZ_ERROR_JSON_NOT_FOUND;
  }

  *out_value = value.data.string;
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result az_json_get_object_member_numeric_value(
    az_span const json,
    az_span const name,
    double * const out_value) {
  AZ_CONTRACT_ARG_NOT_NULL(out_value);
  az_json_value value;
  AZ_RETURN_IF_FAILED(az_json_get_object_member_value(json, name, &value));

  if (value.kind != AZ_JSON_VALUE_NUMBER) {
    return AZ_ERROR_JSON_NOT_FOUND;
  }

  *out_value = value.data.number;
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result az_json_get_object_member_boolean_value(
    az_span const json,
    az_span const name,
    bool * const out_value) {
  AZ_CONTRACT_ARG_NOT_NULL(out_value);

  az_json_value value;
  AZ_RETURN_IF_FAILED(az_json_get_object_member_value(json, name, &value));

  if (value.kind != AZ_JSON_VALUE_NUMBER) {
    return AZ_ERROR_JSON_NOT_FOUND;
  }

  *out_value = value.data.boolean;
  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif

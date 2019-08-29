// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_READ_H
#define AZ_JSON_READ_H

#include <az_cstr.h>
#include <az_error.h>

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  AZ_JSON_ERROR_UNEXPECTED_END  = AZ_JSON_ERROR + 1,
  AZ_JSON_ERROR_UNEXPECTED_CHAR,
} az_json_error;

typedef struct {
  size_t begin;
  size_t end;
} az_json_string;

typedef enum {
  AZ_JSON_NULL,
  AZ_JSON_BOOLEAN,
  AZ_JSON_STRING,
  AZ_JSON_NUMBER,
  AZ_JSON_OBJECT,
  AZ_JSON_ARRAY,
} az_json_value_tag;

typedef struct {
  az_json_value_tag tag;
  union {
    bool boolean;
    az_json_string string;
    double number;
    // true - is done (an empty object).
    // false - the object has properties (use `az_json_read_property` and `az_json_read_object_end`).
    bool object;
    // true - is done (an empty array).
    // false - the array has items (use `az_json_read_value` and `az_json_read_array_end`).
    bool array;
  };
} az_json_value;

inline az_json_value az_json_value_create_null() {
  return (az_json_value){ .tag = AZ_JSON_NULL };
}

inline az_json_value az_json_value_create_boolean(bool const boolean) {
  return (az_json_value){ .tag = AZ_JSON_BOOLEAN, .boolean = boolean };
}

inline az_json_value az_json_value_create_string(az_json_string const string) {
  return (az_json_value){ .tag = AZ_JSON_STRING, .string = string };
}

inline az_json_value az_json_value_create_number(double number) {
  return (az_json_value){ .tag = AZ_JSON_NUMBER, .number = number };
}

inline az_json_value az_json_value_create_object(bool has_properties) {
  return (az_json_value){ .tag = AZ_JSON_OBJECT, .object = has_properties };
}

inline az_json_value az_json_value_create_array(bool has_properties) {
  return (az_json_value){ .tag = AZ_JSON_OBJECT, .array = has_properties };
}

az_error az_json_read_value(az_cstr const buffer, size_t *const p_position, az_json_value *const out_json_value);

typedef struct {
  az_json_string name;
  az_json_value value;
} az_json_property;

az_error az_json_read_object_property(az_cstr const buffer, size_t *const p_position, az_json_property *const out_property);

az_error az_json_read_object_end(az_cstr const buffer, size_t *const p_position, bool *const out_end);

az_error az_json_read_array_end(az_cstr const buffer, size_t *const p_position, bool *const out_end);

#ifdef __cplusplus
}
#endif

#endif

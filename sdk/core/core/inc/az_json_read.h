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
    // true - the object has properties, use `read_property`.
    // false - the object is empty.
    bool object;
    // true - the object has properties, use `read_item`.
    // false - the array is empty.
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

az_error az_json_read_value(az_cstr const buffer, size_t *p_position, az_json_value *out_json_value);

typedef struct {
  az_json_string name;
  az_json_value value;
  bool more_properties;
} az_json_property;

az_error az_json_read_property(az_cstr const buffer, size_t *p_position, az_json_property *out_property);

typedef struct {
  az_json_value value;
  bool more_items;
} az_json_item;

az_error az_json_read_item(az_cstr const buffer, size_t *p_position, az_json_item *out_item);

#ifdef __cplusplus
}
#endif

#endif

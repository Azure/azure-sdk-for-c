// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_types.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  AZ_JSON_TYPE_NULL = 0,
  AZ_JSON_TYPE_BOOLEAN = 1,
  AZ_JSON_TYPE_NUMBER = 3,
  AZ_JSON_TYPE_STRING = 4,
  AZ_JSON_TYPE_OBJECT = 5,
  AZ_JSON_TYPE_ARRAY = 6,
} az_json_type;

typedef struct az_json az_json;
typedef struct az_json_property az_json_property;

AZ_DEFINE_RANGE(az_json_property const, az_json_object);
AZ_DEFINE_RANGE(az_json const, az_json_array);

typedef struct az_json {
  az_json_type type;
  union {
    // AZ_JSON_BOOL
    bool boolean;

    // AZ_JSON_NUMBER
    double number;

    // AZ_JSON_STRING
    az_string string;

    // AZ_JSON_OBJECT
    az_json_object object;

    // AZ_JSON_ARRAY
    az_json_array array;
  };
} az_json;

typedef struct az_json_property {
  az_string name;
  az_json value;
} az_json_property;

typedef void const *az_error;

#define AZ_OK NULL

typedef struct {
  void *context;
  az_error (*write)(void *context, az_string s);
} az_json_write_string;

az_error az_json_write(az_json_write_string const write_string, az_json const json);

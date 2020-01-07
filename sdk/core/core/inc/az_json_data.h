// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_DATA_H
#define AZ_JSON_DATA_H

#include <az_json_token.h>

#include <az_mut_span.h>

#include <_az_cfg_prefix.h>

typedef struct az_json_data az_json_data;

typedef struct {
  az_json_data const * begin;
  size_t size;
} az_json_array;

typedef struct az_json_object_member az_json_object_member;

typedef struct {
  az_json_object_member const * begin;
  size_t size;
} az_json_object;

typedef enum {
  AZ_JSON_DATA_NULL = 0,
  AZ_JSON_DATA_BOOLEAN = 1,
  AZ_JSON_DATA_NUMBER = 2,
  AZ_JSON_DATA_STRING = 3,
  AZ_JSON_DATA_OBJECT = 4,
  AZ_JSON_DATA_ARRAY = 5,
} az_json_data_kind;

struct az_json_data {
  az_json_data_kind kind;
  union {
    bool boolean;
    az_span string;
    double number;
    az_json_array array;
    az_json_object object;
  } data;
};

AZ_NODISCARD AZ_INLINE az_json_data az_json_data_null() {
  return (az_json_data){ .kind = AZ_JSON_DATA_NULL };
}

AZ_NODISCARD AZ_INLINE az_json_data az_json_data_boolean(bool const data) {
  return (az_json_data){ .kind = AZ_JSON_DATA_BOOLEAN, .data.boolean = data };
}

AZ_NODISCARD AZ_INLINE az_json_data az_json_data_number(double const data) {
  return (az_json_data){ .kind = AZ_JSON_DATA_NUMBER, .data.number = data };
}

AZ_NODISCARD AZ_INLINE az_json_data az_json_data_string(az_span const data) {
  return (az_json_data){ .kind = AZ_JSON_DATA_STRING, .data.string = data };
}

AZ_NODISCARD AZ_INLINE az_json_data az_json_data_object(az_json_object const data) {
  return (az_json_data){ .kind = AZ_JSON_DATA_OBJECT, .data.object = data };
}

AZ_NODISCARD AZ_INLINE az_json_data az_json_data_array(az_json_array const data) {
  return (az_json_data){ .kind = AZ_JSON_DATA_ARRAY, .data.array = data };
}

struct az_json_object_member {
  az_span name;
  az_json_data value;
};

AZ_NODISCARD az_result
az_json_to_data(az_span const json, az_mut_span const buffer, az_json_data const ** const out);

#include <_az_cfg_suffix.h>

#endif

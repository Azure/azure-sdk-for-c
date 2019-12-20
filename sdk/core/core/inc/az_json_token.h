// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_TOKEN_H
#define AZ_JSON_TOKEN_H

#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

typedef enum {
  AZ_JSON_TOKEN_NULL = 0,
  AZ_JSON_TOKEN_BOOLEAN = 1,
  AZ_JSON_TOKEN_NUMBER = 2,
  AZ_JSON_TOKEN_STRING = 3,
  AZ_JSON_TOKEN_OBJECT = 4,
  AZ_JSON_TOKEN_ARRAY = 5,
  // A special case for non-JSON strings. This field is used to serialize `az_span` into JSON
  // string. Currently, our JSON parser doesn't return this kind of values.
  AZ_JSON_TOKEN_SPAN = 6,
} az_json_token_kind;

typedef struct {
  az_json_token_kind kind;
  union {
    bool boolean;
    az_span string;
    double number;
    az_span span;
  } data;
} az_json_token;

AZ_NODISCARD AZ_INLINE az_json_token az_json_token_null() {
  return (az_json_token){ .kind = AZ_JSON_TOKEN_NULL };
}

AZ_NODISCARD AZ_INLINE az_json_token az_json_token_boolean(bool const value) {
  return (az_json_token){
    .kind = AZ_JSON_TOKEN_BOOLEAN,
    .data.boolean = value,
  };
}

AZ_NODISCARD AZ_INLINE az_json_token az_json_token_string(az_span const value) {
  return (az_json_token){
    .kind = AZ_JSON_TOKEN_STRING,
    .data.string = value,
  };
}

AZ_NODISCARD AZ_INLINE az_json_token az_json_token_number(double const value) {
  return (az_json_token){
    .kind = AZ_JSON_TOKEN_NUMBER,
    .data.number = value,
  };
}

AZ_NODISCARD AZ_INLINE az_json_token az_json_token_object() {
  return (az_json_token){ .kind = AZ_JSON_TOKEN_OBJECT };
}

AZ_NODISCARD AZ_INLINE az_json_token az_json_token_array() {
  return (az_json_token){ .kind = AZ_JSON_TOKEN_ARRAY };
}

AZ_NODISCARD AZ_INLINE az_json_token az_json_token_span(az_span const span) {
  return (az_json_token){
    .kind = AZ_JSON_TOKEN_SPAN,
    .data.span = span,
  };
}

/**
 * Copies a boolean value to @var out from the given JSON value.
 *
 * If the JSON value is not a boolean then the function returns an error.
 */
AZ_NODISCARD az_result
az_json_token_get_boolean(az_json_token const self, bool * const out);

/**
 * Copies a string span to @var out from the given JSON value.
 *
 * If the JSON value is not a string then the function returns an error.
 */
AZ_NODISCARD az_result
az_json_token_get_string(az_json_token const self, az_span * const out);

/**
 * Copies a number to @var out from the given JSON value.
 *
 * If the JSON value is not a number then the function returns an error.
 */
AZ_NODISCARD az_result
az_json_token_get_number(az_json_token const self, double * const out);

typedef struct {
  az_span name;
  az_json_token value;
} az_json_token_member;

#include <_az_cfg_suffix.h>

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_json.h
 *
 * @brief non allocating utilities for reading/parsing and building json
 */

#ifndef _az_JSON_H
#define _az_JSON_H

#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

/************************************ JSON TOKEN ******************/

typedef enum {
  AZ_JSON_TOKEN_NULL = 0,
  AZ_JSON_TOKEN_BOOLEAN = 1,
  AZ_JSON_TOKEN_NUMBER = 2,
  AZ_JSON_TOKEN_STRING = 3,
  AZ_JSON_TOKEN_OBJECT = 4,
  AZ_JSON_TOKEN_ARRAY = 5,
  // A special case for non-JSON strings. This field is used to serialize `az_span` into JSON
  // string. Currently, our JSON parser doesn't return this kind of value.
  AZ_JSON_TOKEN_SPAN = 6,
} az_json_token_kind;

typedef struct {
  az_json_token_kind kind;
  union {
    bool boolean;
    double number;
    az_span string;
    az_span span;
  } value;
} az_json_token;

AZ_NODISCARD AZ_INLINE az_json_token az_json_token_null() {
  return (az_json_token){ .kind = AZ_JSON_TOKEN_NULL, .value = { 0 } };
}

AZ_NODISCARD AZ_INLINE az_json_token az_json_token_boolean(bool value) {
  return (az_json_token){
    .kind = AZ_JSON_TOKEN_BOOLEAN,
    .value.boolean = value,
  };
}

AZ_NODISCARD AZ_INLINE az_json_token az_json_token_number(double value) {
  return (az_json_token){
    .kind = AZ_JSON_TOKEN_NUMBER,
    .value.number = value,
  };
}

AZ_NODISCARD AZ_INLINE az_json_token az_json_token_string(az_span value) {
  return (az_json_token){
    .kind = AZ_JSON_TOKEN_STRING,
    .value.string = value,
  };
}

AZ_NODISCARD AZ_INLINE az_json_token az_json_token_span(az_span span) {
  return (az_json_token){
    .kind = AZ_JSON_TOKEN_SPAN,
    .value.span = span,
  };
}

AZ_NODISCARD AZ_INLINE az_json_token az_json_token_object() {
  return (az_json_token){ .kind = AZ_JSON_TOKEN_OBJECT, .value = { 0 } };
}

AZ_NODISCARD AZ_INLINE az_json_token az_json_token_array() {
  return (az_json_token){ .kind = AZ_JSON_TOKEN_ARRAY, .value = { 0 } };
}

/**
 * Copies a boolean value to @var out from the given JSON value.
 *
 * If the JSON value is not a boolean then the function returns an error.
 */
AZ_NODISCARD az_result az_json_token_get_boolean(az_json_token self, bool * out);

/**
 * Copies a number to @var out from the given JSON value.
 *
 * If the JSON value is not a number then the function returns an error.
 */
AZ_NODISCARD az_result az_json_token_get_number(az_json_token self, double * out);

/**
 * Copies a string span to @var out from the given JSON value.
 *
 * If the JSON value is not a string then the function returns an error.
 */
AZ_NODISCARD az_result az_json_token_get_string(az_json_token self, az_span * out);

/************************************ JSON BUILDER ******************/

typedef struct {
  struct {
    az_span json;
    bool need_comma;
  } _internal;
} az_json_builder;

AZ_NODISCARD AZ_INLINE az_result az_json_builder_init(az_json_builder * self, az_span json_buffer) {
  *self = (az_json_builder){ ._internal = { .json = json_buffer, .need_comma = false } };
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_span az_json_builder_span_get(az_json_builder const * self) {
  return self->_internal.json;
}

AZ_NODISCARD az_result az_json_builder_append_token(az_json_builder * self, az_json_token token);

AZ_NODISCARD az_result
az_json_builder_append_object(az_json_builder * self, az_span name, az_json_token token);

AZ_NODISCARD az_result az_json_builder_append_object_close(az_json_builder * self);

AZ_NODISCARD az_result
az_json_builder_append_array_item(az_json_builder * self, az_json_token token);

AZ_NODISCARD az_result az_json_builder_append_array_close(az_json_builder * self);

/************************************ JSON PARSER ******************/

typedef uint64_t az_json_stack;
typedef struct {
  struct {
    az_span reader;
    az_json_stack stack;
  } _internal;
} az_json_parser;

typedef struct {
  az_span name;
  az_json_token token;
} az_json_token_member;

AZ_NODISCARD az_result az_json_parser_init(az_json_parser * self, az_span json_buffer);

AZ_NODISCARD az_result az_json_parser_parse_token(az_json_parser * self, az_json_token * out_token);

AZ_NODISCARD az_result
az_json_parser_parse_token_member(az_json_parser * self, az_json_token_member * out_token_member);

AZ_NODISCARD az_result
az_json_parser_parse_array_item(az_json_parser * self, az_json_token * out_token);

AZ_NODISCARD az_result az_json_parser_done(az_json_parser * self);

/**
 * Read all nested values and ignore them.
 */
AZ_NODISCARD az_result az_json_parser_skip_children(az_json_parser * self, az_json_token token);

/************************************ JSON POINTER ******************/

/**
 * Get JSON value by JSON pointer https://tools.ietf.org/html/rfc6901.
 */
AZ_NODISCARD az_result
az_json_parse_by_pointer(az_span json, az_span pointer, az_json_token * out_token);

#include <_az_cfg_suffix.h>

#endif

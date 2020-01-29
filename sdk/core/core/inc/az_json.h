// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_JSON_BUILDER_H
#define _az_JSON_BUILDER_H

#include <az_result.h>
#include <az_span.h>
#include <az_span_reader.h>

#include <stdbool.h>

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

AZ_NODISCARD AZ_INLINE az_span az_json_builder_span_get(az_json_builder self) {
  return self._internal.json;
}

AZ_NODISCARD az_result az_json_builder_append_token(az_json_builder * self, az_json_token token);

AZ_NODISCARD az_result
az_json_builder_append_object(az_json_builder * self, az_span name, az_json_token token);

AZ_NODISCARD az_result az_json_builder_append_object_close(az_json_builder * self);

AZ_NODISCARD az_result
az_json_builder_append_array_item(az_json_builder * self, az_json_token token);

AZ_NODISCARD az_result az_json_builder_append_array_close(az_json_builder * self);

/************************************ JSON POINTER ******************/

/**
 * Returns a next reference token in the JSON pointer. The JSON pointer parser is @var
 * az_span_reader.
 *
 * See https://tools.ietf.org/html/rfc6901
 */
AZ_NODISCARD az_result
az_span_reader_read_json_pointer_token(az_span_reader * const self, az_span * const out);

/**
 * Returns a next character in the given span reader of JSON pointer reference token.
 */
AZ_NODISCARD az_result
az_span_reader_read_json_pointer_token_char(az_span_reader * const self, uint32_t * const out);

/************************************ JSON GET ******************/

AZ_NODISCARD az_result
az_json_get_object_member(az_span const json, az_span const name, az_json_token * const out_token);

/**
 * Get JSON value by JSON pointer https://tools.ietf.org/html/rfc6901.
 */
AZ_NODISCARD az_result
az_json_get_by_pointer(az_span const json, az_span const pointer, az_json_token * const out_token);

/************************************ JSON PARSER ******************/

enum { AZ_JSON_STACK_SIZE = 63 };

typedef uint64_t az_json_stack;

typedef struct {
  az_span name;
  az_json_token token;
} az_json_token_member;

typedef enum {
  AZ_JSON_STACK_OBJECT = 0,
  AZ_JSON_STACK_ARRAY = 1,
} az_json_stack_item;

typedef struct {
  az_span_reader reader;
  az_json_stack stack;
} az_json_parser;

AZ_NODISCARD az_json_parser az_json_parser_create(az_span const buffer);

AZ_NODISCARD az_result
az_json_parser_read(az_json_parser * const self, az_json_token * const out_token);

AZ_NODISCARD az_result az_json_parser_read_object_member(
    az_json_parser * const self,
    az_json_token_member * const out_token);

AZ_NODISCARD az_result
az_json_parser_read_array_element(az_json_parser * const self, az_json_token * const out_token);

AZ_NODISCARD az_result az_json_parser_done(az_json_parser const * const self);

/**
 * Read all nested values and ignore them.
 */
AZ_NODISCARD az_result az_json_parser_skip(az_json_parser * const self, az_json_token const token);

#include <_az_cfg_suffix.h>

#endif

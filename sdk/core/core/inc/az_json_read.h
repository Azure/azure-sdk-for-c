// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_READ_H
#define AZ_JSON_READ_H

#include <az_str.h>
#include <az_result.h>

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  // success codes
  AZ_JSON_NO_MORE_ITEMS         = AZ_MAKE_RESULT(AZ_JSON_FACILITY, 1),

  // error codes
  AZ_JSON_ERROR_UNEXPECTED_CHAR = AZ_MAKE_ERROR(AZ_JSON_FACILITY, 2),
  AZ_JSON_ERROR_INVALID_STATE   = AZ_MAKE_ERROR(AZ_JSON_FACILITY, 3),
  AZ_JSON_ERROR_STACK_OVERFLOW  = AZ_MAKE_ERROR(AZ_JSON_FACILITY, 4),
};

typedef enum {
  AZ_JSON_VALUE_NONE    = 0,
  AZ_JSON_VALUE_NULL    = 1,
  AZ_JSON_VALUE_BOOLEAN = 2,
  AZ_JSON_VALUE_NUMBER  = 3,
  AZ_JSON_VALUE_STRING  = 4,
  AZ_JSON_VALUE_OBJECT  = 5,
  AZ_JSON_VALUE_ARRAY   = 6,
} az_json_value_kind;

typedef struct {
  az_json_value_kind kind;
  union {
    bool boolean;
    az_const_span string;
    double number;
  } data;
} az_json_value;

typedef struct {
  az_const_span name;
  az_json_value value;
} az_json_member;

enum {
  AZ_JSON_STACK_SIZE = 63
};

typedef uint64_t az_json_stack;

typedef enum {
  AZ_JSON_STACK_OBJECT,
  AZ_JSON_STACK_ARRAY,
} az_json_stack_item;

typedef struct {
  az_const_span buffer;
  size_t i;
  az_json_stack stack;
} az_json_state;

az_json_state az_json_state_create(az_const_span const buffer);

az_result az_json_read(az_json_state *const p_state, az_json_value *const out_value);

az_result az_json_read_object_member(az_json_state *const p_state, az_json_member *const out_member);

az_result az_json_read_array_element(az_json_state *const p_state, az_json_value *const out_value);

az_result az_json_state_done(az_json_state const *const p_state);

#ifdef __cplusplus
}
#endif

#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_READ_H
#define AZ_JSON_READ_H

#include <az_const_str.h>
#include <az_result.h>

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  AZ_JSON_NO_MORE_ITEMS = AZ_JSON_RESULT + 1,
  AZ_JSON_ERROR_UNEXPECTED_END,
  AZ_JSON_ERROR_UNEXPECTED_CHAR,
  AZ_JSON_ERROR_INVALID_STATE,
  AZ_JSON_ERROR_STACK_OVERFLOW,
};

typedef az_const_str az_json_string;

typedef enum {
  AZ_JSON_NONE    = 0,
  AZ_JSON_NULL    = 1,
  AZ_JSON_BOOLEAN = 2,
  AZ_JSON_NUMBER  = 3,
  AZ_JSON_STRING  = 4,
  AZ_JSON_OBJECT  = 5,
  AZ_JSON_ARRAY   = 6,
} az_json_value_tag;

typedef struct {
  az_json_value_tag tag;
  union {
    bool boolean;
    az_json_string string;
    double number;
  };
} az_json_value;

typedef struct {
  az_json_string name;
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
  az_const_str buffer;
  size_t i;
  az_json_stack stack;
} az_json_state;

az_json_state az_json_state_create(az_const_str const buffer);

az_result az_json_read(az_json_state *const p_state, az_json_value *const out_value);

az_result az_json_read_object_member(az_json_state *const p_state, az_json_member *const out_member);

az_result az_json_read_array_element(az_json_state *const p_state, az_json_value *const out_value);

az_result az_json_state_done(az_json_state const *const p_state);

#ifdef __cplusplus
}
#endif

#endif

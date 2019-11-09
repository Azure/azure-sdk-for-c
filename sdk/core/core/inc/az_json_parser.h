// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_PARSER_H
#define AZ_JSON_PARSER_H

#include <az_json_value.h>
#include <az_result.h>
#include <az_span_reader.h>
#include <az_str.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span name;
  az_json_value value;
} az_json_member;

enum { AZ_JSON_STACK_SIZE = 63 };

typedef uint64_t az_json_stack;

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
az_json_parser_get(az_json_parser * const self, az_json_value * const out_value);

AZ_NODISCARD az_result
az_json_parser_get_object_member(az_json_parser * const self, az_json_member * const out_member);

AZ_NODISCARD az_result
az_json_parser_get_array_element(az_json_parser * const self, az_json_value * const out_value);

AZ_NODISCARD az_result az_json_parser_done(az_json_parser const * const self);

AZ_NODISCARD az_result az_json_parser_skip(az_json_parser * const self, az_json_value const value);

#include <_az_cfg_suffix.h>

#endif

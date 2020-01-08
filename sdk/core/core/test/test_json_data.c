// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_data.h>
#include <az_str.h>

#include "./az_test.h"

#include <_az_cfg.h>

void test_json_data() {
  uint8_t buffer[100] = { 0 };
  az_json_data const * data = { 0 };
  az_result const result
      = az_json_to_data(AZ_STR("true"), (az_mut_span)AZ_SPAN_FROM_ARRAY(buffer), data);
  TEST_ASSERT(result == AZ_OK);
}

typedef struct {
  size_t a;
} foo_data;

void foo(foo_data const ** pp) { *pp = NULL; }

void bar() {
  foo_data const * p = NULL;
  foo(p);
}
// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "./az_test.h"

#include <az_mut_span.h>
#include <az_str.h>

#include <_az_cfg.h>

void test_mut_span() {
  // swap
  {
    az_mut_span const a = AZ_CONST_STR("Hello world!");
    az_mut_span const b = AZ_CONST_STR("Goodbye!");
    az_mut_span_swap(a, b);
    TEST_ASSERT(az_span_is_equal(az_mut_span_to_span(a), AZ_STR("Goodbye!rld!")));
    TEST_ASSERT(az_span_is_equal(az_mut_span_to_span(b), AZ_STR("Hello wo")));
  }
}

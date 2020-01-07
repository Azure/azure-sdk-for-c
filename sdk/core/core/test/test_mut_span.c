// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "./az_test.h"

#include <az_mut_span.h>
#include <az_str.h>

#include <_az_cfg.h>

void test_mut_span() {
  // swap
  {
    uint8_t a_array[] = "Hello world!";
    uint8_t b_array[] = "Goodbye!";
    az_mut_span const a = AZ_SPAN_FROM_ARRAY(a_array);
    az_mut_span const b = AZ_SPAN_FROM_ARRAY(b_array);
    az_mut_span_swap(a, b);
    TEST_ASSERT(az_span_is_equal(az_mut_span_to_span(a), AZ_STR("Goodbye!\0ld!\0")));
    TEST_ASSERT(az_span_is_equal(az_mut_span_to_span(b), AZ_STR("Hello wor")));
  }
  // swap an empty span
  {
    uint8_t a_array[] = "Hello world!";
    az_mut_span const a = AZ_SPAN_FROM_ARRAY(a_array);
    az_mut_span const b = { 0 };
    az_mut_span_swap(a, b);
    TEST_ASSERT(az_span_is_equal(az_mut_span_to_span(a), AZ_STR("Hello world!\0")));
    TEST_ASSERT(az_span_is_equal(az_mut_span_to_span(b), AZ_STR("")));
  }
}
